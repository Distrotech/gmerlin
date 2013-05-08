/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <gavl/gavl.h>
#include <gavl/metadata.h>

#include <gmerlin/upnp/ssdp.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/translation.h>
#include <gmerlin/http.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "ssdp"

#include <gmerlin/utils.h>

#include <unistd.h>

// #define DUMP_UDP

#define DUMP_DEVS

// #define DUMP_HEADERS

#define UDP_BUFFER_SIZE 2048

#define QUEUE_SIZE 10

// #define META_METHOD "$METHOD"
// #define META_STATUS "$STATUS"

#define NOTIFY_INTERVAL (900*GAVL_TIME_SCALE) // max-age / 2

typedef struct
  {
  char * st;
  bg_socket_address_t * addr;
  gavl_time_t time; // GAVL_TIME_UNDEFINED means empty
  int times_sent;
  } queue_element_t;

struct bg_ssdp_s
  {
  int mcast_fd;
  int ucast_fd;
  gavl_timer_t * timer;
  bg_socket_address_t * sender_addr;
  bg_socket_address_t * local_addr;
  bg_socket_address_t * mcast_addr;
  uint8_t buf[UDP_BUFFER_SIZE];

  int discover_remote;
  const bg_ssdp_root_device_t * local_dev;

  int num_remote_devs;
  int remote_devs_alloc;

  bg_ssdp_root_device_t * remote_devs;

  const char * server_string;

  queue_element_t queue[QUEUE_SIZE];

#ifdef DUMP_DEVS
  gavl_time_t last_dump_time;
#endif

  gavl_time_t last_notify_time;
  };


// TYPE:VERSION

static char * extract_type_version(const char * str, int * version)
  {
  char * ret;
  const char * pos;
  
  pos = strchr(str, ':');
  if(!pos)
    return NULL;

  ret = gavl_strndup(str, pos);
  pos++;
  if(*pos == '\0')
    {
    free(ret);
    return NULL;
    }
  *version = atoi(pos);
  return ret;
  }


static int 
find_root_dev_by_location(bg_ssdp_t * s, const char * loc)
  {
  int i;
  for(i = 0; i < s->num_remote_devs; i++)
    {
    if(!strcmp(s->remote_devs[i].url, loc))
      return i;
    }
  return -1;
  }

static int 
find_embedded_dev_by_uuid(const bg_ssdp_root_device_t * dev, const char * uuid)
  {
  int i;
  for(i = 0; i < dev->num_devices; i++)
    {
    if(!strcmp(dev->devices[i].uuid, uuid))
      return i;
    }
  return -1;
  }

static const char * is_device_type(const char * str)
  {
  if(gavl_string_starts_with_i(str, "urn:schemas-upnp-org:device:"))
    return str + 28;
  return NULL;
  }

static const char * is_service_type(const char * str)
  {
  if(gavl_string_starts_with_i(str, "urn:schemas-upnp-org:service:"))
    return str + 29;
  return NULL;
  }

static bg_ssdp_service_t * find_service(bg_ssdp_service_t * s, int num, const char * type)
  {
  int i;
  int len;
  const char * pos;
  
  if((pos = strchr(type, ':')))
    len = pos - type;
  else
    len = strlen(type);

  for(i = 0; i < num; i++)
    {
    if((strlen(s[i].type) == len) &&
       !strncmp(s[i].type, type, len))
      return &s[i];
    }
  return NULL;
  }

static bg_ssdp_service_t * device_find_service(bg_ssdp_device_t * dev, const char * type)
  {
  return find_service(dev->services, dev->num_services, type);
  }

static bg_ssdp_service_t * root_device_find_service(bg_ssdp_root_device_t * dev, const char * type)
  {
  return find_service(dev->services, dev->num_services, type);
  }

static bg_ssdp_service_t * add_service(bg_ssdp_service_t ** sp, int * nump)
  {
  bg_ssdp_service_t * ret;

  bg_ssdp_service_t * s = *sp;
  int num = *nump;

  s = realloc(s, (num+1) * sizeof(*s));

  ret = &s[num];
  memset(ret, 0, sizeof(*ret));
  num++;

  *sp = s;
  *nump = num;

  return ret;  
  }

