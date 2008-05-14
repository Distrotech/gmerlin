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

#include <config.h>

typedef struct
  {
  bg_track_info_t * track_info;
  bg_track_info_t * current_track;
  int num_tracks;
  bgav_t * dec;
  bgav_options_t * opt;
  
  bg_input_callbacks_t * bg_callbacks;
  
  bg_edl_t * edl;
  } avdec_priv;

bg_edl_t * bg_avdec_convert_edl(const bgav_edl_t * edl);

const bg_edl_t * bg_avdec_get_edl(void * priv);

void * bg_avdec_create();

void bg_avdec_close(void * priv);
void bg_avdec_destroy(void * priv);
bg_track_info_t * bg_avdec_get_track_info(void * priv, int track);

int bg_avdec_read_video(void * priv,
                        gavl_video_frame_t * frame,
                        int stream);

int bg_avdec_read_audio(void * priv,
                        gavl_audio_frame_t * frame,
                        int stream,
                        int num_samples);

int bg_avdec_read_subtitle_overlay(void * priv,
                                   gavl_overlay_t * ovl, int stream);
  
int bg_avdec_read_subtitle_text(void * priv,
                                char ** text, int * text_alloc,
                                int64_t * start_time,
                                int64_t * duration,
                                int stream);

int bg_avdec_has_subtitle(void * priv, int stream);

int bg_avdec_set_audio_stream(void * priv,
                                  int stream,
                              bg_stream_action_t action);
int bg_avdec_set_video_stream(void * priv,
                              int stream,
                              bg_stream_action_t action);

int bg_avdec_set_subtitle_stream(void * priv,
                                 int stream,
                                 bg_stream_action_t action);
int bg_avdec_start(void * priv);
void bg_avdec_seek(void * priv, int64_t * t, int scale);

int bg_avdec_init(avdec_priv * avdec);

const char * bg_avdec_get_disc_name(void * priv);


void
bg_avdec_set_parameter(void * p, const char * name,
                       const bg_parameter_value_t * val);
int bg_avdec_get_num_tracks(void * p);

int bg_avdec_set_track(void * priv, int track);

void bg_avdec_set_callbacks(void * priv,
                            bg_input_callbacks_t * callbacks);

bg_device_info_t * bg_avdec_get_devices(bgav_device_info_t *);

/* Commonly used parameters */

#define PARAM_DYNRANGE \
  {                    \
  .name = "audio_dynrange",    \
  .long_name = TRS("Dynamic range control"),         \
  .type = BG_PARAMETER_CHECKBUTTON,           \
  .val_default = { .val_i = 1 },              \
  .help_string = TRS("Enable dynamic range control for codecs, which support this (currently only A52 and DTS).") \
  }

#define PARAM_PP_LEVEL \
  {                    \
  .name = "video_pp_level",    \
  .long_name = TRS("Postprocessing level"),         \
  .opt = "pp", \
  .type = BG_PARAMETER_SLIDER_INT,           \
  .val_default = { .val_i = 1 },              \
  .val_min =     { .val_i = 0 },              \
  .val_max = { .val_i = 6 },              \
  .help_string = TRS("Set postprocessing (to remove compression artifacts). 0 means no postprocessing, 6 means maximum postprocessing.") \
  }

