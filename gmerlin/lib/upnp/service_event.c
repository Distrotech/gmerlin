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
#include <upnp/devicepriv.h>

#include <gmerlin/utils.h>
#include <gmerlin/http.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "upnp.event"

#include <string.h>
#include <unistd.h> // close

static char * create_event(bg_upnp_service_t * s, int * len)
  {
  char * ret;
  int i;
  xmlNodePtr propset;
  xmlNodePtr node;
  
  xmlNsPtr ns;
  xmlDocPtr doc = xmlNewDoc((xmlChar*)"1.0");
  propset = xmlNewDocRawNode(doc, NULL, (xmlChar*)"propertyset", NULL);
  xmlDocSetRootElement(doc, propset);
  ns = xmlNewNs(propset,
                (xmlChar*)"urn:schemas-upnp-org:event-1-0",
                (xmlChar*)"e");
  xmlSetNs(propset, ns);
  
  for(i = 0; i < s->num_event_vars; i++)
    {
    node = xmlNewChild(propset, ns, (xmlChar*)"property", NULL);
    switch(s->event_vars[i].sv->type)
      {
      case BG_UPNP_SV_INT4:
        {
        char buf[16];
        snprintf(buf, 16, "%d", s->event_vars[i].val.i);
        node = xmlNewChild(node, NULL, (xmlChar*)s->event_vars[i].sv->name, (xmlChar*)buf);
        xmlSetNs(node, NULL);
        }
        break;
      case BG_UPNP_SV_STRING:
        node = xmlNewChild(node, NULL, (xmlChar*)s->event_vars[i].sv->name,
                    (xmlChar*)(s->event_vars[i].val.s ?
                               s->event_vars[i].val.s : ""));
        xmlSetNs(node, NULL);
        break;
      }
    }
  ret = bg_xml_save_to_memory(doc);
  *len = strlen(ret);
  xmlFreeDoc(doc);
  return ret;
  }

static int send_event(bg_upnp_event_subscriber_t * es,
                      char * event, int len)
  {
  gavl_metadata_t m;
  bg_socket_address_t * addr = NULL;
  char * path = NULL;
  char * host = NULL;
  int port;
  int fd = -1;
  char * tmp_string;
  int result = 0;
  
  gavl_metadata_init(&m);
  
  /* Parse URL */
  if(!bg_url_split(es->url,
                   NULL, NULL, NULL,
                   &host, &port, &path))
    goto fail;
  
  addr = bg_socket_address_create();

  if(!bg_socket_address_set(addr, host, port, SOCK_STREAM))
    goto fail;
  
  if((fd = bg_socket_connect_inet(addr, 500)) < 0)
    goto fail;

  if(path)
    bg_http_request_init(&m, "NOTIFY", path, "HTTP/1.1");
  else
    bg_http_request_init(&m, "NOTIFY", "/", "HTTP/1.1");
    
  tmp_string = bg_sprintf("%s:%d", host, port);
  gavl_metadata_set(&m, "HOST", tmp_string);
  free(tmp_string);
  
  gavl_metadata_set(&m, "CONTENT-TYPE", "text/xml");
  gavl_metadata_set_int(&m, "CONTENT-LENGTH", len);
  gavl_metadata_set(&m, "NT", "upnp:event");
  gavl_metadata_set(&m, "NTS", "upnp:propchange");
  tmp_string = bg_sprintf("uuid:%s", es->uuid);  
  gavl_metadata_set(&m, "SID", tmp_string);
  free(tmp_string);
  gavl_metadata_set_long(&m, "SEQ", es->key++);

  //  fprintf(stderr, "Sending event: %s (%d %d)\n", es->url, len, strlen(event));
  gavl_metadata_dump(&m, 0);
  fprintf(stderr, "%s\n", event);
  
  if(!bg_http_request_write(fd, &m))
    {
    fprintf(stderr, "Writing header failed\n");
    goto fail;
    }
  if(!bg_socket_write_data(fd, (uint8_t*)event, len))
    {
    fprintf(stderr, "Writing event failed\n");
    goto fail;
    }
  gavl_metadata_free(&m);
  gavl_metadata_init(&m);
  
  if(!bg_http_response_read(fd, &m, 10000))
    goto fail;
  fprintf(stderr, "Got response:\n");
  gavl_metadata_dump(&m, 0);

  if(bg_http_response_get_status_int(&m) != 200)
    goto fail;
  result = 1;
  fail:

  gavl_metadata_free(&m);
  if(path)
    free(path);
  if(host)
    free(host);
  if(fd >= 0)
    close(fd);
  if(addr)
    bg_socket_address_destroy(addr);
  
  return result;
  }

static int get_timeout_seconds(const char * timeout, int * ret)
  {
  if(!strncmp(timeout, "Second-", 7))
    {
    *ret = atoi(timeout + 7);
    return 1;
    }
  else if(!strcmp(timeout, "infinite"))
    {
    *ret = 1800; // We don't like infinite subscriptions
    return 1;
    }
  return 0;
  }