static bg_ssdp_service_t * device_add_service(bg_ssdp_device_t * dev)
  {
  return add_service(&dev->services, &dev->num_services);
  }

static bg_ssdp_service_t * root_device_add_service(bg_ssdp_root_device_t * dev)
  {
  return add_service(&dev->services, &dev->num_services);
  }

static bg_ssdp_root_device_t *
add_root_dev(bg_ssdp_t * s, const char * loc)
  {
  bg_ssdp_root_device_t * ret;
  
  if(s->num_remote_devs + 1 > s->remote_devs_alloc)
    {
    s->remote_devs_alloc += 16;
    s->remote_devs = realloc(s->remote_devs,
                             s->remote_devs_alloc * sizeof(*s->remote_devs));
    memset(s->remote_devs + s->num_remote_devs, 0,
           (s->remote_devs_alloc - s->num_remote_devs) * sizeof(*s->remote_devs));
    }
  ret = s->remote_devs + s->num_remote_devs;
  ret->url = gavl_strdup(loc);
  s->num_remote_devs++;
  return ret;
  }

static void
del_remote_dev(bg_ssdp_t * s, int idx)
  {
  bg_ssdp_root_device_free(&s->remote_devs[idx]); 
  if(idx < s->num_remote_devs - 1)
    memmove(&s->remote_devs[idx], &s->remote_devs[idx+1], 
            sizeof(s->remote_devs[idx]) * (s->num_remote_devs - 1 - idx));
  s->num_remote_devs--;
  }

static char * search_string =
"M-SEARCH * HTTP/1.1\r\n"
"Host:239.255.255.250:1900\r\n"
"ST: ssdp:all\r\n"
"Man:\"ssdp:discover\"\r\n"
"MX:3\r\n\r\n";

bg_ssdp_t * bg_ssdp_create(bg_ssdp_root_device_t * local_dev,
                           int discover_remote, const char * server_string)
  {
  char addr_str[BG_SOCKET_ADDR_STR_LEN];
  bg_ssdp_t * ret = calloc(1, sizeof(*ret));
  
  ret->discover_remote = discover_remote;
  ret->local_dev = local_dev;
  ret->server_string = server_string;
  ret->last_notify_time = GAVL_TIME_UNDEFINED;
  
#ifdef DUMP_DEVS
  if(ret->local_dev)
    {
    gavl_dprintf("Local root device:\n");
    bg_ssdp_root_device_dump(ret->local_dev);
    }
#endif
  
  ret->sender_addr = bg_socket_address_create();
  ret->local_addr  = bg_socket_address_create();
  ret->mcast_addr  = bg_socket_address_create();

  if(ret->local_dev)
    {
    /* If we advertise a device, we need to bind onto the same interface */
    char * host = NULL;
    bg_url_split(ret->local_dev->url,
                 NULL,
                 NULL,
                 NULL,
                 &host,
                 NULL,
                 NULL);
    bg_socket_address_set(ret->local_addr, host, 0, SOCK_DGRAM);
    }
  else if(!bg_socket_address_set_local(ret->local_addr, 0))
    goto fail;

  if(!bg_socket_address_set(ret->mcast_addr, "239.255.255.250", 1900, SOCK_DGRAM))
    goto fail;

  ret->mcast_fd = bg_udp_socket_create_multicast(ret->mcast_addr);
  ret->ucast_fd = bg_udp_socket_create(ret->local_addr);
  
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Unicast socket bound at %s", 
         bg_socket_address_to_string(ret->local_addr, addr_str));
  
  /* Send seach packet */
  if(ret->discover_remote)
    bg_udp_socket_send(ret->ucast_fd, (uint8_t*)search_string,
                       strlen(search_string), ret->mcast_addr);
  
  ret->timer = gavl_timer_create(ret->timer);
  gavl_timer_start(ret->timer);

  /* Create send queue */
  if(ret->local_dev)
    {
    int i;
    for(i = 0; i < QUEUE_SIZE; i++)
      {
      ret->queue[i].addr = bg_socket_address_create();
      ret->queue[i].time = GAVL_TIME_UNDEFINED;
      }
    }
  return ret;

  fail:
  bg_ssdp_destroy(ret);
  return NULL;
  }

