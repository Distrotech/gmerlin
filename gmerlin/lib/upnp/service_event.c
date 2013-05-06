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
  s->num_es++;
  result = 1;
  
  fail:

  if(!result && es->url)
    free(es->url);
  bg_http_response_write(fd, &res);
  gavl_metadata_free(&res);
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
