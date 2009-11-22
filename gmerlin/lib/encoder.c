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

#include <config.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/encoder.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "encoder"

/* Encoder flags */

typedef struct
  {
  int in_index;
  int out_index;

  bg_encoder_plugin_t * plugin;
  void * priv;

  gavl_audio_format_t format;

  bg_cfg_section_t * section;
  const bg_parameter_info_t * parameters;

  char * language;
  
  } audio_stream_t;

typedef struct
  {
  int in_index;
  int out_index;

  bg_encoder_plugin_t * plugin;
  void * priv;

  gavl_video_format_t format;
  
  bg_cfg_section_t * section;
  const bg_parameter_info_t * parameters;

  } video_stream_t;

typedef struct
  {
  int in_index;
  int out_index;

  bg_encoder_plugin_t * plugin;
  void * priv;

  int timescale;
  
  bg_cfg_section_t * section;
  const bg_parameter_info_t * parameters;

  char * language;

  } subtitle_text_stream_t;

typedef struct
  {
  int in_index;
  int out_index;

  bg_encoder_plugin_t * plugin;
  void * priv;

  gavl_video_format_t format;

  bg_cfg_section_t * section;
  const bg_parameter_info_t * parameters;

  char * language;
  
  } subtitle_overlay_stream_t;

typedef struct
  {
  const bg_plugin_info_t * info;
  bg_cfg_section_t * section;
  } plugin_config_t;

typedef struct
  {
  bg_cfg_section_t * section;
  } stream_config_t;

struct bg_encoder_s
  {
  plugin_config_t audio_plugin;
  plugin_config_t video_plugin;
  plugin_config_t subtitle_text_plugin;
  plugin_config_t subtitle_overlay_plugin;

  stream_config_t audio_stream;
  stream_config_t video_stream;
  stream_config_t subtitle_text_stream;
  stream_config_t subtitle_overlay_stream;
  
  int num_audio_streams;
  int num_video_streams;
  int num_subtitle_text_streams;
  int num_subtitle_overlay_streams;

  int total_streams;
  
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  subtitle_text_stream_t * subtitle_text_streams;
  subtitle_overlay_stream_t * subtitle_overlay_streams;
  
  int num_plugins;
  bg_plugin_handle_t ** plugins;

  int separate;
  
  bg_plugin_registry_t * plugin_reg;

  /* Config stuff */
  bg_cfg_section_t * es;
  bg_transcoder_track_t * tt;

  int stream_mask;

  bg_encoder_callbacks_t * cb_ext;
  bg_encoder_callbacks_t cb_int;

  char * filename_base;

  const bg_metadata_t * metadata;
  const bg_chapter_list_t * chapter_list;
  
  };

static int cb_create_output_file(void * data, const char * filename)
  {
  int ret;
  bg_encoder_t * e = data;
  
  if(e->cb_ext && e->cb_ext->create_output_file)
    ret = e->cb_ext->create_output_file(e->cb_ext->data, filename);
  else
    ret = 1;

  if(ret)
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Created output file %s\n",
           filename);
  return ret;
  }

static int cb_create_temp_file(void * data, const char * filename)
  {
  int ret;
  bg_encoder_t * e = data;
  
  if(e->cb_ext && e->cb_ext->create_temp_file)
    ret = e->cb_ext->create_temp_file(e->cb_ext->data, filename);
  else
    ret = 1;

  if(ret)
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Created temp file %s\n",
           filename);
  return ret;
  }

static void init_plugin_from_section(bg_encoder_t * e, plugin_config_t * ret,
                                     bg_stream_type_t type)
  {
  const char * name;
  name = bg_encoder_section_get_plugin(e->plugin_reg, e->es,
                                       type, e->stream_mask);
  if(name)
    {
    ret->info = bg_plugin_find_by_name(e->plugin_reg, name);
    bg_encoder_section_get_plugin_config(e->plugin_reg,
                                         e->es, type, e->stream_mask,
                                         &ret->section, NULL);
    }
  }

