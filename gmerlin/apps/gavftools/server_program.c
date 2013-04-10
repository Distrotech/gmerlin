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
#include <string.h>

#define BUF_ELEMENTS 20

static client_t * element_used(program_t * p, buffer_element_t * el)
  {
  int i;
  for(i = 0; i < p->num_clients; i++)
    {
    if(p->clients[i]->cur == el)
      return p->clients[i];
    }
  return NULL;
  }

static int program_iteration(program_t * p)
  {
  /* Kill dead clients: We need at least 2 elements in the pool */
  
  while(!p->buf->pool || !p->buf->pool->next)
    {
    client_t * cl;

    while((cl = element_used(p, p->buf->first)))
      {
      
      }
    buffer_advance(p->buf);
    }
  
  /* Read packet from source */

  if(!bg_mediaconnector_iteration(&p->conn))
    return 0;
  
  /* Distribute to clients */

  /* Throw away old packets */

  while(!element_used(p, p->buf->first))
    buffer_advance(p->buf);
  
  /* Delay */

  return 1;
  }


static void * thread_func(void * priv)
  {
  program_t * p = priv;
  while(program_iteration(p))
    ;
  return NULL;
  }

static int conn_write_func(void * priv, const uint8_t * data, int len)
  {
  return len;
  }

static int conn_cb_func(void * priv, int type, const void * data)
  {
  buffer_element_t * el;
  program_t * p = priv;
  
  switch(type)
    {
    case GAVF_IO_CB_PROGRAM_HEADER_START:
      fprintf(stderr, "Got program header:\n");
      gavf_program_header_dump(data);

      if(!(p->flags & PROGRAM_HAVE_HEADER))
        {
        gavf_program_header_copy(&p->ph, data);
        p->flags |= PROGRAM_HAVE_HEADER;
        }
      break;
    case GAVF_IO_CB_PROGRAM_HEADER_END:
      break;
    case GAVF_IO_CB_PACKET_START:
      fprintf(stderr, "Got packet:\n");
      gavl_packet_dump(data);

      

      break;
    case GAVF_IO_CB_PACKET_END:
      break;
    case GAVF_IO_CB_METADATA_START:
      fprintf(stderr, "Got metadata:\n");
      gavl_metadata_dump(data, 0);
      break;
    case GAVF_IO_CB_METADATA_END:
      break;
    case GAVF_IO_CB_SYNC_HEADER_START:
      fprintf(stderr, "Got sync_header\n");
      break;
    case GAVF_IO_CB_SYNC_HEADER_END:
      break;
    }
  
  return 1;
  }

program_t * program_create_from_socket(const char * name, int fd)
  {
  gavf_io_t * io;
  program_t * ret;
  int flags = 0;
  ret = calloc(1, sizeof(*ret));

  pthread_mutex_init(&ret->mutex, NULL);

  ret->name = bg_strdup(NULL, name);

  ret->buf = buffer_create(BUF_ELEMENTS);
    
  ret->in_plug = bg_plug_create_reader(plugin_reg);
  
  io = bg_plug_io_open_socket(fd, BG_PLUG_IO_METHOD_READ, &flags);

  if(!bg_plug_open(ret->in_plug, io,
                   NULL, NULL, flags))
    goto fail;

  gavftools_set_stream_actions(ret->in_plug);

  if(!bg_plug_start(ret->in_plug))
    goto fail;

  /* Set up media connector */
  
  bg_plug_setup_reader(ret->in_plug, &ret->conn);
  bg_mediaconnector_create_conn(&ret->conn);
  
  /* Setup connection plug (TODO: Compress this) */
  io = gavf_io_create(NULL,
                      conn_write_func,
                      NULL,
                      NULL,
                      NULL,
                      ret);

  gavf_io_set_cb(io, conn_cb_func, ret);

  ret->conn_plug = bg_plug_create_writer(plugin_reg);

  bg_plug_open(ret->conn_plug, io,
               bg_plug_get_metadata(ret->in_plug), NULL, 0);

  if(!bg_plug_setup_writer(ret->conn_plug, &ret->conn))
    goto fail;

  
  return ret;
  
  fail:
  program_destroy(ret);
  return NULL;
  
  }

void program_destroy(program_t * p)
  {
  if(p->in_plug)
    bg_plug_destroy(p->in_plug);
  if(p->name)
    free(p->name);
  pthread_mutex_destroy(&p->mutex);

  free(p);
  }

void program_attach_client(program_t * p, int fd)
  {
  client_t * cl = client_create(fd, &p->ph);

  if(!cl)
    return;
  
  pthread_mutex_lock(&p->mutex);

  if(p->num_clients + 1 > p->clients_alloc)
    {
    p->clients_alloc += 16;
    p->clients = realloc(p->clients,
                         p->clients_alloc * sizeof(*p->clients));
    memset(p->clients + p->num_clients, 0,
           (p->clients_alloc - p->num_clients) * sizeof(*p->clients));
    }

  p->clients[p->num_clients] = cl;
  p->num_clients++;
  pthread_mutex_unlock(&p->mutex);
  }
