/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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
#include <gmerlin/translation.h>
#include "ffmpeg_common.h"

static const ffmpeg_format_info_t formats[] =
  {
    {
      .name = "SUN AU Format",
      .short_name = "au",
      .extension =  "au",
      .max_audio_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_PCM_MULAW,
                                       CODEC_ID_PCM_S16BE,
                                       CODEC_ID_PCM_ALAW,
                                       CODEC_ID_NONE },
    },
    {
      .name = "Raw AC3",
      .short_name = "ac3",
      .extension =  "ac3",
      .max_audio_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_AC3,
                                       CODEC_ID_NONE },
      .flags = FLAG_PIPE,
    },
    {
      .name =       "AIFF",
      .short_name = "aiff",
      .extension =  "aif",
      .max_audio_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_PCM_S16BE,
                                       CODEC_ID_PCM_S8,
                                       CODEC_ID_PCM_ALAW,
                                       CODEC_ID_PCM_MULAW,
                                       CODEC_ID_NONE },
    },
    {
      .name =       "MP2",
      .short_name = "mp2",
      .extension =  "mp2",
      .max_audio_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_MP2,
                                       CODEC_ID_NONE },
      .flags = FLAG_PIPE,
    },
#if LIBAVCODEC_BUILD >= ((51<<16)+(32<<8)+0)
    {
      .name =       "WMA",
      .short_name = "asf",
      .extension =  "wma",
      .max_audio_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_WMAV2,
                                       CODEC_ID_WMAV1,
                                       CODEC_ID_NONE },
    },
#endif

#if 0 
    {
      .name =       "ADTS",
      .short_name = "adts",
      .extension =  "aac",
      .max_audio_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_AAC,
                                       CODEC_ID_NONE },
      .flags = FLAG_PIPE,
    },
#endif
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
      .name =           "e_ffmpeg_audio",     /* Unique short name */
      .long_name =      TRS("FFmpeg audio encoder"),
      .description =    TRS("Plugin for encoding various audio formats with ffmpeg \
(http://www.ffmpeg.org)."),
      .type =           BG_PLUGIN_ENCODER_AUDIO,
      .flags =          BG_PLUGIN_FILE | BG_PLUGIN_PIPE,
      .priority =       5,
      .create =         create_ffmpeg,
      .destroy =        bg_ffmpeg_destroy,
      .get_parameters = bg_ffmpeg_get_parameters,
      .set_parameter =  bg_ffmpeg_set_parameter,
    },
    
    .max_audio_streams =         1,
    
    .get_audio_parameters = bg_ffmpeg_get_audio_parameters,

    .set_callbacks =        bg_ffmpeg_set_callbacks,
    
    .open =                 bg_ffmpeg_open,
    
    .add_audio_stream =     bg_ffmpeg_add_audio_stream,
    
    .set_audio_parameter =  bg_ffmpeg_set_audio_parameter,
    
    .get_audio_sink =       bg_ffmpeg_get_audio_sink,
    .get_audio_packet_sink =       bg_ffmpeg_get_audio_packet_sink,

    .start =                bg_ffmpeg_start,
    
    .close =                bg_ffmpeg_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
