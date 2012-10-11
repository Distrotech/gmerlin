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

#include <gavftools.h>

typedef struct
  {
  gavl_audio_source_t * asrc;
  gavl_packet_source_t * psrc;

  gavl_audio_connector_t * aconn;
  gavl_packet_connector_t * pconn;
  } audio_stream_t;

typedef struct
  {
  gavl_video_source_t * asrc;
  gavl_packet_source_t * psrc;

  gavl_video_connector_t * aconn;
  gavl_packet_connector_t * pconn;
  } video_stream_t;

typedef struct
  {
  gavl_packet_source_t * psrc;
  gavl_packet_connector_t * pconn;
  } text_stream_t;


struct bg_program_s
  {
  int num_audio_streams;
  int num_video_streams;
  int num_text_streams;
  
  audio_stream_t ** audio_streams;
  video_stream_t ** video_streams;
  text_stream_t ** text_streams;
  };

bg_program_t * bg_program_create()
  {
  bg_program_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

#define APPEND_STREAM(arr, num) \
  arr = realloc(arr, (num+1)*sizeof(**(arr)));  \
  arr[num] = calloc(1, sizeof(*(arr[num]))); \
  s = arr[num];                              \
  num++

void bg_program_add_audio_stream(bg_program_t * p,
                                 gavl_audio_source_t * asrc,
                                 gavl_packet_source_t * psrc)
  {
  audio_stream_t * s;
  APPEND_STREAM(p->audio_streams, p->num_audio_streams);
  }

void bg_program_add_video_stream(bg_program_t * p,
                                 gavl_video_source_t * vsrc,
                                 gavl_packet_source_t * psrc)
  {
  video_stream_t * s;
  APPEND_STREAM(p->video_streams, p->num_video_streams);
  }

void bg_program_add_text_stream(bg_program_t * p,
                                gavl_packet_source_t * psrc)
  {
  text_stream_t * s;
  APPEND_STREAM(p->text_streams, p->num_text_streams);
  }

void bg_program_destroy(bg_program_t * p)
  {
  
  }
