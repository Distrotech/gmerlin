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

  pthread_mutex_init(&ret->program_mutex, NULL);
  
  
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
      char * name = gavl_strdup(ret->listen_addresses[i]+7);
      bg_url_get_vars(name, NULL);
      
      ret->listen_sockets[i] =
        bg_listen_socket_create_unix(name, 10);
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
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Listening at %s",
           ret->listen_addresses[i]);
    }
  
  return ret;
  fail:
  server_destroy(ret);
  return NULL;
  }

static program_t * find_program(server_t * s, const char * name)
  {
  int i;
  program_t * ret = NULL;
  
  pthread_mutex_lock(&s->program_mutex);

  for(i = 0; i < s->num_programs; i++)
    {
    if(!strcmp(s->programs[i]->name, name))
      {
      ret = s->programs[i];
      break;
      }
    }
  pthread_mutex_unlock(&s->program_mutex);
  return ret;
  }

static void append_program(server_t * s, program_t * p)
  {
  pthread_mutex_lock(&s->program_mutex);

  if(s->num_programs+1 > s->programs_alloc)
    {
    s->programs_alloc += 16;
    s->programs = realloc(s->programs,
                          s->programs_alloc * sizeof(*s->programs));

    memset(s->programs + s->num_programs, 0,
           (s->programs_alloc - s->num_programs) * sizeof(*s->programs));
    }
  
  s->programs[s->num_programs] = p;
  s->num_programs++;
  pthread_mutex_unlock(&s->program_mutex);
  }

static void remove_program(server_t * s, program_t * p)
  {
  int i;
  pthread_mutex_lock(&s->program_mutex);

  for(i = 0; i < s->num_programs; i++)
    {
    if(p == s->programs[i])
      {
      if(i < s->num_programs - 1)
        {
        memmove(&s->programs[i], &s->programs[i+1],
                sizeof(s->programs[i]) * (s->num_programs - 1 - i));
        }
      s->num_programs--;
      break;
      }
    }
  
  pthread_mutex_unlock(&s->program_mutex);
  }

static void handle_client_connection(server_t * s, int fd)
  {
  int method;
  int status = 0;

  char * location;
  const char * var;

  program_t * p;
  gavl_metadata_t req;
  gavl_metadata_t res;
  gavl_metadata_t vars;
  int write_response_now = 0;

  gavl_metadata_init(&req);
  gavl_metadata_init(&res);
  gavl_metadata_init(&vars);
  
  if(!bg_plug_request_read(fd, &req, CLIENT_TIMEOUT))
    goto fail;

  /* Set common fields */
  gavl_metadata_set(&res, "Server", bg_plug_app_id);
  
  if(!bg_plug_request_get_method(&req, &method))
    {
    status = BG_PLUG_IO_STATUS_400;
    goto fail;
    }
  if(!(var = bg_plug_request_get_location(&req)))
    {
    status = BG_PLUG_IO_STATUS_400;
    goto fail;
    }

  location = gavl_strdup(var);
  bg_url_get_vars(location, &vars);
  
  if(!strcmp(location, "/"))
    {
    status = BG_PLUG_IO_STATUS_404;
    goto fail;
    }

  p = find_program(s, location);
  
  switch(method)
    {
    case BG_PLUG_IO_METHOD_READ:
      if(!p)
        {
        status = BG_PLUG_IO_STATUS_404;
        goto fail;
        }

      bg_plug_response_set_status(&res, BG_PLUG_IO_STATUS_200);
      program_attach_client(p, fd, &req, &res, &vars);
      fd = -1;
      break;
    case BG_PLUG_IO_METHOD_WRITE:
      if(p)
        {
        status = BG_PLUG_IO_STATUS_423; // Locked
        goto fail;
        }

      var = gavl_metadata_get(&req, "Expect");
      if(var)
        {
        if(!strcmp(var, "100-continue"))
          write_response_now = 1;
        else
          {
          status = BG_PLUG_IO_STATUS_417;
          goto fail;
          }
        }
      if(write_response_now)
        {
        bg_plug_response_set_status(&res, BG_PLUG_IO_STATUS_100);
        if(!bg_plug_response_write(fd, &res))
          {
          bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing response failed");
          goto fail;
          }
        }
      else
        bg_plug_response_set_status(&res, BG_PLUG_IO_STATUS_200);
      
      p = program_create_from_socket(location, fd, &vars);
      if(!p)
        goto fail;
      append_program(s, p);
      fd = -1;
      break;
    default:
      status = BG_PLUG_IO_STATUS_405;
      goto fail;
      
    }
  
  fail:

  if(status && (fd >= 0))
    {
    bg_plug_response_set_status(&res, status);
    if(!bg_plug_response_write(fd, &res))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing response failed");
    }
  
  gavl_metadata_free(&req);
  gavl_metadata_free(&res);
  gavl_metadata_free(&vars);

  if(location)
    free(location);
  
  if(fd >= 0)
    close(fd);
  }



int server_iteration(server_t * s)
  {
  int i;
  int fd;

  /* Remove dead programs */
  i = 0;
  while(i < s->num_programs)
    {
    program_t * p = s->programs[i];
    
    if(program_get_status(p) == PROGRAM_STATUS_DONE)
      {
      remove_program(s, p);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing dead program %s",
             p->name);
      program_destroy(p);
      }
    else
      i++;
    }
  
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

  pthread_mutex_destroy(&s->program_mutex);

  for(i = 0; i < s->num_listen_sockets; i++)
    {
    close(s->listen_sockets[i]);
    if(!strncmp(s->listen_addresses[i], "unix://", 7))
      {
      char * name = gavl_strdup(s->listen_addresses[i]+7);
      bg_url_get_vars(name, NULL);
      
      unlink(name);
      free(name);
      }
    }

  if(s->listen_sockets)
    free(s->listen_sockets);
  
  while(s->num_programs)
    {
    program_t * p = s->programs[0];
    remove_program(s, s->programs[0]);
    program_destroy(p);
    }
  if(s->programs)
    free(s->programs);
  free(s);
  
  }