static void init_stream_from_section(bg_encoder_t * e,
                                     stream_config_t * ret,
                                     bg_stream_type_t type)
  {
  bg_encoder_section_get_stream_config(e->plugin_reg, e->es,
                                       type, e->stream_mask,
                                       &ret->section, NULL);
  }

static void init_from_section(bg_encoder_t * e)
  {
  if(e->stream_mask & BG_STREAM_AUDIO)
    {
    init_plugin_from_section(e, &e->audio_plugin, BG_STREAM_AUDIO);
    init_stream_from_section(e, &e->audio_stream, BG_STREAM_AUDIO);
    }
  if(e->stream_mask & BG_STREAM_SUBTITLE_TEXT)
    {
    init_plugin_from_section(e, &e->subtitle_text_plugin,
                             BG_STREAM_SUBTITLE_TEXT);
    init_stream_from_section(e, &e->subtitle_text_stream,
                             BG_STREAM_SUBTITLE_TEXT);
    }
  if(e->stream_mask & BG_STREAM_SUBTITLE_OVERLAY)
    {
    init_plugin_from_section(e, &e->subtitle_overlay_plugin,
                             BG_STREAM_SUBTITLE_OVERLAY);
    init_stream_from_section(e, &e->subtitle_overlay_stream,
                             BG_STREAM_SUBTITLE_OVERLAY);
    }
  if(e->stream_mask & BG_STREAM_VIDEO)
    {
    init_plugin_from_section(e, &e->video_plugin, BG_STREAM_VIDEO);
    init_stream_from_section(e, &e->video_stream, BG_STREAM_VIDEO);
    }
  }

static void init_from_tt(bg_encoder_t * e)
  {
  const char * plugin_name;

  /* Video plugin (must come first) */
  plugin_name = bg_transcoder_track_get_video_encoder(e->tt);
  
  if(plugin_name)
    {
    e->video_plugin.info = bg_plugin_find_by_name(e->plugin_reg, plugin_name);
    e->video_plugin.section    = e->tt->video_encoder_section;
    }
  /* Audio plugin */

  plugin_name = bg_transcoder_track_get_audio_encoder(e->tt);
  if(plugin_name)
    {
    e->audio_plugin.info = bg_plugin_find_by_name(e->plugin_reg, plugin_name);
    e->audio_plugin.section    = e->tt->audio_encoder_section;
    }
  
  /* Subtitle text plugin */
  plugin_name = bg_transcoder_track_get_subtitle_text_encoder(e->tt);
  if(plugin_name)
    {
    e->subtitle_text_plugin.info = bg_plugin_find_by_name(e->plugin_reg, plugin_name);
    e->subtitle_text_plugin.section    = e->tt->subtitle_text_encoder_section;
    }
  
  /* Subtitle overlay plugin */
  plugin_name = bg_transcoder_track_get_subtitle_overlay_encoder(e->tt);
  if(plugin_name)
    {
    e->subtitle_overlay_plugin.info = bg_plugin_find_by_name(e->plugin_reg, plugin_name);
    e->subtitle_overlay_plugin.section    = e->tt->subtitle_overlay_encoder_section;
    }  
  
  }

bg_encoder_t * bg_encoder_create(bg_plugin_registry_t * plugin_reg,
                                 bg_cfg_section_t * es,
                                 bg_transcoder_track_t * tt,
                                 int stream_mask, int flag_mask)
  {
  bg_encoder_t * ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;
  ret->stream_mask = stream_mask;

  ret->cb_int.create_output_file = cb_create_output_file;
  ret->cb_int.create_temp_file = cb_create_temp_file;
  ret->cb_int.data = ret;
  
  /* Set plugin infos */

  if(es)
    {
    ret->es = es;
    init_from_section(ret);
    }
  else if(tt)
    {
    ret->tt = tt;
    init_from_tt(ret);
    }
  
  return ret;
  }

