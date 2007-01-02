#include "ffmpeg_common.h"

static ffmpeg_format_info_t formats[] =
  {
    {
      name:       "AVI",
      short_name: "avi",
      extension:  ".avi",
      max_audio_streams: 1,
      max_video_streams: 1,
      audio_codecs: (enum CodecID[]){  CODEC_ID_PCM_S16LE,
                                       CODEC_ID_PCM_U8,
                                       CODEC_ID_PCM_ALAW,
                                       CODEC_ID_PCM_MULAW,
                                       CODEC_ID_MP3,
                                       CODEC_ID_MP2,
                                       CODEC_ID_NONE },

      video_codecs: (enum CodecID[]){  CODEC_ID_MPEG4,
                                       CODEC_ID_MSMPEG4V3,
                                       CODEC_ID_MJPEG,
                                       CODEC_ID_NONE },
    },
    {
      name:       "MPEG-1",
      short_name: "mpeg",
      extension:  ".mpg",
      max_audio_streams: -1,
      max_video_streams: -1,
      audio_codecs: (enum CodecID[]){  CODEC_ID_MP2,
                                       CODEC_ID_MP3,
                                       CODEC_ID_NONE },

      video_codecs: (enum CodecID[]){  CODEC_ID_MPEG1VIDEO,
                                       CODEC_ID_NONE },
    },
    {
      name:       "MPEG-2",
      short_name: "vob",
      extension:  ".vob",
      max_audio_streams: -1,
      max_video_streams: -1,
      audio_codecs: (enum CodecID[]){  CODEC_ID_MP2,
                                       CODEC_ID_MP3,
                                       CODEC_ID_AC3,
                                       CODEC_ID_NONE },

      video_codecs: (enum CodecID[]){  CODEC_ID_MPEG2VIDEO,
                                       CODEC_ID_NONE },
    },
    {
      name:       "Flash Video",
      short_name: "flv",
      extension:  ".flv",
      max_audio_streams: 1,
      max_video_streams: 1,
      audio_codecs: (enum CodecID[]){  CODEC_ID_MP3,
                                       CODEC_ID_NONE },
      
      video_codecs: (enum CodecID[]){  CODEC_ID_FLV1,
                                       CODEC_ID_NONE },
    },
    {
      name:       "ASF",
      short_name: "asf",
      extension:  ".asf",
      max_audio_streams: 1,
      max_video_streams: 1,
      audio_codecs: (enum CodecID[]){  CODEC_ID_MP3,
                                       CODEC_ID_MP2,
                                       CODEC_ID_NONE },
      
      video_codecs: (enum CodecID[]){  CODEC_ID_WMV1,
                                       // CODEC_ID_WMV2, /* Crash */
                                       CODEC_ID_NONE },
    },
#if 0 // Encoded file is messed up
    {
      name:       "Real Media",
      short_name: "rm",
      extension:  ".rm",
      max_audio_streams: 1,
      max_video_streams: 1,
      audio_codecs: (enum CodecID[]){  CODEC_ID_AC3,
                                       CODEC_ID_NONE },
      
      video_codecs: (enum CodecID[]){  CODEC_ID_RV10,
                                       CODEC_ID_NONE },
    },
#endif
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
      name:           "e_ffmpeg",       /* Unique short name */
      long_name:      "FFmpeg audio/video encoder",
      mimetypes:      NULL,
      extensions:     "avi",
      type:           BG_PLUGIN_ENCODER,
      flags:          BG_PLUGIN_FILE,
      priority:       5,
      create:         create_ffmpeg,
      destroy:        bg_ffmpeg_destroy,
      get_parameters: bg_ffmpeg_get_parameters,
      set_parameter:  bg_ffmpeg_set_parameter,
      //      get_error:      get_error_lqt,
    },
    
    max_audio_streams:         -1,
    max_video_streams:         -1,
    
    get_audio_parameters: bg_ffmpeg_get_audio_parameters,
    get_video_parameters: bg_ffmpeg_get_video_parameters,

    get_extension:        bg_ffmpeg_get_extension,
    
    open:                 bg_ffmpeg_open,
    
    add_audio_stream:     bg_ffmpeg_add_audio_stream,
    
    add_video_stream:     bg_ffmpeg_add_video_stream,
    set_video_pass:       bg_ffmpeg_set_video_pass,
    set_audio_parameter:  bg_ffmpeg_set_audio_parameter,
    set_video_parameter:  bg_ffmpeg_set_video_parameter,
    
    get_audio_format:     bg_ffmpeg_get_audio_format,
    get_video_format:     bg_ffmpeg_get_video_format,

    start:                bg_ffmpeg_start,
    
    write_audio_frame:    bg_ffmpeg_write_audio_frame,
    write_video_frame:    bg_ffmpeg_write_video_frame,
    close:                bg_ffmpeg_close,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
