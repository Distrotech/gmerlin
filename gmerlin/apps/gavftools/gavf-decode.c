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
#define LOG_DOMAIN "gavf-decode"

static const char * input_file   = NULL;
static const char * input_plugin = NULL;
static bg_plug_t * out_plug = NULL;
int use_edl = 0;
int track;

static char * outfile = "-";

static void opt_t(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -t requires an argument\n");
    exit(-1);
    }
  track = atoi((*_argv)[arg]) - 1;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


static void opt_in(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -i requires an argument\n");
    exit(-1);
    }
  input_file = (*_argv)[arg];
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_plugin(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -p requires an argument\n");
    exit(-1);
    }
  input_plugin = (*_argv)[arg];
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_edl(void * data, int * argc, char *** _argv, int arg)
  {
  use_edl = 1;
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
      .arg =         "-i",
      .help_arg =    "<location>",
      .help_string = "Set input file or location",
      .callback =    opt_in,
    },
    {
      .arg =         "-p",
      .help_arg =    "<plugin>",
      .help_string = "Set input file plugin to use",
      .callback =    opt_plugin,
      
    },
    {
      .arg =         "-t",
      .help_arg =    "<track>",
      .help_string = "Track (starting with 1)",
      .callback =    opt_t,
    },
    {
      .arg =         "-edl",
      .help_string = "Use EDL",
      .callback =    opt_edl,
    },
    GAVFTOOLS_AUDIO_STREAM_OPTIONS,
    GAVFTOOLS_VIDEO_STREAM_OPTIONS,
    GAVFTOOLS_TEXT_STREAM_OPTIONS,
    GAVFTOOLS_OVERLAY_STREAM_OPTIONS,
    GAVFTOOLS_AUDIO_COMPRESSOR_OPTIONS,
    GAVFTOOLS_VIDEO_COMPRESSOR_OPTIONS,
    GAVFTOOLS_OVERLAY_COMPRESSOR_OPTIONS,
    {
      .arg =         "-o",
      .help_arg =    "<location>",
      .help_string = "Set output file or location",
      .callback =    opt_out,
    },
    GAVFTOOLS_VERBOSE_OPTIONS,
    {
      /* End */
    }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-decode",
    .long_name = TRS("Decode a media source and output a gavf stream"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf decoder\n"),
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

typedef struct
  {
  bg_plug_t * out_plug;
  } cb_data_t;

static void update_metadata(void * priv, const gavl_metadata_t * m)
  {
  cb_data_t * d = priv;
  gavf_t * g = bg_plug_get_gavf(d->out_plug);
  gavf_update_metadata(g, m);
  }

int main(int argc, char ** argv)
  {
  int i;
  int ret = 1;
  bg_mediaconnector_t conn;

  bg_plugin_handle_t * h = NULL;
  bg_input_plugin_t * plugin;
  const bg_plugin_info_t * plugin_info;
  bg_input_callbacks_t cb;
  cb_data_t cb_data;
  bg_track_info_t * track_info;

  bg_stream_action_t * audio_actions;
  bg_stream_action_t * video_actions;
  bg_stream_action_t * text_actions;
  bg_stream_action_t * overlay_actions;

  gavl_audio_source_t * asrc;
  gavl_video_source_t * vsrc;
  gavl_packet_source_t * psrc;

  const gavl_metadata_t * stream_metadata;
  
  memset(&cb, 0, sizeof(cb));
  memset(&cb_data, 0, sizeof(cb_data));
  
  gavftools_block_sigpipe();
  bg_mediaconnector_init(&conn);
  gavftools_init_registries();

  gavftools_set_compresspor_options(global_options);
  
  /* Handle commandline options */
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  if(!bg_cmdline_check_unsupported(argc, argv))
    return -1;
  
  /* Create out plug */
  out_plug = bg_plug_create_writer(plugin_reg);

  cb_data.out_plug = out_plug;
  cb.data = &cb_data;
  cb.metadata_changed = update_metadata;
  
  bg_plug_set_compressor_config(out_plug,
                                gavftools_ac_params(),
                                gavftools_vc_params(),
                                gavftools_oc_params());

  /* Open location */

  if(!input_file)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No input file or source given");
    return ret;
    }
  
  if(input_plugin)
    {
    plugin_info = bg_plugin_find_by_name(plugin_reg, input_plugin);
    if(!plugin_info)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Input plugin %s not found", input_plugin);
      return ret;
      }
    }
  else
    plugin_info = NULL;

  if(!bg_input_plugin_load(plugin_reg,
                           input_file,
                           plugin_info,
                           &h, &cb, use_edl))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Couldn't load %s", input_file);
    goto fail;
    }
  
  plugin = (bg_input_plugin_t*)h->plugin;
  
  /* Select track and get track info */
  if((track < 0) || (track >= plugin->get_num_tracks(h->priv)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No such track %d", track);
    goto fail;
    }

  if(plugin->set_track)
    plugin->set_track(h->priv, track);
  
  track_info = plugin->get_track_info(h->priv, track);

  /* Select A/V streams */
  
  audio_actions = gavftools_get_stream_actions(track_info->num_audio_streams,
                                               GAVF_STREAM_AUDIO);
  video_actions = gavftools_get_stream_actions(track_info->num_video_streams,
                                               GAVF_STREAM_VIDEO);
  text_actions = gavftools_get_stream_actions(track_info->num_text_streams,
                                              GAVF_STREAM_TEXT);
  overlay_actions = gavftools_get_stream_actions(track_info->num_overlay_streams,
                                                 GAVF_STREAM_OVERLAY);

  /* Check for compressed reading */

  for(i = 0; i < track_info->num_audio_streams; i++)
    {
    if((audio_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (!plugin->get_audio_compression_info ||
        !plugin->get_audio_compression_info(h->priv, i, NULL)))
      {
      audio_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Audio stream %d cannot be read compressed",
             i+1);
      }
    }

  for(i = 0; i < track_info->num_video_streams; i++)
    {
    if((video_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (!plugin->get_video_compression_info ||
        !plugin->get_video_compression_info(h->priv, i, NULL)))
      {
      video_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Video stream %d cannot be read compressed",
             i+1);
      }
    }

  for(i = 0; i < track_info->num_overlay_streams; i++)
    {
    if((overlay_actions[i] == BG_STREAM_ACTION_READRAW) &&
       (!plugin->get_overlay_compression_info(h->priv, i, NULL)))
      {
      overlay_actions[i] = BG_STREAM_ACTION_DECODE;
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Overlay stream %d cannot be read compressed",
             i+1);
      }
    }

  /* Enable streams */
  
  if(plugin->set_audio_stream)
    {
    for(i = 0; i < track_info->num_audio_streams; i++)
      plugin->set_audio_stream(h->priv, i, audio_actions[i]);
    }
  if(plugin->set_video_stream)
    {
    for(i = 0; i < track_info->num_video_streams; i++)
      plugin->set_video_stream(h->priv, i, video_actions[i]);
    }
  if(plugin->set_text_stream)
    {
    for(i = 0; i < track_info->num_text_streams; i++)
      plugin->set_text_stream(h->priv, i, text_actions[i]);
    }
  if(plugin->set_overlay_stream)
    {
    for(i = 0; i < track_info->num_overlay_streams; i++)
      plugin->set_overlay_stream(h->priv, i, overlay_actions[i]);
    }
  
  /* Start plugin */
  if(plugin->start && !plugin->start(h->priv))
    goto fail;
  
  for(i = 0; i < track_info->num_audio_streams; i++)
    {
    stream_metadata = &track_info->audio_streams[i].m;
    
    switch(audio_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        asrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        asrc = plugin->get_audio_source(h->priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_audio_packet_source(h->priv, i);
        asrc = NULL;
        break;
      }

    if(asrc || psrc)
      bg_mediaconnector_add_audio_stream(&conn, stream_metadata, asrc, psrc,
                                         gavftools_ac_section());
    }
  
  for(i = 0; i < track_info->num_video_streams; i++)
    {
    stream_metadata = &track_info->video_streams[i].m;
    switch(video_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        vsrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        vsrc = plugin->get_video_source(h->priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_video_packet_source(h->priv, i);
        vsrc = NULL;
        break;
      }
    if(vsrc || psrc)
      bg_mediaconnector_add_video_stream(&conn, stream_metadata, vsrc, psrc,
                                         gavftools_vc_section());
    }
  
  for(i = 0; i < track_info->num_text_streams; i++)
    {
    stream_metadata = &track_info->text_streams[i].m;
    switch(text_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
      case BG_STREAM_ACTION_READRAW:
        psrc = plugin->get_text_source(h->priv, i);
        break;
      }
    if(psrc)
      bg_mediaconnector_add_text_stream(&conn, stream_metadata, psrc,
                                        track_info->text_streams[i].timescale);
    }
  
  for(i = 0; i < track_info->num_overlay_streams; i++)
    {
    stream_metadata = &track_info->overlay_streams[i].m;
    switch(text_actions[i])
      {
      case BG_STREAM_ACTION_OFF:
        vsrc = NULL;
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_DECODE:
        vsrc = plugin->get_overlay_source(h->priv, i);
        psrc = NULL;
        break;
      case BG_STREAM_ACTION_READRAW:
        vsrc = NULL;
        psrc = plugin->get_overlay_packet_source(h->priv, i);
        break;
      }
    if(vsrc || psrc)
      bg_mediaconnector_add_overlay_stream(&conn, stream_metadata, vsrc, psrc,
                                           gavftools_oc_section());
    }

  /* Open output plug */
  if(!bg_plug_open_location(out_plug, outfile,
                            &track_info->metadata,
                            track_info->chapter_list))
    goto fail;
  
  /* Initialize output plug */
  if(!bg_plug_setup_writer(out_plug, &conn))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up plug writer failed");
    goto fail;
    }

  
  ret = 1;

  bg_mediaconnector_start(&conn);

  while(1)
    {
    if(bg_plug_got_error(out_plug) ||
       !bg_mediaconnector_iteration(&conn))
      break;
    }
  ret = 0;
  fail:

  /* Cleanup */
  if(audio_actions)
    free(audio_actions);
  if(video_actions)
    free(video_actions);
  if(text_actions)
    free(text_actions);
  if(overlay_actions)
    free(overlay_actions);
  
  bg_mediaconnector_free(&conn);
  bg_plug_destroy(out_plug);

  bg_plugin_unref(h);
  
  gavftools_cleanup();
  
  return ret;
  }
