/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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
#include <gmerlin/translation.h>

#include "ffmpeg_common.h"


static const ffmpeg_format_info_t formats[] =
  {
    {
      .name =       "MPEG-1 video",
      .short_name = "mpeg1video",
      .extension =  "m1v",
      .max_video_streams = 1,
      .video_codecs = (enum CodecID[]){  CODEC_ID_MPEG1VIDEO,
                                       CODEC_ID_NONE },
      .flags = FLAG_CONSTANT_FRAMERATE,
      .framerates = bg_ffmpeg_mpeg_framerates,
    },
    {
      .name =       "MPEG-2 video",
      .short_name = "mpeg2video",
      .extension =  "m2v",
      .max_video_streams = 1,
      .video_codecs = (enum CodecID[]){  CODEC_ID_MPEG2VIDEO,
                                       CODEC_ID_NONE },
      .flags = FLAG_CONSTANT_FRAMERATE,
      .framerates = bg_ffmpeg_mpeg_framerates,
    },
    { /* End of formats */ }
  };

static void * create_ffmpeg()
  {
  return bg_ffmpeg_create(formats);
  }

const bg_encoder_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "e_ffmpeg_video",       /* Unique short name */
      .long_name =      TRS("FFmpeg video encoder"),
      .description =    TRS("Plugin for encoding various video formats with ffmpeg \
(http://www.ffmpeg.org)."),
      .type =           BG_PLUGIN_ENCODER_VIDEO,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         create_ffmpeg,
      .destroy =        bg_ffmpeg_destroy,
      .get_parameters = bg_ffmpeg_get_parameters,
      .set_parameter =  bg_ffmpeg_set_parameter,
    },
    
    .max_video_streams =         1,
    
    .get_video_parameters = bg_ffmpeg_get_video_parameters,

    .set_callbacks =        bg_ffmpeg_set_callbacks,
    
    .open =                 bg_ffmpeg_open,
    
    
    .add_video_stream =     bg_ffmpeg_add_video_stream,
    .set_video_pass =       bg_ffmpeg_set_video_pass,
    .set_video_parameter =  bg_ffmpeg_set_video_parameter,
    
    .get_video_format =     bg_ffmpeg_get_video_format,

    .start =                bg_ffmpeg_start,
    
    .write_video_frame =    bg_ffmpeg_write_video_frame,
    .close =                bg_ffmpeg_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