static char * uuid_from_usn(const char * usn)
  {
  const char * pos;
  if(gavl_string_starts_with(usn, "uuid:"))
    {
    pos = strchr(usn+5, ':');
    if(!pos)
      pos = usn + 5 + strlen(usn + 5);
    return gavl_strndup(usn+5, pos);
    }
  return NULL;
  }

static void update_device(bg_ssdp_t * s, const char * type,
                          gavl_metadata_t * m)
  {
  int idx;
  int max_age = 0;
  const char * loc;
  const char * usn;
  const char * cc; 
  const char * pos;
  char * uuid;
  const char * type_version;

  bg_ssdp_root_device_t * dev;
  bg_ssdp_device_t * edev;
  
  loc = gavl_metadata_get_i(m, "LOCATION");
  if(!loc)
    return;

  /* Get max age */
  cc = gavl_metadata_get_i(m, "CACHE-CONTROL");
  if(!cc || 
     !gavl_string_starts_with(cc, "max-age") ||
     !(pos = strchr(cc, '=')))
    return;
  pos++;
  max_age = atoi(pos);

  /* Get UUID */
  usn = gavl_metadata_get_i(m, "USN");
  if(!usn)
    return;

  if(!(uuid = uuid_from_usn(usn)))
    return;
  
  idx = find_root_dev_by_location(s, loc);

  if(idx >= 0)
    dev = &s->remote_devs[idx];
  else
    dev = add_root_dev(s, loc);
  
  dev->expire_time = gavl_timer_get(s->timer) + GAVL_TIME_SCALE * max_age;

  if(!strcasecmp(type, "upnp:rootdevice"))
    {
    /* Set root uuid if not known already */
    if(!dev->uuid)
      dev->uuid = gavl_strdup(uuid);
    goto end;
    }

  /* If we have no uuid of the root device yet, we can't do much more */
  if(!dev->uuid)
    goto end;

  if(gavl_string_starts_with_i(type, "uuid:"))
    {
    /* Can't do much here */
    goto end;
    }

  if(!strcmp(uuid, dev->uuid))
    {
    /* Message is for root device */
    edev = NULL;
    }
  else
    {
    idx = find_embedded_dev_by_uuid(dev, uuid);
    if(idx >= 0)
      edev = &dev->devices[idx];
    else
      edev = bg_ssdp_device_add_device(dev, uuid);
    }

  if((type_version = is_device_type(type)))
    {
    if(edev)
      {
      if(!edev->type)
        edev->type = extract_type_version(type_version, &edev->version);
      }
    else if(!dev->type)
      dev->type = extract_type_version(type_version, &dev->version);
    }
  else if((type_version = is_service_type(type)))
    {
    bg_ssdp_service_t * s = NULL;
    
    if(edev)
      {
      if(!device_find_service(edev, type_version))
        s = device_add_service(edev);
      }
    else
      {
      if(!root_device_find_service(dev, type_version))
        s = root_device_add_service(dev);
      }    
    if(s)
      s->type = extract_type_version(type_version, &s->version);
    }
  end:
  if(uuid)
    free(uuid);  
  }

static void schedule_reply(bg_ssdp_t * s, const char * st,
                           const bg_socket_address_t * sender,
                           gavl_time_t current_time, int mx)
  {
  int i, idx = -1;
  for(i = 0; i < QUEUE_SIZE; i++)
    {
    if(!s->queue[i].st)
      {
      idx = i;
      break;
      }
    }
  if(idx == -1)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Cannot schedule search reply, queue full");
    return;
    }
  s->queue[idx].st = gavl_strdup(st);

  s->queue[idx].time = current_time;
  s->queue[idx].time += (int64_t)((double)rand() / (double)RAND_MAX * (double)(mx * GAVL_TIME_SCALE));
  bg_socket_address_copy(s->queue[idx].addr, sender);
  }

