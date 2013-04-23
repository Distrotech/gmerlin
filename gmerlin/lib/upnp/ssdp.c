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
#include <sys/utsname.h>

#include <gavl/gavl.h>
#include <gavl/metadata.h>

#include <gmerlin/upnp/ssdp.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "ssdp"

#include <gmerlin/utils.h>


// #define DUMP_UDP
#define UDP_BUFFER_SIZE 2048

#define META_METHOD "$METHOD"
#define META_STATUS "$STATUS"

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

  char * server_string;
  };

void
bg_ssdp_service_free(bg_ssdp_service_t * s)
  {
  if(s->type)
    free(s->type);
  }

void
bg_ssdp_device_free(bg_ssdp_device_t * dev)
  {
  int i;
  for(i = 0; i < dev->num_services; i++)
    bg_ssdp_service_free(&dev->services[i]);
  if(dev->services)
    free(dev->services);
  if(dev->type)
    free(dev->type);
  }

void
bg_ssdp_root_device_free(bg_ssdp_root_device_t * dev)
  {
  int i;
  for(i = 0; i < dev->num_services; i++)
    bg_ssdp_service_free(&dev->services[i]);
  if(dev->services)
    free(dev->services);

  for(i = 0; i < dev->num_devices; i++)
    bg_ssdp_device_free(&dev->devices[i]);
  if(dev->devices)
    free(dev->devices);

  if(dev->uuid)
    free(dev->uuid);
  if(dev->url)
    free(dev->url);
  }

static int 
find_remote_dev_by_location(bg_ssdp_t * s, const char * loc)
  {
  int i;
  for(i = 0; i < s->num_remote_devs; i++)
    {
    if(!strcmp(s->remote_devs[i].url, loc))
      return i;
    }
  return -1;
  }

static bg_ssdp_root_device_t *
add_remote_dev(bg_ssdp_t * s, const char * loc, const char * uuid)
  {
  bg_ssdp_root_device_t * ret;
  
  if(s->num_remote_devs + 1 > s->remote_devs_alloc)
    {
    s->remote_devs_alloc += 16;
    s->remote_devs = realloc(s->remote_devs, s->remote_devs_alloc * sizeof(s->remote_devs));
    memset(s->remote_devs + s->num_remote_devs, 0,
           (s->remote_devs_alloc - s->num_remote_devs) * sizeof(s->remote_devs));
    }
  ret = s->remote_devs + s->num_remote_devs;
  ret->uuid = gavl_strdup(uuid);
  ret->url = gavl_strdup(loc);
  s->num_remote_devs++;
  return ret;
  }

static void
del_remote_dev(bg_ssdp_t * s, int idx)
  {
  
  }


static char * search_string =
"M-SEARCH * HTTP/1.1\r\n"
"Host:239.255.255.250:1900\r\n"
"ST: ssdp:all\r\n"
"Man:\"ssdp:discover\"\r\n"
"MX:3\r\n\r\n";

bg_ssdp_t * bg_ssdp_create(bg_ssdp_root_device_t * local_dev,
                           int discover_remote)
  {
  struct utsname os_info;
  
  char addr_str[BG_SOCKET_ADDR_STR_LEN];

  bg_ssdp_t * ret = calloc(1, sizeof(*ret));

  ret->discover_remote = discover_remote;
  ret->local_dev = local_dev;
  
  ret->sender_addr = bg_socket_address_create();
  ret->local_addr  = bg_socket_address_create();
  ret->mcast_addr  = bg_socket_address_create();

  if(!bg_socket_address_set_local(ret->local_addr, 0))
    goto fail;

  if(!bg_socket_address_set(ret->mcast_addr, "239.255.255.250", 1900, SOCK_DGRAM))
    goto fail;

  ret->mcast_fd = bg_udp_socket_create_multicast(ret->mcast_addr);
  ret->ucast_fd = bg_udp_socket_create(ret->local_addr);

  uname(&os_info);
  ret->server_string = bg_sprintf("%s/%s, UPnP/1.0, "PACKAGE"/"VERSION,
                                  os_info.sysname, os_info.release);

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using server string: %s", ret->server_string);
  
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Unicast socket bound at %s", 
         bg_socket_address_to_string(ret->local_addr, addr_str));
  
  /* Send seach packet */
  bg_udp_socket_send(ret->ucast_fd, (uint8_t*)search_string, strlen(search_string), ret->mcast_addr);

  ret->timer = gavl_timer_create(ret->timer);
  gavl_timer_start(ret->timer);
  
  return ret;

  fail:
  bg_ssdp_destroy(ret);
  return NULL;
  }

static int parse_vars(char ** str, gavl_metadata_t * m)
  {
  char * pos;
  int i = 1;
  while(str[i])
    {
    pos = strchr(str[i], '\r');
    if(pos)
      *pos = '\0';
    
    if(*(str[i]) == '\0') // Last empty line
      break;
    
    pos = strchr(str[i], ':');
    if(!pos)
      return 0;
    *pos = '\0';
    pos++;

    while(isspace(*pos) && (*pos != '\0'))
      pos++;

    if(*pos != '\0') // Pos can be '\0' for empty "EXT:" field
      gavl_metadata_set(m, str[i], pos);
    
    i++;
    }
  return 1;
  }

