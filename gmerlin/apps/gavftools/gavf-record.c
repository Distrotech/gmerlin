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

#include <string.h>

#include "gavftools.h"

#include <gavl/metatags.h>

#include <language_table.h>

#define LOG_DOMAIN "gavf-record"

/* Global stuff */


static bg_plug_t * out_plug;

static bg_cfg_section_t * audio_section;
static bg_cfg_section_t * video_section;

/* Recorder module */

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name      = "plugin",
      .long_name = TRS("Plugin"),
      .type      = BG_PARAMETER_MULTI_MENU,
      .flags     = BG_PARAMETER_PLUGIN,
    },
    {
      .name      = "language",
      .long_name = TRS("Language"),
      .type      = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "eng" },
      .multi_names = bg_language_codes,
      .multi_labels = bg_language_labels,
    },
    { /* End */ },
  };

static const bg_parameter_info_t video_parameters[] =
  {
    {
      .name      = "plugin",
      .long_name = TRS("Plugin"),
      .type      = BG_PARAMETER_MULTI_MENU,
      .flags     = BG_PARAMETER_PLUGIN,
    },
    { /* End */ },
  };

typedef struct
  {
  bg_plugin_handle_t * h;
  bg_recorder_plugin_t * plugin;
  gavl_audio_source_t * asrc;
  gavl_video_source_t * vsrc;
  bg_parameter_info_t * parameters;
  gavl_metadata_t m;

  gavl_audio_connector_t * aconn;
  gavl_video_connector_t * vconn;
  
  int index; /* Index in the program_header structure */
  
  gavl_time_t time;
  } recorder_stream_t;

recorder_stream_t as;
recorder_stream_t vs;

static void recorder_stream_init(recorder_stream_t * s, int type)
  {
  memset(s, 0, sizeof(*s));

  if(type == GAVF_STREAM_AUDIO)
    {
    s->parameters = bg_parameter_info_copy_array(audio_parameters);

    if(!audio_section)
      audio_section =
        bg_cfg_section_create_from_parameters("audio", s->parameters);
    
    bg_plugin_registry_set_parameter_info(plugin_reg,
                                          BG_PLUGIN_RECORDER_AUDIO,
                                          BG_PLUGIN_RECORDER,
                                          &s->parameters[0]);
    }
  else if(type == GAVF_STREAM_VIDEO)
    {
    s->parameters = bg_parameter_info_copy_array(video_parameters);
    if(!video_section)
      video_section =
        bg_cfg_section_create_from_parameters("video", s->parameters);
    
    bg_plugin_registry_set_parameter_info(plugin_reg,
                                          BG_PLUGIN_RECORDER_VIDEO,
                                          BG_PLUGIN_RECORDER,
                                          &s->parameters[0]);
    }
  }

static void recorder_stream_set_parameter(void * sp, const char * name,
                                          const bg_parameter_value_t * val)
  {
  recorder_stream_t * s= sp;
  
  if(!name)
    return;
  else if(!strcmp(name, "language"))
    gavl_metadata_set(&s->m, GAVL_META_LANGUAGE, val->val_str);
  else if(!strcmp(name, "plugin"))
    {
    const bg_plugin_info_t * info;
    
    if(s->h &&
       !strcmp(s->h->info->name, val->val_str))
      return;
    
    if(s->h)
      bg_plugin_unref(s->h);
    
    info = bg_plugin_find_by_name(plugin_reg, val->val_str);
    s->h = bg_plugin_load(plugin_reg, info);
    s->plugin = (bg_recorder_plugin_t*)(s->h->plugin);
    // if(s->input_plugin->set_callbacks)
    // s->input_plugin->set_callbacks(s->h->priv, &rec->recorder_cb);
    }
  else if(s->h && s->plugin->common.set_parameter)
    s->plugin->common.set_parameter(s->h->priv, name, val);
  
  }

