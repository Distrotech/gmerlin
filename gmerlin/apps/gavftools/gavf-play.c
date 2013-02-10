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

#include <gmerlin/ov.h>
#include <gmerlin/textrenderer.h>
#include <gmerlin/subtitle.h>

#define LOG_DOMAIN "gavf-play"

static bg_plug_t * in_plug = NULL;

static char * infile = NULL;

static bg_cfg_section_t * audio_section = NULL;
static bg_cfg_section_t * video_section = NULL;
static bg_cfg_section_t * subrender_section = NULL;

static int audio_stream = -1;
static int video_stream = -1;
static int text_stream = -1;
static int overlay_stream = -1;

typedef struct player_s player_t;

#define FLAG_ACTIVE    (1<<0)
#define FLAG_HAS_DELAY (1<<1) // Audio plugin reports latency

static gavl_time_t player_get_time(player_t * p);

typedef struct
  {
  gavl_audio_format_t fmt;
  bg_plugin_handle_t * h;
  bg_oa_plugin_t * plugin;
  bg_parameter_info_t * parameters;

  int flags;
  
  gavl_audio_sink_t * sink_int;
  gavl_audio_sink_t * sink_ext;
  int64_t samples_written;
  
  } audio_stream_t;

typedef struct
  {
  gavl_video_format_t fmt;
  bg_plugin_handle_t * h;
  bg_ov_plugin_t * plugin;
  bg_ov_t * ov;
  bg_parameter_info_t * parameters;
  
  player_t * p; // For synchronizing

  int flags;

  bg_text_renderer_t * renderer;
  bg_subtitle_handler_t * sh;
  
  } video_stream_t;

