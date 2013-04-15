/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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
#include <string.h>
#include <unistd.h> // isatty

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
  bg_plugin_handle_t * h;
  const gavl_metadata_t * m;
  
  gavl_packet_sink_t * psink;

  bg_cfg_section_t * section;
  bg_cfg_section_t * section_priv; // If copied
  
  const bg_parameter_info_t * parameters;

  const gavl_compression_info_t * ci;
  } stream_common_t;

typedef struct
  {
  stream_common_t com;
  gavl_audio_format_t format;
  gavl_audio_sink_t * sink;
  } audio_stream_t;

typedef struct
  {
  stream_common_t com;
  
  gavl_video_format_t format;
  
  int pass;
  int total_passes;
  char * stats_file;
  
  //  int64_t last_timestamp;
  gavl_video_sink_t * sink;
  } video_stream_t;

typedef struct
  {
  stream_common_t com;
  uint32_t timescale;
  } text_stream_t;

typedef struct
  {
  stream_common_t com;
  gavl_video_format_t format;
  gavl_video_sink_t * sink;
  } overlay_stream_t;

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
  plugin_config_t text_plugin;
  plugin_config_t overlay_plugin;

  stream_config_t audio_stream;
  stream_config_t video_stream;
  stream_config_t text_stream;
  stream_config_t overlay_stream;
  
  int num_audio_streams;
  int num_video_streams;
  int num_text_streams;
  int num_overlay_streams;

  int total_streams;
  
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  text_stream_t * text_streams;
  overlay_stream_t * overlay_streams;
  
  int num_plugins;
  bg_plugin_handle_t ** plugins;

  int separate;
  int is_stdout;
  
  bg_plugin_registry_t * plugin_reg;

  /* Config stuff */
  bg_cfg_section_t * es;
  bg_transcoder_track_t * tt;

  int stream_mask;

  bg_encoder_callbacks_t * cb_ext;
  bg_encoder_callbacks_t cb_int;

  char * filename_base;

  const gavl_metadata_t * metadata;
  const gavl_chapter_list_t * chapter_list;
  
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
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Created output file %s",
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
  if(e->stream_mask & BG_STREAM_TEXT)
    {
    init_plugin_from_section(e, &e->text_plugin, BG_STREAM_TEXT);
    init_stream_from_section(e, &e->text_stream, BG_STREAM_TEXT);
    }
  if(e->stream_mask & BG_STREAM_OVERLAY)
    {
    init_plugin_from_section(e, &e->overlay_plugin, BG_STREAM_OVERLAY);
    init_stream_from_section(e, &e->overlay_stream, BG_STREAM_OVERLAY);
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
  plugin_name = bg_transcoder_track_get_text_encoder(e->tt);
  if(plugin_name)
    {
    e->text_plugin.info = bg_plugin_find_by_name(e->plugin_reg, plugin_name);
    e->text_plugin.section    = e->tt->text_encoder_section;
    }
  
  /* Subtitle overlay plugin */
  plugin_name = bg_transcoder_track_get_overlay_encoder(e->tt);
  if(plugin_name)
    {
    e->overlay_plugin.info = bg_plugin_find_by_name(e->plugin_reg, plugin_name);
    e->overlay_plugin.section    = e->tt->overlay_encoder_section;
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

void bg_encoder_destroy(bg_encoder_t * enc, int do_delete)
  {
  int i;
  /* Close all encoder plugins */

  for(i = 0; i < enc->num_plugins; i++)
    {
    bg_encoder_plugin_t * encoder =
      (bg_encoder_plugin_t *)enc->plugins[i]->plugin;
    encoder->close(enc->plugins[i]->priv, do_delete);
    bg_plugin_unref(enc->plugins[i]);
    }

  for(i = 0; i < enc->num_audio_streams; i++)
    {
    if(enc->audio_streams[i].com.section_priv)
      bg_cfg_section_destroy(enc->audio_streams[i].com.section_priv);
    }

  for(i = 0; i < enc->num_video_streams; i++)
    {
    if(enc->video_streams[i].com.section_priv)
      bg_cfg_section_destroy(enc->video_streams[i].com.section_priv);
    }

  for(i = 0; i < enc->num_text_streams; i++)
    {
    if(enc->text_streams[i].com.section_priv)
      bg_cfg_section_destroy(enc->text_streams[i].com.section_priv);
    }
  for(i = 0; i < enc->num_overlay_streams; i++)
    {
    if(enc->overlay_streams[i].com.section_priv)
      bg_cfg_section_destroy(enc->overlay_streams[i].com.section_priv);
    }
  
  if(enc->plugins)
    free(enc->plugins);
  
  if(enc->filename_base)
    free(enc->filename_base);

  if(enc->audio_streams)
    free(enc->audio_streams);
  if(enc->video_streams)
    free(enc->video_streams);
  if(enc->text_streams)
    free(enc->text_streams);
  if(enc->overlay_streams)
    free(enc->overlay_streams);
  
  
  free(enc);
  }

int bg_encoder_open(bg_encoder_t * enc, const char * filename_base,
                    const gavl_metadata_t * metadata,
                    const gavl_chapter_list_t * chapter_list)
  {
  enc->filename_base = gavl_strrep(enc->filename_base, filename_base);

  if(enc->filename_base && !strcmp(enc->filename_base, "-"))
    {
    if(isatty(fileno(stdout)))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Won't write media file to a TTY");
      return 0;
      }
    enc->is_stdout = 1;
    }
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
  
  if(enc->is_stdout)
    {
    if(enc->num_plugins)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Writing to stdout is only supported for single file output");
      return NULL;
      }
    else if(!(info->flags & BG_PLUGIN_PIPE))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Plugin %s cannot write to stdout", info->name);
      return NULL;
      }
    }

  /* For file output we need to be careful about the filename */
  
  if(info->flags & BG_PLUGIN_FILE)
    {
    if(!filename_base)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "No filename base specified");
      return NULL;
      }
    else if(bg_string_is_url(filename_base))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Invalid filename base: %s", filename_base);
      return NULL;
      }
    }

  
  enc->plugins = realloc(enc->plugins,
                         (enc->num_plugins+1)* sizeof(enc->plugins));

  enc->plugins[enc->num_plugins] =
    bg_plugin_load(enc->plugin_reg, info);
  ret = enc->plugins[enc->num_plugins];

  plugin = (bg_encoder_plugin_t *)ret->plugin;

  if(plugin->set_callbacks)
    plugin->set_callbacks(ret->priv, &enc->cb_int);

  if(plugin->common.set_parameter && section)
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
  if(enc->num_text_streams)
    {
    if((enc->text_plugin.info) ||
       ((enc->video_plugin.info->max_text_streams > 0) &&
        (enc->num_text_streams > enc->video_plugin.info->max_text_streams)))
      enc->separate |= BG_STREAM_TEXT;
    }

  /* Check for separate subtitle overlay streams */
  if(enc->num_overlay_streams)
    {
    if((enc->overlay_plugin.info) ||
       ((enc->video_plugin.info->max_overlay_streams > 0) &&
        (enc->num_overlay_streams >
         enc->video_plugin.info->max_overlay_streams)))
      enc->separate |= BG_STREAM_OVERLAY;
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
    enc->separate |= BG_STREAM_OVERLAY |
      BG_STREAM_TEXT |
      BG_STREAM_AUDIO;
    }
  else
    {
    /* Under some conditions, we want the video to be marked as separate as well */
    if(((enc->separate & BG_STREAM_OVERLAY) || !enc->num_overlay_streams) &&
       ((enc->separate & BG_STREAM_TEXT) || !enc->num_text_streams) &&
       ((enc->separate & BG_STREAM_AUDIO) || !enc->num_audio_streams))
      enc->separate |= BG_STREAM_VIDEO;
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



static bg_plugin_handle_t * get_stream_handle(bg_encoder_t * enc,
                                              bg_stream_type_t type,
                                              int stream, int in_index)
  {
  bg_plugin_handle_t * ret;
  const bg_plugin_info_t * info = NULL;
  bg_cfg_section_t * section = NULL;
  char * filename_base;
  const char * type_string = NULL;
  
  if(enc->separate & type)
    {
    switch(type)
      {
      case BG_STREAM_AUDIO:
        type_string = "audio";
        info = enc->audio_plugin.info;
        section = enc->audio_plugin.section;
        break;
      case BG_STREAM_VIDEO:
        type_string = "video";
        info = enc->video_plugin.info;
        section = enc->video_plugin.section;
        break;
      case BG_STREAM_TEXT:
        type_string = "subtext";
        info = enc->text_plugin.info;
        section = enc->text_plugin.section;
        break;
      case BG_STREAM_OVERLAY:
        type_string = "subovl";
        info = enc->overlay_plugin.info;
        section = enc->overlay_plugin.section;
        break;
      }

    if(enc->total_streams > 1)
      {
      if(strcmp(enc->filename_base, "-"))
        filename_base = bg_sprintf("%s_%s_%02d", enc->filename_base, type_string, in_index+1);
      else
        filename_base = gavl_strdup(enc->filename_base);

      ret = load_encoder(enc, info, section, filename_base);
      free(filename_base);
      }
    else
      ret = load_encoder(enc, info, section, enc->filename_base);
    }
  else
    {
    if(enc->num_plugins)
      ret = enc->plugins[0];
    else
      ret = load_encoder(enc, enc->video_plugin.info, enc->video_plugin.section,
                         enc->filename_base);
    }
  
  return ret;
  }

static int start_audio(bg_encoder_t * enc, int stream)
  {
  audio_stream_t * s;
  bg_plugin_handle_t * h;
  set_stream_param_struct_t st;
  
  s = &enc->audio_streams[stream];

  /* Get handle */

  h = get_stream_handle(enc, BG_STREAM_AUDIO, stream, s->com.in_index);

  if(!h)
    return 0;
  
  s->com.plugin = (bg_encoder_plugin_t*)h->plugin;
  s->com.priv = h->priv;
  s->com.h = h;
  
  /* Add stream */

  if(s->com.ci)
    {
    s->com.out_index =
      s->com.plugin->add_audio_stream_compressed(s->com.priv, s->com.m,
                                                 &s->format, s->com.ci);
    if(s->com.out_index < 0)
      return 0;
    }
  else
    {
    s->com.out_index =
      s->com.plugin->add_audio_stream(s->com.priv, s->com.m, &s->format);
    if(s->com.out_index < 0)
      return 0;
  
    /* Apply parameters */ 

    if(s->com.plugin->set_audio_parameter)
      {
      st.func =  s->com.plugin->set_audio_parameter;
      st.data =  s->com.priv;
      st.index = s->com.out_index;
    
      bg_cfg_section_apply(s->com.section,
                           s->com.parameters,
                           set_stream_param,
                           &st);
      }
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
  h = get_stream_handle(enc, BG_STREAM_VIDEO, stream, s->com.in_index);

  if(!h)
    return 0;
  
  s->com.plugin = (bg_encoder_plugin_t*)h->plugin;
  s->com.priv = h->priv;
  s->com.h = h;
  
  /* Add stream */
  
  if(s->com.ci)
    {
    s->com.out_index =
      s->com.plugin->add_video_stream_compressed(s->com.priv, s->com.m,
                                                 &s->format, s->com.ci);
    if(s->com.out_index < 0)
      return 0;
    }
  else
    {
    s->com.out_index = s->com.plugin->add_video_stream(s->com.priv, s->com.m,
                                                       &s->format);
    if(s->com.out_index < 0)
      return 0;
  
    /* Apply parameters */ 
    if(s->com.plugin->set_video_parameter)
      {
      st.func =  s->com.plugin->set_video_parameter;
      st.data =  s->com.priv;
      st.index = s->com.out_index;
      
      bg_cfg_section_apply(s->com.section,
                           s->com.parameters,
                           set_stream_param,
                           &st);
      }
    
    /* Set pass */
    
    if(s->total_passes)
      {
      if(!s->com.plugin->set_video_pass ||
         !s->com.plugin->set_video_pass(s->com.priv, s->com.out_index, s->pass, s->total_passes,
                                    s->stats_file))
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "Multipass encoding not supported by encoder plugin");
        return 0;
        }
      }
    }
  
  return 1;
  }