static int parse_request(const char * buffer, gavl_metadata_t * m)
  {
  int ret = 0;
  char * pos;
  char ** str = bg_strbreak(buffer, '\n');

  pos = strchr(str[0], ' ');
  if(!pos)
    goto fail;

  *pos = '\0';

  gavl_metadata_set(m, META_METHOD, str[0]);
  
  ret = parse_vars(str, m);
  fail:
  bg_strbreak_free(str);
  return ret;
  }

static int parse_response(const char * buffer, gavl_metadata_t * m)
  {
  int ret = 0;
  int status;
  char * pos;
  char ** str = bg_strbreak(buffer, '\n');

  pos = strchr(str[0], ' ');
  if(!pos)
    goto fail;

  while(isspace(*pos) && (*pos != '\0'))
    pos++;

  if(*pos == '\0') // Pos can be '\0' for empty "EXT:" field
    goto fail;
    
  status = atoi(pos);

  if(status != 200)
    goto fail;
  
  ret = parse_vars(str, m);
  fail:
  bg_strbreak_free(str);
  return ret;
  }

static void update_device(bg_ssdp_t * s, const char * type,
                          gavl_metadata_t * m)
  {
  int idx;
  const char * loc;
  const char * usn;
  
  bg_ssdp_root_device_t * dev;
  
  loc = gavl_metadata_get_i(m, "LOCATION");
  if(!loc)
    return;

  usn = gavl_metadata_get_i(m, "USN");
  if(!usn)
    return;
  
  idx = find_remote_dev_by_location(s, loc);

  if(idx >= 0)
    dev = &s->remote_devices[idx];
  else
    {
    /* Extract UUID from USN */
    char * pos, * tmp_string;
    if(gavl_string_starts_with(usn, "uuid:"))
      {
      pos = strchr(usn+5, ':');
      if(!pos)
        pos = usn + 5 + strlen(usn + 5);
      tmp_string = gavl_strndup(usn+5, pos);
      dev = add_remote_dev(s, loc, tmp_string);
      }
    else // Unknown USN
      return;
    }

  /* Set expiry date */
  
  
  if(!strcasecmp(type, "upnp:rootdevice"))
    {
    
    }
  else if(gavl_string_starts_with_i(type, "uuid:"))
    {
    
    }
  else if(gavl_string_starts_with_i(type, "urn:schemas-upnp-org:device:"))
    {
    
    }
  else if(gavl_string_starts_with_i(type, "urn:schemas-upnp-org:service:"))
    {
    
    }
  
  }
                          

static void handle_multicast(bg_ssdp_t * s, const char * buffer,
                             bg_socket_address_t * sender)
  {
  const char * var;
  gavl_metadata_t m;
  gavl_metadata_init(&m);
  if(!parse_request(buffer, &m))
    goto fail;

  fprintf(stderr, "handle_multicast\n"); 
  gavl_metadata_dump(&m, 2);

  var = gavl_metadata_get_i(&m, META_METHOD);
  
  if(!var)
    goto fail;

  if(!strcasecmp(var, "M-SEARCH"))
    {
    /* Got search request */
    if(!s->local_dev)
      goto fail;

    var = gavl_metadata_get_i(&m, "ST");

    if(!strcasecmp(var, "ssdp:all"))
      {
      
      }
    else if(!strcasecmp(var, "upnp:rootdevice"))
      {
      
      }
    else if(gavl_string_starts_with_i(var, "uuid:"))
      {
      
      }
    else if(gavl_string_starts_with_i(var, "urn:schemas-upnp-org:device:"))
      {
      
      }
    else if(gavl_string_starts_with_i(var, "urn:schemas-upnp-org:service:"))
      {
      
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
  if(!parse_response(buffer, &m))
    goto fail;

  fprintf(stderr, "handle_unicast\n"); 
  gavl_metadata_dump(&m, 2);

  if(!(st = gavl_metadata_get_i(&m, "ST")))
    goto fail;
  
  update_device(s, st, &m);

  
  fail:
  gavl_metadata_free(&m);
  }

void bg_ssdp_update(bg_ssdp_t * s)
  {
  int len;
#ifdef DUMP_UDP
  char addr_str[BG_SOCKET_ADDR_STR_LEN];
#endif
  /* Read multicast messages */
  while(bg_socket_can_read(s->mcast_fd, 0))
    {
    len = bg_udp_socket_receive(s->mcast_fd, s->buf, UDP_BUFFER_SIZE, s->sender_addr);
    if(len <= 0)
      continue;
    s->buf[len] = '\0';
    handle_multicast(s, (const char *)s->buf, s->sender_addr);
#ifdef DUMP_UDP
    fprintf(stderr, "Got SSDP multicast message from %s\n%s",
            bg_socket_address_to_string(s->sender_addr, addr_str), (char*)s->buf);
#endif
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
    }


  }

void bg_ssdp_destroy(bg_ssdp_t * s)
  {
  if(s->local_addr) 
    bg_socket_address_destroy(s->local_addr);
  if(s->mcast_addr) 
    bg_socket_address_destroy(s->mcast_addr);
  if(s->sender_addr) 
    bg_socket_address_destroy(s->sender_addr);
  }

