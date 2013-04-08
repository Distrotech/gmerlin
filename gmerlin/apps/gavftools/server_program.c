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
