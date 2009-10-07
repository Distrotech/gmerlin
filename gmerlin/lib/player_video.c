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
#include <stdio.h>

#include <gmerlin/player.h>
#include <playerprivate.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "player.video"

void bg_player_video_create(bg_player_t * p,
                            bg_plugin_registry_t * plugin_reg)
  {
  bg_player_video_stream_t * s = &p->video_stream;
  
  s->th = bg_player_thread_create(p->thread_common);
  
  bg_gavl_video_options_init(&s->options);

  s->fc = bg_video_filter_chain_create(&s->options,
                                       plugin_reg);
  
  pthread_mutex_init(&s->config_mutex,NULL);
  s->ss = &p->subtitle_stream;
  }

void bg_player_video_destroy(bg_player_t * p)
  {
  bg_player_video_stream_t * s = &p->video_stream;
  pthread_mutex_destroy(&s->config_mutex);
  bg_gavl_video_options_free(&s->options);
  bg_video_filter_chain_destroy(s->fc);
  bg_player_thread_destroy(s->th);
  }

int bg_player_video_init(bg_player_t * player, int video_stream)
  {
  bg_player_video_stream_t * s;
  s = &(player->video_stream);

  if(s->do_still)
    s->in_func = bg_player_input_read_video_still;
  else
    s->in_func = bg_player_input_read_video;

  s->in_data = player;
  s->in_stream = player->current_video_stream;
  
  if(!DO_VIDEO(player->flags))
    return 1;
  
  if(!DO_SUBTITLE_ONLY(player->flags))
    {
    bg_player_input_get_video_format(player);
    
    bg_video_filter_chain_connect_input(s->fc, s->in_func,
                                        s->in_data, s->in_stream);
    
    bg_video_filter_chain_init(s->fc, &s->input_format, &s->output_format);
    
    s->in_func = bg_video_filter_chain_read;
    s->in_data = s->fc;
    s->in_stream = 0;
    }
  
  if(!bg_player_ov_init(&player->video_stream))
    return 0;

  if(!DO_SUBTITLE_ONLY(player->flags))
    bg_video_filter_chain_set_out_format(s->fc, &(s->output_format));
  
  if(DO_SUBTITLE_ONLY(player->flags))
    {
    /* Video output already initialized */
    bg_player_ov_set_subtitle_format(&player->video_stream);

    bg_player_subtitle_init_converter(player);
    
    s->in_func = bg_player_input_read_video_subtitle_only;
    s->in_data = player;
    s->in_stream = 0;
    }
  
  return 1;
  }

void bg_player_video_cleanup(bg_player_t * player)
  {
  
  }

/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
#if 0
    {
      .name =      "video",
      .long_name = TRS("Video"),
      .type =      BG_PARAMETER_SECTION,
    },
#endif
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_ALPHA,
    BG_GAVL_PARAM_RESAMPLE_CHROMA,
    {
      .name = "still_framerate",
      .long_name = TRS("Still image framerate"),
      .type =      BG_PARAMETER_FLOAT,
      .val_min =     { .val_f =   1.0 },
      .val_max =     { .val_f = 100.0 },
      .val_default = { .val_f =  10.0 },
      .num_digits =  2,
      .help_string = TRS("Set framerate with which still images will be redisplayed periodically"),
    },
    BG_GAVL_PARAM_THREADS,
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_player_get_video_parameters(bg_player_t * p)
  {
  return parameters;
  }

void bg_player_set_video_parameter(void * data, const char * name,
                                   const bg_parameter_value_t * val)
  {
  bg_player_t * p = (bg_player_t*)data;
  int need_restart = 0;
  int is_interrupted;
  int do_init;
  int check_restart;
  
  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);
  
  pthread_mutex_lock(&(p->video_stream.config_mutex));

  is_interrupted = p->video_stream.interrupted;
  
  bg_gavl_video_set_parameter(&(p->video_stream.options),
                              name, val);

  if(!do_init && !is_interrupted)
    check_restart = 1;
  else
    check_restart = 0;
  
  if(check_restart)
    need_restart = p->video_stream.options.options_changed;
  
  pthread_mutex_unlock(&(p->video_stream.config_mutex));

  if(!need_restart && check_restart)
    {
    bg_video_filter_chain_lock(p->video_stream.fc);
    need_restart =
        bg_video_filter_chain_need_restart(p->video_stream.fc);
    bg_video_filter_chain_unlock(p->video_stream.fc);
    }

  if(need_restart)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Restarting playback due to changed video options");
    bg_player_interrupt(p);
    
    pthread_mutex_lock(&(p->video_stream.config_mutex));
    p->video_stream.interrupted = 1;
    pthread_mutex_unlock(&(p->video_stream.config_mutex));
    }
  
  if(!name && is_interrupted)
    {
    bg_player_interrupt_resume(p);
    pthread_mutex_lock(&(p->video_stream.config_mutex));
    p->video_stream.interrupted = 0;
    pthread_mutex_unlock(&(p->video_stream.config_mutex));
    }
  }



const bg_parameter_info_t *
bg_player_get_video_filter_parameters(bg_player_t * p)
  {
  return bg_video_filter_chain_get_parameters(p->video_stream.fc);
  }

void bg_player_set_video_filter_parameter(void * data, const char * name,
                                          const bg_parameter_value_t * val)
  {
  int need_rebuild = 0, need_restart = 0;
  int is_interrupted;
  int do_init;
  bg_player_t * p = (bg_player_t*)data;
  
  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);
  
  pthread_mutex_lock(&(p->video_stream.config_mutex));
  is_interrupted = p->video_stream.interrupted;
  pthread_mutex_unlock(&(p->video_stream.config_mutex));
  
  bg_video_filter_chain_lock(p->video_stream.fc);
  bg_video_filter_chain_set_parameter(p->video_stream.fc, name, val);
  
  need_rebuild =
    bg_video_filter_chain_need_rebuild(p->video_stream.fc);
  need_restart =
    bg_video_filter_chain_need_restart(p->video_stream.fc);
  
  bg_video_filter_chain_unlock(p->video_stream.fc);
  
  if(!do_init && (need_rebuild || need_restart) && !is_interrupted)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Restarting playback due to changed video filters");
    bg_player_interrupt(p);
    
    pthread_mutex_lock(&(p->video_stream.config_mutex));
    p->video_stream.interrupted = 1;
    pthread_mutex_unlock(&(p->video_stream.config_mutex));
    }
  if(!name && is_interrupted)
    {
    bg_player_interrupt_resume(p);
    pthread_mutex_lock(&(p->video_stream.config_mutex));
    p->video_stream.interrupted = 0;
    pthread_mutex_unlock(&(p->video_stream.config_mutex));
    }

  
  }

int
bg_player_read_video(bg_player_t * p, gavl_video_frame_t * frame,
                     int * state)
  {
  bg_player_video_stream_t * s = &p->video_stream;
  
  *state = bg_player_get_state(p);
  
  if(*state != BG_PLAYER_STATE_PLAYING)
    return 0;
  
  return s->in_func(s->in_data, frame, s->in_stream);
  }
