/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <avdec_private.h>

#define MAX_PACKETS 16

struct bgav_packet_timer_s
  {
  bgav_packet_t * packets[MAX_PACKETS];
  int num_packets;
  bgav_stream_t * s;

  bgav_packet_source_t src;

  int eof;
  int64_t last_duration;
  
  bgav_packet_t * out_packet;
  };

/* Simple: No B-frames */
static void peek_simple(void * pt1, int force)
  {
  
  }

static void get_simple(void * pt1)
  {
  
  }

bgav_packet_timer_t * bgav_packet_timer_create(bgav_stream_t * s)
  {
  bgav_packet_timer_t * ret = calloc(1, sizeof(*ret));
  ret->s = s;

  bgav_packet_source_copy(&ret->src, &s->src);
  
  return ret;
  }

void bgav_packet_timer_destroy(bgav_packet_timer_t * pt)
  {
  free(pt);
  }

bgav_packet_t * bgav_packet_timer_get(void * pt1)
  {
  bgav_packet_timer_t * pt = pt1;
  
  }

bgav_packet_t * bgav_packet_timer_peek(void * pt1, int force)
  {
  bgav_packet_timer_t * pt = pt1;
  
  }

void bgav_packet_timer_reset(bgav_packet_timer_t * pt)
  {
  
  }
