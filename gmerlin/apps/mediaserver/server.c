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

#include "server.h"

#include <string.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/http.h>

#include <unistd.h>

#define LOG_DOMAIN "server"

#define TIMEOUT GAVL_TIME_SCALE/2

int server_init(server_t * s)
  {
  bg_socket_address_t * a;
  char addr_str[BG_SOCKET_ADDR_STR_LEN];
  
  memset(s, 0, sizeof(*s));

  /* Create listen socket: After that we'll have the root URL we can
     advertise */
  
  a = bg_socket_address_create();

  if(!bg_socket_address_set_local(a, 0))
    return 0;

  s->fd = bg_listen_socket_create_inet(a, 0 /* Port */, 10 /* queue_size */,
                                       0 /* flags */);

  if(s->fd < 0)
    return 0;

  if(!bg_socket_get_address(s->fd, a, NULL))
    return 0;

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Socket listening on %s",
         bg_socket_address_to_string(a, addr_str));

  s->root_url = bg_sprintf("http://%s", addr_str);

  /* Create UPNP device(s) */
  
  return 1;
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

  method = bg_http_request_get_method(&req);
  path = bg_http_request_get_path(&req);
  protocol = bg_http_request_get_protocol(&req);

  /* Try to handle things */

  
  
  fail:
  if(fd >= 0)
    close(fd);
  gavl_metadata_free(&req);
  }

int server_iteration(server_t * s)
  {
  int fd;

  /* Remove dead clients */

  /* Handle incoming connections */

  while((fd = bg_listen_socket_accept(s->fd, 0)) >= 0)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got client connection");
    handle_client_connection(s, fd);
    }
  
  
  /* Ping upnp device so it can do it's ssdp stuff */
  
  return 1;
  }

void server_cleanup(server_t * s)
  {
  
  }
