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

#include "gavf-server.h"
#include <gmerlin/bgsocket.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_DOMAIN "gavf-server.server"

server_t * server_create(char ** listen_addresses,
                         int num_listen_addresses)
  {
  int i;
  server_t * ret = calloc(1, sizeof(*ret));

  ret->listen_addresses = listen_addresses;
  ret->num_listen_sockets = num_listen_addresses;
  ret->listen_sockets = malloc(ret->num_listen_sockets *
                               sizeof(ret->listen_sockets));

  for(i = 0; i < ret->num_listen_sockets; i++)
    ret->listen_sockets[i] = -1;
  
  for(i = 0; i < ret->num_listen_sockets; i++)
    {
    if(!strncmp(ret->listen_addresses[i], "tcp://", 6))
      {
      bg_host_address_t * a = NULL;
      int port;
      char * host = NULL;

      if(!bg_url_split(ret->listen_addresses[i],
                       NULL,
                       NULL,
                       NULL,
                       &host,
                       &port,
                       NULL))
        {
        if(host)
          free(host);
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "Invalid TCP address %s", ret->listen_addresses[i]);
        goto fail;
        }

      a = bg_host_address_create();
      if(!bg_host_address_set(a, host,
                              port, SOCK_STREAM))
        {
        bg_host_address_destroy(a);
        goto fail;
        }

      ret->listen_sockets[i] = bg_listen_socket_create_inet(a, 0, 10, 0);
      bg_host_address_destroy(a);

      if(ret->listen_sockets[i] < 0)
        goto fail;
      }
    else if(!strncmp(ret->listen_addresses[i], "unix://", 7))
      {
      char * name = NULL;

      if(!bg_url_split(ret->listen_addresses[i],
                       NULL,
                       NULL,
                       NULL,
                       &name,
                       NULL,
                       NULL))
        {
        if(name)
          free(name);
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "Invalid UNIX domain address address %s",
               ret->listen_addresses[i]);
        return NULL;
        }
      ret->listen_sockets[i] = bg_listen_socket_create_unix(name, 10);
      free(name);
      
      if(ret->listen_sockets[i] < 0)
        goto fail;

      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Invalid listen address %s", ret->listen_addresses[i]);
      goto fail;
      }
    }
  
  return ret;
  fail:
  server_destroy(ret);
  return NULL;
  }

static void handle_client_connection(server_t * s, int fd)
  {
  close(fd);
  }

int server_iteration(server_t * s)
  {
  int i;
  int fd;

  for(i = 0; i < s->num_listen_sockets; i++)
    {
    if((fd = bg_listen_socket_accept(s->listen_sockets[i], 0)) >= 0)
      {
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Got client connection");
      handle_client_connection(s, fd);
      }
    }
  return 1;
  }

void server_destroy(server_t * s)
  {
  int i;
  for(i = 0; i < s->num_listen_sockets; i++)
    {
    close(s->listen_sockets[i]);
    if(!strncmp(s->listen_addresses[i], "unix://", 7))
      {
      char * name = NULL;

      bg_url_split(s->listen_addresses[i],
                   NULL,
                   NULL,
                   NULL,
                   &name,
                   NULL,
                   NULL);
      unlink(name);
      free(name);
      }
    }
  }