void
bg_encoder_set_callbacks(bg_encoder_t * e, bg_encoder_callbacks_t * cb)
  {
  e->cb_ext = cb;
  }

void bg_encoder_destroy(bg_encoder_t * enc)
     /* Also closes all internal encoders */
  {

  if(enc->filename_base)
    free(enc->filename_base);
  
  free(enc);
  }

int bg_encoder_open(bg_encoder_t * enc, const char * filename_base,
                    const bg_metadata_t * metadata,
                    const bg_chapter_list_t * chapter_list)
  {
  enc->filename_base = bg_strdup(enc->filename_base, filename_base);
  enc->metadata = metadata;
  enc->chapter_list = chapter_list;
  return 1;
  }

static bg_plugin_handle_t * load_encoder(bg_encoder_t * enc,
                                         const bg_plugin_info_t * info,
                                         bg_cfg_section_t * section,
                                         const char * filename_base)
  {
  bg_plugin_handle_t * ret;
  bg_encoder_plugin_t * plugin;
  
  enc->plugins = realloc(enc->plugins,
                         (enc->num_plugins+1)* sizeof(enc->plugins));

  enc->plugins[enc->num_plugins] =
    bg_plugin_load(enc->plugin_reg, info);
  ret = enc->plugins[enc->num_plugins];

  plugin = (bg_encoder_plugin_t *)ret->plugin;

  if(plugin->set_callbacks)
    plugin->set_callbacks(ret->priv, &enc->cb_int);

  if(plugin->common.set_parameter)
    bg_cfg_section_apply(section,
                         info->parameters,
                         plugin->common.set_parameter,
                         ret->priv);

  if(!plugin->open(ret->priv, filename_base, enc->metadata, enc->chapter_list))
    {
    bg_plugin_unref(ret);
    return NULL;
    }
  
  enc->num_plugins++;
  return ret;
  }

static void check_separate(bg_encoder_t * enc)
  {
  /* Check for separate audio streams */

  if(enc->num_audio_streams)
    {
    if((enc->audio_plugin.info) ||
       ((enc->video_plugin.info->max_audio_streams > 0) &&
        (enc->num_audio_streams > enc->video_plugin.info->max_audio_streams)))
      enc->separate |= BG_STREAM_AUDIO;
    }
  
  /* Check for separate subtitle text streams */
  if(enc->num_subtitle_text_streams)
    {
    if((enc->subtitle_text_plugin.info) ||
       ((enc->video_plugin.info->max_subtitle_text_streams > 0) &&
        (enc->num_subtitle_text_streams > enc->video_plugin.info->max_subtitle_text_streams)))
      enc->separate |= BG_STREAM_SUBTITLE_TEXT;
    }

  /* Check for separate subtitle overlay streams */
  if(enc->num_subtitle_overlay_streams)
    {
    if((enc->subtitle_overlay_plugin.info) ||
       ((enc->video_plugin.info->max_subtitle_overlay_streams > 0) &&
        (enc->num_subtitle_overlay_streams >
         enc->video_plugin.info->max_subtitle_overlay_streams)))
      enc->separate |= BG_STREAM_SUBTITLE_OVERLAY;
    }
  
  /* Check for separate video streams */
  if(enc->num_video_streams)
    {
    if((enc->video_plugin.info->max_video_streams > 0) &&
       (enc->num_video_streams >
        enc->video_plugin.info->max_video_streams))
      enc->separate |= BG_STREAM_VIDEO;
    }

  /* If video is separate, all other streams get separate as well */
  if(enc->separate & BG_STREAM_VIDEO)
    {
    enc->separate |= BG_STREAM_SUBTITLE_OVERLAY |
      BG_STREAM_SUBTITLE_TEXT |
      BG_STREAM_AUDIO;
    }
  
  }

