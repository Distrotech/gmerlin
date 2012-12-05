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
#include <signal.h>

#include "gavftools.h"

#include <gavl/metatags.h>

#include <language_table.h>

#define LOG_DOMAIN "gavf-record"

int got_sigint = 0;
static void sigint_handler(int sig)
  {
  fprintf(stderr, "\nCTRL+C caught\n");
  got_sigint = 1;
  }

static void set_sigint_handler()
  {
  struct sigaction sa;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = sigint_handler;
  if (sigaction(SIGINT, &sa, NULL) == -1)
    {
    fprintf(stderr, "sigaction failed\n");
    }
  }


/* Global stuff */


static bg_plug_t * out_plug;

static bg_cfg_section_t * audio_section;
static bg_cfg_section_t * video_section;

static char * outfile = NULL;

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
  bg_parameter_info_t * parameters;
  gavl_metadata_t m;
  } recorder_stream_t;

typedef struct
  {
  recorder_stream_t as;
  recorder_stream_t vs;
  } recorder_t;

static recorder_t rec;

static void recorder_stream_init(recorder_stream_t * s,
                                 const bg_parameter_info_t * parameters,
                                 const char * name, bg_cfg_section_t ** section,
                                 uint32_t type_mask)
  {
  memset(s, 0, sizeof(*s));
  s->parameters = bg_parameter_info_copy_array(parameters);
  
  if(!(*section))
    *section = bg_cfg_section_create_from_parameters(name, s->parameters);
  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        type_mask,
                                        BG_PLUGIN_RECORDER,
                                        &s->parameters[0]);
  }

static void recorder_init(recorder_t * rec)
  {
  recorder_stream_init(&rec->as,
                       audio_parameters,
                       "audio", &audio_section,
                       BG_PLUGIN_RECORDER_AUDIO);
  recorder_stream_init(&rec->vs,
                       video_parameters,
                       "video", &video_section,
                       BG_PLUGIN_RECORDER_VIDEO);
  }
                         

static void recorder_stream_set_parameter(void * sp, const char * name,
                                          const bg_parameter_value_t * val)
  {
  recorder_stream_t * s= sp;

  fprintf(stderr, "recorder_stream_set_parameter %s\n", name);
  
  if(!name)
    {
    if(s->h && s->plugin->common.set_parameter)
      s->plugin->common.set_parameter(s->h->priv, NULL, NULL);
    return;
    }
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
                                bg_mediaconnector_t * conn)
  {
  gavl_compression_info_t ci;
  gavl_audio_format_t afmt;
  gavl_video_format_t vfmt;
  
  if(!s->h)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Not recording %s: No plugin set",
           (type == GAVF_STREAM_AUDIO ? "audio" : "video"));
    return 0;
    }
  memset(&ci, 0, sizeof(ci));
  memset(&afmt, 0, sizeof(afmt));
  memset(&vfmt, 0, sizeof(vfmt));

  if(!s->plugin->open(s->h->priv, &afmt, &vfmt))
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Opening %s recorder failed",
           (type == GAVF_STREAM_AUDIO ? "audio" : "video"));
    return 0;
    }
  if(type == GAVF_STREAM_AUDIO)
    {
    gavl_audio_source_t * asrc;
    asrc = s->plugin->get_audio_source(s->h->priv);
    bg_mediaconnector_add_audio_stream(conn, NULL, asrc, NULL);
    }
  else if(type == GAVF_STREAM_VIDEO)
    {
    gavl_video_source_t * vsrc;
    vsrc = s->plugin->get_video_source(s->h->priv);
    bg_mediaconnector_add_video_stream(conn, NULL, vsrc, NULL);
    }
  return 1;
  }

static int recorder_open(recorder_t * rec, bg_mediaconnector_t * conn)
  {
  int do_audio;
  int do_video;

  do_audio = recorder_stream_open(&rec->as, GAVF_STREAM_AUDIO, conn);
  do_video = recorder_stream_open(&rec->vs, GAVF_STREAM_VIDEO, conn);
  
  if(!do_audio && !do_video)
    return 0;
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
                               &rec.as,
                               rec.as.parameters,
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
                               &rec.vs,
                               rec.vs.parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_out(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -o requires an argument\n");
    exit(-1);
    }
  outfile = (*_argv)[arg];
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
      .arg =         "-o",
      .help_arg =    "<location>",
      .help_string = "Set output file or location",
      .callback =    opt_out,
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
  bg_mediaconnector_t conn;
  bg_mediaconnector_init(&conn);
  
  gavftools_init_registries();
  
  out_plug = bg_plug_create_writer(plugin_reg);
  
  /* Create plugin regsitry */
  
  /* Initialize streams */
  recorder_init(&rec);
  
  global_options[0].parameters = rec.as.parameters;
  global_options[1].parameters = rec.vs.parameters;
  
  /* Handle commandline options */
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  /* Set output file */
  if(!outfile)
    outfile = "-";
  
  /* Open plugins */
  if(!recorder_open(&rec, &conn))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Opening recorder plugins failed");
    return -1;
    }

  /* Open output plug */
  if(!bg_plug_open_location(out_plug, outfile,
                            (const gavl_metadata_t *)0,
                            (const gavl_chapter_list_t*)0))
    return 0;
  
  /* Initialize output plug */
  if(!bg_plug_setup_writer(out_plug, &conn))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up plug writer failed");
    return -1;
    }
  
  bg_mediaconnector_start(&conn);

  set_sigint_handler();
  
  /* Main loop */
  while(1)
    {
    if(got_sigint)
      break;
    if(!bg_mediaconnector_iteration(&conn))
      break;
    }
  
  /* Cleanup */

  bg_plug_destroy(out_plug);
  
  return 0;
  }
