/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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
#include <stdlib.h>
#include <stdio.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "player.audio"

#include <gmerlin/player.h>
#include <playerprivate.h>

void bg_player_audio_create(bg_player_t * p, bg_plugin_registry_t * plugin_reg)
  {
  bg_player_audio_stream_t * s = &p->audio_stream;
  
  bg_gavl_audio_options_init(&(s->options));

  s->th = bg_player_thread_create(p->thread_common);
  
  s->fc =
    bg_audio_filter_chain_create(&s->options,
                                 plugin_reg);
  
  s->cnv_out = gavl_audio_converter_create();

  s->volume = gavl_volume_control_create();
  s->peak_detector = gavl_peak_detector_create();
  
  pthread_mutex_init(&(s->volume_mutex),NULL);
  pthread_mutex_init(&(s->config_mutex),NULL);
  pthread_mutex_init(&(s->time_mutex),NULL);
  pthread_mutex_init(&(s->mute_mutex), NULL);
  pthread_mutex_init(&(s->eof_mutex),NULL);
  
  s->timer = gavl_timer_create();
  }

void bg_player_audio_destroy(bg_player_t * p)
  {
  bg_player_audio_stream_t * s = &p->audio_stream;
  gavl_audio_converter_destroy(s->cnv_out);
  bg_gavl_audio_options_free(&(s->options));
  bg_audio_filter_chain_destroy(s->fc);
  
  gavl_volume_control_destroy(s->volume);
  gavl_peak_detector_destroy(s->peak_detector);
  pthread_mutex_destroy(&(s->volume_mutex));
  pthread_mutex_destroy(&(s->eof_mutex));

  pthread_mutex_destroy(&(s->time_mutex));
  gavl_timer_destroy(s->timer);
  
  if(s->plugin_handle)
    bg_plugin_unref(s->plugin_handle);
  
  bg_player_thread_destroy(s->th); 
  
  }

int bg_player_audio_init(bg_player_t * player, int audio_stream)
  {
  gavl_sample_format_t force_format;
  gavl_audio_options_t * opt;
  bg_player_audio_stream_t * s;
  //  int do_filter;

  if(!DO_AUDIO(player->flags))
    return 1;
  
  s = &player->audio_stream;
  s->send_silence = 0;
  
  s->options.options_changed = 0;
  
  s->in_func   = bg_player_input_read_audio;
  s->in_data   = player;
  s->in_stream = player->current_audio_stream;
  
  bg_player_input_get_audio_format(player);

  bg_audio_filter_chain_connect_input(s->fc,
                                      s->in_func,
                                      s->in_data,
                                      s->in_stream);
  s->in_func = bg_audio_filter_chain_read;
  s->in_data = s->fc;
  s->in_stream = 0;
  
  pthread_mutex_lock(&(s->config_mutex));
  force_format = s->options.force_format;
  bg_audio_filter_chain_init(s->fc, &(s->input_format), &(s->fifo_format));
  pthread_mutex_unlock(&(s->config_mutex));
  
  
  gavl_audio_format_copy(&(s->output_format),
                           &(s->fifo_format));

  if(!bg_player_oa_init(s))
    return 0;

  gavl_audio_format_copy(&(s->fifo_format),
                         &(s->output_format));
  
  if(force_format != GAVL_SAMPLE_NONE)
    s->fifo_format.sample_format = force_format;

  bg_audio_filter_chain_set_out_format(s->fc, &(s->fifo_format));
  
  /* Volume control */
  gavl_volume_control_set_format(s->volume,
                                 &(s->fifo_format));
  gavl_peak_detector_set_format(s->peak_detector,
                                &(s->fifo_format));

  /* Output conversion */
  opt = gavl_audio_converter_get_options(s->cnv_out);
  gavl_audio_options_copy(opt, s->options.opt);

  s->fifo_frame = gavl_audio_frame_create(&(s->output_format));
  
  if(!gavl_audio_converter_init(s->cnv_out,
                                &(s->fifo_format),
                                &(s->output_format)))
    {
    s->do_convert_out = 0;
    s->output_frame = s->fifo_frame;
    }
  else
    {
    s->do_convert_out = 1;
    s->output_frame =
      gavl_audio_frame_create(&(s->output_format));
    }
  return 1;
  }

void bg_player_audio_cleanup(bg_player_t * player)
  {
  bg_player_audio_stream_t * s;
  s = &player->audio_stream;
  
  if(s->fifo_frame)
    {
    gavl_audio_frame_destroy(s->fifo_frame);
    s->fifo_frame = (gavl_audio_frame_t*)0;
    }
  if(s->output_frame && s->do_convert_out)
    {
    gavl_audio_frame_destroy(s->output_frame);
    }
  s->output_frame = (gavl_audio_frame_t*)0;
  }

