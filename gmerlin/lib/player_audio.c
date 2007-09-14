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

#include <log.h>
#define LOG_DOMAIN "player.audio"

#include "player.h"
#include "playerprivate.h"

void bg_player_audio_create(bg_player_t * p, bg_plugin_registry_t * plugin_reg)
  {
  bg_gavl_audio_options_init(&(p->audio_stream.options));

  p->audio_stream.fc =
    bg_audio_filter_chain_create(&p->audio_stream.options,
                                 plugin_reg);
  
  p->audio_stream.cnv_out = gavl_audio_converter_create();

  p->audio_stream.volume = gavl_volume_control_create();
  pthread_mutex_init(&(p->audio_stream.volume_mutex),(pthread_mutexattr_t *)0);

  
  pthread_mutex_init(&(p->audio_stream.config_mutex),(pthread_mutexattr_t *)0);
  }

void bg_player_audio_destroy(bg_player_t * p)
  {
  gavl_audio_converter_destroy(p->audio_stream.cnv_out);
  bg_gavl_audio_options_free(&(p->audio_stream.options));

  gavl_volume_control_destroy(p->audio_stream.volume);
  pthread_mutex_destroy(&(p->audio_stream.volume_mutex));

  }

int bg_player_audio_init(bg_player_t * player, int audio_stream)
  {
  int force_float;
  gavl_audio_options_t * opt;
  bg_player_audio_stream_t * s;
  //  int do_filter;

  if(!DO_AUDIO(player))
    return 1;
  
  s = &player->audio_stream;
  
  s->in_func   = bg_player_input_read_audio;
  s->in_data   = player->input_context;
  s->in_stream = player->current_audio_stream;
  
  bg_player_input_get_audio_format(player->input_context);

  bg_audio_filter_chain_connect_input(s->fc,
                                      s->in_func,
                                      s->in_data,
                                      s->in_stream);
  s->in_func = bg_audio_filter_chain_read;
  s->in_data = s->fc;
  s->in_stream = 0;
  
  pthread_mutex_lock(&(s->config_mutex));
  force_float = s->options.force_float;
  bg_audio_filter_chain_init(s->fc, &(s->input_format), &(s->fifo_format));
  pthread_mutex_unlock(&(s->config_mutex));
  
  
  gavl_audio_format_copy(&(s->output_format),
                           &(s->fifo_format));

  if(!bg_player_oa_init(player->oa_context))
    return 0;

  gavl_audio_format_copy(&(s->fifo_format),
                         &(s->output_format));
  
  if(force_float)
    s->fifo_format.sample_format = GAVL_SAMPLE_FLOAT;

  bg_audio_filter_chain_set_out_format(s->fc, &(s->fifo_format));
  
  /* Initialize audio fifo */

  s->fifo =
    bg_fifo_create(NUM_AUDIO_FRAMES, bg_player_oa_create_frame,
                   (void*)player->oa_context);
  /* Volume control */
  gavl_volume_control_set_format(s->volume,
                                 &(s->fifo_format));

  /* Output conversion */
  opt = gavl_audio_converter_get_options(s->cnv_out);
  gavl_audio_options_copy(opt, s->options.opt);
  
  if(!gavl_audio_converter_init(s->cnv_out,
                                &(s->fifo_format),
                                &(s->output_format)))
    {
    s->do_convert_out = 0;
    }
  else
    {
    s->do_convert_out = 1;
    s->frame_out =
      gavl_audio_frame_create(&(s->output_format));
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
  if(player->audio_stream.frame_out)
    {
    gavl_audio_frame_destroy(player->audio_stream.frame_out);
    player->audio_stream.frame_out = (gavl_audio_frame_t*)0;
    }
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
#if 0
    {
      name:      "audio",
      long_name: TRS("Audio"),
      type:      BG_PARAMETER_SECTION,
    },
#endif
    BG_GAVL_PARAM_FORCE_FLOAT,
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_AUDIO_DITHER_MODE,
    BG_GAVL_PARAM_SAMPLERATE,
    BG_GAVL_PARAM_RESAMPLE_MODE,
    BG_GAVL_PARAM_CHANNEL_SETUP,
    { /* End of parameters */ }
  };



bg_parameter_info_t * bg_player_get_audio_parameters(bg_player_t * p)
  {
  return parameters;
  }

bg_parameter_info_t * bg_player_get_audio_filter_parameters(bg_player_t * p)
  {
  return bg_audio_filter_chain_get_parameters(p->audio_stream.fc);
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

void bg_player_set_audio_filter_parameter(void * data, char * name,
                                          bg_parameter_value_t * val)
  {
  int need_rebuild;
  int is_interrupted;
  bg_player_t * p = (bg_player_t*)data;

  pthread_mutex_lock(&(p->audio_stream.config_mutex));
  is_interrupted = p->audio_stream.interrupted;
  pthread_mutex_unlock(&(p->audio_stream.config_mutex));
  
  bg_audio_filter_chain_lock(p->audio_stream.fc);
  bg_audio_filter_chain_set_parameter(p->audio_stream.fc, name, val);
  need_rebuild =
    bg_audio_filter_chain_need_rebuild(p->audio_stream.fc);
  bg_audio_filter_chain_unlock(p->audio_stream.fc);

  if(bg_player_get_state(p) == BG_PLAYER_STATE_INIT)
    need_rebuild = 0;
  
  if(need_rebuild && !is_interrupted)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Restarting playback due to changed audio filters");
    bg_player_interrupt(p);
    bg_audio_filter_chain_rebuild(p->audio_stream.fc);
    
    pthread_mutex_lock(&(p->audio_stream.config_mutex));
    p->audio_stream.interrupted = 1;
    pthread_mutex_unlock(&(p->audio_stream.config_mutex));
    }
  
  if(!name && is_interrupted)
    {
    bg_player_interrupt_resume(p);
    pthread_mutex_lock(&(p->audio_stream.config_mutex));
    p->audio_stream.interrupted = 0;
    pthread_mutex_unlock(&(p->audio_stream.config_mutex));
    }

  
  }