static const bg_parameter_info_t audio_parameters[] =
  {
    {
      .name      = "plugin",
      .long_name = TRS("Plugin"),
      .type      = BG_PARAMETER_MULTI_MENU,
      .flags     = BG_PARAMETER_PLUGIN,
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

static void init_audio(audio_stream_t * as)
  {
  as->parameters =
    bg_parameter_info_copy_array(audio_parameters);
  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        BG_PLUGIN_OUTPUT_AUDIO,
                                        BG_PLUGIN_PLAYBACK,
                                        &as->parameters[0]);

  audio_section = bg_cfg_section_create_from_parameters("audio",
                                                        as->parameters);

  }

static void cleanup_audio(audio_stream_t * as)
  {
  bg_parameter_info_destroy_array(as->parameters);
  if(as->h)
    bg_plugin_unref(as->h);
  if(as->sink_ext)
    gavl_audio_sink_destroy(as->sink_ext);
  }

static void cleanup_video(video_stream_t * vs)
  {
  bg_parameter_info_destroy_array(vs->parameters);
  if(vs->h)
    bg_plugin_unref(vs->h);
  }

static void init_video(video_stream_t * vs)
  {
  vs->parameters =
    bg_parameter_info_copy_array(video_parameters);
  bg_plugin_registry_set_parameter_info(plugin_reg,
                                        BG_PLUGIN_OUTPUT_VIDEO,
                                        BG_PLUGIN_PLAYBACK,
                                        &vs->parameters[0]);
  video_section = bg_cfg_section_create_from_parameters("video",
                                                        vs->parameters);
  }

static gavl_audio_frame_t * get_audio_frame(void * priv)
  {
  audio_stream_t * as = priv;
  return gavl_audio_sink_get_frame(as->sink_int);
  }

static gavl_sink_status_t put_audio_frame(void * priv,
                                          gavl_audio_frame_t * f)
  {
  gavl_time_t wait_time;
  gavl_sink_status_t ret;
  audio_stream_t * as = priv;

  bg_plugin_lock(as->h);
  if((ret = gavl_audio_sink_put_frame(as->sink_int, f)) != GAVL_SINK_OK)
    {
    bg_plugin_unlock(as->h);
    return ret;
    }
  as->samples_written += f->valid_samples;
  bg_plugin_unlock(as->h);

  wait_time = gavl_time_unscale(as->fmt.samplerate,
                                f->valid_samples / 2);
  gavl_time_delay(&wait_time);
  return ret;
  }

static void open_audio(audio_stream_t * as,
                       bg_mediaconnector_stream_t * s)
  {
  as->flags |= FLAG_ACTIVE;
  gavl_audio_format_copy(&as->fmt,
                         gavl_audio_source_get_src_format(s->asrc));
  as->plugin->open(as->h->priv, &as->fmt);

  if(as->plugin->get_delay)
    {
    as->flags |= FLAG_HAS_DELAY;
    as->sink_int = as->plugin->get_sink(as->h->priv);
    as->sink_ext = gavl_audio_sink_create(get_audio_frame,
                                          put_audio_frame, as,
                                          gavl_audio_sink_get_format(as->sink_int));
    gavl_audio_connector_connect(s->aconn, as->sink_ext);
    }
  else
    gavl_audio_connector_connect(s->aconn, as->plugin->get_sink(as->h->priv));
  }

static void start_audio(audio_stream_t * as)
  {
  if(as->plugin->start)
    as->plugin->start(as->h->priv);
  }


static gavl_time_t audio_stream_get_time(audio_stream_t * as)
  {
  gavl_time_t ret;
  bg_plugin_lock(as->h);
  ret = gavl_time_unscale(as->fmt.samplerate,
                          as->samples_written - as->plugin->get_delay(as->h->priv));
  bg_plugin_unlock(as->h);
  return ret;
  }

static void
process_cb_video(void * priv, gavl_video_frame_t * frame)
  {
  gavl_time_t frame_time, cur_time, diff_time;
  player_t * p;
  video_stream_t * vs = priv;

  /* Handle subtitle */
  
  //  fprintf(stderr, "Process video\n");

  p = vs->p;
  frame_time = gavl_time_unscale(vs->fmt.timescale,
                                 frame->timestamp);
  cur_time = player_get_time(p);
  
  diff_time = frame_time - cur_time;

  //  fprintf(stderr, "cur: %ld, frame: %ld, diff: %ld\n",
  //          cur_time, frame_time, diff_time);
  
  if(diff_time > 0)
    gavl_time_delay(&diff_time);

  if(vs->sh)
    bg_subtitle_handler_update(vs->sh, frame);
  
  //  fprintf(stderr, "Process video done\n");
  }

static void open_video(video_stream_t * vs,
                       bg_mediaconnector_stream_t * s)
  {
  vs->flags |= FLAG_ACTIVE;
  gavl_video_format_copy(&vs->fmt,
                         gavl_video_source_get_src_format(s->vsrc));

  vs->ov = bg_ov_create(vs->h);
  bg_ov_open(vs->ov, &vs->fmt, 1);
  gavl_video_connector_connect(s->vconn, bg_ov_get_sink(vs->ov));
  gavl_video_connector_set_process_func(s->vconn, process_cb_video, vs);
  }

static void open_subtitle(video_stream_t * vs,
                          bg_mediaconnector_stream_t * s)
  {
  gavl_video_format_t format;
  gavl_video_sink_t * sink;
  gavl_video_source_t * src;
  
  if(s->type == GAVF_STREAM_TEXT)
    {
    vs->renderer = bg_text_renderer_create();
    bg_cfg_section_apply(subrender_section,
                         bg_text_renderer_get_parameters(),
                         bg_text_renderer_set_parameter,
                         vs->renderer);

    gavl_video_format_copy(&format, &vs->fmt);

    format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
    format.timescale = s->timescale;

    sink = bg_ov_add_overlay_stream(vs->ov, &format);
    src = bg_text_renderer_connect(vs->renderer, s->psrc, &vs->fmt, &format);
    }
  else if(s->type == GAVF_STREAM_OVERLAY)
    {
    src = s->vsrc;
    gavl_video_format_copy(&format, gavl_video_source_get_src_format(src));
    sink = bg_ov_add_overlay_stream(vs->ov, &format);
    }
  
  vs->sh = bg_subtitle_handler_create();
  bg_subtitle_handler_init(vs->sh, &vs->fmt, src, sink);
  }

static void start_video(video_stream_t * vs)
  {
  bg_ov_show_window(vs->ov, 1);
  }

static void set_audio_parameter(void * data,
                                const char * name,
                                const bg_parameter_value_t * val)
  {
  audio_stream_t * s = data;
  
  if(!name)
    return;

  if(!strcmp(name, "plugin"))
    {
    const bg_plugin_info_t * info;
    
    if(s->h &&
       !strcmp(s->h->info->name, val->val_str))
      return;
    
    if(s->h)
      bg_plugin_unref(s->h);
    
    info = bg_plugin_find_by_name(plugin_reg, val->val_str);
    s->h = bg_plugin_load(plugin_reg, info);
    s->plugin = (bg_oa_plugin_t*)(s->h->plugin);
    // if(s->input_plugin->set_callbacks)
    // s->input_plugin->set_callbacks(s->h->priv, &rec->recorder_cb);
    }
  }

static void set_video_parameter(void * data,
                                const char * name,
                                const bg_parameter_value_t * val)
  {
  video_stream_t * s = data;
  
  if(!name)
    return;
  
  if(!strcmp(name, "plugin"))
    {
    const bg_plugin_info_t * info;
    
    if(s->h &&
       !strcmp(s->h->info->name, val->val_str))
      return;
    
    if(s->h)
      bg_plugin_unref(s->h);
    
    info = bg_plugin_find_by_name(plugin_reg, val->val_str);
    s->h = bg_plugin_load(plugin_reg, info);
    s->plugin = (bg_ov_plugin_t*)(s->h->plugin);
    // if(s->input_plugin->set_callbacks)
    // s->input_plugin->set_callbacks(s->h->priv, &rec->recorder_cb);
    }
  }

struct player_s
  {
  audio_stream_t as;
  video_stream_t vs;
  gavl_timer_t * timer;
  };

static player_t player;

static void player_init(player_t * p)
  {
  memset(p, 0, sizeof(*p));
  
  p->vs.p = p;
  init_audio(&p->as);
  init_video(&p->vs);
  
  subrender_section =
    bg_cfg_section_create_from_parameters("subrender",
                                          bg_text_renderer_get_parameters());
  }

static void player_cleanup(player_t * p)
  {
  cleanup_audio(&p->as);
  cleanup_video(&p->vs);
  if(p->timer)
    gavl_timer_destroy(p->timer);
  }

static void player_open(player_t * p, bg_mediaconnector_t * conn)
  {
  int i;

  /* A/V */
  for(i = 0; i < conn->num_streams; i++)
    {
    switch(conn->streams[i]->type)
      {
      case GAVF_STREAM_AUDIO:
        open_audio(&p->as, conn->streams[i]);
        break;
      case GAVF_STREAM_VIDEO:
        open_video(&p->vs, conn->streams[i]);
        break;
      case GAVF_STREAM_TEXT:
      case GAVF_STREAM_OVERLAY:
        break;
      }
    }

  /* subtitle */
  for(i = 0; i < conn->num_streams; i++)
    {
    switch(conn->streams[i]->type)
      {
      case GAVF_STREAM_AUDIO:
      case GAVF_STREAM_VIDEO:
        break;
      case GAVF_STREAM_TEXT:
      case GAVF_STREAM_OVERLAY:
        open_subtitle(&p->vs, conn->streams[i]);
        break;
      }
    }

  

  if(!(p->as.flags & FLAG_HAS_DELAY))
    p->timer = gavl_timer_create();
  
  }

static void player_start(player_t * p)
  {
  if(p->timer)
    gavl_timer_start(p->timer);
  
  if(p->as.flags & FLAG_ACTIVE)
    start_audio(&p->as);
  if(p->vs.flags & FLAG_ACTIVE)
    start_video(&p->vs);
  }

static gavl_time_t player_get_time(player_t * p)
  {
  if(p->timer)
    return gavl_timer_get(p->timer);
  else
    {
    return audio_stream_get_time(&p->as);
    }
  
  return GAVL_TIME_UNDEFINED;
  }

/* Option processing */

static void opt_aud(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -aud requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(audio_section,
                               set_audio_parameter,
                               &player.as,
                               player.as.parameters,
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
                               set_video_parameter,
                               &player.vs,
                               player.vs.parameters,
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_subrender(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -subrender requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_apply_options(subrender_section,
                               NULL,
                               NULL,
                               bg_text_renderer_get_parameters(),
                               (*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


static void opt_i(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -i requires an argument\n");
    exit(-1);
    }
  infile = (*_argv)[arg];
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_as(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -as requires an argument\n");
    exit(-1);
    }
  audio_stream = atoi((*_argv)[arg]) - 1;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vs(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vs requires an argument\n");
    exit(-1);
    }
  video_stream = atoi((*_argv)[arg]) - 1;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_ts(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -ts requires an argument\n");
    exit(-1);
    }
  text_stream = atoi((*_argv)[arg]) - 1;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_os(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -os requires an argument\n");
    exit(-1);
    }
  overlay_stream = atoi((*_argv)[arg]) - 1;
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
      .arg =         "-as",
      .help_arg =    "<stream>",
      .help_string = "Select audio stream (default: 1)",
      .callback =    opt_as,
    },
    {
      .arg =         "-vs",
      .help_arg =    "<stream>",
      .help_string = "Select video stream (default: 1)",
      .callback =    opt_vs,
    },
    {
      .arg =         "-ts",
      .help_arg =    "<stream>",
      .help_string = "Select text stream (default: disabled)",
      .callback =    opt_ts,
    },
    {
      .arg =         "-os",
      .help_arg =    "<stream>",
      .help_string = "Select overlay stream (default: disabled)",
      .callback =    opt_os,
    },
    {
      .arg =         "-i",
      .help_arg =    "<location>",
      .help_string = "Set input file or location",
      .callback =    opt_i,
    },
    {
      .arg =         "-subrender",
      .help_arg =    "<options>",
      .help_string = "Set the options for rendering text subtitles",
      .callback =    opt_subrender,
    },
    {
      /* End */
    }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-play",
    .long_name = TRS("Playback of a gavf stream"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf player\n"),
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

static const gavf_stream_header_t *
find_stream(gavf_t * g, int type, int index)
  {
  const gavf_stream_header_t * ret;
  if(index < 0)
    ret = gavf_get_stream(g, 0, type);
  else
    {
    ret = gavf_get_stream(g, index, type);
    if(!ret)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No %s stream %d",
             gavf_stream_type_name(type),
             index + 1);
      }
    }
  return ret;
  }

static int stream_equal(const gavf_stream_header_t * h1,
                        const gavf_stream_header_t * h2)
  {
  if(!h1 || !h2)
    return 0;
  return (h1->id == h2->id);
  }
                         

int main(int argc, char ** argv)
  {
  const gavf_stream_header_t * as;
  const gavf_stream_header_t * vs;
  const gavf_stream_header_t * ts;
  const gavf_stream_header_t * os;
  
  const gavf_program_header_t * ph;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 100; // 10 ms
  
  gavf_t * g;
  int ret = 1;
  int i;
  
  bg_mediaconnector_t conn;
  
  bg_mediaconnector_init(&conn);
  gavftools_init_registries();

  player_init(&player);

  bg_cmdline_arg_set_parameters(global_options, "-aud", player.as.parameters);
  bg_cmdline_arg_set_parameters(global_options, "-vid", player.vs.parameters);
  bg_cmdline_arg_set_parameters(global_options, "-subrender",
                                bg_text_renderer_get_parameters());
  
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  if(!bg_cmdline_check_unsupported(argc, argv))
    return -1;
  
  in_plug = bg_plug_create_reader(plugin_reg);

  /* Open */

  if(!bg_plug_open_location(in_plug, infile, NULL, NULL))
    return ret;
  
  /* Select streams */
  
  g = bg_plug_get_gavf(in_plug);

  as = find_stream(g, GAVF_STREAM_AUDIO, audio_stream);
  vs = find_stream(g, GAVF_STREAM_VIDEO, video_stream);

  if(text_stream >= 0)
    ts = find_stream(g, GAVF_STREAM_TEXT, text_stream);
  else
    ts = NULL;
  
  if(overlay_stream >= 0)
    os = find_stream(g, GAVF_STREAM_OVERLAY, overlay_stream);
  else
    os = NULL;

  if(os && ts)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Both overlay and text stream specified, omitting overlay stream");
    os = NULL;
    }
  
  if(!player.as.plugin && as)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Won't play audio: No plugin specified");
    as = NULL;
    }
  
  if(!player.vs.plugin && vs)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Won't play video: No plugin specified");
    vs = NULL;
    }

  if(!as && !vs)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Neither audio nor video can be played, exiting");
    return ret;
    }

  ph = gavf_get_program_header(g);
  
  for(i = 0; i < ph->num_streams; i++)
    {
    if(stream_equal(&ph->streams[i], as) ||
       stream_equal(&ph->streams[i], vs) ||
       stream_equal(&ph->streams[i], ts) ||
       stream_equal(&ph->streams[i], os))
      bg_plug_set_stream_action(in_plug, &ph->streams[i],
                                BG_STREAM_ACTION_DECODE);
    else
      bg_plug_set_stream_action(in_plug, &ph->streams[i],
                                BG_STREAM_ACTION_OFF);
    }
  
  /* Start plug and set up media connector */
  
  if(!bg_plug_setup_reader(in_plug, &conn))
    return ret;

  bg_mediaconnector_create_threads(&conn, 0);
  bg_mediaconnector_threads_init_separate(&conn);
  
  /* Initialize output plugins */

  player_open(&player, &conn);
  
  /* Start media connector */

  bg_mediaconnector_start(&conn);

  player_start(&player);
  
  bg_mediaconnector_threads_start(&conn);
  
  /* Main loop */
  while(1)
    {
    /* Check for end */
    if(bg_mediaconnector_done(&conn))
      break;

    gavl_time_delay(&delay_time);
    }
  
  /* Cleanup */
  bg_mediaconnector_threads_stop(&conn);
  bg_mediaconnector_free(&conn);

  bg_plug_destroy(in_plug);

  player_cleanup(&player);
  
  gavftools_destroy_registries();

  if(audio_section)
    bg_cfg_section_destroy(audio_section);
  if(video_section)
    bg_cfg_section_destroy(video_section);
    
  ret = 0;
  return ret;
  }
