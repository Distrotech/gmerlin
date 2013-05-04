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

//#include <upnp_service.h>
#include <upnp_device.h>

#include <gmerlin/utils.h>
#include <gmerlin/http.h>

#include <string.h>

static void add_subscription(bg_upnp_service_t * s, int fd,
                            const char * callback, const char * timeout)
  {
  const char * start, *end;
  uuid_t uuid;
  bg_upnp_event_subscriber_t * es;
  gavl_metadata_t res;
  int seconds;
  int result = 0;

  gavl_metadata_init(&res);

  if(s->num_es + 1 > s->es_alloc)
    {
    s->es_alloc += 8;
    s->es = realloc(s->es, s->es_alloc * sizeof(*s->es));
    memset(s->es + s->num_es, 0, (s->es_alloc - s->num_es) * sizeof(*s->es));
    }
  es = s->es + s->num_es;

  /* UUID */
  uuid_generate(uuid);
  uuid_unparse(uuid, es->uuid);
  
  /* Timeout */
  if(!strncmp(timeout, "Second-", 7))
    seconds = atoi(timeout + 7);
  else if(!strcmp(timeout, "infinite"))
    seconds = 1800;
  else
    goto fail;
  
  es->expire_time = gavl_timer_get(s->dev->timer) + seconds * GAVL_TIME_SCALE;
  
  /* Callback */
  start = strchr(callback, '<');
  if(!start)
    goto fail;
  
  start++;
  end = strchr(callback, '>');
  if(!end)
    goto fail;
  es->url = gavl_strndup(start, end);

  bg_http_response_init(&res, "HTTP/1.1", 200, "OK");
  gavl_metadata_set(&res, "SERVER", s->dev->server_string);
  gavl_metadata_set_nocpy(&res, "SID", bg_sprintf("uuid:%s", es->uuid));
  gavl_metadata_set_nocpy(&res, "TIMEOUT", bg_sprintf("Second-%d", seconds));
  s->num_es++;
  bg_http_response_write(fd, &res);
  result = 1;
  
  fail:

  if(!result)
    {
    bg_http_response_init(&res, "HTTP/1.1", 400, "Bad Request");
    bg_http_response_write(fd, &res);
    }
  gavl_metadata_free(&res);
  
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

  nt = gavl_metadata_get(header, "NT");
  callback = gavl_metadata_get(header, "CALLBACK");
  timeout = gavl_metadata_get(header, "TIMEOUT");
  sid = gavl_metadata_get(header, "SID");

  gavl_metadata_init(&res);
  
  if(!strcmp(method, "SUBSCRIBE"))
    {
    if(nt && callback && !sid) // New subscription
      {
      if(!strcmp(nt, "upnp:event"))
        add_subscription(s, fd, callback, timeout);
      else
        {
        bg_http_response_init(&res, "HTTP/1.1", 412, "Precondition Failed");
        }
      }
    else if(!nt && !callback && sid) // Renew subscription
      {
      
      }
    else
      {
      bg_http_response_init(&res, "HTTP/1.1", 400, "Bad Request");
      
      }
    
    }
  return 1;
  }