// static void setup_header(bg_ssdp_root_device_t * dev, 

static void flush_reply_packet(bg_ssdp_t * s, const gavl_metadata_t * h, bg_socket_address_t * sender)
  {
  int len = 0;
  char * str = bg_http_response_to_string(h, &len);
  //  fprintf(stderr, "Sending search reply:\n%s\n", str);
  bg_udp_socket_send(s->ucast_fd, (uint8_t*)str, len, sender);
  free(str);
  }
                               
static void flush_reply(bg_ssdp_t * s, queue_element_t * q)
  {
  const char * type_version;
  gavl_metadata_t h;
  int i;
  gavl_metadata_init(&h);
  bg_http_response_init(&h, "HTTP/1.1", 200, "OK");
  gavl_metadata_set(&h, "CACHE-CONTROL", "max-age=1800");
  bg_http_header_set_empty_var(&h, "EXT");
  gavl_metadata_set(&h, "LOCATION", s->local_dev->url);
  gavl_metadata_set(&h, "SERVER", s->server_string);
  
  if(!strcasecmp(q->st, "ssdp:all"))
    {
    /*
     *  uuid:device-UUID::upnp:rootdevice
     *  uuid:device-UUID (for each root- and embedded dev)
     *  uuid:device-UUID::urn:schemas-upnp-org:device:deviceType:v (for each root- and embedded dev)
     *  uuid:device-UUID::urn:schemas-upnp-org:service:serviceType:v  (for each service)
     */
    /* Root device */
    gavl_metadata_set_nocpy(&h, "USN", bg_ssdp_get_root_usn(s->local_dev));
    gavl_metadata_set_nocpy(&h, "ST",  bg_ssdp_get_root_nt(s->local_dev));
    flush_reply_packet(s, &h, q->addr);
  
    /* Device UUID */
    gavl_metadata_set_nocpy(&h, "USN", bg_ssdp_get_device_uuid_usn(s->local_dev, -1));
    gavl_metadata_set_nocpy(&h, "ST",  bg_ssdp_get_device_uuid_nt(s->local_dev, -1));
    flush_reply_packet(s, &h, q->addr);
  
    /* TODO: Embedded devices would come here */

    /* Device Type */
    gavl_metadata_set_nocpy(&h, "USN", bg_ssdp_get_device_type_usn(s->local_dev, -1));
    gavl_metadata_set_nocpy(&h, "ST",  bg_ssdp_get_device_type_nt(s->local_dev, -1));
    flush_reply_packet(s, &h, q->addr);
    /* TODO: Embedded devices would come here */

    for(i = 0; i < s->local_dev->num_services; i++)
      {
      gavl_metadata_set_nocpy(&h, "USN", bg_ssdp_get_service_type_usn(s->local_dev, -1, i));
      gavl_metadata_set_nocpy(&h, "ST",  bg_ssdp_get_service_type_nt(s->local_dev, -1, i));
      flush_reply_packet(s, &h, q->addr);
      }
    gavl_metadata_free(&h);
    }
  else if(!strcasecmp(q->st, "upnp:rootdevice"))
    {
    /* Root device */
    gavl_metadata_set_nocpy(&h, "USN", bg_ssdp_get_root_usn(s->local_dev));
    gavl_metadata_set_nocpy(&h, "ST",  bg_ssdp_get_root_nt(s->local_dev));
    flush_reply_packet(s, &h, q->addr);
    }
  else if(gavl_string_starts_with_i(q->st, "uuid:"))
    {
    int dev = find_embedded_dev_by_uuid(s->local_dev, q->st+5);
    gavl_metadata_set_nocpy(&h, "USN", bg_ssdp_get_device_uuid_usn(s->local_dev, dev));
    gavl_metadata_set_nocpy(&h, "ST",  bg_ssdp_get_device_uuid_nt(s->local_dev, dev));
    flush_reply_packet(s, &h, q->addr);
    }
  else if((type_version = is_device_type(q->st)))
    {
    int dev;
    bg_ssdp_has_device_str(s->local_dev, type_version, &dev);
    gavl_metadata_set_nocpy(&h, "USN", bg_ssdp_get_device_type_usn(s->local_dev, dev));
    gavl_metadata_set_nocpy(&h, "ST",  bg_ssdp_get_device_type_nt(s->local_dev, dev));
    flush_reply_packet(s, &h, q->addr);
    }
  else if((type_version = is_service_type(q->st)))
    {
    int dev, srv;
    bg_ssdp_has_service_str(s->local_dev, type_version, &dev, &srv);
    gavl_metadata_set_nocpy(&h, "USN", bg_ssdp_get_service_type_usn(s->local_dev, dev, srv));
    gavl_metadata_set_nocpy(&h, "ST",  bg_ssdp_get_service_type_nt(s->local_dev, dev, srv));
    flush_reply_packet(s, &h, q->addr);
    }
  free(q->st);
  q->st = NULL;
  gavl_metadata_free(&h);
  }

