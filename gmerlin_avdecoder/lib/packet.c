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
  if(size > p->data_alloc)
    {
    p->data_alloc = size + 16;
    p->data = realloc(p->data, p->data_alloc);
    }
  }

void bgav_packet_done_write(bgav_packet_t * p)
  {
  p->valid = 1;
  p->stream->in_position++;
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
  
  p->pts = start;
  p->duration = duration;
  p->data_size = len + 1;
  p->data[len]   = '\0';
  p->data[len+1] = '\0';
  }

#if 0
void bgav_packet_get_text_subtitle(bgav_packet_t * p,
                                   char ** text,
                                   int * text_alloc,
                                   int &text_len,
                                   gavl_time_t * start,
                                   gavl_time_t * duration)
  {
  int len;
  len = p->data_size;

  if(*text_alloc < len)
    {
    *text_alloc = len + 128;
    *text = realloc(*text, *text_alloc);
    }
  
  if(len)
    {
    strcpy(*text, (char*)p->data);
    }

  *start    = gavl_time_unscale(p->stream->timescale, p->pts);
  *duration = gavl_time_unscale(p->stream->timescale, p->duration_scaled);
  
  }
#endif
