/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

/* Frontend for encoder plugins:
   It transparently handles the case that each stream goes to a
   separate file or not.
*/

#include <gmerlin/transcoder_track.h>

typedef struct bg_encoder_s bg_encoder_t;

bg_encoder_t * bg_encoder_create(bg_plugin_registry_t * plugin_reg,
                                 bg_cfg_section_t * section,
                                 bg_transcoder_track_t * tt,
                                 int stream_mask, int flag_mask);

void bg_encoder_destroy(bg_encoder_t * enc); /* Also closes all internal encoders */

int bg_encoder_open(bg_encoder_t * enc, const char * filename_base);

/* Add streams */
int bg_encoder_add_audio_stream(bg_encoder_t *, const char * language,
                                gavl_audio_format_t * format,
                                int index);

int bg_encoder_add_video_stream(bg_encoder_t *,
                                gavl_video_format_t * format,
                                int index);

int bg_encoder_add_subtitle_text_stream(bg_encoder_t *, const char * language,
                                        int timescale,
                                        int index);

int bg_encoder_add_subtitle_overlay_stream(bg_encoder_t *, const char * language,
                                           gavl_video_format_t * format,
                                           int index);


/* Get formats */
void bg_encoder_get_audio_format(bg_encoder_t *, int stream, gavl_audio_format_t*ret);
void bg_encoder_get_video_format(bg_encoder_t *, int stream, gavl_video_format_t*ret);
void bg_encoder_get_subtitle_overlay_format(bg_encoder_t *, int stream, gavl_video_format_t*ret);

/* Start encoding */
int bg_encoder_start(bg_encoder_t *);

/* Write frame */
int bg_encoder_write_audio_frame(bg_encoder_t *, gavl_audio_frame_t * frame, int stream);
int bg_encoder_write_video_frame(bg_encoder_t *, gavl_video_frame_t * frame, int stream);
int bg_encoder_write_subtitle_text(bg_encoder_t *,const char * text,
                                   int64_t start, int64_t duration, int stream);
int bg_encoder_write_subtitle_overlay(bg_encoder_t *, gavl_overlay_t * ovl, int stream);
