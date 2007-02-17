#include <config.h>
#include <gmerlin/translation.h>

#include "ffmpeg_common.h"


static ffmpeg_format_info_t formats[] =
  {
    {
      name:       "MPEG-1 video",
      short_name: "mpeg1video",
      extension:  ".m1v",
      max_video_streams: 1,
      video_codecs: (enum CodecID[]){  CODEC_ID_MPEG1VIDEO,
                                       CODEC_ID_NONE },
    },
    {
      name:       "MPEG-2 video",
      short_name: "mpeg2video",
      extension:  ".m2v",
      max_video_streams: 1,
      video_codecs: (enum CodecID[]){  CODEC_ID_MPEG2VIDEO,
                                       CODEC_ID_NONE },
    },
    { /* End of formats */ }
  };

static void * create_ffmpeg()
  {
  return bg_ffmpeg_create(formats);
  }

bg_encoder_plugin_t the_plugin =
  {
    common:
    {
      BG_LOCALE,
      name:           "e_ffmpeg_video",       /* Unique short name */
      long_name:      TRS("FFmpeg video encoder"),
      description:    TRS("Plugin for encoding various video formats with ffmpeg \
(http://www.ffmpeg.org)."),
      mimetypes:      NULL,
      extensions:     "m1v",
      type:           BG_PLUGIN_ENCODER_VIDEO,
      flags:          BG_PLUGIN_FILE,
      priority:       5,
      create:         create_ffmpeg,
      destroy:        bg_ffmpeg_destroy,
      get_parameters: bg_ffmpeg_get_parameters,
      set_parameter:  bg_ffmpeg_set_parameter,
    },
    
    max_video_streams:         1,
    
    get_video_parameters: bg_ffmpeg_get_video_parameters,

    get_extension:        bg_ffmpeg_get_extension,
    
    open:                 bg_ffmpeg_open,
    
    
    add_video_stream:     bg_ffmpeg_add_video_stream,
    set_video_pass:       bg_ffmpeg_set_video_pass,
    set_video_parameter:  bg_ffmpeg_set_video_parameter,
    
    get_video_format:     bg_ffmpeg_get_video_format,

    start:                bg_ffmpeg_start,
    
    write_video_frame:    bg_ffmpeg_write_video_frame,
    close:                bg_ffmpeg_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
