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

#ifndef __BG_ENCODERINFO_H_
#define __BG_ENCODERINFO_H_

#include <gmerlin/pluginregistry.h>

/* Information about an encoder setup */

typedef struct bg_transcoder_track_s bg_transcoder_track_t;

/* Container structure for all encoding related
   stuff. */

typedef struct
  {
  const bg_plugin_info_t * audio_info;
  const bg_plugin_info_t * video_info;
  const bg_plugin_info_t * subtitle_text_info;
  const bg_plugin_info_t * subtitle_overlay_info;

  /* Global sections for encoders */
  bg_cfg_section_t * audio_encoder_section;
  bg_cfg_section_t * video_encoder_section;
  bg_cfg_section_t * subtitle_text_encoder_section;
  bg_cfg_section_t * subtitle_overlay_encoder_section;

  bg_parameter_info_t * audio_encoder_parameters;
  bg_parameter_info_t * video_encoder_parameters;
  bg_parameter_info_t * subtitle_text_encoder_parameters;
  bg_parameter_info_t * subtitle_overlay_encoder_parameters;
  
  /* Per stream sections for encoders */
  bg_cfg_section_t * audio_stream_section;
  bg_cfg_section_t * video_stream_section;
  bg_cfg_section_t * subtitle_text_stream_section;
  bg_cfg_section_t * subtitle_overlay_stream_section;

  bg_parameter_info_t * audio_stream_parameters;
  bg_parameter_info_t * video_stream_parameters;
  bg_parameter_info_t * subtitle_text_stream_parameters;
  bg_parameter_info_t * subtitle_overlay_stream_parameters;
    
  } bg_encoder_info_t;

int
bg_encoder_info_get_from_registry(bg_plugin_registry_t * plugin_reg,
                                  bg_encoder_info_t * encoder_info);


int
bg_encoder_info_get_from_track(bg_plugin_registry_t * plugin_reg,
                               bg_transcoder_track_t * track,
                               bg_encoder_info_t * encoder_info);

void
bg_encoder_info_get_sections_from_track(bg_transcoder_track_t * track,
                                        bg_encoder_info_t * encoder_info);

#endif // __BG_ENCODERINFO_H_
