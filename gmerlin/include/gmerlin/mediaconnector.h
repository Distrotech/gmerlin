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

typedef struct bg_mediaconnector_s bg_mediaconnector_t;

typedef struct bg_mediaconnector_stream_s
  {
  gavl_audio_source_t    * asrc;
  gavl_audio_connector_t * aconn;

  gavl_video_source_t    * vsrc;
  gavl_video_connector_t * vconn;

  gavl_packet_source_t    * psrc;
  gavl_packet_connector_t * pconn;

  int timescale;
  gavf_stream_type_t type;        // GAVF_STREAM_*
  
  int flags;
  
  gavl_metadata_t m;
  gavl_time_t time;

  int64_t counter;
  
  bg_cfg_section_t * encode_section;

  bg_thread_t * th;
  bg_mediaconnector_t * conn;
  
  /* Discontinuous stream support: Don't output a packet
     too early */
  
  gavl_video_frame_t * discont_vframe;
  gavl_packet_t      * discont_packet;
  
  gavl_video_source_t * discont_vsrc;
  gavl_packet_source_t * discont_psrc;

  int src_index; // index in the primary source
  int dst_index; // index in the destination

  void * priv;
  void (*free_priv)(struct bg_mediaconnector_stream_s *);

  gavl_source_status_t last_status;
  
  } bg_mediaconnector_stream_t;

struct bg_mediaconnector_s
  {
  bg_mediaconnector_stream_t ** streams;
  int num_streams;
  int num_threads; // Can be < num_streams
  
  bg_thread_common_t * tc;

  bg_thread_t ** th;
  
  pthread_mutex_t time_mutex;
  gavl_time_t time;

  pthread_mutex_t running_threads_mutex;
  int running_threads;
  };

void
bg_mediaconnector_init(bg_mediaconnector_t * conn);

bg_mediaconnector_stream_t *
bg_mediaconnector_add_audio_stream(bg_mediaconnector_t * conn,
                                   const gavl_metadata_t * m,
                                   gavl_audio_source_t * asrc,
                                   gavl_packet_source_t * psrc);

bg_mediaconnector_stream_t *
bg_mediaconnector_add_video_stream(bg_mediaconnector_t * conn,
                                   const gavl_metadata_t * m,
                                   gavl_video_source_t * vsrc,
                                   gavl_packet_source_t * psrc);

bg_mediaconnector_stream_t *
bg_mediaconnector_add_overlay_stream(bg_mediaconnector_t * conn,
                                     const gavl_metadata_t * m,
                                     gavl_video_source_t * vsrc,
                                     gavl_packet_source_t * psrc);

bg_mediaconnector_stream_t *
bg_mediaconnector_add_text_stream(bg_mediaconnector_t * conn,
                                  const gavl_metadata_t * m,
                                  gavl_packet_source_t * psrc,
                                  int timescale);

int bg_mediaconnector_get_num_streams(bg_mediaconnector_t * conn,
                                      gavf_stream_type_t type);

bg_mediaconnector_stream_t *
bg_mediaconnector_get_stream(bg_mediaconnector_t * conn,
                             gavf_stream_type_t type, int idx);


void
bg_mediaconnector_create_conn(bg_mediaconnector_t * conn);


void
bg_mediaconnector_update_time(bg_mediaconnector_t * conn,
                              gavl_time_t time);

gavl_time_t
bg_mediaconnector_get_time(bg_mediaconnector_t * conn);

gavl_time_t
bg_mediaconnector_get_min_time(bg_mediaconnector_t * conn);

void
bg_mediaconnector_create_threads(bg_mediaconnector_t * conn, int all);

void
bg_mediaconnector_create_threads_common(bg_mediaconnector_t * conn);

void
bg_mediaconnector_create_thread(bg_mediaconnector_t * conn, int index, int all);

void
bg_mediaconnector_start(bg_mediaconnector_t * conn);

void
bg_mediaconnector_free(bg_mediaconnector_t * conn);

int bg_mediaconnector_iteration(bg_mediaconnector_t * conn);

void
bg_mediaconnector_threads_init_separate(bg_mediaconnector_t * conn);



void
bg_mediaconnector_threads_start(bg_mediaconnector_t * conn);

void
bg_mediaconnector_threads_stop(bg_mediaconnector_t * conn);

int bg_mediaconnector_done(bg_mediaconnector_t * conn);


#endif // __MEDIACONNECTOR_H_
