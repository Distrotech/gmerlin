/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

void
bg_encoder_set_callbacks(bg_encoder_t * e, bg_encoder_callbacks_t * cb);

void
bg_encoder_set_video_pass(bg_encoder_t * enc,
                          int stream, int pass, int total_passes,
                          const char * stats_file);


/* Also closes all internal encoders */
void bg_encoder_destroy(bg_encoder_t * enc, int do_delete); 

int bg_encoder_open(bg_encoder_t * enc, const char * filename_base,
                    const bg_metadata_t * metadata,
                    const bg_chapter_list_t * chapter_list);

int bg_encoder_writes_compressed_audio(bg_encoder_t * enc,
                                       const gavl_audio_format_t * format,
                                       const gavl_compression_info_t * info);

int bg_encoder_writes_compressed_video(bg_encoder_t * enc,
                                       const gavl_video_format_t * format,
                                       const gavl_compression_info_t * info);

/* Add streams */
int bg_encoder_add_audio_stream(bg_encoder_t *, const char * language,
                                const gavl_audio_format_t * format,
                                int index);

int bg_encoder_add_video_stream(bg_encoder_t *,
                                const gavl_video_format_t * format,
                                int index);

int bg_encoder_add_audio_stream_compressed(bg_encoder_t *, const char * language,
                                           const gavl_audio_format_t * format,
                                           const gavl_compression_info_t * info,
                                           int index);

int bg_encoder_add_video_stream_compressed(bg_encoder_t *,
                                           const gavl_video_format_t * format,
                                           const gavl_compression_info_t * info,
                                           int index);


int bg_encoder_add_subtitle_text_stream(bg_encoder_t *, const char * language,
                                        int timescale,
                                        int index);

int bg_encoder_add_subtitle_overlay_stream(bg_encoder_t *, const char * language,
                                           const gavl_video_format_t * format,
                                           int index, bg_stream_type_t source_format);


/* Get formats */
void bg_encoder_get_audio_format(bg_encoder_t *, int stream, gavl_audio_format_t*ret);
void bg_encoder_get_video_format(bg_encoder_t *, int stream, gavl_video_format_t*ret);
void bg_encoder_get_subtitle_overlay_format(bg_encoder_t *, int stream, gavl_video_format_t*ret);
void bg_encoder_get_subtitle_text_timescale(bg_encoder_t *, int stream, int * ret);


/* Start encoding */
int bg_encoder_start(bg_encoder_t *);

/* Write frame */
int bg_encoder_write_audio_frame(bg_encoder_t *, gavl_audio_frame_t * frame, int stream);
int bg_encoder_write_video_frame(bg_encoder_t *, gavl_video_frame_t * frame, int stream);
int bg_encoder_write_subtitle_text(bg_encoder_t *,const char * text,
                                   int64_t start, int64_t duration, int stream);
int bg_encoder_write_subtitle_overlay(bg_encoder_t *, gavl_overlay_t * ovl, int stream);

int bg_encoder_write_audio_packet(bg_encoder_t *, gavl_packet_t * p, int stream);
int bg_encoder_write_video_packet(bg_encoder_t *, gavl_packet_t * p, int stream);
