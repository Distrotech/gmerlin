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

#include "gavftools.h"

#include <gmerlin/filters.h>
#include <gmerlin/bggavl.h>

#define LOG_DOMAIN "gavf-filter"

static gavl_metadata_t af_options;
static gavl_metadata_t vf_options;

static bg_parameter_info_t * af_parameters;
static bg_parameter_info_t * vf_parameters;

static bg_plug_t * in_plug = NULL;
static bg_plug_t * out_plug = NULL;


static void opt_af(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -af requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&af_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vf(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vf requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&vf_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    GAVFTOOLS_INPLUG_OPTIONS,
    GAVFTOOLS_AQ_OPTIONS,
    GAVFTOOLS_VQ_OPTIONS,
    GAVFTOOLS_AUDIO_STREAM_OPTIONS,
    GAVFTOOLS_VIDEO_STREAM_OPTIONS,
    GAVFTOOLS_TEXT_STREAM_OPTIONS,
    GAVFTOOLS_OVERLAY_STREAM_OPTIONS,
    {
      .arg =         "-af",
      .help_arg =    "<options>",
      .help_string = "Audio compression options",
      .callback =    opt_af,
    },
    {
      .arg =         "-vf",
      .help_arg =    "<options>",
      .help_string = "Video compression options",
      .callback =    opt_vf,
    },
    GAVFTOOLS_OUTPLUG_OPTIONS,
    GAVFTOOLS_VERBOSE_OPTIONS,
    { /* End */ },
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-filter",
    .long_name = TRS("Apply A/V filter to a gavf stream"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf filter\n"),
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

static bg_parameter_info_t * create_af_parameters()
  {
  bg_parameter_info_t * ret;
  bg_audio_filter_chain_t * ch =
    bg_audio_filter_chain_create(&gavltools_aopt, plugin_reg);

  ret =
    bg_parameter_info_copy_array(bg_audio_filter_chain_get_parameters(ch));
  
  bg_audio_filter_chain_destroy(ch);
  return ret;
  }

static bg_parameter_info_t * create_vf_parameters()
  {
  bg_parameter_info_t * ret;
  bg_video_filter_chain_t * ch =
    bg_video_filter_chain_create(&gavltools_vopt, plugin_reg);

  ret =
    bg_parameter_info_copy_array(bg_video_filter_chain_get_parameters(ch));
  
  bg_video_filter_chain_destroy(ch);
  return ret;
  }
  

int main(int argc, char ** argv)
  {
  int ret = 1;
  bg_mediaconnector_t conn;
  bg_stream_action_t * audio_actions = NULL;
  bg_stream_action_t * video_actions = NULL;
  bg_stream_action_t * text_actions = NULL;
  bg_stream_action_t * overlay_actions = NULL;

  bg_audio_filter_chain_t ** audio_filters;
  bg_video_filter_chain_t ** video_filters;
  int num_audio_streams;
  int num_video_streams;
  int num, i;
  gavf_t * g;
  const gavf_stream_header_t * sh;
  
  gavftools_init();
  
  gavftools_block_sigpipe();
  bg_mediaconnector_init(&conn);
  

  /* Create dummy audio and video filter chain to obtain the parameters */
  af_parameters = create_af_parameters();
  vf_parameters = create_vf_parameters();

  bg_cmdline_arg_set_parameters(global_options, "-af",
                                af_parameters);
  bg_cmdline_arg_set_parameters(global_options, "-vf",
                                vf_parameters);
  
  gavl_metadata_init(&af_options);
  gavl_metadata_init(&vf_options);

  gavftools_set_cmdline_parameters(global_options);

  
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  in_plug = gavftools_create_in_plug();
  out_plug = gavftools_create_out_plug();

  if(!bg_plug_open_location(in_plug, gavftools_in_file, NULL, NULL))
    goto fail;
  
  g = bg_plug_get_gavf(in_plug);
  
  /* Get stream actions */

  num_audio_streams = gavf_get_num_streams(g, GAVF_STREAM_AUDIO);
  audio_actions = gavftools_get_stream_actions(num_audio_streams,
                                               GAVF_STREAM_AUDIO);

  for(i = 0; i < num_audio_streams; i++)
    {
    sh = gavf_get_stream(g, i, GAVF_STREAM_AUDIO);

    if((sh->ci.id != GAVL_CODEC_ID_NONE) &&
       (bg_cmdline_get_stream_options(&af_options, i)))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN,
             "Decompressing audio stream %d", i+1);
      bg_plug_set_stream_action(in_plug, sh, BG_STREAM_ACTION_DECODE);
      }
    else
      {
      bg_plug_set_stream_action(in_plug, sh, audio_actions[i]);
      }
    }
  
  num_video_streams = gavf_get_num_streams(g, GAVF_STREAM_VIDEO);
  video_actions = gavftools_get_stream_actions(num_video_streams,
                                               GAVF_STREAM_VIDEO);

  for(i = 0; i < num_video_streams; i++)
    {
    sh = gavf_get_stream(g, i, GAVF_STREAM_VIDEO);

    if((sh->ci.id != GAVL_CODEC_ID_NONE) &&
       (bg_cmdline_get_stream_options(&vf_options, i)))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN,
             "Decompressing video stream %d", i+1);
      bg_plug_set_stream_action(in_plug, sh, BG_STREAM_ACTION_DECODE);
      }
    else
      {
      bg_plug_set_stream_action(in_plug, sh, video_actions[i]);
      }
    }

  num = gavf_get_num_streams(g, GAVF_STREAM_TEXT);
  text_actions = gavftools_get_stream_actions(num, GAVF_STREAM_TEXT);

  for(i = 0; i < num; i++)
    {
    sh = gavf_get_stream(g, i, GAVF_STREAM_TEXT);
    bg_plug_set_stream_action(in_plug, sh, text_actions[i]);
    }

  num = gavf_get_num_streams(g, GAVF_STREAM_OVERLAY);
  overlay_actions = gavftools_get_stream_actions(num, GAVF_STREAM_OVERLAY);

  for(i = 0; i < num; i++)
    {
    sh = gavf_get_stream(g, i, GAVF_STREAM_OVERLAY);
    bg_plug_set_stream_action(in_plug, sh, overlay_actions[i]);
    }
  
  
  /* Start input plug */

  if(!bg_plug_start(in_plug))
    goto fail;
  
  /* Set up filters */
  audio_filters = calloc(num_audio_streams, sizeof(*audio_filters));
  video_filters = calloc(num_audio_streams, sizeof(*video_filters));

  for(i = 0; i < num_audio_streams; i++)
    {
    gavl_audio_source_t * asrc;
    
    const char * filter_options;
    bg_cfg_section_t * sec;

    filter_options = bg_cmdline_get_stream_options(&af_options, i);
    if(!filter_options)
      continue;

    sh = gavf_get_stream(g, i, GAVF_STREAM_AUDIO);
    
    audio_filters[i] =
      bg_audio_filter_chain_create(&gavltools_aopt, plugin_reg);
    
    sec = bg_cfg_section_create_from_parameters("af", af_parameters);

    if(!bg_cmdline_apply_options(sec,
                                 bg_audio_filter_chain_set_parameter,
                                 audio_filters[i],
                                 af_parameters,
                                 filter_options))
      exit(-1);

    asrc = NULL;
    
    bg_plug_get_stream_source(in_plug, sh, &asrc, NULL, NULL);
    asrc = bg_audio_filter_chain_connect(audio_filters[i], asrc);
    bg_plug_set_audio_proxy(in_plug, i, asrc, NULL);
    bg_cfg_section_destroy(sec);
    }

  for(i = 0; i < num_video_streams; i++)
    {
    gavl_video_source_t * vsrc;
    
    const char * filter_options;
    bg_cfg_section_t * sec;

    filter_options = bg_cmdline_get_stream_options(&vf_options, i);
    if(!filter_options)
      continue;

    sh = gavf_get_stream(g, i, GAVF_STREAM_VIDEO);
    
    video_filters[i] =
      bg_video_filter_chain_create(&gavltools_vopt, plugin_reg);
    
    sec = bg_cfg_section_create_from_parameters("vf", vf_parameters);

    if(!bg_cmdline_apply_options(sec,
                                 bg_video_filter_chain_set_parameter,
                                 video_filters[i],
                                 vf_parameters,
                                 filter_options))
      exit(-1);

    vsrc = NULL;
    
    bg_plug_get_stream_source(in_plug, sh, NULL, &vsrc, NULL);
    vsrc = bg_video_filter_chain_connect(video_filters[i], vsrc);
    bg_plug_set_video_proxy(in_plug, i, vsrc, NULL);
    bg_cfg_section_destroy(sec);
    }

  if(!bg_plug_setup_reader(in_plug, &conn))
    goto fail;
    
  /* Set up out plug */

  g = bg_plug_get_gavf(in_plug);

  /* Open output plug */

  if(!gavftools_open_out_plug_from_in_plug(out_plug, in_plug))
    goto fail;
  
  
  if(!bg_plug_setup_writer(out_plug, &conn))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Setting up plug writer failed");
    goto fail;
    }

  /* Fire up connector */

  bg_mediaconnector_start(&conn);
  
  /* Run */

  while(1)
    {
    if(bg_plug_got_error(in_plug))
      break;

    if(gavftools_stop() ||
       !bg_mediaconnector_iteration(&conn))
      break;
    }
    
  /* Cleanup */

  for(i = 0; i < num_audio_streams; i++)
    {
    if(audio_filters[i])
      bg_audio_filter_chain_destroy(audio_filters[i]);
    }
  if(audio_filters)
    free(audio_filters);

  for(i = 0; i < num_video_streams; i++)
    {
    if(video_filters[i])
      bg_video_filter_chain_destroy(video_filters[i]);
    }
  if(video_filters)
    free(video_filters);

  
  bg_mediaconnector_free(&conn);
  bg_plug_destroy(in_plug);
  bg_plug_destroy(out_plug);
  
  gavftools_cleanup();

  if(audio_actions)
    free(audio_actions);
  if(video_actions)
    free(video_actions);
  if(text_actions)
    free(text_actions);
  if(overlay_actions)
    free(overlay_actions);

  
  ret = 0;
  fail:
  return ret;
  }
