/*****************************************************************
 * Gmerlin - a general purpose multimedia framework and applications
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include <gmerlin/player.h>
#include <playerprivate.h>

#define LOG_DOMAIN "player"
#include <gmerlin/log.h>

/* Input callbacks */


static void duration_changed(void * data, gavl_time_t duration)
  {
  bg_player_t * p = data;
  bg_player_set_duration(p, duration, p->can_seek);
  }

static void metadata_changed(void * data, const gavl_metadata_t * m)
  {
  bg_player_t * p = data;
  bg_player_set_metadata(p, m);
  }

static void buffer_notify(void * data, float percentage)
  {
  bg_player_t * p = data;
  bg_player_set_state(p, BG_PLAYER_STATE_BUFFERING,
                      &percentage, NULL);
  }

static void aspect_changed(void * data, int stream, int pixel_width,
                           int pixel_height)
  {
  bg_player_t * p = data;
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Aspect ratio changed");

  bg_player_ov_update_aspect(&p->video_stream,
                             pixel_width, pixel_height);
  }


bg_player_t * bg_player_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_player_t * ret;
    
  ret = calloc(1, sizeof(*ret));

  /* Callbacks */
  ret->input_callbacks.data = ret;
  ret->input_callbacks.duration_changed = duration_changed;
  ret->input_callbacks.metadata_changed = metadata_changed;
  ret->input_callbacks.buffer_notify    = buffer_notify;
  ret->input_callbacks.aspect_changed   = aspect_changed;
  
  /* Create message queues */

  ret->command_queue  = bg_msg_queue_create();
  
  ret->message_queues = bg_msg_queue_list_create();
  
  ret->visualizer = bg_visualizer_create(plugin_reg);

  ret->thread_common = bg_thread_common_create();
  
  /* Create contexts */

  bg_player_audio_create(ret, plugin_reg);
  bg_player_video_create(ret, plugin_reg);
  bg_player_subtitle_create(ret);
  
  bg_player_input_create(ret);
  bg_player_ov_create(ret);

  ret->threads[0] = ret->audio_stream.th;
  ret->threads[1] = ret->video_stream.th;
  
  pthread_mutex_init(&ret->state_mutex, NULL);
  pthread_mutex_init(&ret->config_mutex, NULL);

  
  

  /* Subtitles are off by default */
  ret->current_subtitle_stream = -1;
  //  ret->current_subtitle_stream = 5;
  ret->state = BG_PLAYER_STATE_INIT;

  ret->wait_time = 10000;
  
  return ret;
  }

void bg_player_destroy(bg_player_t * player)
  {
    
  bg_player_input_destroy(player);
  bg_player_ov_destroy(player);
  bg_player_audio_destroy(player);
  bg_player_video_destroy(player);
  bg_player_subtitle_destroy(player);

  bg_visualizer_destroy(player->visualizer);
  
  bg_msg_queue_destroy(player->command_queue);
 
  bg_msg_queue_list_destroy(player->message_queues);
  
  pthread_mutex_destroy(&player->state_mutex);
  pthread_mutex_destroy(&player->config_mutex);

  bg_thread_common_destroy(player->thread_common);
  
  free(player);
  }

void bg_player_add_message_queue(bg_player_t * player,
                                 bg_msg_queue_t * message_queue)
  {
  bg_msg_queue_list_add(player->message_queues, message_queue);
  }

void bg_player_delete_message_queue(bg_player_t * player,
                                    bg_msg_queue_t * message_queue)
  {
  bg_msg_queue_list_remove(player->message_queues, message_queue);
  }


int  bg_player_get_state(bg_player_t * player)
  {
  int ret;
  pthread_mutex_lock(&player->state_mutex);
  ret = player->state;
  pthread_mutex_unlock(&player->state_mutex);
  return ret;
  }

struct state_struct
  {
  int state;
  float percentage;
  int want_new;
  int can_pause;
  };

static void msg_state(bg_msg_t * msg,
                      const void * data)
  {
  struct state_struct * s;
  s = (struct state_struct *)data;
  

  bg_msg_set_id(msg, BG_PLAYER_MSG_STATE_CHANGED);
  bg_msg_set_arg_int(msg, 0, s->state);

  if(s->state == BG_PLAYER_STATE_BUFFERING)
    {
    bg_msg_set_arg_float(msg, 1, s->percentage);
    }
  else if(s->state == BG_PLAYER_STATE_CHANGING)
    {
    bg_msg_set_arg_int(msg, 1, s->want_new);
    }
  else if(s->state == BG_PLAYER_STATE_PLAYING)
    {
    bg_msg_set_arg_int(msg, 1, s->can_pause);
    }
  }

