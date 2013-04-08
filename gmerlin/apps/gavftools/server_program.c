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

static int program_iteration(program_t * p)
  {
  /* Read packet from source */

  /* Distribute to sinks */

  /* Throw away old packets */

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
  program_t * p = priv;

  switch(type)
    {
    case GAVF_IO_CB_PROGRAM_HEADER_START:
      fprintf(stderr, "Got program header:\n");
      gavf_program_header_dump(data);
      break;
    case GAVF_IO_CB_PROGRAM_HEADER_END:
      break;
    case GAVF_IO_CB_PACKET_START:
      break;
    case GAVF_IO_CB_PACKET_END:
      break;
    case GAVF_IO_CB_METADATA_START:
      break;
    case GAVF_IO_CB_METADATA_END:
      break;
    case GAVF_IO_CB_SYNC_HEADER_START:
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
  ret->name = bg_strdup(NULL, name);

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
  bg_plug_setup_writer(ret->conn_plug, &ret->conn);

  if(bg_plug_start(ret->conn_plug))
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
  free(p);
  }

void program_attach_client(program_t * p, int fd)
  {

  }
