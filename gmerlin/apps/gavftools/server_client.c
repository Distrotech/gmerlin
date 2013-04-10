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

#define LOG_DOMAIN "gavf-server.client"

client_t * client_create(int fd, const gavf_program_header_t * ph,
                         buffer_t * buf)
  {
  int flags = 0;
  gavf_io_t * io;

  client_t * ret = calloc(1, sizeof(*ret));

  ret->fd = fd;
  ret->buf = buf;

  ret->seq = buf->num_elements ? buf->elements[0]->seq : buf->seq;
  
  ret->plug = bg_plug_create_writer(plugin_reg);
  ret->status = CLIENT_WAIT_SYNC;
  
  if(!(io = bg_plug_io_open_socket(fd, BG_PLUG_IO_METHOD_WRITE, &flags)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Creating client I/O module failed");
    goto fail;
    }

  if(!bg_plug_open(ret->plug, io, NULL, NULL, flags))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "bg_plug_open failed");
    goto fail;
    }
  
  if(!bg_plug_set_from_ph(ret->plug, ph))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "bg_plug_set_from_ph failed");
    goto fail;
    }
  
  return ret;
  
  fail:
  client_destroy(ret);
  return NULL;
  }


void client_destroy(client_t * cl)
  {
  if(cl->plug)
    bg_plug_destroy(cl->plug);
  free(cl);
  }

static buffer_element_t * next_element(client_t * cl)
  {
  buffer_element_t * ret = buffer_get_read(cl->buf, cl->seq);
  if(ret)
    cl->seq++;
  return ret;
  }

int client_iteration(client_t * cl)
  {
  buffer_element_t * el;

  fprintf(stderr, "Client iteration %d %"PRId64"\n", cl->status, cl->seq);
  
  if(cl->status == CLIENT_WAIT_SYNC)
    {
    while(1)
      {
      el = next_element(cl);

      if(!el)
        return 1;
      
      if(el->type == BUFFER_TYPE_SYNC_HEADER)
        {
        cl->status = CLIENT_RUNNING;
        break;
        }
      }
    }

  while(bg_socket_can_write(cl->fd, 0))
    {
    if(!(el = next_element(cl)))
      return 1;
    
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