static int flush_queue(bg_ssdp_t * s, gavl_time_t current_time)
  {
  int ret = 0;
  int i;
  for(i = 0; i < QUEUE_SIZE; i++)
    {
    if(!s->queue[i].st || (s->queue[i].time > current_time))
      continue;

    flush_reply(s, &s->queue[i]);
    ret++;
    }
  return ret;
  }


static void handle_multicast(bg_ssdp_t * s, const char * buffer,
                             bg_socket_address_t * sender, gavl_time_t current_time)
  {
  const char * var;
  gavl_metadata_t m;
  gavl_metadata_init(&m);
   
  if(!bg_http_request_from_string(&m, buffer))
    goto fail;
#ifdef DUMP_HEADERS
  gavl_dprintf("handle_multicast\n"); 
  gavl_metadata_dump(&m, 2);
#endif
  
  var = bg_http_request_get_method(&m);
  
  if(!var)
    goto fail;

  if(!strcasecmp(var, "M-SEARCH"))
    {
    int mx;
    const char * type_version;
    
    /* Got search request */
    if(!s->local_dev)
      goto fail;

    //    fprintf(stderr, "Got search request\n");
    //    gavl_metadata_dump(&m, 0);
    
    if(!gavl_metadata_get_int_i(&m, "MX", &mx))
      goto fail;
    
    var = gavl_metadata_get_i(&m, "ST");
    
    if(!strcasecmp(var, "ssdp:all"))
      {
      schedule_reply(s, var, sender,
                     current_time, mx);
      }
    else if(!strcasecmp(var, "upnp:rootdevice"))
      {
      schedule_reply(s, var, sender,
                     current_time, mx);
      }
    else if(gavl_string_starts_with_i(var, "uuid:"))
      {
      if(!strcmp(var + 5, s->local_dev->uuid) ||
         find_embedded_dev_by_uuid(s->local_dev, var + 5))
        schedule_reply(s, var, sender,
                       current_time, mx);
      }
    else if((type_version = is_device_type(var)))
      {
      if(bg_ssdp_has_device_str(s->local_dev, type_version, NULL))
        schedule_reply(s, var, sender,
                       current_time, mx);
      }
    else if((type_version = is_service_type(var)))
      {
      //      fprintf(stderr, "Got service search\n");
      if(bg_ssdp_has_service_str(s->local_dev, type_version, NULL, NULL))
        schedule_reply(s, var, sender,
                       current_time, mx);
      }
    
    /*
     *  Ignoring
     *  urn:domain-name:device:deviceType:v 
     *  and
     *  urn:domain-name:service:serviceType:v
     */
    
    }
  else if(!strcasecmp(var, "NOTIFY"))
    {
    const char * nt;
    const char * nts;
    /* Got notify request */
    if(!s->discover_remote)
      goto fail;

    nts = gavl_metadata_get_i(&m, "NTS");
    
    if(!strcmp(nts, "ssdp:alive"))
      {
      if(!(nt = gavl_metadata_get_i(&m, "NT")))
        goto fail;
      
      update_device(s, nt, &m);
      }
    else if(!strcmp(nts, "ssdp:byebye"))
      {
      /* We delete the entire root device */
      char * uuid;
      const char * usn;
      int i;

      if(!(usn = gavl_metadata_get(&m, "USN")) ||
         !(uuid = uuid_from_usn(usn)))
        goto fail;
      
      for(i = 0; i < s->num_remote_devs; i++)
        {
        if((s->remote_devs[i].uuid && !strcmp(s->remote_devs[i].uuid, uuid)) ||
           (find_embedded_dev_by_uuid(&s->remote_devs[i], uuid) >= 0))
          {
          del_remote_dev(s, i);
          break;
          }
        }
      free(uuid);
      }
    }
  
  fail:
  gavl_metadata_free(&m);
  }

