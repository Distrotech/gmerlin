/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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
#include <string.h>

#include <avdec_private.h>


bgav_packet_t * bgav_packet_create()
  {
  bgav_packet_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bgav_packet_destroy(bgav_packet_t * p)
  {
  if(p->data)
    free(p->data);
  if(p->audio_frame)
    gavl_audio_frame_destroy(p->audio_frame);
  if(p->video_frame)
    gavl_video_frame_destroy(p->video_frame);
  
  free(p);
  }

void bgav_packet_alloc(bgav_packet_t * p, int size)
  {
  if(size + PACKET_PADDING > p->data_alloc)
    {
    p->data_alloc = size + PACKET_PADDING + 1024;
    p->data = realloc(p->data, p->data_alloc);
    }
  }

void bgav_packet_pad(bgav_packet_t * p)
  {
  /* Padding */
  memset(p->data + p->data_size, 0, PACKET_PADDING);
  }

void bgav_packet_set_text_subtitle(bgav_packet_t * p,
                                   const char * text,
                                   int len,
                                   int64_t start,
                                   int64_t duration)
  {
  if(len < 0)
    len = strlen(text);
  
  bgav_packet_alloc(p, len+2);
  memcpy(p->data, text, len);
  p->data_size = len;
  PACKET_SET_KEYFRAME(p);
  p->pts = start;
  p->duration = duration;
  p->data_size = len + 1;
  p->data[len]   = '\0';
  p->data[len+1] = '\0';
  }

void bgav_packet_dump(bgav_packet_t * p)
  {
  int type;
  
  bgav_dprintf("pos: %"PRId64", K: %d, ", p->position, !!PACKET_GET_KEYFRAME(p));

  if(p->field2_offset)
    bgav_dprintf("f2: %d, ", p->field2_offset);

  type = PACKET_GET_CODING_TYPE(p);
  if(type)
    bgav_dprintf("T: %c ", type);
  
  if(p->dts != GAVL_TIME_UNDEFINED)
    bgav_dprintf("dts: %"PRId64", ", p->dts);

  if(p->pts == GAVL_TIME_UNDEFINED)
    bgav_dprintf("pts: (none), ");
  else
    bgav_dprintf("pts: %"PRId64", ", p->pts);
  bgav_dprintf("Len: %d, dur: %"PRId64, p->data_size, p->duration);

  if(p->header_size)
    bgav_dprintf(", head: %d", p->header_size);

  if(p->sequence_end_pos)
    bgav_dprintf(", end: %d", p->sequence_end_pos);

  if(p->tc != GAVL_TIMECODE_UNDEFINED)
    {
    bgav_dprintf(", TC: ");
    gavl_timecode_dump(NULL, p->tc);
    }
  
  if(p->ilace == GAVL_INTERLACE_TOP_FIRST)
    bgav_dprintf(", il: t");
  else if(p->ilace == GAVL_INTERLACE_BOTTOM_FIRST)
    bgav_dprintf(", il: b");
  
  bgav_dprintf("\n");
  //  bgav_hexdump(p->data, p->data_size < 16 ? p->data_size : 16, 16);
  }

void bgav_packet_dump_data(bgav_packet_t * p, int bytes)
  {
  if(bytes > p->data_size)
    bytes = p->data_size;
  bgav_hexdump(p->data, bytes, 16);
  }

#define SWAP(n1, n2) \
  swp = n1; n1 = n2; n2 = swp;

void bgav_packet_swap_data(bgav_packet_t * p1, bgav_packet_t * p2)
  {
  uint8_t * swp_ptr;
  int64_t swp;
  
  swp_ptr = p1->data;
  p1->data = p2->data;
  p2->data = swp_ptr;
  
  SWAP(p1->data_size, p2->data_size);
  SWAP(p1->data_alloc, p2->data_alloc);
  }

void bgav_packet_reset(bgav_packet_t * p)
  {
  p->pts = BGAV_TIMESTAMP_UNDEFINED;
  p->dts = BGAV_TIMESTAMP_UNDEFINED;
  p->tc = GAVL_TIMECODE_UNDEFINED;
  p->flags = 0;
  p->data_size = 0;
  p->header_size = 0;
  p->duration = -1;
  bgav_packet_free_palette(p);
  }

void bgav_packet_copy_metadata(bgav_packet_t * dst,
                               const bgav_packet_t * src)
  {
  dst->pts      = src->pts;
  dst->dts      = src->dts;
  dst->duration = src->duration;
  dst->flags    = src->flags;
  dst->tc       = src->tc;
  }

void bgav_packet_source_copy(bgav_packet_source_t * dst,
                             const bgav_packet_source_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

void bgav_packet_alloc_palette(bgav_packet_t * p, int size)
  {
  p->palette = malloc(sizeof(*p->palette) * size);
  p->palette_size = size;
  }

void bgav_packet_free_palette(bgav_packet_t * p)
  {
  if(p->palette)
    {
    free(p->palette);
    p->palette = NULL;
    }
  p->palette_size = 0;
  }