static int start_text(bg_encoder_t * enc, int stream)
  {
  text_stream_t * s;
  bg_plugin_handle_t * h;
  set_stream_param_struct_t st;

  s = &enc->text_streams[stream];

  /* Get handle */
  h = get_stream_handle(enc, BG_STREAM_TEXT, stream, s->com.in_index);

  if(!h)
    return 0;
  
  s->com.plugin = (bg_encoder_plugin_t*)h->plugin;
  s->com.priv = h->priv;
  s->com.h = h;
  /* Add stream */
  
  s->com.out_index = s->com.plugin->add_text_stream(s->com.priv, s->com.m, &s->timescale);
  if(s->com.out_index < 0)
    return 0;

  /* Apply parameters */ 
  if(s->com.plugin->set_text_parameter)
    {
    st.func =  s->com.plugin->set_text_parameter;
    st.data =  s->com.priv;
    st.index = s->com.out_index;
    
    bg_cfg_section_apply(s->com.section,
                         s->com.parameters,
                         set_stream_param,
                         &st);
    }
  
  return 1;
  }

static int start_overlay(bg_encoder_t * enc, int stream)
  {
  overlay_stream_t * s;
  bg_plugin_handle_t * h;
  set_stream_param_struct_t st;

  s = &enc->overlay_streams[stream];

  /* Get handle */
  h = get_stream_handle(enc, BG_STREAM_OVERLAY, stream, s->com.in_index);

  if(!h)
    return 0;
  
  s->com.plugin = (bg_encoder_plugin_t*)h->plugin;
  s->com.priv = h->priv;
  s->com.h = h;
  
  /* Add stream */

  if(s->com.ci)
    {
    s->com.out_index =
      s->com.plugin->add_overlay_stream_compressed(s->com.priv, s->com.m,
                                                   &s->format, s->com.ci);
    if(s->com.out_index < 0)
      return 0;
    return 1;
    }
  
  s->com.out_index =
    s->com.plugin->add_overlay_stream(s->com.priv,
                                      s->com.m, &s->format);
  if(s->com.out_index < 0)
    return 0;
  
  /* Apply parameters */ 
  if(s->com.plugin->set_overlay_parameter)
    {
    st.func =  s->com.plugin->set_overlay_parameter;
    st.data =  s->com.priv;
    st.index = s->com.out_index;
    
    bg_cfg_section_apply(s->com.section,
                         s->com.parameters,
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
    enc->num_text_streams +
    enc->num_overlay_streams;

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

  for(i = 0; i < enc->num_text_streams; i++)
    {
    if(!start_text(enc, i))
      return 0;
    }
  
  for(i = 0; i < enc->num_overlay_streams; i++)
    {
    if(!start_overlay(enc, i))
      return 0;
    }

  /* Start encoders */
  for(i = 0; i < enc->num_plugins; i++)
    {
    bg_encoder_plugin_t * plugin =
      (bg_encoder_plugin_t *)enc->plugins[i]->plugin;
    if(plugin->start && !plugin->start(enc->plugins[i]->priv))
      return 0;
    }

  /* Get sinks */

  for(i = 0; i < enc->num_audio_streams; i++)
    {
    audio_stream_t * as = &enc->audio_streams[i];
    if(!as->com.ci)
      {
      as->sink = as->com.plugin->get_audio_sink(as->com.priv, as->com.out_index);
      gavl_audio_sink_set_lock_funcs(as->sink, bg_plugin_lock, bg_plugin_unlock, as->com.h);
      }
    else
      {
      as->com.psink = as->com.plugin->get_audio_packet_sink(as->com.priv, as->com.out_index);
      gavl_packet_sink_set_lock_funcs(as->com.psink, bg_plugin_lock, bg_plugin_unlock, as->com.h);
      }
    }

  
  for(i = 0; i < enc->num_video_streams; i++)
    {
    video_stream_t * vs = &enc->video_streams[i];
    if(!vs->com.ci)
      {
      vs->sink =
        vs->com.plugin->get_video_sink(vs->com.priv, vs->com.out_index);
      gavl_video_sink_set_lock_funcs(vs->sink, bg_plugin_lock, bg_plugin_unlock, vs->com.h);
      }
    else
      {
      vs->com.psink = vs->com.plugin->get_video_packet_sink(vs->com.priv,
                                                            vs->com.out_index);
      gavl_packet_sink_set_lock_funcs(vs->com.psink,
                                      bg_plugin_lock,
                                      bg_plugin_unlock,
                                      vs->com.h);
      }
    }

  for(i = 0; i < enc->num_text_streams; i++)
    {
    text_stream_t * ts = &enc->text_streams[i];
    ts->com.psink = ts->com.plugin->get_text_sink(ts->com.priv,
                                                  ts->com.out_index);
    gavl_packet_sink_set_lock_funcs(ts->com.psink,
                                    bg_plugin_lock,
                                    bg_plugin_unlock,
                                    ts->com.h);
    }

  for(i = 0; i < enc->num_overlay_streams; i++)
    {
    overlay_stream_t * os = &enc->overlay_streams[i];

    if(!os->com.ci)
      {
      os->sink = os->com.plugin->get_overlay_sink(os->com.priv,
                                                  os->com.out_index);
      gavl_video_sink_set_lock_funcs(os->sink,
                                     bg_plugin_lock,
                                     bg_plugin_unlock,
                                     os->com.h);
      }
    else
      {
      os->com.psink =
        os->com.plugin->get_overlay_packet_sink(os->com.priv,
                                                os->com.out_index);
      gavl_packet_sink_set_lock_funcs(os->com.psink,
                                     bg_plugin_lock,
                                     bg_plugin_unlock,
                                     os->com.h);
      }
    }
  
  
  return 1;
  }

int bg_encoder_write_audio_frame(bg_encoder_t * enc, gavl_audio_frame_t * f,
                                 int stream)
  {
  audio_stream_t * as = &enc->audio_streams[stream];
  return (gavl_audio_sink_put_frame(as->sink, f) == GAVL_SINK_OK);
  }

int bg_encoder_write_video_frame(bg_encoder_t * enc, gavl_video_frame_t * f,
                                 int stream)
  {
  video_stream_t * vs = &enc->video_streams[stream];
  return (gavl_video_sink_put_frame(vs->sink, f) == GAVL_SINK_OK);
  }

int bg_encoder_write_audio_packet(bg_encoder_t * enc,
                                  gavl_packet_t * p, int stream)
  {
  audio_stream_t * s = &enc->audio_streams[stream];
  return (gavl_packet_sink_put_packet(s->com.psink, p) == GAVL_SINK_OK);
  }

int bg_encoder_write_video_packet(bg_encoder_t * enc,
                                  gavl_packet_t * p, int stream)
  {
  video_stream_t * s = &enc->video_streams[stream];
  return (gavl_packet_sink_put_packet(s->com.psink, p) == GAVL_SINK_OK);
  }

int bg_encoder_write_text(bg_encoder_t * enc,
                                   const char * text,
                                   int64_t start,
                                   int64_t duration, int stream)
  {
  gavl_packet_t p;
  text_stream_t * s = &enc->text_streams[stream];

  gavl_packet_init(&p);
  p.data = (uint8_t*)text;
  p.data_len = strlen(text);
  p.pts = start;
  p.duration = duration;
  return (gavl_packet_sink_put_packet(s->com.psink, &p) == GAVL_SINK_OK);
  }

int bg_encoder_write_overlay(bg_encoder_t * enc,
                                      gavl_overlay_t * ovl, int stream)
  {
  overlay_stream_t * os = &enc->overlay_streams[stream];
  return (gavl_video_sink_put_frame(os->sink, ovl) == GAVL_SINK_OK);
  }

/* Add streams */

#define REALLOC_STREAM(streams, num) \
  streams = realloc(streams, sizeof(*streams)*(num+1));\
  s = &streams[num]; \
  memset(s, 0, sizeof(*s));

/* Obtain sections */

const bg_cfg_section_t * bg_encoder_get_stream_section(bg_encoder_t * enc,
                                                       bg_stream_type_t type)
  {
  switch(type)
    {
    case BG_STREAM_AUDIO:
      return enc->audio_stream.section;
      break;
    case BG_STREAM_VIDEO:
      return enc->video_stream.section;
      break;
    case BG_STREAM_TEXT:
      return enc->text_stream.section;
      break;
    case BG_STREAM_OVERLAY:
      return enc->overlay_stream.section;
      break;
    }
  return NULL;
  }

const bg_parameter_info_t * bg_encoder_get_stream_parameters(bg_encoder_t * enc,
                                                             bg_stream_type_t type)
  {
  switch(type)
    {
    case BG_STREAM_AUDIO:
      if(enc->audio_plugin.info)
        return enc->audio_plugin.info->audio_parameters;
      else if(enc->video_plugin.info)
        return enc->video_plugin.info->audio_parameters;
      break;
    case BG_STREAM_VIDEO:
      return enc->video_plugin.info->video_parameters;
      break;
    case BG_STREAM_TEXT:
      if(enc->text_plugin.info)
        return enc->text_plugin.info->text_parameters;
      else if(enc->video_plugin.info)
        return enc->video_plugin.info->text_parameters;
      break;
    case BG_STREAM_OVERLAY:
      if(enc->overlay_plugin.info)
        return enc->overlay_plugin.info->overlay_parameters;
      else if(enc->video_plugin.info)
        return enc->video_plugin.info->overlay_parameters;
      break;
    }
  return NULL;
  }
  

/* Add streams */
int bg_encoder_add_audio_stream(bg_encoder_t * enc,
                                const gavl_metadata_t * m,
                                const gavl_audio_format_t * format,
                                int source_index, const bg_cfg_section_t * sec)
  {
  int ret;
  audio_stream_t * s;
    
  REALLOC_STREAM(enc->audio_streams,
                 enc->num_audio_streams);

  gavl_audio_format_copy(&s->format, format);
  s->com.in_index = source_index;

  if(sec)
    {
    s->com.section_priv = bg_cfg_section_copy(sec);
    s->com.section = s->com.section_priv;
    }
  else if(enc->tt)
    s->com.section = enc->tt->audio_streams[source_index].encoder_section;
  else
    s->com.section = enc->audio_stream.section;

  s->com.parameters = bg_encoder_get_stream_parameters(enc, BG_STREAM_AUDIO);
  
  s->com.m = m;
  
  ret = enc->num_audio_streams;
  enc->num_audio_streams++;
  return ret;
  }

int bg_encoder_add_video_stream(bg_encoder_t * enc,
                                const gavl_metadata_t * m,
                                const gavl_video_format_t * format,
                                int source_index, const bg_cfg_section_t * sec)
  {
  int ret;
  video_stream_t * s;

  REALLOC_STREAM(enc->video_streams,
                 enc->num_video_streams);

  gavl_video_format_copy(&s->format, format);
  s->com.in_index = source_index;

  if(sec)
    {
    s->com.section_priv = bg_cfg_section_copy(sec);
    s->com.section = s->com.section_priv;
    }
  else if(enc->tt)
    s->com.section = enc->tt->video_streams[source_index].encoder_section;
  else
    s->com.section = enc->video_stream.section;

  s->com.parameters = bg_encoder_get_stream_parameters(enc, BG_STREAM_VIDEO);
  s->com.m = m;
  
  ret = enc->num_video_streams;
  enc->num_video_streams++;
  return ret;
  }

int bg_encoder_add_audio_stream_compressed(bg_encoder_t * enc,
                                           const gavl_metadata_t * m,
                                           const gavl_audio_format_t * format,
                                           const gavl_compression_info_t * info,
                                           int source_index)
  {
  int ret;
  audio_stream_t * s;
    
  REALLOC_STREAM(enc->audio_streams,
                 enc->num_audio_streams);

  gavl_audio_format_copy(&s->format, format);
  s->com.in_index = source_index;
  
  s->com.ci = info;
  s->com.m = m;
  
  ret = enc->num_audio_streams;
  enc->num_audio_streams++;
  return ret;
  
  }

int bg_encoder_add_video_stream_compressed(bg_encoder_t * enc,
                                           const gavl_metadata_t * m,
                                           const gavl_video_format_t * format,
                                           const gavl_compression_info_t * info,
                                           int source_index)
  {
  int ret;
  video_stream_t * s;

  REALLOC_STREAM(enc->video_streams,
                 enc->num_video_streams);

  gavl_video_format_copy(&s->format, format);
  s->com.in_index = source_index;
  
  s->com.ci = info;
  s->com.m = m;
  ret = enc->num_video_streams;
  enc->num_video_streams++;
  return ret;
  }

int bg_encoder_add_overlay_stream_compressed(bg_encoder_t * enc,
                                             const gavl_metadata_t * m,
                                             const gavl_video_format_t * format,
                                             const gavl_compression_info_t * info,
                                             int source_index)
  {
  int ret;
  overlay_stream_t * s;

  REALLOC_STREAM(enc->overlay_streams,
                 enc->num_overlay_streams);

  gavl_video_format_copy(&s->format, format);
  s->com.in_index = source_index;
  
  s->com.ci = info;
  s->com.m = m;
  ret = enc->num_overlay_streams;
  enc->num_overlay_streams++;
  return ret;
  }


int bg_encoder_add_text_stream(bg_encoder_t * enc,
                               const gavl_metadata_t * m,
                               int timescale,
                               int source_index, const bg_cfg_section_t * sec)
  {
  int ret;
  text_stream_t * s;

  REALLOC_STREAM(enc->text_streams,
                 enc->num_text_streams);

  s->timescale = timescale;
  s->com.in_index = source_index;

  s->com.m = m;

  if(sec)
    {
    s->com.section_priv = bg_cfg_section_copy(sec);
    s->com.section = s->com.section_priv;
    }
  else if(enc->tt)
    s->com.section = enc->tt->text_streams[source_index].encoder_section_text;
  else
    s->com.section = enc->text_stream.section;

  s->com.parameters = bg_encoder_get_stream_parameters(enc, BG_STREAM_TEXT);

  
  ret = enc->num_text_streams;
  enc->num_text_streams++;
  return ret;

  }

int bg_encoder_add_overlay_stream(bg_encoder_t * enc,
                                  const gavl_metadata_t * m,
                                  const gavl_video_format_t * format,
                                  int source_index,
                                  bg_stream_type_t source_format,
                                  const bg_cfg_section_t * sec)
  {
  int ret;
  overlay_stream_t * s;

  REALLOC_STREAM(enc->overlay_streams,
                 enc->num_overlay_streams);

  gavl_video_format_copy(&s->format, format);
  s->com.in_index = source_index;
  s->com.m = m;

  if(sec)
    {
    s->com.section_priv = bg_cfg_section_copy(sec);
    s->com.section = s->com.section_priv;
    }
  else if(enc->tt)
    {
    if(source_format == BG_STREAM_TEXT)
      s->com.section = enc->tt->text_streams[source_index].encoder_section_overlay;
    else
      s->com.section = enc->tt->overlay_streams[source_index].encoder_section;
    }
  else
    s->com.section = enc->overlay_stream.section;

  s->com.parameters = bg_encoder_get_stream_parameters(enc, BG_STREAM_OVERLAY);
  
  ret = enc->num_overlay_streams;
  enc->num_overlay_streams++;
  return ret;
  }

void
bg_encoder_set_video_pass(bg_encoder_t * enc,
                          int stream, int pass, int total_passes,
                          const char * stats_file)
  {
  video_stream_t * s = &enc->video_streams[stream];

  s->pass = pass;
  s->total_passes = total_passes;
  s->stats_file = gavl_strrep(s->stats_file, stats_file);
  
  }


/* Get formats */
void bg_encoder_get_audio_format(bg_encoder_t * enc,
                                 int stream,
                                 gavl_audio_format_t*ret)
  {
  audio_stream_t * s = &enc->audio_streams[stream];
  gavl_audio_format_copy(ret, gavl_audio_sink_get_format(s->sink));
  }

void bg_encoder_get_video_format(bg_encoder_t * enc,
                                 int stream,
                                 gavl_video_format_t*ret)
  {
  video_stream_t * s = &enc->video_streams[stream];
  gavl_video_format_copy(ret, gavl_video_sink_get_format(s->sink));
  }

void bg_encoder_get_overlay_format(bg_encoder_t * enc,
                                            int stream,
                                            gavl_video_format_t*ret)
  {
  overlay_stream_t * s = &enc->overlay_streams[stream];
  gavl_video_format_copy(ret, gavl_video_sink_get_format(s->sink));
  }

void bg_encoder_get_text_timescale(bg_encoder_t * enc,
                                            int stream,
                                            uint32_t * ret)
  {
  text_stream_t * s = &enc->text_streams[stream];
  *ret = s->timescale;
  }


static bg_plugin_handle_t *
open_dummy_encoder(bg_encoder_t * enc,
                   const bg_plugin_info_t * plugin_info,
                   bg_cfg_section_t * plugin_section)
  {
  bg_encoder_plugin_t * plugin;
  bg_plugin_handle_t * ret;
  ret = bg_plugin_load(enc->plugin_reg, plugin_info);
  
  plugin = (bg_encoder_plugin_t *)ret->plugin;

  
  if(plugin->common.set_parameter)
    bg_cfg_section_apply(plugin_section,
                         plugin_info->parameters,
                         plugin->common.set_parameter,
                         ret->priv);
  return ret;
  }

int bg_encoder_writes_compressed_audio(bg_encoder_t * enc,
                                       const gavl_audio_format_t * format,
                                       const gavl_compression_info_t * info)
  {
  int ret;
  bg_encoder_plugin_t * plugin;
  const bg_plugin_info_t * plugin_info;
  bg_plugin_handle_t * h;
  bg_cfg_section_t * plugin_section;
  
  if(enc->audio_plugin.info)
    {
    plugin_info = enc->audio_plugin.info;
    plugin_section = enc->audio_plugin.section;
    }
  else if(enc->video_plugin.info)
    {
    plugin_info = enc->video_plugin.info;
    plugin_section = enc->video_plugin.section;
    }
  else
    return 0;
  
  h = open_dummy_encoder(enc, plugin_info, plugin_section);
  plugin = (bg_encoder_plugin_t *)h->plugin;

  if(plugin->writes_compressed_audio)
    ret = plugin->writes_compressed_audio(h->priv,
                                          format, info);
  else
    ret = 0;
  bg_plugin_unref(h);
  return ret;
  }

int bg_encoder_writes_compressed_overlay(bg_encoder_t * enc,
                                         const gavl_video_format_t * format,
                                         const gavl_compression_info_t * info)
  {
  int ret;
  bg_encoder_plugin_t * plugin;
  const bg_plugin_info_t * plugin_info;
  bg_plugin_handle_t * h;
  bg_cfg_section_t * plugin_section;

  if(enc->overlay_plugin.info)
    {
    plugin_info = enc->audio_plugin.info;
    plugin_section = enc->audio_plugin.section;
    }
  else if(enc->video_plugin.info)
    {
    plugin_info = enc->video_plugin.info;
    plugin_section = enc->video_plugin.section;
    }
  else
    return 0;

  h = open_dummy_encoder(enc, plugin_info, plugin_section);
  plugin = (bg_encoder_plugin_t *)h->plugin;

  if(plugin->writes_compressed_overlay)
    ret = plugin->writes_compressed_overlay(h->priv,
                                           format, info);
  else
    ret = 0;
  bg_plugin_unref(h);
  return ret;
  }


int bg_encoder_writes_compressed_video(bg_encoder_t * enc,
                                       const gavl_video_format_t * format,
                                       const gavl_compression_info_t * info)
  {
  int ret;
  bg_encoder_plugin_t * plugin;
  const bg_plugin_info_t * plugin_info;
  bg_plugin_handle_t * h;
  bg_cfg_section_t * plugin_section;

  if(enc->video_plugin.info)
    {
    plugin_info = enc->video_plugin.info;
    plugin_section = enc->video_plugin.section;
    }
  else
    return 0;
  
  h = open_dummy_encoder(enc, plugin_info, plugin_section);
  plugin = (bg_encoder_plugin_t *)h->plugin;
  
  if(plugin->writes_compressed_video)
    ret = plugin->writes_compressed_video(h->priv,
                                          format, info);
  else
    ret = 0;
  bg_plugin_unref(h);
  return ret;
  }



gavl_audio_sink_t *
bg_encoder_get_audio_sink(bg_encoder_t * enc, int stream)
  {
  return enc->audio_streams[stream].sink;
  }

gavl_video_sink_t *
bg_encoder_get_video_sink(bg_encoder_t * enc, int stream)
  {
  return enc->video_streams[stream].sink;
  }

gavl_packet_sink_t *
bg_encoder_get_audio_packet_sink(bg_encoder_t * enc, int stream)
  {
  return enc->audio_streams[stream].com.psink;
  }

gavl_packet_sink_t *
bg_encoder_get_video_packet_sink(bg_encoder_t * enc, int stream)
  {
  return enc->video_streams[stream].com.psink;
  }

gavl_packet_sink_t * bg_encoder_get_text_sink(bg_encoder_t * enc, int stream)
  {
  return enc->text_streams[stream].com.psink;
  }

gavl_video_sink_t * bg_encoder_get_overlay_sink(bg_encoder_t * enc, int stream)
  {
  return enc->overlay_streams[stream].sink;
  }

gavl_packet_sink_t *
bg_encoder_get_overlay_packet_sink(bg_encoder_t * enc, int stream)
  {
  return enc->overlay_streams[stream].com.psink;
  }

void bg_encoder_update_metadata(bg_encoder_t * enc,
                                const gavl_metadata_t * m)
  {
  int i;
  for(i = 0; i < enc->num_plugins; i++)
    {
    bg_encoder_plugin_t * encoder =
      (bg_encoder_plugin_t *)enc->plugins[i]->plugin;
    if(encoder->update_metadata)
      {
      bg_plugin_lock(enc->plugins[i]);
      encoder->update_metadata(enc->plugins[i]->priv, m);
      bg_plugin_unlock(enc->plugins[i]);
      }
    }
  }