static int add_subscription(bg_upnp_service_t * s, int fd,
                            const char * callback, const char * timeout)
  {
  const char * start, *end;
  uuid_t uuid;
  bg_upnp_event_subscriber_t * es;
  gavl_metadata_t res;
  int seconds;
  int result = 0;
  char * event = NULL;
  int len;
  
  gavl_metadata_init(&res);

  if(s->num_es + 1 > s->es_alloc)
    {
    s->es_alloc += 8;
    s->es = realloc(s->es, s->es_alloc * sizeof(*s->es));
    memset(s->es + s->num_es, 0, (s->es_alloc - s->num_es) * sizeof(*s->es));
    }
  es = s->es + s->num_es;
  memset(es, 0, sizeof(*es));
  
  /* UUID */
  uuid_generate(uuid);
  uuid_unparse(uuid, es->uuid);
  
  /* Timeout */

  if(!get_timeout_seconds(timeout, &seconds))
    {
    bg_http_response_init(&res, "HTTP/1.1", 400, "Bad Request");
    goto fail;
    }
  es->expire_time = gavl_timer_get(s->dev->timer) + seconds * GAVL_TIME_SCALE;
  
  /* Callback */
  start = strchr(callback, '<');
  if(!start)
    {
    bg_http_response_init(&res, "HTTP/1.1", 412, "Precondition Failed");
    goto fail;
    }
  start++;
  end = strchr(callback, '>');
  if(!end)
    {
    bg_http_response_init(&res, "HTTP/1.1", 412, "Precondition Failed");
    goto fail;
    }
  es->url = gavl_strndup(start, end);

  bg_http_response_init(&res, "HTTP/1.1", 200, "OK");
  gavl_metadata_set(&res, "SERVER", s->dev->server_string);
  gavl_metadata_set_nocpy(&res, "SID", bg_sprintf("uuid:%s", es->uuid));
  gavl_metadata_set_nocpy(&res, "TIMEOUT", bg_sprintf("Second-%d", seconds));
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got new event subscription: uuid: %s, url: %s",
         es->uuid, es->url);

  bg_http_response_write(fd, &res);  

  event = create_event(s, &len);
  fprintf(stderr, "%s", event);

  result = send_event(es, event, len);
  
  fail:

  if(!result)
    {
    if(es->url)
      free(es->url);
    bg_http_response_write(fd, &res);
    }
  else
    s->num_es++;

  if(event)
    free(event);
  gavl_metadata_free(&res);
  return result;
  }

static int find_subscription(bg_upnp_service_t * s, const char * sid)
  {
  int i;
  for(i = 0; i < s->num_es; i++)
    {
    if(!strcmp(s->es[i].uuid, sid + 5))
      return i;
    }
  return -1;
  }

static void delete_subscription(bg_upnp_service_t * s, int idx)
  {
  if(s->es[idx].url)
    free(s->es[idx].url);
  if(idx < s->num_es - 1)
    memmove(s->es + idx, s->es + idx + 1, (s->num_es - 1 - idx) * sizeof(*s->es));
  s->num_es--;
  }

int
bg_upnp_service_handle_event_request(bg_upnp_service_t * s, int fd,
                                     const char * method,
                                     const gavl_metadata_t * header)
  {
  gavl_metadata_t res;
  const char * nt;
  const char * callback;
  const char * timeout;
  const char * sid;
  
  nt =       gavl_metadata_get(header, "NT");
  callback = gavl_metadata_get(header, "CALLBACK");
  timeout =  gavl_metadata_get(header, "TIMEOUT");
  sid =      gavl_metadata_get(header, "SID");

  gavl_metadata_init(&res);
  
  if(!strcmp(method, "SUBSCRIBE"))
    {
    if(nt && callback && !sid) // New subscription
      {
      if(!strcmp(nt, "upnp:event"))
        {
        add_subscription(s, fd, callback, timeout);
        return 1;
        }
      else
        {
        bg_http_response_init(&res, "HTTP/1.1", 412, "Precondition Failed");
        }
      }
    else if(!nt && !callback && sid) // Renew subscription
      {
      int seconds = 1800;
      int idx = find_subscription(s, sid);

      if(idx < 0)
        bg_http_response_init(&res, "HTTP/1.1", 412, "Precondition Failed");
      else if(timeout && !get_timeout_seconds(timeout, &seconds))
        bg_http_response_init(&res, "HTTP/1.1", 400, "Bad Request");
      else
        {
        bg_http_response_init(&res, "HTTP/1.1", 200, "Ok");
        s->es[idx].expire_time = gavl_timer_get(s->dev->timer) + seconds * GAVL_TIME_SCALE;
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Renewing subscription: uuid: %s, url: %s",
               s->es[idx].uuid, s->es[idx].url);
        }
      }
    else
      {
      bg_http_response_init(&res, "HTTP/1.1", 400, "Bad Request");
      }
    }
  else if(!strcmp(method, "UNSUBSCRIBE"))
    {
    if(sid)
      {
      int idx = find_subscription(s, sid);
      if(idx >= 0)
        {
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got unsubscription: uuid: %s, url: %s",
               s->es[idx].uuid, s->es[idx].url);
        delete_subscription(s, idx);
        bg_http_response_init(&res, "HTTP/1.1", 200, "Ok");
        }
      else
        bg_http_response_init(&res, "HTTP/1.1", 412, "Precondition Failed");
      }
    else
      bg_http_response_init(&res, "HTTP/1.1", 400, "Bad Request");
    }
  else
    bg_http_response_init(&res, "HTTP/1.1", 400, "Bad Request");
  
  bg_http_response_write(fd, &res);
  gavl_metadata_free(&res);
  return 1;
  }
