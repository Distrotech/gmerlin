/*****************************************************************
 
  player_audio.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "player.h"
#include "playerprivate.h"

void bg_player_audio_create(bg_player_t * p)
  {
  p->audio_stream.cnv = gavl_audio_converter_create();
  gavl_audio_default_options(&(p->audio_stream.opt));

  pthread_mutex_init(&(p->audio_stream.config_mutex),(pthread_mutexattr_t *)0);
  }

void bg_player_audio_destroy(bg_player_t * p)
  {
  gavl_audio_converter_destroy(p->audio_stream.cnv);
  }

int bg_player_audio_init(bg_player_t * player, int audio_stream)
  {
  player->do_audio = 0;
  
  if(player->track_info->num_audio_streams)
    player->do_audio =
      bg_player_input_set_audio_stream(player->input_context, audio_stream);
  
  if(!player->do_audio)
    return 1;

  /* Set custom format */

  gavl_audio_format_copy(&(player->audio_stream.output_format),
                         &(player->audio_stream.input_format));
  /* Sample per frame is the same in input and output */
  
  pthread_mutex_lock(&(player->audio_stream.config_mutex));

  if(player->audio_stream.fixed_channel_setup)
    {
    //    fprintf(stderr, "Fixed channel setup: %s\n",
    //            gavl_channel_setup_to_string(player->audio_stream.channel_setup));

    player->audio_stream.output_format.channel_setup =
      player->audio_stream.channel_setup;
    player->audio_stream.output_format.num_channels =
      gavl_num_channels(player->audio_stream.channel_setup);
    player->audio_stream.output_format.lfe = 0;

    /* Let gavl find the channel indices also */
    player->audio_stream.output_format.channel_locations[0] = GAVL_CHID_NONE;
    gavl_set_channel_setup(&(player->audio_stream.output_format));
    
    }
  //  else
  //    fprintf(stderr, "NO Fixed channel setup\n");

  if(player->audio_stream.fixed_samplerate)
    {
    player->audio_stream.output_format.samplerate =
      player->audio_stream.samplerate; 
    }

  pthread_mutex_unlock(&(player->audio_stream.config_mutex));
  
#if 0
  fprintf(stderr, "======= Input format: ===========\n");
  gavl_audio_format_dump(&(player->audio_stream.input_format));
#endif
  
  /* Set up formats */
  
  if(!bg_player_oa_init(player->oa_context))
    {
    player->audio_stream.error_msg = bg_player_oa_get_error(player->oa_context);
    return 0;
    }

  if(player->audio_stream.input_format.samplerate !=
     player->audio_stream.output_format.samplerate)
    {
    player->audio_stream.input_format.samples_per_frame =
      ((player->audio_stream.output_format.samples_per_frame - 10) *
       player->audio_stream.input_format.samplerate) /
      player->audio_stream.output_format.samplerate;
    }
  else
    {
    player->audio_stream.input_format.samples_per_frame =
      player->audio_stream.output_format.samples_per_frame;
    }
  
  /* Initialize audio fifo */

  player->audio_stream.fifo =
    bg_fifo_create(NUM_AUDIO_FRAMES, bg_player_oa_create_frame,
                   (void*)player->oa_context);
  
  /* Initialize audio converter */

#if 0
  fprintf(stderr, "======= Output format: ==========\n");
  gavl_audio_format_dump(&(player->audio_stream.output_format));
  fprintf(stderr, "=================================\n");
#endif
  
  if(!gavl_audio_converter_init(player->audio_stream.cnv, &(player->audio_stream.opt),
                                &(player->audio_stream.input_format),
                                &(player->audio_stream.output_format)))
    {
    player->audio_stream.do_convert = 0;
    //    fprintf(stderr, "**** No Conversion ****\n");
    }
  else
    {
    player->audio_stream.do_convert = 1;
    player->audio_stream.frame =
      gavl_audio_frame_create(&(player->audio_stream.input_format));
    //    fprintf(stderr, "**** Doing Conversion %d ****\n",
    //            player->audio_stream.input_format.samples_per_frame);
    }
  
  return 1;
  }