static void handle_unicast(bg_ssdp_t * s, const char * buffer,
                           bg_socket_address_t * sender)
  {
  const char * st;
  gavl_metadata_t m;
  gavl_metadata_init(&m);

  if(!bg_http_response_from_string(&m, buffer))
    goto fail;

#ifdef DUMP_HEADERS
  gavl_dprintf("handle_unicast\n");
  gavl_metadata_dump(&m, 2);
#endif
  
  if(!(st = gavl_metadata_get_i(&m, "ST")))
    goto fail;
  
  update_device(s, st, &m);

  
  fail:
  gavl_metadata_free(&m);
  }

static void flush_notify(bg_ssdp_t * s, const gavl_metadata_t * h)
  {
  int len = 0;
  char * str = bg_http_request_to_string(h, &len);
  bg_udp_socket_send(s->ucast_fd, (uint8_t*)str, len, s->mcast_addr);
  //  fprintf(stderr, "Notify:\n%s", str);
  free(str);
  }

static void notify(bg_ssdp_t * s, int alive)
  {
  int i;
  gavl_metadata_t m;
  gavl_metadata_init(&m);

  /* Common fields */
  bg_http_request_init(&m, "NOTIFY", "*", "HTTP/1.1");
  gavl_metadata_set(&m, "HOST", "239.255.255.250:1900");

  if(alive)
    {
    gavl_metadata_set(&m, "CACHE-CONTROL", "max-age=1800");
    gavl_metadata_set(&m, "NTS", "ssdp:alive");
    }
  else
    {
    gavl_metadata_set(&m, "NTS", "ssdp:byebye");
    }
  
  gavl_metadata_set(&m, "SERVER", s->server_string);
  gavl_metadata_set(&m, "LOCATION", s->local_dev->url);
  
  /* Root device */
  gavl_metadata_set_nocpy(&m, "USN", bg_ssdp_get_root_usn(s->local_dev));
  gavl_metadata_set_nocpy(&m, "NT",  bg_ssdp_get_root_nt(s->local_dev));
  flush_notify(s, &m);
  
  /* Device UUID */
  gavl_metadata_set_nocpy(&m, "USN", bg_ssdp_get_device_uuid_usn(s->local_dev, -1));
  gavl_metadata_set_nocpy(&m, "NT",  bg_ssdp_get_device_uuid_nt(s->local_dev, -1));
  flush_notify(s, &m);
  
  /* TODO: Embedded devices would come here */

  /* Device Type */
  gavl_metadata_set_nocpy(&m, "USN", bg_ssdp_get_device_type_usn(s->local_dev, -1));
  gavl_metadata_set_nocpy(&m, "NT",  bg_ssdp_get_device_type_nt(s->local_dev, -1));
  flush_notify(s, &m);
  /* TODO: Embedded devices would come here */

  for(i = 0; i < s->local_dev->num_services; i++)
    {
    gavl_metadata_set_nocpy(&m, "USN", bg_ssdp_get_service_type_usn(s->local_dev, -1, i));
    gavl_metadata_set_nocpy(&m, "NT",  bg_ssdp_get_service_type_nt(s->local_dev, -1, i));
    flush_notify(s, &m);
    }
  gavl_metadata_free(&m);
  }

