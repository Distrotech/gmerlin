/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <avdec_private.h>
#include <rtp.h>

#define MAX_PACKETS 10

struct bgav_rtp_packet_buffer_s
  {
  rtp_packet_t packets[MAX_PACKETS];
  int next_seq;
  const bgav_options_t * opt;
  int num;
  };

bgav_rtp_packet_buffer_t * rtp_packet_buffer_create(const bgav_options_t * opt)
  {
  int i;
  bgav_rtp_packet_buffer_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->next_seq = -1;
  ret->opt = opt;
  }

void rtp_packet_buffer_destroy(bgav_rtp_packet_buffer_t * b)
  {
  free(b);
  }

rtp_packet_t *
rtp_packet_buffer_get_write(bgav_rtp_packet_buffer_t * b)
  {
  int i;
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(!b->packets[i].valid)
      return &b->packets[i];
    }
  return (rtp_packet_t*)0;
  }

void rtp_packet_buffer_done_write(bgav_rtp_packet_buffer_t * b, rtp_packet_t * p)
  {
  p->valid = 1;
  b->num++;
  }

rtp_packet_t *
rtp_packet_buffer_get_read(bgav_rtp_packet_buffer_t * b)
  {
  
  }

void rtp_packet_buffer_done_read(bgav_rtp_packet_buffer_t * b, rtp_packet_t * p)
  {
  p->valid = 0;
  b->num--;
  }
