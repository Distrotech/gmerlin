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

#define LOG_DOMAIN "gavf-server.program"

#define BUF_ELEMENTS 30

static void program_remove_client(program_t * p, int idx);


static void set_status(program_t * p, int status)
  {
  pthread_mutex_lock(&p->status_mutex);
  p->status = status;
  pthread_mutex_unlock(&p->status_mutex);
  }

static client_t * element_used(program_t * p, uint64_t seq)
  {
  int i;
  for(i = 0; i < p->num_clients; i++)
    {
    if(p->clients[i]->seq == seq)
      return p->clients[i];
    }
  return NULL;
  }

static int program_iteration(program_t * p)
  {
  int i;
  gavl_time_t conn_time;
  gavl_time_t program_time;

  /* Kill dead clients: We need at least 3 elements in the pool */
  
  while(p->buf->elements_alloc - p->buf->num_elements < 3)
    {
    client_t * cl;
    
    while((cl = element_used(p, p->buf->elements[0]->seq)))
      {
      
      }
    buffer_advance(p->buf);
    }
  
  /* Read packet from source */

  if(!bg_mediaconnector_iteration(&p->conn))
    return 0;
  
  /* Distribute to clients */

  for(i = 0; i < p->num_clients; i++)
    {
    if(!client_iteration(p->clients[i]))
      program_remove_client(p, i);
    }
  
  /* Throw away old packets */

  while(p->buf->num_elements &&
        !element_used(p, p->buf->elements[0]->seq))
    buffer_advance(p->buf);
  
  /* Delay */
  conn_time = bg_mediaconnector_get_time(&p->conn);
  program_time = gavl_timer_get(p->timer);

  if(conn_time - program_time > GAVL_TIME_SCALE/2)
    {
    gavl_time_t delay_time;
    delay_time = conn_time - program_time - GAVL_TIME_SCALE/2;
    //    fprintf(stderr, "delay %"PRId64"\n", delay_time);
    gavl_time_delay(&delay_time);
    }
  
  return 1;
  }

static void * thread_func(void * priv)
  {
  program_t * p = priv;
  while(program_iteration(p))
    ;
  set_status(p, PROGRAM_STATUS_DONE);
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
      //      fprintf(stderr, "Got program header:\n");
      //      gavf_program_header_dump(data);

      if(!(p->flags & PROGRAM_HAVE_HEADER))
        {
        gavf_program_header_copy(&p->ph, data);
        p->flags |= PROGRAM_HAVE_HEADER;
        }
      break;
    case GAVF_IO_CB_PROGRAM_HEADER_END:
      break;
    case GAVF_IO_CB_PACKET_START:
      //      fprintf(stderr, "Got packet: %d\n", ((gavl_packet_t*)data)->id);
      //      gavl_packet_dump(data);

      el = buffer_get_write(p->buf);
      buffer_element_set_packet(el, data);

      break;
    case GAVF_IO_CB_PACKET_END:
      break;
    case GAVF_IO_CB_METADATA_START:
      //      fprintf(stderr, "Got metadata:\n");
      //      gavl_metadata_dump(data, 0);

      el = buffer_get_write(p->buf);
      buffer_element_set_metadata(el, data);

      break;
    case GAVF_IO_CB_METADATA_END:
      break;
    case GAVF_IO_CB_SYNC_HEADER_START:
      //      fprintf(stderr, "Got sync_header\n");

      if(!gavl_timer_is_running(p->timer))
        gavl_timer_start(p->timer);
      
      el = buffer_get_write(p->buf);
      buffer_element_set_sync_header(el);
      
      break;
    case GAVF_IO_CB_SYNC_HEADER_END:
      break;
    }
  
  return 1;
  }

program_t * program_create_from_socket(const char * name, int fd)
  {
  bg_plug_t * plug = NULL;
  gavf_io_t * io = NULL;
  int flags = 0;

  io = bg_plug_io_open_socket(fd, BG_PLUG_IO_METHOD_READ, &flags);
  if(!io)
    goto fail;
  
  plug = bg_plug_create_reader(plugin_reg);
  if(!plug)
    goto fail;

  if(!bg_plug_open(plug, io,
                   NULL, NULL, flags))
    goto fail;

  gavftools_set_stream_actions(plug);

  if(!bg_plug_start(plug))
    goto fail;

  return program_create_from_plug(name, plug);
  
  fail:

  if(plug)
    bg_plug_destroy(plug);
  return NULL;
  
  }