void bg_player_set_state(bg_player_t * player, int state,
                         const void * arg1, const void * arg2)
  {
  struct state_struct s;
  pthread_mutex_lock(&player->state_mutex);
  player->state = state;
  pthread_mutex_unlock(&player->state_mutex);

  /* Broadcast this message */
 
  //  memset(&state, 0, sizeof(state));
    
  s.state = state;

  if(state == BG_PLAYER_STATE_BUFFERING)
    s.percentage = *((const float*)arg1);
  else if(state == BG_PLAYER_STATE_CHANGING)
    s.want_new = *((const int*)arg1);
  else if(state == BG_PLAYER_STATE_PLAYING)
    s.can_pause = *((const int*)arg1);
  
  bg_msg_queue_list_send(player->message_queues,
                         msg_state,
                         &s);
  }

const bg_parameter_info_t *
bg_player_get_visualization_parameters(bg_player_t *  player)
  {
  return bg_visualizer_get_parameters(player->visualizer);
  }

void
bg_player_set_visualization_parameter(void*data,
                                      const char * name,
                                      const bg_parameter_value_t*val)
  {
  bg_player_t * p;
  int do_init;

  p = (bg_player_t*)data;
  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);
  
  bg_visualizer_set_parameter(p->visualizer, name, val);
  
  if(!do_init)
    {
    if(bg_visualizer_need_restart(p->visualizer))
      {
      bg_player_interrupt(p);
      bg_player_interrupt_resume(p);
      }
    }
  }

void
bg_player_set_visualization_plugin_parameter(void*data,
                                             const char * name,
                                             const bg_parameter_value_t*val)
  {
  bg_player_t * p;
  p = (bg_player_t*)data;
  bg_visualizer_set_vis_parameter(p->visualizer, name, val);
  }

void
bg_player_set_visualization(bg_player_t * p,
                            int enable)
  {
  int was_enabled;
  int do_init;
  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);

  pthread_mutex_lock(&p->config_mutex);
  was_enabled = p->visualizer_enabled;
  p->visualizer_enabled = enable;
  pthread_mutex_unlock(&p->config_mutex);

  if((was_enabled != enable) && !do_init)
    {
    bg_player_interrupt(p);
    bg_player_interrupt_resume(p);
    }
  }

void
bg_player_set_visualization_plugin(bg_player_t * p, const bg_plugin_info_t * plugin_info)
  {
  int do_init;
  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);
  
  bg_visualizer_set_vis_plugin(p->visualizer, plugin_info);
  
  if(!do_init)
    {
    if(bg_visualizer_need_restart(p->visualizer))
      {
      bg_player_interrupt(p);
      bg_player_interrupt_resume(p);
      }
    }

  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name        = "message_interval",
      .long_name   = TRS("Control loop interval"),
      .type        = BG_PARAMETER_INT,
      .val_default = { .val_i = 10 },
    },
    {
      .name         = "time_update",
      .long_name    = TRS("Time update interval"),
      .type         = BG_PARAMETER_STRINGLIST,
      .multi_names  = (char const *[]){ "seconds", "frames", NULL },
      .multi_labels = (char const *[]){ TRS("Seconds"), TRS("frames"), NULL },
      .val_default  = { .val_str = "seconds" },
    },
    {
      .name         = "report_peak",
      .long_name    = TRS("Report peak values for audio"),
      .type         = BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name         = "finish_mode",
      .long_name    = TRS("Finish mode"),
      .type         = BG_PARAMETER_STRINGLIST,
      .multi_names  = (char const *[]){ "change", "pause", NULL },
      .multi_labels = (char const *[]){ TRS("Change"), TRS("Pause"), NULL },
      .val_default  = { .val_str = "change" },
    },
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_player_get_parameters(bg_player_t * player)
  {
  return parameters;
  }


void bg_player_set_parameter(void * player, const char * name,
                             const bg_parameter_value_t * val)
  {
  bg_player_t * p = player;
  if(!name)
    return;
  else if(!strcmp(name, "message_interval"))
    {
    p->wait_time = val->val_i;
    p->wait_time *= 1000;
    }
  else if(!strcmp(name, "time_update"))
    {
    if(!strcmp(val->val_str, "second"))
      {
      p->time_update_mode = TIME_UPDATE_SECOND;
      }
    else if(!strcmp(val->val_str, "frame"))
      {
      p->time_update_mode = TIME_UPDATE_FRAME;
      }
    }
  else if(!strcmp(name, "finish_mode"))
    {
    if(!strcmp(val->val_str, "change"))
      {
      p->finish_mode = BG_PLAYER_FINISH_CHANGE;
      }
    else if(!strcmp(val->val_str, "pause"))
      {
      p->finish_mode = BG_PLAYER_FINISH_PAUSE;
      }
    }
  else if(!strcmp(name, "report_peak"))
    {
    if(val->val_i)
      p->flags |= PLAYER_DO_REPORT_PEAK;
    else
      p->flags &= ~PLAYER_DO_REPORT_PEAK;
    }
  }
