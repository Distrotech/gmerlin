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

#include <config.h>

typedef struct
  {
  bg_track_info_t * track_info;
  bg_track_info_t * current_track;
  int num_tracks;
  bgav_t * dec;
  bgav_options_t * opt;
  
  bg_input_callbacks_t * bg_callbacks;
  } avdec_priv;

const gavl_edl_t * bg_avdec_get_edl(void * priv);

void * bg_avdec_create();

void bg_avdec_close(void * priv);
void bg_avdec_destroy(void * priv);
bg_track_info_t * bg_avdec_get_track_info(void * priv, int track);

gavl_video_source_t *
bg_avdec_get_video_source(void * priv, int stream);

gavl_audio_source_t *
bg_avdec_get_audio_source(void * priv, int stream);

gavl_packet_source_t *
bg_avdec_get_video_packet_source(void * priv, int stream);

gavl_packet_source_t *
bg_avdec_get_audio_packet_source(void * priv, int stream);

gavl_packet_source_t *
bg_avdec_get_text_packet_source(void * priv, int stream);

gavl_video_source_t *
bg_avdec_get_overlay_source(void * priv, int stream);

gavl_packet_source_t *
bg_avdec_get_overlay_packet_source(void * priv, int stream);


int bg_avdec_read_video(void * priv,
                        gavl_video_frame_t * frame,
                        int stream);

void bg_avdec_skip_video(void * priv, int stream, int64_t * time,
                         int scale, int exact);


int bg_avdec_has_still(void * priv,
                       int stream);

int bg_avdec_read_audio(void * priv,
                        gavl_audio_frame_t * frame,
                        int stream,
                        int num_samples);

int bg_avdec_set_audio_stream(void * priv,
                                  int stream,
                              bg_stream_action_t action);
int bg_avdec_set_video_stream(void * priv,
                              int stream,
                              bg_stream_action_t action);

int bg_avdec_set_overlay_stream(void * priv,
                                 int stream,
                                 bg_stream_action_t action);

int bg_avdec_set_text_stream(void * priv,
                                 int stream,
                                 bg_stream_action_t action);
int bg_avdec_start(void * priv);
void bg_avdec_seek(void * priv, int64_t * t, int scale);

gavl_frame_table_t * bg_avdec_get_frame_table(void * priv, int stream);


int bg_avdec_init(avdec_priv * avdec);

const char * bg_avdec_get_disc_name(void * priv);


void
bg_avdec_set_parameter(void * p, const char * name,
                       const bg_parameter_value_t * val);

int bg_avdec_get_num_tracks(void * p);

int bg_avdec_set_track(void * priv, int track);

void bg_avdec_set_callbacks(void * priv,
                            bg_input_callbacks_t * callbacks);

int bg_avdec_get_audio_compression_info(void * priv, int stream,
                                        gavl_compression_info_t * info);


int bg_avdec_get_video_compression_info(void * priv, int stream,
                                        gavl_compression_info_t * info);

int bg_avdec_read_audio_packet(void * priv, int stream, gavl_packet_t * p);

int bg_avdec_read_video_packet(void * priv, int stream, gavl_packet_t * p);



bg_device_info_t * bg_avdec_get_devices(bgav_device_info_t *);

#include "options.h"