int bg_ssdp_update(bg_ssdp_t * s)
  {
  int len;
  int i;
  gavl_time_t current_time;
#ifdef DUMP_UDP
  char addr_str[BG_SOCKET_ADDR_STR_LEN];
#endif
  int ret = 0;
  
  /* Delete expired devices */
  current_time = gavl_timer_get(s->timer);
  i = 0;  
  while(i < s->num_remote_devs)
    {
    if(s->remote_devs[i].expire_time < current_time)
      del_remote_dev(s, i);
    else
      i++;
    }

  /* Read multicast messages */
  while(bg_socket_can_read(s->mcast_fd, 0))
    {
    len = bg_udp_socket_receive(s->mcast_fd, s->buf, UDP_BUFFER_SIZE, s->sender_addr);
    if(len <= 0)
      continue;
    s->buf[len] = '\0';
    handle_multicast(s, (const char *)s->buf, s->sender_addr, current_time);
#ifdef DUMP_UDP
    fprintf(stderr, "Got SSDP multicast message from %s\n%s",
            bg_socket_address_to_string(s->sender_addr, addr_str), (char*)s->buf);
#endif
    ret++;
    }

  /* Read unicast messages */
  while(bg_socket_can_read(s->ucast_fd, 0))
    {
    len = bg_udp_socket_receive(s->ucast_fd, s->buf, UDP_BUFFER_SIZE, s->sender_addr);
    if(len <= 0)
      continue;
    s->buf[len] = '\0';
    handle_unicast(s, (const char *)s->buf, s->sender_addr);
#ifdef DUMP_UDP
    fprintf(stderr, "Got SSDP unicast message from %s\n%s",
            bg_socket_address_to_string(s->sender_addr, addr_str), (char*)s->buf);
#endif
    ret++;
    }

#ifdef DUMP_DEVS
  if((current_time - s->last_dump_time > 10 * GAVL_TIME_SCALE) && s->discover_remote)
    {
    gavl_dprintf("Root devices: %d\n", s->num_remote_devs);
    for(i = 0; i < s->num_remote_devs; i++)
      bg_ssdp_root_device_dump(&s->remote_devs[i]);
    s->last_dump_time = current_time;
    }
#endif

  ret += flush_queue(s, current_time);

  if(s->local_dev &&
     ((s->last_notify_time == GAVL_TIME_UNDEFINED) ||
      (current_time - s->last_notify_time > NOTIFY_INTERVAL)))
    {
    notify(s, 1);
    s->last_notify_time = current_time;
    }
  
  return ret;
  }

void bg_ssdp_destroy(bg_ssdp_t * s)
  {
  int i;
  /* If we advertised a local device, unadvertise it */
  if(s->local_dev)
    {
    // notify(s, 0);
    notify(s, 0);
    }
  for(i = 0; i < QUEUE_SIZE; i++)
    {
    if(s->queue[i].addr)
      bg_socket_address_destroy(s->queue[i].addr);
    if(s->queue[i].st)
      free(s->queue[i].st);
    }

  if(s->remote_devs)
    {
    for(i = 0; i < s->num_remote_devs; i++)
      bg_ssdp_root_device_free(&s->remote_devs[i]);
    free(s->remote_devs);
    }

  close(s->ucast_fd);
  close(s->mcast_fd);
  
  if(s->local_addr) 
    bg_socket_address_destroy(s->local_addr);
  if(s->mcast_addr) 
    bg_socket_address_destroy(s->mcast_addr);
  if(s->sender_addr) 
    bg_socket_address_destroy(s->sender_addr);
  }

