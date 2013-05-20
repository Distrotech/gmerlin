/*****************************************************************
 * Gmerlin - a general purpose multimedia framework and applications
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

#include "server.h"

#include <string.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/http.h>

#include <unistd.h>

#define LOG_DOMAIN "server"

#define TIMEOUT GAVL_TIME_SCALE/2
#define ID3_CACHE_SIZE 50

#define ONDEMAND_IDLE_TIME (30*GAVL_TIME_SCALE)

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =      "db",
      .long_name = TRS("Database path"),
      .type = BG_PARAMETER_DIRECTORY,
      .val_default = { .val_str = "." },
    },
    {
      .name =      "port",
      .type = BG_PARAMETER_INT,
      .long_name =  TRS("Listen port"),
      .val_default = { .val_i = 0 },
      .val_min     = { .val_i = 0 },
      .val_max     = { .val_i = 65535 },
    },
    { /* End */ },
  };

const bg_parameter_info_t * server_get_parameters()
  {
  return parameters;
  }

void server_set_parameter(void * priv, const char * name, const bg_parameter_value_t * val)
  {
  server_t * s = priv;
  if(!name)
    return;
  if(!strcmp(name, "db"))
    s->dbpath = gavl_strrep(s->dbpath, val->val_str);
  else if(!strcmp(name, "port"))
    s->port = val->val_i;
  }

void server_init(server_t * s)
  {
  memset(s, 0, sizeof(*s));
  pthread_mutex_init(&s->clients_mutex, NULL);
  s->timer = gavl_timer_create();
  }

int server_start(server_t * s)
  {
  char addr_str[BG_SOCKET_ADDR_STR_LEN];
  char * tmp_path;
  bg_cfg_section_t * cfg_section;
  int i;

  /* Create plugin registry */

  s->cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(s->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(s->cfg_reg, "plugins");
  s->plugin_reg = bg_plugin_registry_create(cfg_section);
  
  /* Create listen socket: After that we'll have the root URL we can
     advertise */
  
  s->addr = bg_socket_address_create();

  if(!bg_socket_address_set_local(s->addr, s->port))
    return 0;

  s->fd = bg_listen_socket_create_inet(s->addr, 0 /* Port */, 10 /* queue_size */,
                                       0 /* flags */);

  if(s->fd < 0)
    return 0;

  if(!bg_socket_get_address(s->fd, s->addr, NULL))
    return 0;

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Socket listening on %s",
         bg_socket_address_to_string(s->addr, addr_str));

  s->root_url = bg_sprintf("http://%s", addr_str);

  /* TODO: Remember UUID */
  uuid_clear(s->uuid);
  //  uuid_generate(s->uuid);
  uuid_parse("41491152-4894-43fe-bf88-307b6aa6eb45", s->uuid);
    
  /* Create DB */
  s->db = bg_db_create(s->dbpath,
                       s->plugin_reg, 0);
  if(!s->db)
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "No database found");
  else
    {
    /* Create UPNP device(s) */
    s->dev = bg_upnp_create_media_server(s->addr,
                                         s->uuid,
                                         "Gmerlin media server",
                                         (const bg_upnp_icon_t *)0,
                                         s->db);
    s->server_string = bg_upnp_device_get_server_string(s->dev);
    s->id3_cache = calloc(ID3_CACHE_SIZE, sizeof(*s->id3_cache));
    s->id3_cache_size = ID3_CACHE_SIZE;

    for(i = 0; i < ID3_CACHE_SIZE; i++)
      {
      s->id3_cache[i].last_used = GAVL_TIME_UNDEFINED;
      s->id3_cache[i].id = -1;
      }
    }
  
  gavl_timer_start(s->timer);
  return 1;
  }

static void send_404(int fd)
  {
  gavl_metadata_t res;
  gavl_metadata_init(&res);
  bg_http_response_init(&res,
                        "HTTP/1.1",
                        404, "Not Found");
  
  bg_http_response_write(fd, &res);
  gavl_metadata_free(&res);
  }

static void handle_client_connection(server_t * s, int fd)
  {
  const char * method;
  const char * path;
  const char * protocol;
  
  gavl_metadata_t req;
  gavl_metadata_init(&req);
  if(!bg_http_request_read(fd, &req, TIMEOUT))
    goto fail;

  fprintf(stderr, "Got request\n");
  gavl_metadata_dump(&req, 2);
  
  method = bg_http_request_get_method(&req);
  path = bg_http_request_get_path(&req);
  protocol = bg_http_request_get_protocol(&req);

  /* Try to handle things */

  if(!bg_upnp_device_handle_request(s->dev, fd, method, path, &req) &&
     !server_handle_media(s, &fd, method, path, &req) &&
     !server_handle_transcode(s, &fd, method, path, &req) &&
     !server_handle_source(s, &fd, method, path, &req) &&
     !server_handle_stream(s, &fd, method, path, &req) &&
     !server_handle_ondemand(s, &fd, method, path, &req) &&
     !server_handle_static(s, &fd, method, path, &req))
    send_404(fd);
  
  fail:
  if(fd >= 0)
    {
    fprintf(stderr, "Closing socket\n");
    close(fd);
    }
  gavl_metadata_free(&req);
  }