/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
#if 0
    {
      .name =      "audio",
      .long_name = TRS("Audio"),
      .type =      BG_PARAMETER_SECTION,
    },
#endif
    BG_GAVL_PARAM_FORCE_SAMPLEFORMAT,
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_AUDIO_DITHER_MODE,
    BG_GAVL_PARAM_SAMPLERATE,
    BG_GAVL_PARAM_RESAMPLE_MODE,
    BG_GAVL_PARAM_CHANNEL_SETUP,
    { /* End of parameters */ }
  };



const bg_parameter_info_t * bg_player_get_audio_parameters(bg_player_t * p)
  {
  return parameters;
  }

const bg_parameter_info_t * bg_player_get_audio_filter_parameters(bg_player_t * p)
  {
  return bg_audio_filter_chain_get_parameters(p->audio_stream.fc);
  }


void bg_player_set_audio_parameter(void * data, const char * name,
                                   const bg_parameter_value_t * val)
  {
  bg_player_t * p = (bg_player_t*)data;
  int need_restart = 0;
  int is_interrupted;
  int do_init;
  int check_restart;
  
  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);
  
  pthread_mutex_lock(&(p->audio_stream.config_mutex));

  is_interrupted = p->audio_stream.interrupted;
  
  bg_gavl_audio_set_parameter(&(p->audio_stream.options),
                              name, val);

  if(!do_init && !is_interrupted)
    check_restart = 1;
  else
    check_restart = 0;
  
  if(check_restart)
    need_restart = p->audio_stream.options.options_changed;
  
  pthread_mutex_unlock(&(p->audio_stream.config_mutex));

  if(!need_restart && check_restart)
    {
    bg_audio_filter_chain_lock(p->audio_stream.fc);
    need_restart =
        bg_audio_filter_chain_need_restart(p->audio_stream.fc);
    bg_audio_filter_chain_unlock(p->audio_stream.fc);
    }

  if(need_restart)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Restarting playback due to changed audio options");
    bg_player_interrupt(p);
    
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

void bg_player_set_audio_filter_parameter(void * data, const char * name,
                                          const bg_parameter_value_t * val)
  {
  int need_rebuild = 0, need_restart = 0;
  int is_interrupted;
  int do_init;
  bg_player_t * p = (bg_player_t*)data;

  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);

  pthread_mutex_lock(&(p->audio_stream.config_mutex));
  is_interrupted = p->audio_stream.interrupted;
  pthread_mutex_unlock(&(p->audio_stream.config_mutex));
    
  bg_audio_filter_chain_lock(p->audio_stream.fc);
  bg_audio_filter_chain_set_parameter(p->audio_stream.fc, name, val);

  need_rebuild =
    bg_audio_filter_chain_need_rebuild(p->audio_stream.fc);
  need_restart =
    bg_audio_filter_chain_need_restart(p->audio_stream.fc);
  
  bg_audio_filter_chain_unlock(p->audio_stream.fc);
  
  if(!do_init && (need_rebuild || need_restart) && !is_interrupted)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Restarting playback due to changed audio filters");
    bg_player_interrupt(p);
    
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

int
bg_player_read_audio(bg_player_t * p, gavl_audio_frame_t * frame)
  {
  bg_player_audio_stream_t * s = &p->audio_stream;
  return s->in_func(s->in_data, frame, s->in_stream,
                    s->fifo_format.samples_per_frame);
  }

int bg_player_audio_set_eof(bg_player_t * p)
  {
  bg_msg_t * msg;
  int ret = 1;

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Detected EOF");
  pthread_mutex_lock(&p->video_stream.eof_mutex);
  pthread_mutex_lock(&p->audio_stream.eof_mutex);

  p->audio_stream.eof = 1;
  
  if(p->video_stream.eof)
    {
    msg = bg_msg_queue_lock_write(p->command_queue);
    bg_msg_set_id(msg, BG_PLAYER_CMD_SETSTATE);
    bg_msg_set_arg_int(msg, 0, BG_PLAYER_STATE_EOF);
    bg_msg_queue_unlock_write(p->command_queue);
    }
  else
    {
    ret = 0;
    p->audio_stream.send_silence = 1;
    }
  pthread_mutex_unlock(&p->audio_stream.eof_mutex);
  pthread_mutex_unlock(&p->video_stream.eof_mutex);
  return ret;
  }
