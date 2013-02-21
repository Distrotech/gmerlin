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

#include <gmerlin/encoder.h>

static bg_plug_t * in_plug = NULL;

#define LOG_DOMAIN "gavf-play"

static gavl_metadata_t ac_options;
static gavl_metadata_t vc_options;
static gavl_metadata_t oc_options;

static void opt_ac(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -ac requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&ac_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_vc(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -vc requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&vc_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_oc(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -oc requires an argument\n");
    exit(-1);
    }
  if(!bg_cmdline_set_stream_options(&oc_options,(*_argv)[arg]))
    exit(-1);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    GAVFTOOLS_INPLUG_OPTIONS,
    GAVFTOOLS_AUDIO_STREAM_OPTIONS,
    GAVFTOOLS_VIDEO_STREAM_OPTIONS,
    GAVFTOOLS_TEXT_STREAM_OPTIONS,
    GAVFTOOLS_OVERLAY_STREAM_OPTIONS,
    {
      .arg =         "-ac",
      .help_arg =    "<options>",
      .help_string = "Audio compression options",
      .callback =    opt_ac,
    },
    {
      .arg =         "-vc",
      .help_arg =    "<options>",
      .help_string = "Video compression options",
      .callback =    opt_vc,
    },
    {
      .arg =         "-oc",
      .help_arg =    "<options>",
      .help_string = "Overlay compression options",
      .callback =    opt_oc,
    },
    GAVFTOOLS_VERBOSE_OPTIONS,
    { /* End */ },
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gavf-encode",
    .long_name = TRS("Encode a gavf stream to another format"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gavf encoder\n"),
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

static void metadata_callback(void * priv, const gavl_metadata_t * m)
  {
  bg_encoder_update_metadata(priv, m);
  }

int main(int argc, char ** argv)
  {
  int do_delete;
  int ret = 1;
  int i, num;
  bg_mediaconnector_t conn;
  bg_encoder_t * enc;
  gavf_program_header_t * ph;
  gavf_t * g;
  const gavf_stream_header_t * sh;
  bg_mediaconnector_stream_t * cs;

  gavl_audio_sink_t * asink;
  gavl_video_sink_t * vsink;
  gavl_packet_sink_t * psink;
  
  bg_stream_action_t * audio_actions = NULL;
  bg_stream_action_t * video_actions = NULL;
  bg_stream_action_t * text_actions = NULL;
  bg_stream_action_t * overlay_actions = NULL;

  gavl_metadata_init(&ac_options);
  gavl_metadata_init(&vc_options);
  gavl_metadata_init(&oc_options);
  
  bg_mediaconnector_init(&conn);
  gavftools_init();
  
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  in_plug = gavftools_create_in_plug();
    
#if 1
  enc = bg_encoder_create(plugin_reg,
                          NULL, // bg_cfg_section_t * section,
                          NULL, // bg_transcoder_track_t * tt,
                          BG_STREAM_AUDIO |
                          BG_STREAM_VIDEO |
                          BG_STREAM_TEXT |
                          BG_STREAM_OVERLAY /* Stream types */,
                          BG_PLUGIN_FILE | BG_PLUGIN_BROADCAST  /* Flags */
                          );
#endif
  
  /* Open */

  g = bg_plug_get_gavf(in_plug);
  ph = gavf_get_program_header(g);

  gavf_options_set_metadata_callback(gavf_get_options(g), metadata_callback, enc);
  
  if(!bg_plug_open_location(in_plug, gavftools_in_file, NULL, NULL))
    return ret;
    
#if 0
  
  if(!bg_encoder_open(enc, const char * filename_base, ph->m, ph->cl))
    return 0;
  
#endif


  
  
  /* Set stream actions */
  
  num = gavf_get_num_streams(g, GAVF_STREAM_AUDIO);
  audio_actions = gavftools_get_stream_actions(num, GAVF_STREAM_AUDIO);
  
  for(i = 0; i < num; i++)
    {
    sh = gavf_get_stream(g, i, GAVF_STREAM_AUDIO);
    /* Check if we can write the stream compressed */
    if((audio_actions[i] = BG_STREAM_ACTION_READRAW) && 
        !bg_encoder_writes_compressed_audio(enc, &sh->format.audio, &sh->ci))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Audio stream %d cannot be written compressed", i+1);
      audio_actions[i] = BG_STREAM_ACTION_DECODE;
      } 
    bg_plug_set_stream_action(in_plug, sh, audio_actions[i]);
    }

  num = gavf_get_num_streams(g, GAVF_STREAM_VIDEO);
  video_actions = gavftools_get_stream_actions(num, GAVF_STREAM_VIDEO);
  
  for(i = 0; i < num; i++)
    {
    sh = gavf_get_stream(g, i, GAVF_STREAM_VIDEO);
    /* Check if we can write the stream compressed */
    if((video_actions[i] = BG_STREAM_ACTION_READRAW) &&
        !bg_encoder_writes_compressed_video(enc, &sh->format.video, &sh->ci))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Video stream %d cannot be written compressed", i+1);
      video_actions[i] = BG_STREAM_ACTION_DECODE;
      }      
    bg_plug_set_stream_action(in_plug, sh, video_actions[i]);
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
    /* Check if we can write the stream compressed */
    if((overlay_actions[i] = BG_STREAM_ACTION_READRAW) &&
        !bg_encoder_writes_compressed_overlay(enc, &sh->format.video, &sh->ci))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Overlay stream %d cannot be written compressed", i+1);
      overlay_actions[i] = BG_STREAM_ACTION_DECODE;
      }
    bg_plug_set_stream_action(in_plug, sh, overlay_actions[i]);
    }
  
  /* Start decoder and initialize media connector */

  if(!bg_plug_setup_reader(in_plug, &conn))
    return ret;
  
  /* Set up encoder */

  for(i = 0; i < conn.num_streams; i++)
    {
    cs = conn.streams[i];
    switch(cs->type)
      {
      case GAVF_STREAM_AUDIO:
        if(cs->asrc)
          cs->dst_index =
            bg_encoder_add_audio_stream(enc, &cs->m,
                                        gavl_audio_source_get_src_format(cs->asrc),
                                        cs->src_index, NULL);
        else
          cs->dst_index =
            bg_encoder_add_audio_stream_compressed(enc, &cs->m,
                                                   gavl_packet_source_get_audio_format(cs->psrc),
                                                   gavl_packet_source_get_ci(cs->psrc),
                                                   cs->src_index);
        break;
      case GAVF_STREAM_VIDEO:
        if(cs->vsrc)
          cs->dst_index =
            bg_encoder_add_video_stream(enc, &cs->m,
                                        gavl_video_source_get_src_format(cs->vsrc),
                                        cs->src_index, NULL);
        else
          cs->dst_index =
            bg_encoder_add_video_stream_compressed(enc, &cs->m,
                                                   gavl_packet_source_get_video_format(cs->psrc),
                                                   gavl_packet_source_get_ci(cs->psrc),
                                                   cs->src_index);
        break;
      case GAVF_STREAM_TEXT:
        cs->dst_index = bg_encoder_add_text_stream(enc, &cs->m,
                                                   cs->timescale, cs->src_index);
        break;
      case GAVF_STREAM_OVERLAY:
        if(cs->vsrc)
          bg_encoder_add_overlay_stream(enc,
                                        &cs->m,
                                        gavl_video_source_get_src_format(cs->vsrc),
                                        cs->src_index,
                                        BG_STREAM_OVERLAY,
                                        NULL);
        else
          bg_encoder_add_overlay_stream_compressed(enc,
                                                   &cs->m,
                                                   gavl_packet_source_get_video_format(cs->psrc),
                                                   gavl_packet_source_get_ci(cs->psrc),
                                                   cs->src_index);
        break;
      }
    }

  /* Start */
  if(!bg_encoder_start(enc))
    return ret;

  /* Connect sinks */

  for(i = 0; i < conn.num_streams; i++)
    {
    cs = conn.streams[i];

    asink = NULL;
    vsink = NULL;
    psink = NULL;
    
    switch(cs->type)
      {
      case GAVF_STREAM_AUDIO:
        if(cs->aconn)
          asink = bg_encoder_get_audio_sink(enc, cs->dst_index);
        else
          psink = bg_encoder_get_audio_packet_sink(enc, cs->dst_index);
        break;
      case GAVF_STREAM_VIDEO:
        if(cs->vconn)
          vsink = bg_encoder_get_video_sink(enc, cs->dst_index);
        else
          psink = bg_encoder_get_video_packet_sink(enc, cs->dst_index);
        break;
      case GAVF_STREAM_TEXT:
        psink = bg_encoder_get_text_sink(enc, cs->dst_index);
        break;
      case GAVF_STREAM_OVERLAY:
        if(cs->vconn)
          vsink = bg_encoder_get_overlay_sink(enc, cs->dst_index);
        else
          psink = bg_encoder_get_overlay_packet_sink(enc, cs->dst_index);
        break;
      }

    if(asink)
      gavl_audio_connector_connect(cs->aconn, asink);
    else if(vsink)
      gavl_video_connector_connect(cs->vconn, vsink);
    else if(psink)
      gavl_packet_connector_connect(cs->pconn, psink);
    }

  /* Fire up connector */

  bg_mediaconnector_start(&conn);
  
  /* Main loop */

  do_delete = 0;
  
  while(1)
    {
    if(bg_plug_got_error(in_plug))
      {
      do_delete = 1;
      break;
      }
    if(gavftools_stop() ||
       !bg_mediaconnector_iteration(&conn))
      break;
    }
  ret = 0;
  
  /* Cleanup */

  bg_plug_destroy(in_plug);
  bg_encoder_destroy(enc, do_delete);
  
  gavl_metadata_free(&ac_options);
  gavl_metadata_free(&vc_options);
  gavl_metadata_free(&oc_options);

  
  return 0;
  }