typedef struct
  {
  void (*func)(void * data, int index, const char * name,
               const bg_parameter_value_t*val);
  void * data;
  int index;
  } set_stream_param_struct_t;

static void set_stream_param(void * priv, const char * name,
                             const bg_parameter_value_t * val)
  {
  set_stream_param_struct_t * s;
  s = (set_stream_param_struct_t *)priv;
  s->func(s->data, s->index, name, val);
  }
     
static int start_audio(bg_encoder_t * enc, int stream)
  {
  audio_stream_t * s;
  bg_plugin_handle_t * h;
  set_stream_param_struct_t st;
  
  s = &enc->audio_streams[stream];

  /* Get handle */
  
  s->plugin = (bg_encoder_plugin_t*)h->plugin;
  s->priv = h->priv;

  /* Add stream */
  
  s->out_index = s->plugin->add_audio_stream(s->priv, s->language, &s->format);
  
  /* Apply parameters */ 

  if(s->plugin->set_audio_parameter)
    {
    st.func =  s->plugin->set_audio_parameter;
    st.data =  s->priv;
    st.index = s->out_index;
    
    bg_cfg_section_apply(s->section,
                         s->parameters,
                         set_stream_param,
                         &st);
    }
  
  return 1;
  }

static int start_video(bg_encoder_t * enc, int stream)
  {
  video_stream_t * s;
  bg_plugin_handle_t * h;
  set_stream_param_struct_t st;
  
  s = &enc->video_streams[stream];

  /* Get handle */
  
  s->plugin = (bg_encoder_plugin_t*)h->plugin;
  s->priv = h->priv;

  /* Add stream */
  
  s->out_index = s->plugin->add_video_stream(s->priv, &s->format);

  
  /* Apply parameters */ 
  if(s->plugin->set_video_parameter)
    {
    st.func =  s->plugin->set_video_parameter;
    st.data =  s->priv;
    st.index = s->out_index;
    
    bg_cfg_section_apply(s->section,
                         s->parameters,
                         set_stream_param,
                         &st);
    }

  return 1;
  }

static int start_subtitle_text(bg_encoder_t * enc, int stream)
  {
  subtitle_text_stream_t * s;
  bg_plugin_handle_t * h;
  set_stream_param_struct_t st;

  s = &enc->subtitle_text_streams[stream];

  /* Get handle */
    
  s->plugin = (bg_encoder_plugin_t*)h->plugin;
  s->priv = h->priv;

  /* Add stream */
  
  s->out_index = s->plugin->add_subtitle_text_stream(s->priv, s->language, &s->timescale);
    
  /* Apply parameters */ 
  if(s->plugin->set_subtitle_text_parameter)
    {
    st.func =  s->plugin->set_subtitle_text_parameter;
    st.data =  s->priv;
    st.index = s->out_index;
    
    bg_cfg_section_apply(s->section,
                         s->parameters,
                         set_stream_param,
                         &st);
    }
  
  return 1;
  }

static int start_subtitle_overlay(bg_encoder_t * enc, int stream)
  {
  subtitle_overlay_stream_t * s;
  bg_plugin_handle_t * h;
  set_stream_param_struct_t st;

  s = &enc->subtitle_overlay_streams[stream];

  /* Get handle */
  
  s->plugin = (bg_encoder_plugin_t*)h->plugin;
  s->priv = h->priv;

  /* Add stream */
  s->out_index = s->plugin->add_subtitle_overlay_stream(s->priv, s->language, &s->format);

  
  /* Apply parameters */ 
  if(s->plugin->set_subtitle_overlay_parameter)
    {
    st.func =  s->plugin->set_subtitle_overlay_parameter;
    st.data =  s->priv;
    st.index = s->out_index;
    
    bg_cfg_section_apply(s->section,
                         s->parameters,
                         set_stream_param,
                         &st);
    }
  
  return 1;
  }
     
     
