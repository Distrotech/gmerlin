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
#include <stdlib.h>

client_t * client_create(int fd, const gavf_program_header_t * ph)
  {
  int flags = 0;
  gavf_io_t * io;

  client_t * ret = calloc(1, sizeof(*ret));

  ret->plug = bg_plug_create_writer(plugin_reg);

  io = bg_plug_io_open_socket(fd, BG_PLUG_IO_METHOD_READ, &flags);

  if(!io ||
     !bg_plug_open(ret->plug, io, NULL, NULL, flags) ||
     !bg_plug_set_from_ph(ret->plug, ph))
    goto fail;

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
