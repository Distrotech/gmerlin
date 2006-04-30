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
  p->audio_stream.cnv_in  = gavl_audio_converter_create();
  p->audio_stream.cnv_out = gavl_audio_converter_create();
  bg_gavl_audio_options_init(&(p->audio_stream.options));

  p->audio_stream.volume = gavl_volume_control_create();
  pthread_mutex_init(&(p->audio_stream.volume_mutex),(pthread_mutexattr_t *)0);

  
  pthread_mutex_init(&(p->audio_stream.config_mutex),(pthread_mutexattr_t *)0);
  }

void bg_player_audio_destroy(bg_player_t * p)
  {
  gavl_audio_converter_destroy(p->audio_stream.cnv_in);
  gavl_audio_converter_destroy(p->audio_stream.cnv_out);
  bg_gavl_audio_options_free(&(p->audio_stream.options));

  gavl_volume_control_destroy(p->audio_stream.volume);
  pthread_mutex_destroy(&(p->audio_stream.volume_mutex));

  }

int bg_player_audio_init(bg_player_t * player, int audio_stream)
  {
  int force_float;
  gavl_audio_options_t * opt;
  if(player->track_info->num_audio_streams)
    player->do_audio =
      bg_player_input_set_audio_stream(player->input_context, audio_stream);
  
  if(!player->do_audio)
    return 1;
  
  pthread_mutex_lock(&(player->audio_stream.config_mutex));

  /* Set custom format */
  bg_gavl_audio_options_set_format(&(player->audio_stream.options),
                                   &(player->audio_stream.input_format),
                                   &(player->audio_stream.output_format));
  force_float = player->audio_stream.options.force_float;
  
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

  gavl_audio_format_copy(&(player->audio_stream.pipe_format),
                         &(player->audio_stream.output_format));
  if(force_float)
    player->audio_stream.pipe_format.sample_format = GAVL_SAMPLE_FLOAT;
  
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
  /* Volume control */
  gavl_volume_control_set_format(player->audio_stream.volume,
                                 &(player->audio_stream.pipe_format));
  
  /* Initialize audio converter */

#if 0
  fprintf(stderr, "======= Output format: ==========\n");
  gavl_audio_format_dump(&(player->audio_stream.output_format));
  fprintf(stderr, "=================================\n");
#endif

  /* Input conversion */
  opt = gavl_audio_converter_get_options(player->audio_stream.cnv_in);
  gavl_audio_options_copy(opt, player->audio_stream.options.opt);
  
  if(!gavl_audio_converter_init(player->audio_stream.cnv_in,
                                &(player->audio_stream.input_format),
                                &(player->audio_stream.pipe_format)))
    {
    player->audio_stream.do_convert_in = 0;
    //    fprintf(stderr, "**** No Conversion ****\n");
    }
  else
    {
    player->audio_stream.do_convert_in = 1;
    player->audio_stream.frame_in =
      gavl_audio_frame_create(&(player->audio_stream.input_format));
    //    fprintf(stderr, "**** Doing Input conversion\n");
    }

  /* Output conversion */
  opt = gavl_audio_converter_get_options(player->audio_stream.cnv_out);
  gavl_audio_options_copy(opt, player->audio_stream.options.opt);
  
  if(!gavl_audio_converter_init(player->audio_stream.cnv_out,
                                &(player->audio_stream.pipe_format),
                                &(player->audio_stream.output_format)))
    {
    player->audio_stream.do_convert_out = 0;
    //    fprintf(stderr, "**** No Conversion ****\n");
    }
  else
    {
    player->audio_stream.do_convert_out = 1;
    player->audio_stream.frame_out =
      gavl_audio_frame_create(&(player->audio_stream.output_format));
    //    fprintf(stderr, "**** Doing Output conversion\n");
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
  if(player->audio_stream.frame_in)
    {
    gavl_audio_frame_destroy(player->audio_stream.frame_in);
    player->audio_stream.frame_in = (gavl_audio_frame_t*)0;
    }
  if(player->audio_stream.frame_out)
    {
    gavl_audio_frame_destroy(player->audio_stream.frame_out);
    player->audio_stream.frame_out = (gavl_audio_frame_t*)0;
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
    BG_GAVL_PARAM_FORCE_FLOAT,
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_SAMPLERATE,
    BG_GAVL_PARAM_CHANNEL_SETUP,
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

  bg_gavl_audio_set_parameter(&(player->audio_stream.options),
                              name, val);
  
  pthread_mutex_unlock(&(player->audio_stream.config_mutex));
  }