/* Start encoding */
int bg_encoder_start(bg_encoder_t * enc)
  {
  int i;

  check_separate(enc);
  
  enc->total_streams = enc->num_audio_streams + 
    enc->num_video_streams +
    enc->num_subtitle_text_streams +
    enc->num_subtitle_overlay_streams;
  
  /* We make sure, that for the case of combined streams the
     video is always the first one */

  for(i = 0; i < enc->num_video_streams; i++)
    {
    if(!start_video(enc, i))
      return 0;
    }

  for(i = 0; i < enc->num_audio_streams; i++)
    {
    if(!start_audio(enc, i))
      return 0;
    }

  for(i = 0; i < enc->num_subtitle_text_streams; i++)
    {
    if(!start_subtitle_text(enc, i))
      return 0;
    }
  
  for(i = 0; i < enc->num_subtitle_overlay_streams; i++)
    {
    if(!start_subtitle_overlay(enc, i))
      return 0;
    }

  /* Start encoders */
  for(i = 0; i < enc->num_plugins; i++)
    {
    bg_encoder_plugin_t * plugin =
      (bg_encoder_plugin_t *)enc->plugins[i]->plugin;
    if(plugin->start && plugin->start(enc->plugins[i]->priv))
      return 0;
    }
  
  return 1;
  }



/* Add streams */

#define REALLOC_STREAM(streams, num) \
  streams = realloc(streams, sizeof(streams)*(num+1));\
  s = &streams[num]

/* Add streams */
int bg_encoder_add_audio_stream(bg_encoder_t * enc,
                                const char * language,
                                gavl_audio_format_t * format,
                                int source_index)
  {
  int ret;
  audio_stream_t * s;
    
  REALLOC_STREAM(enc->audio_streams,
                 enc->num_audio_streams);

  gavl_audio_format_copy(&s->format, format);
  s->in_index = source_index;

  if(enc->tt)
    s->section = enc->tt->audio_streams[source_index].encoder_section;
  else
    s->section = enc->audio_stream.section;

  if(enc->audio_plugin.info)
    s->parameters = enc->audio_plugin.info->audio_parameters;
  else if(enc->video_plugin.info)
    s->parameters = enc->video_plugin.info->audio_parameters;
  
  s->language = bg_strdup(s->language, language);
  
  ret = enc->num_audio_streams;
  enc->num_audio_streams++;
  return ret;
  }

int bg_encoder_add_video_stream(bg_encoder_t * enc,
                                gavl_video_format_t * format,
                                int source_index)
  {
  int ret;
  video_stream_t * s;

  REALLOC_STREAM(enc->video_streams,
                 enc->num_video_streams);

  gavl_video_format_copy(&s->format, format);
  s->in_index = source_index;

  if(enc->tt)
    s->section = enc->tt->video_streams[source_index].encoder_section;
  else
    s->section = enc->video_stream.section;

  s->parameters = enc->video_plugin.info->video_parameters;
  
  ret = enc->num_video_streams;
  enc->num_video_streams++;
  return ret;
  }

int bg_encoder_add_subtitle_text_stream(bg_encoder_t * enc,
                                        const char * language,
                                        int timescale,
                                        int source_index)
  {
  int ret;
  subtitle_text_stream_t * s;

  REALLOC_STREAM(enc->subtitle_text_streams,
                 enc->num_subtitle_text_streams);

  s->timescale = timescale;
  s->in_index = source_index;
  
  if(enc->tt)
    s->section = enc->tt->subtitle_text_streams[source_index].encoder_section_text;
  else
    s->section = enc->subtitle_text_stream.section;

  if(enc->subtitle_text_plugin.info)
    s->parameters = enc->subtitle_text_plugin.info->subtitle_text_parameters;
  else if(enc->video_plugin.info)
    s->parameters = enc->video_plugin.info->subtitle_text_parameters;

  
  s->language = bg_strdup(s->language, language);

  
  ret = enc->num_subtitle_text_streams;
  enc->num_subtitle_text_streams++;
  return ret;

  }

