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
      .name =       "AVI",
      .short_name = "avi",
      .extension =  "avi",
      .max_audio_streams = 1,
      .max_video_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_PCM_S16LE,
                                         CODEC_ID_PCM_U8,
                                         CODEC_ID_PCM_ALAW,
                                         CODEC_ID_PCM_MULAW,
                                         CODEC_ID_MP3,
                                         CODEC_ID_MP2,
                                         CODEC_ID_AC3,
                                         CODEC_ID_NONE },

      .video_codecs = (enum CodecID[]){  CODEC_ID_MPEG4,
                                       CODEC_ID_MSMPEG4V3,
                                       CODEC_ID_MJPEG,
                                       CODEC_ID_NONE },
      .flags = FLAG_CONSTANT_FRAMERATE,
    },
    {
      .name =       "MPEG-1",
      .short_name = "mpeg",
      .extension =  "mpg",
      .max_audio_streams = -1,
      .max_video_streams = -1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_MP2,
                                       CODEC_ID_MP3,
                                       CODEC_ID_NONE },

      .video_codecs = (enum CodecID[]){  CODEC_ID_MPEG1VIDEO,
                                       CODEC_ID_NONE },
      .flags = FLAG_CONSTANT_FRAMERATE | FLAG_PIPE,
    },
    {
      .name =       "MPEG-2 (generic)",
      .short_name = "vob",
      .extension =  "vob",
      .max_audio_streams = -1,
      .max_video_streams = -1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_MP2,
                                         CODEC_ID_MP3,
                                         CODEC_ID_AC3,
                                         CODEC_ID_NONE },

      .video_codecs = (enum CodecID[]){  CODEC_ID_MPEG2VIDEO,
                                         CODEC_ID_NONE },
      .flags = FLAG_CONSTANT_FRAMERATE | FLAG_PIPE,
    },
    {
      .name =       "MPEG-2 (dvd)",
      .short_name = "dvd",
      .extension =  "vob",
      .max_audio_streams = -1,
      .max_video_streams = -1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_MP2,
                                         CODEC_ID_AC3,
                                         CODEC_ID_NONE },

      .video_codecs = (enum CodecID[]){  CODEC_ID_MPEG2VIDEO,
                                         CODEC_ID_NONE },
      .flags = FLAG_CONSTANT_FRAMERATE | FLAG_PIPE,
    },
    {
      .name =       "Flash Video",
      .short_name = "flv",
      .extension =  "flv",
      .max_audio_streams = 1,
      .max_video_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_MP3,
                                         CODEC_ID_AAC,
                                         CODEC_ID_NONE },
      
      .video_codecs = (enum CodecID[]){  CODEC_ID_FLV1,
                                         CODEC_ID_H264,
                                         CODEC_ID_NONE },
    },
    {
      .name =       "ASF",
      .short_name = "asf",
      .extension =  "asf",
      .max_audio_streams = 1,
      .max_video_streams = 1,
      .audio_codecs = (enum CodecID[]){
#if LIBAVCODEC_BUILD >= ((51<<16)+(32<<8)+0)
                                       CODEC_ID_WMAV2,
                                       CODEC_ID_WMAV1,
#endif
                                       CODEC_ID_MP3,
                                       CODEC_ID_MP2,
                                       CODEC_ID_NONE },
      
      .video_codecs = (enum CodecID[]){  CODEC_ID_WMV1,
                                       // CODEC_ID_WMV2, /* Crash */
                                       CODEC_ID_NONE },
    },
    {
      .name =       "MPEG-2 Transport stream",
      .short_name = "mpegts",
      .extension =  "ts",
      .max_audio_streams = 1,
      .max_video_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_MP3,
                                       CODEC_ID_MP2,
                                       CODEC_ID_AC3,
                                       CODEC_ID_NONE },
      
      .video_codecs = (enum CodecID[]){  CODEC_ID_MPEG1VIDEO,
                                       CODEC_ID_MPEG2VIDEO,
                                       CODEC_ID_NONE },
      .flags = FLAG_CONSTANT_FRAMERATE | FLAG_PIPE,
    },
    {
      .name =       "Matroska",
      .short_name = "matroska",
      .extension =  "mkv",
      .max_audio_streams = -1,
      .max_video_streams = -1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_MP3,
                                         CODEC_ID_MP2,
                                         CODEC_ID_AC3,
                                         CODEC_ID_NONE },
      
      .video_codecs = (enum CodecID[]){
                                         CODEC_ID_H264,
                                         CODEC_ID_MPEG4,
                                         CODEC_ID_MPEG1VIDEO,
                                         CODEC_ID_MPEG2VIDEO,
                                         CODEC_ID_NONE },
      //      .flags = FLAG_CONSTANT_FRAMERATE,
      //      .framerates = bg_ffmpeg_mpeg_framerates,
    },
#if 0 // Encoded file is messed up
    {
      .name =       "Real Media",
      .short_name = "rm",
      .extension =  "rm",
      .max_audio_streams = 1,
      .max_video_streams = 1,
      .audio_codecs = (enum CodecID[]){  CODEC_ID_AC3,
                                       CODEC_ID_NONE },
      
      .video_codecs = (enum CodecID[]){  CODEC_ID_RV10,
                                       CODEC_ID_NONE },
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
      .name =           "e_ffmpeg",       /* Unique short name */
      .long_name =      TRS("FFmpeg audio/video encoder"),
      .description =    TRS("Plugin for encoding various audio/video formats with ffmpeg \
(http://www.ffmpeg.org)."),
      .type =           BG_PLUGIN_ENCODER,
      .flags =          BG_PLUGIN_FILE | BG_PLUGIN_PIPE,
      .priority =       5,
      .create =         create_ffmpeg,
      .destroy =        bg_ffmpeg_destroy,
      .get_parameters = bg_ffmpeg_get_parameters,
      .set_parameter =  bg_ffmpeg_set_parameter,
    },
    
    .max_audio_streams =         -1,
    .max_video_streams =         -1,
    .max_text_streams = -1,
    
    .get_audio_parameters = bg_ffmpeg_get_audio_parameters,
    .get_video_parameters = bg_ffmpeg_get_video_parameters,

    .set_callbacks =        bg_ffmpeg_set_callbacks,
    
    .open =                 bg_ffmpeg_open,

    .writes_compressed_audio = bg_ffmpeg_writes_compressed_audio,
    .writes_compressed_video = bg_ffmpeg_writes_compressed_video,
    
    .add_audio_stream =     bg_ffmpeg_add_audio_stream,
    .add_video_stream =     bg_ffmpeg_add_video_stream,
    .add_text_stream =     bg_ffmpeg_add_text_stream,

    .add_audio_stream_compressed =     bg_ffmpeg_add_audio_stream_compressed,
    .add_video_stream_compressed =     bg_ffmpeg_add_video_stream_compressed,

    .set_video_pass =       bg_ffmpeg_set_video_pass,
    .set_audio_parameter =  bg_ffmpeg_set_audio_parameter,
    .set_video_parameter =  bg_ffmpeg_set_video_parameter,

    .get_audio_sink =     bg_ffmpeg_get_audio_sink,
    .get_audio_packet_sink =     bg_ffmpeg_get_audio_packet_sink,

    .get_video_sink =     bg_ffmpeg_get_video_sink,
    .get_video_packet_sink =     bg_ffmpeg_get_video_packet_sink,
    
    .start =                bg_ffmpeg_start,
    
    .get_text_sink = bg_ffmpeg_get_text_packet_sink,

    
    .close =                bg_ffmpeg_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
