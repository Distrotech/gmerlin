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


static bg_cmdline_arg_t global_options[] =
  {
    GAVFTOOLS_INPLUG_OPTIONS,
    GAVFTOOLS_AUDIO_STREAM_OPTIONS,
    GAVFTOOLS_VIDEO_STREAM_OPTIONS,
    GAVFTOOLS_TEXT_STREAM_OPTIONS,
    GAVFTOOLS_OVERLAY_STREAM_OPTIONS,
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
  int ret = 1;
  int i, num;
  bg_mediaconnector_t conn;
  bg_encoder_t * enc;
  gavf_program_header_t * ph;
  gavf_t * g;
  const gavf_stream_header_t * sh;

  bg_stream_action_t * audio_actions = NULL;
  bg_stream_action_t * video_actions = NULL;
  bg_stream_action_t * text_actions = NULL;
  bg_stream_action_t * overlay_actions = NULL;
  
  bg_mediaconnector_init(&conn);
  gavftools_init();
  
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  in_plug = gavftools_create_in_plug();
    
#if 0
  enc = bg_encoder_create(bg_plugin_registry_t * plugin_reg,
                          bg_cfg_section_t * section,
                          bg_transcoder_track_t * tt,
                          int stream_mask, int flag_mask);
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
    switch(conn.streams[i]->type)
      {
      case GAVF_STREAM_AUDIO:
        switch(audio_actions[i])
          {
          case BG_STREAM_ACTION_DECODE:
            break;
          case BG_STREAM_ACTION_READRAW:
            break;
          case BG_STREAM_ACTION_OFF:
            break;
          }
        break;
      case GAVF_STREAM_VIDEO:
        switch(video_actions[i])
          {
          case BG_STREAM_ACTION_DECODE:
            break;
          case BG_STREAM_ACTION_READRAW:
            break;
          case BG_STREAM_ACTION_OFF:
            break;
          }
        break;
      case GAVF_STREAM_TEXT:
        switch(text_actions[i])
          {
          case BG_STREAM_ACTION_DECODE:
            break;
          case BG_STREAM_ACTION_READRAW:
            break;
          case BG_STREAM_ACTION_OFF:
            break;
          }
        break;
      case GAVF_STREAM_OVERLAY:
        switch(overlay_actions[i])
          {
          case BG_STREAM_ACTION_DECODE:
            break;
          case BG_STREAM_ACTION_READRAW:
            break;
          case BG_STREAM_ACTION_OFF:
            break;
          }
        break;
      }
    }
    
  /* Main loop */

  /* Cleanup */

  return 0;
  }
