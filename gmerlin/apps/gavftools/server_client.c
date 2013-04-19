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
#include <unistd.h>

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
                         const gavl_metadata_t * req,
                         gavl_metadata_t * res,
                         const gavl_metadata_t * url_vars,
                         const gavl_metadata_t * inline_metadata)
  {
  int flags = 0;
  int write_response = 0;
  gavf_io_t * io = NULL;
  int method = 0;

  client_t * ret = calloc(1, sizeof(*ret));

  /* Already checked that there is a valid method */
  bg_plug_request_get_method(req, &method);

  fprintf(stderr, "Method: %d\n", method);
  
  pthread_mutex_init(&ret->seq_mutex, NULL);
  pthread_mutex_init(&ret->status_mutex, NULL);

  client_set_status(ret, CLIENT_STATUS_STARTING);
  
  ret->fd = fd;
  ret->buf = buf;

  if(!(ret->f = filter_find(ph, req)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No filter found");
    bg_plug_response_set_status(res, BG_PLUG_IO_STATUS_406);
    write_response = 1;
    goto fail;
    }
  
  if(!(ret->priv = ret->f->create(ph, req, url_vars, inline_metadata, res)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up filter failed");
    bg_plug_response_set_status(res, BG_PLUG_IO_STATUS_406);
    write_response = 1;
    goto fail;
    }
  
  /* Send response */

#ifdef DUMP_HTTP_HEADERS
  fprintf(stderr, "Sending response\n");
  gavl_metadata_dump(res, 2);
#endif  
  
  if(!bg_plug_response_write(fd, res))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing response failed");
    goto fail;
    }
  
  if(method == BG_PLUG_IO_METHOD_HEAD)
    {
    fprintf(stderr, "Got head\n");
    goto fail;
    }
  /* Start filter */
    
  if(!(io = bg_plug_io_open_socket(fd, BG_PLUG_IO_METHOD_WRITE, &flags,
                                   CLIENT_TIMEOUT)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Creating client I/O module failed");
//    bg_plug_response_set_status(res, BG_PLUG_IO_STATUS_406);
    goto fail;
    }

  fd = -1; /* Don't close */

  if(!ret->f->start(ret->priv, ph, req, url_vars, inline_metadata, io, flags))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Starting filter failed");
 //   bg_plug_response_set_status(res, BG_PLUG_IO_STATUS_406);
    goto fail;
    }

  io = NULL; // Don't destroy

  //  fprintf(stderr, "Client I/O flags: %08x\n", flags);

  /* Start thread */
  
  client_set_status(ret, CLIENT_STATUS_WAIT_SYNC);
  pthread_create(&ret->thread, NULL, thread_func, ret);
  
  return ret;
  
  fail:

  if(write_response)
    {
#ifdef DUMP_HTTP_HEADERS
    fprintf(stderr, "Sending response\n");
    gavl_metadata_dump(res, 2);
#endif  
    bg_plug_response_write(fd, res);
    }
  if(io)
    gavf_io_destroy(io);
  if(fd >= 0)
    close(fd);

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

  if(client_get_status(cl) != CLIENT_STATUS_STARTING)
    {
    client_set_status(cl, CLIENT_STATUS_STOP);
    pthread_join(cl->thread, NULL);
    }
  
  pthread_mutex_destroy(&cl->seq_mutex);
  pthread_mutex_destroy(&cl->status_mutex);

  if(cl->priv)
    cl->f->destroy(cl->priv);
  
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

  status = client_get_status(cl);

  if(status == CLIENT_STATUS_STOP)
    return 0;
  
  if(status == CLIENT_STATUS_WAIT_SYNC)
    {
    while(1)
      {
      // fprintf(stderr, "Client iteration %d %"PRId64"\n", cl->status, cl->seq);

      el = next_element(cl);

      if(!el)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not get data");
        return 0;
        }
      if(el->type == BUFFER_TYPE_SYNC_HEADER)
        {
        client_set_status(cl, CLIENT_STATUS_RUNNING);
        break;
        }

      
      }
    }

  while(bg_socket_can_write(cl->fd, 0))
    {
    //  fprintf(stderr, "Client iteration %d %"PRId64"\n", cl->status, cl->seq);
    if(!(el = next_element(cl)))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not get data");
      return 0;
      }
    if(!cl->f->put_buffer(cl->priv, el))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Sending data failed");
      return 0;
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