void bg_player_audio_cleanup(bg_player_t * player)
  {
  if(player->audio_stream.fifo)
    {
    bg_fifo_destroy(player->audio_stream.fifo,
                    bg_player_oa_destroy_frame, NULL);
    player->audio_stream.fifo = (bg_fifo_t *)0;
    }
  if(player->audio_stream.frame)
    {
    gavl_audio_frame_destroy(player->audio_stream.frame);
    player->audio_stream.frame = (gavl_audio_frame_t*)0;
    }
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:      "audio",
      long_name: "Audio",
      type:      BG_PARAMETER_SECTION,
    },
    {
      name:        "conversion_quality",
      long_name:   "Conversion Quality",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: GAVL_QUALITY_FASTEST },
      val_max:     { val_i: GAVL_QUALITY_BEST    },
      val_default: { val_i: GAVL_QUALITY_DEFAULT }
    },
    {
      name:      "fixed_samplerate",
      long_name: "Fixed samplerate",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:        "samplerate",
      long_name:   "Samplerate",
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 8000 },
      val_max:     { val_i: 192000 },
      val_default: { val_i: 44100 },
    },
    {
      name:      "fixed_channel_setup",
      long_name: "Fixed channel setup",
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:        "channel_setup",
      long_name:   "Channel setup",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Stereo" },
      multi_names: (char*[]){ "Mono",
                              "Stereo",
                              "3 Front",
                              "2 Front 1 Rear",
                              "3 Front 1 Rear",
                              "2 Front 2 Rear",
                              "3 Front 2 Rear",
                              (char*)0 },
    },
    {
      name:        "front_to_rear",
      long_name:   "Front to rear mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Copy" },
      multi_names: (char*[]){ "Mute",
                              "Copy",
                              "Diff",
                              (char*)0 },
    },
    {
      name:        "stereo_to_mono",
      long_name:   "Stereo to mono mode",
      type:        BG_PARAMETER_STRINGLIST,
      val_default: { val_str: "Mix" },
      multi_names: (char*[]){ "Choose left",
                              "Choose right",
                              "Mix",
                              (char*)0 },
    },
    { /* End of parameters */ }
  };



bg_parameter_info_t * bg_player_get_audio_parameters(bg_player_t * p)
  {
  return parameters;
  }

void bg_player_set_audio_parameter(void * data, char * name,
                                   bg_parameter_value_t * val)
  {
  bg_player_t * player = (bg_player_t*)data;

  
  if(!name)
    return;

  pthread_mutex_lock(&(player->audio_stream.config_mutex));

  if(!strcmp(name, "conversion_quality"))
    {
    player->audio_stream.opt.quality = val->val_i;
    }
  else if(!strcmp(name, "fixed_channel_setup"))
    {
    player->audio_stream.fixed_channel_setup = val->val_i;
    }
  else if(!strcmp(name, "channel_setup"))
    {
    if(!strcmp(val->val_str, "Mono"))
      player->audio_stream.channel_setup = GAVL_CHANNEL_MONO;
    else if(!strcmp(val->val_str, "Stereo"))
      player->audio_stream.channel_setup = GAVL_CHANNEL_STEREO;
    else if(!strcmp(val->val_str, "3 Front"))
      player->audio_stream.channel_setup = GAVL_CHANNEL_3F;
    else if(!strcmp(val->val_str, "2 Front 1 Rear"))
      player->audio_stream.channel_setup = GAVL_CHANNEL_2F1R;
    else if(!strcmp(val->val_str, "3 Front 1 Rear"))
      player->audio_stream.channel_setup = GAVL_CHANNEL_3F1R;
    else if(!strcmp(val->val_str, "2 Front 2 Rear"))
      player->audio_stream.channel_setup = GAVL_CHANNEL_2F2R;
    else if(!strcmp(val->val_str, "3 Front 2 Rear"))
      player->audio_stream.channel_setup = GAVL_CHANNEL_3F2R;
    }
  else if(!strcmp(name, "fixed_samplerate"))
    {
    player->audio_stream.fixed_samplerate = val->val_i;
    }
  else if(!strcmp(name, "samplerate"))
    {
    player->audio_stream.samplerate = val->val_i;
    }
  else if(!strcmp(name, "front_to_rear"))
    {
    player->audio_stream.opt.conversion_flags &= ~GAVL_AUDIO_FRONT_TO_REAR_MASK;
    
    if(!strcmp(val->val_str, "Copy"))
      {
      player->audio_stream.opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_COPY;
      }
    else if(!strcmp(val->val_str, "Mute"))
      {
      player->audio_stream.opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_MUTE;
      }
    else if(!strcmp(val->val_str, "Diff"))
      {
      player->audio_stream.opt.conversion_flags |= GAVL_AUDIO_FRONT_TO_REAR_DIFF;
      }
    }
  else if(!strcmp(name, "stereo_to_mono"))
    {
    player->audio_stream.opt.conversion_flags &= ~GAVL_AUDIO_STEREO_TO_MONO_MASK;

    if(!strcmp(val->val_str, "Choose left"))
      {
      player->audio_stream.opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_LEFT;
      }
    else if(!strcmp(val->val_str, "Choose right"))
      {
      player->audio_stream.opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_RIGHT;
      }
    else if(!strcmp(val->val_str, "Mix"))
      {
      player->audio_stream.opt.conversion_flags |= GAVL_AUDIO_STEREO_TO_MONO_MIX;
      }
    }
          
  pthread_mutex_unlock(&(player->audio_stream.config_mutex));
  }
