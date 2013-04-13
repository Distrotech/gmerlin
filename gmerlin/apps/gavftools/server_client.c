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

#include <sys/socket.h>

#include <stdlib.h>

#define LOG_DOMAIN "gavf-server.client"


void client_set_status(client_t * c, int status)
  {
  pthread_mutex_lock(&c->status_mutex);
  c->status = status;
  pthread_mutex_unlock(&c->status_mutex);
  }


static void * thread_func(void * priv)
  {
  client_t * c = priv;
  while(client_iteration(c))
    ;
  client_set_status(c, CLIENT_STATUS_DONE);
  return NULL;
  }


client_t * client_create(int fd, const gavf_program_header_t * ph,
                         buffer_t * buf,
                         const gavl_metadata_t * url_vars)
  {
  bg_parameter_value_t val;
  int flags = 0;
  gavf_io_t * io;

  client_t * ret = calloc(1, sizeof(*ret));

  pthread_mutex_init(&ret->seq_mutex, NULL);
  pthread_mutex_init(&ret->status_mutex, NULL);

  ret->fd = fd;
  ret->buf = buf;
  
  ret->plug = bg_plug_create_writer(plugin_reg);
  ret->status = CLIENT_STATUS_WAIT_SYNC;
  
  if(!(io = bg_plug_io_open_socket(fd, BG_PLUG_IO_METHOD_WRITE, &flags,
                                   CLIENT_TIMEOUT)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Creating client I/O module failed");
    goto fail;
    }

  if(!bg_plug_open(ret->plug, io, NULL, NULL, flags))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "bg_plug_open failed");
    goto fail;
    }

  //  fprintf(stderr, "Client I/O flags: %08x\n", flags);

  val.val_i = 1024;
  gavl_metadata_get_int(url_vars, "shm", &val.val_i);

  bg_plug_set_parameter(ret->plug, "shm", &val);
  
  if(!bg_plug_set_from_ph(ret->plug, ph))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "bg_plug_set_from_ph failed");
    goto fail;
    }

  pthread_create(&ret->thread, NULL, thread_func, ret);
    
  return ret;
  
  fail:
  client_destroy(ret);
  return NULL;
  }

int client_get_status(client_t * cl)
  {
  int ret;
  pthread_mutex_lock(&cl->status_mutex);
  ret = cl->status;
  pthread_mutex_unlock(&cl->status_mutex);
  return ret;  
  }


void client_destroy(client_t * cl)
  {
  /* Shutdown the socket to interrupt R/W operations */
  shutdown(cl->fd, SHUT_RDWR);
  
  client_set_status(cl, CLIENT_STATUS_STOP);
  pthread_join(cl->thread, NULL);
  pthread_mutex_destroy(&cl->seq_mutex);
  pthread_mutex_destroy(&cl->status_mutex);

  if(cl->plug)
    bg_plug_destroy(cl->plug);
  
  free(cl);
  }

static buffer_element_t * next_element(client_t * cl)
  {
  buffer_element_t * ret = NULL;
  int result;

  while(1)
    {
    if(client_get_status(cl) == CLIENT_STATUS_STOP)
      return 0;
    
    pthread_mutex_lock(&cl->seq_mutex);
    result = buffer_get_read(cl->buf, &cl->seq, &ret);
    pthread_mutex_unlock(&cl->seq_mutex);

    if(!result)
      return NULL;

    if(ret)
      return ret;

    buffer_wait(cl->buf);
    }
  
  return NULL;
  }

int client_iteration(client_t * cl)
  {
  buffer_element_t * el;
  int status;
  // fprintf(stderr, "Client iteration %d %"PRId64"\n", cl->status, cl->seq);

  status = client_get_status(cl);

  if(status == CLIENT_STATUS_STOP)
    return 0;
  
  if(status == CLIENT_STATUS_WAIT_SYNC)
    {
    while(1)
      {
      el = next_element(cl);

      if(!el)
        return 0;
      
      if(el->type == BUFFER_TYPE_SYNC_HEADER)
        {
        client_set_status(cl, CLIENT_STATUS_RUNNING);
        break;
        }

      
      }
    }

  while(bg_socket_can_write(cl->fd, 0))
    {
    if(!(el = next_element(cl)))
      return 0;
    
    switch(el->type)
      {
      case BUFFER_TYPE_PACKET:
        if(bg_plug_put_packet(cl->plug,
                              &el->p) != GAVL_SINK_OK)
          return 0;
        break;
      case BUFFER_TYPE_METADATA:
        if(!gavf_update_metadata(bg_plug_get_gavf(cl->plug),
                                 &el->m))
          return 0;
        break;
      }
    }
  
  return 1;
  }

int64_t client_get_seq(client_t * cl)
  {
  int64_t ret;
  pthread_mutex_lock(&cl->seq_mutex);
  ret = cl->seq;
  pthread_mutex_unlock(&cl->seq_mutex);
  return ret;
  }

