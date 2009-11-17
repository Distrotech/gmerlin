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

#include <gmerlin/pluginregistry.h>
#include <gmerlin/encoder.h>

#define SEPARATE_AUDIO            (1<<0)
#define SEPARATE_VIDEO            (1<<1)
#define SEPARATE_SUBTITLE_TEXT    (1<<3)
#define SEPARATE_SUBTITLE_OVERLAY (1<<4)

typedef struct
  {
  int index;

  bg_encoder_plugin_t * plugin;
  void * priv;
  
  } audio_stream_t;

typedef struct
  {
  int index;

  bg_encoder_plugin_t * plugin;
  void * priv;
  
  } video_stream_t;

typedef struct
  {
  int index;

  bg_encoder_plugin_t * plugin;
  void * priv;
  
  } subtitle_text_stream_t;

typedef struct
  {
  int index;

  bg_encoder_plugin_t * plugin;
  void * priv;

  } subtitle_overlay_stream_t;

struct bg_encoder_s
  {
  const bg_plugin_info_t * audio_info;
  const bg_plugin_info_t * video_info;
  const bg_plugin_info_t * subtitle_text_info;
  const bg_plugin_info_t * subtitle_overlay_info;
  
  int num_audio_streams;
  int num_video_streams;
  int num_subtitle_text_streams;
  int num_subtitle_overlay_streams;

  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  subtitle_text_stream_t * subtitle_text_streams;
  subtitle_overlay_stream_t * subtitle_overlay_streams;
  
  int num_plugins;
  bg_plugin_handle_t ** plugins;

  int flags;
  };

bg_encoder_t * bg_encoder_create(bg_plugin_registry_t * plugin_reg,
                                 bg_cfg_section_t * section,
                                 int type_mask, int flag_mask)
  {
  bg_encoder_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_encoder_destroy(bg_encoder_t * enc)
     /* Also closes all internal encoders */
  {
  free(enc);
  }

int bg_encoder_open(bg_encoder_t * enc, const char * filename_base)
  {
  return 0;
  }

#define REALLOC_STREAM(streams, num) \
  streams = realloc(streams, sizeof(streams)*(num+1));\
  s = &streams[num]

static bg_plugin_handle_t *
load_plugin_separate(bg_encoder_t * enc, const char * name)
  {
  bg_plugin_info_t * info;
  enc->plugins = realloc(enc->plugins,
                         (enc->num_plugins+1)* sizeof(enc->plugins));
  
  }

static bg_plugin_handle_t *
load_plugin_common(bg_encoder_t * enc)
  {
  
  }

/* Add streams */
int bg_encoder_add_audio_stream(bg_encoder_t * enc,
                                const char * language,
                                gavl_audio_format_t * format,
                                bg_cfg_section_t * section)
  {
  int ret;
  audio_stream_t * s;
  
  REALLOC_STREAM(enc->audio_streams,
                 enc->num_audio_streams);
  
  ret = enc->num_audio_streams;
  enc->num_audio_streams++;
  return ret;
  }

int bg_encoder_add_video_stream(bg_encoder_t * enc,
                                gavl_video_format_t * format,
                                bg_cfg_section_t * section)
  {
  int ret;
  video_stream_t * s;

  REALLOC_STREAM(enc->video_streams,
                 enc->num_video_streams);
  
  ret = enc->num_video_streams;
  enc->num_video_streams++;
  return ret;
  }

int bg_encoder_add_subtitle_text_stream(bg_encoder_t * enc,
                                        const char * language,
                                        int timescale,
                                        bg_cfg_section_t * section)
  {
  int ret;
  subtitle_text_stream_t * s;

  REALLOC_STREAM(enc->subtitle_text_streams,
                 enc->num_subtitle_text_streams);

  ret = enc->num_subtitle_text_streams;
  enc->num_subtitle_text_streams++;
  return ret;

  }

int bg_encoder_add_subtitle_overlay_stream(bg_encoder_t * enc,
                                           const char * language,
                                           gavl_video_format_t * format,
                                           bg_cfg_section_t * section)
  {
  int ret;
  subtitle_overlay_stream_t * s;

  REALLOC_STREAM(enc->subtitle_overlay_streams,
                 enc->num_subtitle_overlay_streams);
  
  ret = enc->num_subtitle_overlay_streams;
  enc->num_subtitle_overlay_streams++;
  return ret;
  }

/* Start encoding */
int bg_encoder_start(bg_encoder_t * enc)
  {
  int i;

  for(i = 0; i < enc->num_plugins; i++)
    {
    
    }
  
  return 0;
  }

/* Get formats */
void bg_encoder_get_audio_format(bg_encoder_t * enc,
                                 int stream,
                                 gavl_audio_format_t*ret)
  {
  audio_stream_t * s = &enc->audio_streams[stream];
  s->plugin->get_audio_format(s->priv, s->index, ret);
  }

void bg_encoder_get_video_format(bg_encoder_t * enc,
                                 int stream,
                                 gavl_video_format_t*ret)
  {
  video_stream_t * s = &enc->video_streams[stream];
  s->plugin->get_video_format(s->priv, s->index, ret);
  }

void bg_encoder_get_subtitle_overlay_format(bg_encoder_t * enc,
                                            int stream,
                                            gavl_video_format_t*ret)
  {
  subtitle_overlay_stream_t * s = &enc->subtitle_overlay_streams[stream];
  s->plugin->get_subtitle_overlay_format(s->priv, s->index, ret);
  }


/* Write frame */
int bg_encoder_write_audio_frame(bg_encoder_t * enc,
                                 gavl_audio_frame_t * frame,
                                 int stream)
  {
  audio_stream_t * s = &enc->audio_streams[stream];
  s->plugin->write_audio_frame(s->priv, frame, s->index);
  }
  

int bg_encoder_write_video_frame(bg_encoder_t * enc,
                                 gavl_video_frame_t * frame,
                                 int stream)
  {
  video_stream_t * s = &enc->video_streams[stream];
  s->plugin->write_video_frame(s->priv, frame, s->index);
  }

int bg_encoder_write_subtitle_text(bg_encoder_t * enc,
                                   const char * text,
                                   int64_t start,
                                   int64_t duration, int stream)
  {
  subtitle_text_stream_t * s = &enc->subtitle_text_streams[stream];
  s->plugin->write_subtitle_text(s->priv, text, start, duration, s->index);
  }

int bg_encoder_write_subtitle_overlay(bg_encoder_t * enc,
                                      gavl_overlay_t * ovl, int stream)
  {
  subtitle_overlay_stream_t * s = &enc->subtitle_overlay_streams[stream];
  s->plugin->write_subtitle_overlay(s->priv, ovl, s->index);
  }