int bg_encoder_add_subtitle_overlay_stream(bg_encoder_t * enc,
                                           const char * language,
                                           gavl_video_format_t * format,
                                           int source_index, bg_stream_type_t source_format)
  {
  int ret;
  subtitle_overlay_stream_t * s;

  REALLOC_STREAM(enc->subtitle_overlay_streams,
                 enc->num_subtitle_overlay_streams);

  gavl_video_format_copy(&s->format, format);
  s->in_index = source_index;

  if(enc->tt)
    {
    if(source_format == BG_STREAM_SUBTITLE_TEXT)
      s->section = enc->tt->subtitle_text_streams[source_index].encoder_section_overlay;
    else
      s->section = enc->tt->subtitle_overlay_streams[source_index].encoder_section;
    }
  else
    s->section = enc->subtitle_overlay_stream.section;

  if(enc->subtitle_overlay_plugin.info)
    s->parameters = enc->subtitle_overlay_plugin.info->subtitle_overlay_parameters;
  else if(enc->video_plugin.info)
    s->parameters = enc->video_plugin.info->subtitle_overlay_parameters;
  
  s->language = bg_strdup(s->language, language);
  
  ret = enc->num_subtitle_overlay_streams;
  enc->num_subtitle_overlay_streams++;
  return ret;
  }

int
bg_encoder_set_video_pass(bg_encoder_t * enc,
                          int stream, int pass, int total_passes,
                          const char * stats_file)
  {
  video_stream_t * s = &enc->video_streams[stream];
  
  if(!s->plugin->set_video_pass)
    return 0;
  
  return s->plugin->set_video_pass(s->priv,
                                   stream, pass, total_passes,
                                   stats_file);
  }


/* Get formats */
void bg_encoder_get_audio_format(bg_encoder_t * enc,
                                 int stream,
                                 gavl_audio_format_t*ret)
  {
  audio_stream_t * s = &enc->audio_streams[stream];
  s->plugin->get_audio_format(s->priv, s->out_index, ret);
  }

void bg_encoder_get_video_format(bg_encoder_t * enc,
                                 int stream,
                                 gavl_video_format_t*ret)
  {
  video_stream_t * s = &enc->video_streams[stream];
  s->plugin->get_video_format(s->priv, s->out_index, ret);
  }

void bg_encoder_get_subtitle_overlay_format(bg_encoder_t * enc,
                                            int stream,
                                            gavl_video_format_t*ret)
  {
  subtitle_overlay_stream_t * s = &enc->subtitle_overlay_streams[stream];
  s->plugin->get_subtitle_overlay_format(s->priv, s->out_index, ret);
  }


/* Write frame */
int bg_encoder_write_audio_frame(bg_encoder_t * enc,
                                 gavl_audio_frame_t * frame,
                                 int stream)
  {
  audio_stream_t * s = &enc->audio_streams[stream];
  return s->plugin->write_audio_frame(s->priv, frame, s->out_index);
  }
  

int bg_encoder_write_video_frame(bg_encoder_t * enc,
                                 gavl_video_frame_t * frame,
                                 int stream)
  {
  video_stream_t * s = &enc->video_streams[stream];
  return s->plugin->write_video_frame(s->priv, frame, s->out_index);
  }

int bg_encoder_write_subtitle_text(bg_encoder_t * enc,
                                   const char * text,
                                   int64_t start,
                                   int64_t duration, int stream)
  {
  subtitle_text_stream_t * s = &enc->subtitle_text_streams[stream];
  return s->plugin->write_subtitle_text(s->priv, text, start, duration, s->out_index);
  }

int bg_encoder_write_subtitle_overlay(bg_encoder_t * enc,
                                      gavl_overlay_t * ovl, int stream)
  {
  subtitle_overlay_stream_t * s = &enc->subtitle_overlay_streams[stream];
  return s->plugin->write_subtitle_overlay(s->priv, ovl, s->out_index);
  }