static int recorder_stream_open(recorder_stream_t * s, int type,
                                bg_plug_t * out_plug)
  {
  gavf_t * g = bg_plug_get_gavf(out_plug);
  gavl_compression_info_t ci;
  
  if(!s->h)
    return 0;

  memset(&ci, 0, sizeof(ci));
  ci.id = GAVL_CODEC_ID_NONE;
  
  if(type == GAVF_STREAM_AUDIO)
    {
    gavl_audio_format_t afmt;
    memset(&afmt, 0, sizeof(afmt));
    if(!s->plugin->open(s->h->priv, &afmt, NULL))
      return 0;
    s->asrc = s->plugin->get_audio_source(s->h->priv);
    s->index = gavf_add_audio_stream(g, &ci, &afmt, &s->m);
    s->aconn = gavl_audio_connector_create(s->asrc);
    }
  else if(type == GAVF_STREAM_VIDEO)
    {
    gavl_video_format_t vfmt;
    memset(&vfmt, 0, sizeof(vfmt));
    if(!s->plugin->open(s->h->priv, NULL, &vfmt))
      return 0;
    s->vsrc = s->plugin->get_video_source(s->h->priv);
    s->index = gavf_add_video_stream(g, &ci, &vfmt, &s->m);
    s->vconn = gavl_video_connector_create(s->vsrc);
    }
  return 1;
  }

static int recorder_stream_connect(recorder_stream_t * s,
                                   bg_plug_t * out_plug)
  {
  const gavf_stream_header_t * h;

  if(!s->h)
    return 1;
  
  h = bg_plug_header_from_index(out_plug, s->index);

  
  if(h->type == GAVF_STREAM_AUDIO)
    {
    gavl_audio_sink_t * asink = NULL;

    if(!bg_plug_get_stream_sink(out_plug,
                                h,
                                &asink,
                                NULL,
                                NULL))
      return 0;
    
    gavl_audio_connector_connect(s->aconn, asink);
    }
  else if(h->type == GAVF_STREAM_VIDEO)
    {
    gavl_video_sink_t * vsink = NULL;

    if(!bg_plug_get_stream_sink(out_plug,
                                h,
                                NULL,
                                &vsink,
                                NULL))
      return 0;
    
    gavl_video_connector_connect(s->vconn, vsink);
    }
  return 1;
  }

/* Config stuff */


static void opt_aud(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -aud requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(audio_section,
                               recorder_stream_set_parameter,
                               &as,
                               as.parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vid(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vid requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(video_section,
                               recorder_stream_set_parameter,
                               &vs,
                               vs.parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-aud",
      .help_arg =    "<audio_options>",
      .help_string = "Set audio recording options",
      .callback =    opt_aud,
    },
    {
      .arg =         "-vid",
      .help_arg =    "<video_options>",
      .help_string = "Set video recording options",
      .callback =    opt_vid,
    },
    {
      /* End */
    }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-record",
    .long_name = TRS("Record an A/V stream and output a gavf stream"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf recorder\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Options"), global_options },
                                       {  } },
    .files = (bg_cmdline_ext_doc_t[])
    { { "~/.gmerlin/plugins.xml",
        TRS("Cache of the plugin registry (shared by all applicatons)") },
      { "~/.gmerlin/generic/config.xml",
        TRS("Default plugin parameters are read from there. Use gmerlin_plugincfg to change them.") },
      { /* End */ }
    },
    
  };

int main(int argc, char ** argv)
  {
  gavftools_init_registries();
  
  out_plug = bg_plug_create_writer();
  
  /* Create plugin regsitry */
  
  /* Initialize streams */
  recorder_stream_init(&as, GAVF_STREAM_AUDIO);
  recorder_stream_init(&vs, GAVF_STREAM_VIDEO);

  global_options[0].parameters = as.parameters;
  global_options[1].parameters = vs.parameters;
  
  /* Handle commandline options */
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  /* Open plugins */
  if(!recorder_stream_open(&as, GAVF_STREAM_AUDIO, out_plug) &&
     !recorder_stream_open(&vs, GAVF_STREAM_VIDEO, out_plug))
    {
    return -1;
    }

  /* Start plug */
  if(!bg_plug_start(out_plug))
    return -1;

  /* Connect streams */

  if(!recorder_stream_connect(&as, out_plug) ||
     !recorder_stream_connect(&vs, out_plug))
    return -1;
  
  /* Main loop */
  while(1)
    {
    
    }

  /* Cleanup */
  
  return 0;
  }
