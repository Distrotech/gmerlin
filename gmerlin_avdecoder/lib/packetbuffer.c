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

#include <stdlib.h>

#include <avdec_private.h>


bgav_packet_buffer_t * bgav_packet_buffer_create()
  {
  bgav_packet_buffer_t * ret;
  ret = calloc(1, sizeof(*ret));
  /* Create 2 packets */

  ret->packets = bgav_packet_create();
  ret->packets->next = bgav_packet_create();
  ret->packets->next->next = ret->packets;

  ret->read_packet  = ret->packets;
  ret->write_packet = ret->packets;
  
  return ret;
  }

void bgav_packet_buffer_destroy(bgav_packet_buffer_t * b)
  {
  bgav_packet_t * tmp_packet;
  bgav_packet_t * packet = b->packets;
  
  do{
  tmp_packet = packet->next;
  bgav_packet_destroy(packet);
  packet = tmp_packet;
  }while(packet != b->packets);
  free(b);
  }

void bgav_packet_buffer_clear(bgav_packet_buffer_t * b)
  {
  bgav_packet_t * tmp_packet;
  tmp_packet = b->packets;

  do{
  tmp_packet->data_size = 0;
  tmp_packet->pts = -1;
  tmp_packet->valid = 0;
  tmp_packet = tmp_packet->next;
  }while(tmp_packet != b->packets);
  b->read_packet = b->write_packet;
  }

int bgav_packet_buffer_total_bytes(bgav_packet_buffer_t * b)
  {
  int ret = 0;
  bgav_packet_t * tmp_packet;
  tmp_packet = b->packets;

  do{
  if(tmp_packet->valid)
    ret += tmp_packet->data_size;

  tmp_packet = tmp_packet->next;
  }while(tmp_packet != b->packets);
  return ret;
  }

bgav_packet_t * bgav_packet_buffer_peek_packet_read(bgav_packet_buffer_t * b, int get_duration)
  {
  if(get_duration)
    {
    if(!b->read_packet->valid || !b->read_packet->next->valid)
      return (bgav_packet_t*)0; 
    if((b->read_packet->dts != BGAV_TIMESTAMP_UNDEFINED) &&
       (b->read_packet->next->dts != BGAV_TIMESTAMP_UNDEFINED))
      b->read_packet->duration =
        b->read_packet->next->dts - b->read_packet->dts;
    else
      b->read_packet->duration =
        b->read_packet->next->pts - b->read_packet->pts;
    }
  else if(!b->read_packet->valid)
    return (bgav_packet_t*)0;
  
  return b->read_packet;
  }

bgav_packet_t *
bgav_packet_buffer_get_packet_read(bgav_packet_buffer_t * b, int get_duration)
  {
  bgav_packet_t * ret;
  ret = bgav_packet_buffer_peek_packet_read(b, get_duration);
  if(!ret)
    return ret;
  b->read_packet = b->read_packet->next;
  return ret;
  }

bgav_packet_t * bgav_packet_buffer_get_packet_write(bgav_packet_buffer_t * b, bgav_stream_t * s)
  {
  bgav_packet_t * cur;
  bgav_packet_t * ret;
  if(b->write_packet->valid)
    {
    /* Insert a packet */
    cur = b->packets;
    while(cur->next != b->write_packet)
      cur = cur->next;
    cur->next = bgav_packet_create();
    cur->next->next = b->write_packet;
    b->write_packet = cur->next;
    }
  ret = b->write_packet;
  b->write_packet = b->write_packet->next;
  ret->pts = BGAV_TIMESTAMP_UNDEFINED;
  ret->dts = BGAV_TIMESTAMP_UNDEFINED;
  ret->flags = 0;
  ret->stream = s;
  return ret;
  }

int bgav_packet_buffer_is_empty(bgav_packet_buffer_t * b)
  {
  if((b->write_packet == b->read_packet) && !b->read_packet->valid)
    return 1;
  return 0;
  }
