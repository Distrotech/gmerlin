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
#include <stdlib.h>

#define MAX_PACKETS 10

struct bgav_rtp_packet_buffer_s
  {
  rtp_packet_t packets[MAX_PACKETS];
  int next_seq;
  const bgav_options_t * opt;
  int num;
  uint64_t seq_offset;
  };

bgav_rtp_packet_buffer_t * bgav_rtp_packet_buffer_create(const bgav_options_t * opt)
  {
  bgav_rtp_packet_buffer_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->next_seq = -1;
  ret->opt = opt;
  return ret;
  }

void bgav_rtp_packet_buffer_destroy(bgav_rtp_packet_buffer_t * b)
  {
  free(b);
  }

rtp_packet_t *
bgav_rtp_packet_buffer_get_write(bgav_rtp_packet_buffer_t * b)
  {
  int i;
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(!b->packets[i].valid)
      return &b->packets[i];
    }
  return (rtp_packet_t*)0;
  }

void bgav_rtp_packet_buffer_done_write(bgav_rtp_packet_buffer_t * b, rtp_packet_t * p)
  {
  p->valid = 1;
  b->num++;
  if(b->next_seq == -1)
    b->next_seq = p->h.sequence_number;
  }

rtp_packet_t *
bgav_rtp_packet_buffer_get_read(bgav_rtp_packet_buffer_t * b)
  {
  int min_seq, max_seq, i;
  int min_index;
  int test;
  
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(b->packets[i].valid & (b->next_seq == b->packets[i].h.sequence_number))
      return &b->packets[i];
    }
  if(b->num < MAX_PACKETS)
    return (rtp_packet_t*)0;

  /* Drop packets */
  
  min_seq = 0x10000;
  max_seq = -1;

  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(b->packets[i].valid)
      {
      if(min_seq > b->packets[i].h.sequence_number)
        {
        min_seq = b->packets[i].h.sequence_number;
        min_index = i;
        }
      if(max_seq < b->packets[i].h.sequence_number)
        max_seq = b->packets[i].h.sequence_number;
      }
    }
  
  /* No wrap */
  if(max_seq - min_seq < 0x8000)
    return &b->packets[min_index];

  min_seq = 0x10000;
  
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(b->packets[i].valid)
      {
      test = b->packets[i].h.sequence_number > 0x8000 ?
        (int)b->packets[i].h.sequence_number - 0x10000 :
        b->packets[i].h.sequence_number;
      
      if(min_seq > test)
        {
        min_seq = test;
        min_index = i;
        }
      }
    }
  return &b->packets[min_index];
  }

void bgav_rtp_packet_buffer_done_read(bgav_rtp_packet_buffer_t * b, rtp_packet_t * p)
  {
  int i;
  p->valid = 0;
  b->num--;
  
  /* Delete obsolete packets */
  for(i = 0; i < MAX_PACKETS; i++)
    {
    if(b->packets[i].valid & (b->packets[i].h.sequence_number <
                              p->h.sequence_number))
      {
      b->packets[i].valid = 0;
      b->num--;
      }
    }

  b->next_seq = p->h.sequence_number+1;
  if(b->next_seq > 0xffff)
    b->next_seq = 0;
  }
