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

#include <config.h>
#include <stdlib.h>

#include <gmerlin/utils.h> 

#include <gmerlin/recorder.h>
#include <recorder_private.h>

bg_recorder_t * bg_recorder_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_recorder_t * ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = plugin_reg;
  ret->tc = bg_player_thread_common_create();
  
  bg_recorder_create_audio(ret);
  bg_recorder_create_video(ret);

  ret->th[0] = ret->as.th;
  ret->th[1] = ret->vs.th;

  ret->msg_queues = bg_msg_queue_list_create();

  
  return ret;
  }

void bg_recorder_destroy(bg_recorder_t * rec)
  {
  if(rec->running)
    bg_recorder_stop(rec);

  bg_recorder_destroy_audio(rec);
  bg_recorder_destroy_video(rec);

  bg_player_thread_common_destroy(rec->tc);

  free(rec->display_string);
  
  bg_msg_queue_list_destroy(rec->msg_queues);
  
  if(rec->encoder_parameters)
    bg_parameter_info_destroy_array(rec->encoder_parameters);
  
  free(rec);
  }

void bg_recorder_add_message_queue(bg_recorder_t * rec,
                               bg_msg_queue_t * q)
  {
  bg_msg_queue_list_add(rec->msg_queues, q);
  }

void bg_recorder_remove_message_queue(bg_recorder_t * rec,
                                      bg_msg_queue_t * q)
  {
  bg_msg_queue_list_remove(rec->msg_queues, q);
  }

int bg_recorder_run(bg_recorder_t * rec)
  {
  if(rec->as.flags & STREAM_ACTIVE)
    {
    if(!bg_recorder_audio_init(rec))
      rec->as.flags |= ~STREAM_ACTIVE;
    }
  
  if(rec->vs.flags & STREAM_ACTIVE)
    {
    if(!bg_recorder_video_init(rec))
      rec->vs.flags |= ~STREAM_ACTIVE;
    }
  
  if(rec->as.flags & STREAM_ACTIVE)
    bg_player_thread_set_func(rec->as.th, bg_recorder_audio_thread, rec);
  else
    bg_player_thread_set_func(rec->as.th, NULL, NULL);

  if(rec->vs.flags & STREAM_ACTIVE)
    bg_player_thread_set_func(rec->vs.th, bg_recorder_video_thread, rec);
  else
    bg_player_thread_set_func(rec->vs.th, NULL, NULL);
  
  
  bg_player_threads_init(rec->th, NUM_THREADS);
  bg_player_threads_start(rec->th, NUM_THREADS);

  rec->running = 1;
  
  return 1;
  }

/* Encoders */

const bg_parameter_info_t *
bg_recorder_get_encoder_parameters(bg_recorder_t * rec)
  {
  if(!rec->encoder_parameters)
    rec->encoder_parameters =
      bg_plugin_registry_create_encoder_parameters(rec->plugin_reg,
                                                   BG_STREAM_AUDIO |
                                                   BG_STREAM_VIDEO |
                                                   BG_STREAM_SUBTITLE_TEXT |
                                                   BG_STREAM_SUBTITLE_OVERLAY,
                                                   BG_PLUGIN_FILE);
  return rec->encoder_parameters;
  }

void bg_recorder_set_encoder_section(bg_recorder_t * rec, bg_cfg_section_t * s)
  {
  rec->encoder_section = s;
  }

void bg_recorder_stop(bg_recorder_t * rec)
  {
  if(!rec->running)
    return;
  bg_player_threads_join(rec->th, NUM_THREADS);
  
  bg_recorder_audio_cleanup(rec);
  bg_recorder_video_cleanup(rec);
  
  rec->running = 0;
  }

void bg_recorder_set_display_string(bg_recorder_t * rec, const char * str)
  {
  rec->display_string = bg_strdup(rec->display_string, str);
  }

/* Message stuff */

static void msg_framerate(bg_msg_t * msg,
                          const void * data)
  {
  const float * f = data;
  bg_msg_set_id(msg, BG_RECORDER_MSG_FRAMERATE);
  bg_msg_set_arg_float(msg, 0, *f);
  }
                       
void bg_recorder_msg_framerate(bg_recorder_t * rec,
                               float framerate)
  {
  bg_msg_queue_list_send(rec->msg_queues,
                         msg_framerate, &framerate);
  }

typedef struct
  {
  double * l;
  int samples;
  } audiolevel_t;

static void msg_audiolevel(bg_msg_t * msg,
                           const void * data)
  {
  const audiolevel_t * d = data;
  
  bg_msg_set_id(msg, BG_RECORDER_MSG_AUDIOLEVEL);
  bg_msg_set_arg_float(msg, 0, d->l[0]);
  bg_msg_set_arg_float(msg, 1, d->l[1]);
  bg_msg_set_arg_int(msg, 2, d->samples);
  }

void bg_recorder_msg_audiolevel(bg_recorder_t * rec,
                                double * level, int samples)
  {
  audiolevel_t d;
  d.l = level;
  d.samples = samples;
  
  bg_msg_queue_list_send(rec->msg_queues,
                         msg_audiolevel, &d);
  }
