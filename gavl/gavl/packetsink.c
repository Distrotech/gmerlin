/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <stdlib.h>
#include <gavl/connectors.h>

struct gavl_packet_sink_s
  {
  gavl_packet_sink_get_func get_func;
  gavl_packet_sink_put_func put_func;
  void * priv;
  };
  
gavl_packet_sink_t *
gavl_packet_sink_create(gavl_packet_sink_get_func get_func,
                        gavl_packet_sink_put_func put_func,
                        void * priv)
  {
  gavl_packet_sink_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->get_func = get_func;
  ret->put_func = put_func;
  ret->priv = priv;
  return ret;
  }

gavl_packet_t *
gavl_packet_sink_get_packet(gavl_packet_sink_t * s)
  {
  return s->get_func ? s->get_func(s->priv) : NULL;
  }

gavl_sink_status_t
gavl_packet_sink_put_packet(gavl_packet_sink_t * s, gavl_packet_t * p)
  {
  return s->put_func(s, p);
  }

void
gavl_packet_sink_destroy(gavl_packet_sink_t * s)
  {
  free(s);
  }
