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

#ifndef __MEDIACONNECTOR_H_
#define __MEDIACONNECTOR_H_


#include <config.h>
#include <gavl/connectors.h>
#include <gavl/gavf.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/bgthread.h>


#define BG_MEDIACONNECTOR_FLAG_EOF     (1<<0)
#define BG_MEDIACONNECTOR_FLAG_DISCONT (1<<1)

typedef struct
  {
  gavl_audio_source_t    * asrc;
  gavl_audio_connector_t * aconn;

  gavl_video_source_t    * vsrc;
  gavl_video_connector_t * vconn;

  gavl_packet_source_t    * psrc;
  gavl_packet_connector_t * pconn;

  int timescale;
  int type;        // GAVF_STREAM_*
  
  int flags;
  
  gavl_metadata_t m;
  gavl_time_t time;

  int64_t counter;
  
  bg_cfg_section_t * encode_section;

  bg_thread_t * th;
  
  } bg_mediaconnector_stream_t;

typedef struct
  {
  bg_mediaconnector_stream_t * streams;
  int num_streams;
  
  bg_thread_common_t * tc;
  } bg_mediaconnector_t;

void
bg_mediaconnector_init(bg_mediaconnector_t * conn);

void
bg_mediaconnector_add_audio_stream(bg_mediaconnector_t * conn,
                                   const gavl_metadata_t * m,
                                   gavl_audio_source_t * asrc,
                                   gavl_packet_source_t * psrc,
                                   bg_cfg_section_t * enc_section);

void
bg_mediaconnector_add_video_stream(bg_mediaconnector_t * conn,
                                   const gavl_metadata_t * m,
                                   gavl_video_source_t * vsrc,
                                   gavl_packet_source_t * psrc,
                                   bg_cfg_section_t * enc_section);

void
bg_mediaconnector_add_text_stream(bg_mediaconnector_t * conn,
                                  const gavl_metadata_t * m,
                                  gavl_packet_source_t * psrc,
                                  int timescale);

void
bg_mediaconnector_init_threads(bg_mediaconnector_t * conn);

void
bg_mediaconnector_start(bg_mediaconnector_t * conn);


void
bg_mediaconnector_free(bg_mediaconnector_t * conn);

int bg_mediaconnector_iteration(bg_mediaconnector_t * conn);

#endif // __MEDIACONNECTOR_H_
