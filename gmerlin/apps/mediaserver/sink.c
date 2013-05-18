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

#include <server.h>
#include <gmerlin/http.h>
#include <gmerlin/bgplug.h>

#define LOG_DOMAIN "server.sink"

#define CLIENT_TIMEOUT 1000

typedef struct
  {
  buffer_t * buf;
  int64_t seq;
  const filter_t * f;
  void * filter_priv;
  } sink_t;

static void sink_destroy(void * data)
  {
  sink_t * s = data;
  if(s->f->destroy)
    s->f->destroy(s->filter_priv);  
  free(s);
  }

static buffer_element_t * next_element(client_t * cl)
  {
  buffer_element_t * ret = NULL;
  int result;
  sink_t * s = cl->data;
  
  while(1)
    {
    if(client_get_status(cl) == CLIENT_STATUS_STOP)
      return 0;
    
    result = buffer_get_read(s->buf, &s->seq, &ret);
    
    if(!result)
      return NULL;

    if(ret)
      return ret;
    buffer_wait(s->buf);
    }
  
  return NULL;
  }

static void sink_func(client_t * cl)
  {
  buffer_element_t * el;
  sink_t * priv = cl->data;

  while(1)
    {
    el = next_element(cl);

    if(!el)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not get data");
      return;
      }
    if(el->type == BUFFER_TYPE_SYNC_HEADER)
      {
      buffer_done_read(priv->buf);
      break;
      }
    buffer_done_read(priv->buf);
    }
  
  /* Got sync header */

  while(bg_socket_can_write(cl->fd, 1000))
    {
    if(!(el = next_element(cl)))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not get data");
      return;
      }
    if(!priv->f->put_buffer(priv->filter_priv, el))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Sending data failed");
      buffer_done_read(priv->buf);
      return;
      }
    buffer_done_read(priv->buf);
    }
  
  
  }

client_t * sink_client_create(int * fd, const gavl_metadata_t * req,
                              bg_plugin_registry_t * plugin_reg,
                              const gavf_program_header_t * ph,
                              const gavl_metadata_t * inline_metadata,
                              buffer_t * buf)
  {
  gavl_metadata_t res;
  int write_response = 0;
  client_t * ret = NULL;
  gavf_io_t * io;
  int flags;
  sink_t * priv = calloc(1, sizeof(*ret));

  gavl_metadata_init(&res);
  
  priv->buf = buf;
  priv->seq = -1;
  
  if(!(priv->f = filter_find(ph, req)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No filter found");
    bg_http_response_init(&res, "HTTP/1.1", 406, "Not Acceptable");
    write_response = 1;
    goto fail;
    }

  bg_http_response_init(&res, "HTTP/1.1", 200, "OK");
  if(!(priv->filter_priv = priv->f->create(ph, req, inline_metadata, &res, plugin_reg)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up filter failed");
    gavl_metadata_free(&res);
    bg_http_response_init(&res, "HTTP/1.1", 406, "Not Acceptable");
    write_response = 1;
    goto fail;
    }

  fprintf(stderr, "Sending response:\n");
  gavl_metadata_dump(&res, 0);
  
  if(!bg_http_response_write(*fd, &res))
    goto fail;
  
  /* Start filter */

  if(!(io = bg_plug_io_open_socket(*fd, BG_PLUG_IO_METHOD_WRITE, &flags,
                                   CLIENT_TIMEOUT)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Creating client I/O module failed");
//    bg_plug_response_set_status(res, BG_PLUG_IO_STATUS_406);
    goto fail;
    }

  if(!priv->f->start(priv->filter_priv, ph, req, inline_metadata, io, flags))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Starting filter failed");
    goto fail;
    }
  io = NULL;
  

  ret = client_create(CLIENT_TYPE_SINK, *fd, priv,
                      sink_destroy, sink_func);

  *fd = -1;
  
  fail:
  
  gavl_metadata_free(&res);
  if(!ret)
    sink_destroy(priv);
  return ret;
  }