program_t * program_create_from_plug(const char * name, bg_plug_t * plug)
  {
  gavf_io_t * io;
  program_t * ret;
  gavf_t * g;
  
  ret = calloc(1, sizeof(*ret));

  ret->in_plug = plug;
  
  pthread_mutex_init(&ret->client_mutex, NULL);
  pthread_mutex_init(&ret->status_mutex, NULL);
  
  ret->name = bg_strdup(NULL, name);

  ret->buf = buffer_create(BUF_ELEMENTS);
  ret->timer = gavl_timer_create();
  ret->status = PROGRAM_STATUS_RUNNING;
  
  /* Set up media connector */
  
  bg_plug_setup_reader(ret->in_plug, &ret->conn);
  bg_mediaconnector_create_conn(&ret->conn);
  
  /* Setup connection plug */
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

  bg_mediaconnector_start(&ret->conn);

  g = bg_plug_get_gavf(ret->conn_plug);
  bg_log(BG_LOG_INFO, LOG_DOMAIN,
         "Created program: %s (AS: %d, VS: %d, TS: %d, OS: %d)",
         name,
         gavf_get_num_streams(g, GAVF_STREAM_AUDIO),
         gavf_get_num_streams(g, GAVF_STREAM_VIDEO),
         gavf_get_num_streams(g, GAVF_STREAM_TEXT),
         gavf_get_num_streams(g, GAVF_STREAM_OVERLAY));
  
  pthread_create(&ret->thread, NULL, thread_func, ret);
  
  return ret;
  
  fail:
  program_destroy(ret);
  return NULL;
  
  }


void program_destroy(program_t * p)
  {
  if(program_get_status(p) == PROGRAM_STATUS_RUNNING)
    set_status(p, PROGRAM_STATUS_STOP);

  pthread_join(p->thread, NULL);
  
  if(p->in_plug)
    bg_plug_destroy(p->in_plug);
  if(p->name)
    free(p->name);
  if(p->buf)
    buffer_destroy(p->buf);
  if(p->timer)
    gavl_timer_destroy(p->timer);
  
  
  pthread_mutex_destroy(&p->client_mutex);
  pthread_mutex_destroy(&p->status_mutex);

  free(p);
  }

void program_attach_client(program_t * p, int fd)
  {
  client_t * cl = client_create(fd, &p->ph, p->buf);

  if(!cl)
    return;
  
  pthread_mutex_lock(&p->client_mutex);

  if(p->num_clients + 1 > p->clients_alloc)
    {
    p->clients_alloc += 16;
    p->clients = realloc(p->clients,
                         p->clients_alloc * sizeof(*p->clients));
    memset(p->clients + p->num_clients, 0,
           (p->clients_alloc - p->num_clients) * sizeof(*p->clients));
    }

  p->clients[p->num_clients] = cl;

  bg_log(BG_LOG_INFO, LOG_DOMAIN,
         "Got new client connection for program %s", p->name);
  
  p->num_clients++;
  pthread_mutex_unlock(&p->client_mutex);
  }

static void program_remove_client(program_t * p, int idx)
  {
  pthread_mutex_lock(&p->client_mutex);

  client_destroy(p->clients[idx]);
  if(idx < p->num_clients-1)
    {
    memmove(&p->clients[idx], &p->clients[idx+1],
            (p->num_clients-1 - idx) * sizeof(p->clients[idx]));
    }
  p->num_clients--;
  pthread_mutex_unlock(&p->client_mutex);

  bg_log(BG_LOG_INFO, LOG_DOMAIN,
         "Closed client connection for program %s", p->name);
  
  }

int program_get_status(program_t * p)
  {
  int ret;
  pthread_mutex_lock(&p->status_mutex);
  ret = p->status;
  pthread_mutex_unlock(&p->status_mutex);
  return ret;  
  }