void server_attach_client(server_t * s, client_t*cl)
  {
  pthread_mutex_lock(&s->clients_mutex);
  if(s->num_clients + 1 >= s->clients_alloc)
    {
    s->clients_alloc += 16;
    s->clients = realloc(s->clients, s->clients_alloc * sizeof(*s->clients));
    memset(s->clients + s->num_clients, 0,
           (s->clients_alloc - s->num_clients) * sizeof(*s->clients));
    }
  s->clients[s->num_clients] = cl;
  s->num_clients++;
  pthread_mutex_unlock(&s->clients_mutex);
  }

void server_detach_client(server_t * s, client_t*cl)
  {
  pthread_mutex_lock(&s->clients_mutex);
  
  pthread_mutex_unlock(&s->clients_mutex);
  }

static void server_destroy_client(server_t * s, int idx)
  {
  int i = idx+1;
  while(i < s->num_clients) // We assume that sinks always come after their source
    {
    if(s->clients[i]->source == s->clients[idx])
      server_destroy_client(s, i);
    else
      i++;
    }
  client_destroy(s->clients[idx]);

  if(idx < s->num_clients - 1)
    {
    memmove(s->clients + idx,
            s->clients + idx + 1,
           (s->num_clients - 1 - idx) * sizeof(*s->clients));
    }
  s->num_clients--;
  }

int server_iteration(server_t * s)
  {
  int ret = 0;
  int fd;
  int i, j;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 100; // 10 ms

  s->current_time = gavl_timer_get(s->timer);  
  /* Remove dead clients */

  i = 0;
  while(i < s->num_clients)
    {
    if(client_get_status(s->clients[i]) == CLIENT_STATUS_DONE)
      server_destroy_client(s, i);
    else
      i++;
    }

  /* Remove on-demand sources with zero sinks */

  i = 0;
  while(i < s->num_clients)
    {
    if(s->clients[i]->type == CLIENT_TYPE_SOURCE_ONDEMAND)
      {
      int have_clients = 0;
      j = 0;
      
      while(j < s->num_clients)
        {
        if(s->clients[j]->source == s->clients[i])
          {
          have_clients = 1;
          break;
          }
        j++;
        }
      if(!have_clients)
        {
        if(s->clients[i]->idle_time == GAVL_TIME_UNDEFINED)
          s->clients[i]->idle_time = s->current_time;
        else if(s->current_time - s->clients[i]->idle_time > ONDEMAND_IDLE_TIME)
          {
          bg_log(BG_LOG_INFO, LOG_DOMAIN, "Shutting down ondemand stream (idle timeout)");
          server_destroy_client(s, i);
          continue;
          }
        }
      else
        s->clients[i]->idle_time = GAVL_TIME_UNDEFINED;
      }
    i++;
    }

  
  /* Handle incoming connections */

  while((fd = bg_listen_socket_accept(s->fd, 0)) >= 0)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got client connection");
    handle_client_connection(s, fd);
    ret++;
    }
  
  /* Ping upnp device so it can do it's ssdp stuff */
  ret += bg_upnp_device_ping(s->dev);

  if(!ret)
    gavl_time_delay(&delay_time);
  
  return 1;
  }

void server_cleanup(server_t * s)
  {
  int i;
  if(s->dev)
    bg_upnp_device_destroy(s->dev);
  if(s->db)
    bg_db_destroy(s->db);
  if(s->dbpath)
    free(s->dbpath);
  if(s->root_url)
    free(s->root_url);

  if(s->addr)
    bg_socket_address_destroy(s->addr);
  
  for(i = 0; i < s->num_clients; i++)
    client_destroy(s->clients[i]);
  if(s->clients)
    free(s->clients);
  if(s->timer)
    gavl_timer_destroy(s->timer);

  if(s->id3_cache)
    {
    for(i = 0; i < s->id3_cache_size; i++)
      {
      if(s->id3_cache[i].data)
        free(s->id3_cache[i].data);
      }
    free(s->id3_cache);
    }

  bg_plugin_registry_destroy(s->plugin_reg);
  bg_cfg_registry_destroy(s->cfg_reg);
  }

