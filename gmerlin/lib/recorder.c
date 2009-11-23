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
#include <gmerlin/translation.h> 

#include <gmerlin/recorder.h>
#include <recorder_private.h>

static const int stream_mask =
BG_STREAM_AUDIO | BG_STREAM_VIDEO;                                   

static const uint32_t plugin_mask = BG_PLUGIN_FILE;

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
  if(rec->flags & FLAG_RUNNING)
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
  if(rec->flags & FLAG_DO_RECORD)
    {
    rec->as.flags |= STREAM_ENCODE;
    rec->vs.flags |= STREAM_ENCODE;
    
    rec->enc = bg_encoder_create(rec->plugin_reg,
                                 rec->encoder_section,
                                 NULL,
                                 stream_mask, plugin_mask);
    //    bg_encoder_open(rec->enc, );
    }
  else
    {
    rec->as.flags &= ~STREAM_ENCODE;
    rec->vs.flags &= ~STREAM_ENCODE;
    }
  
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
  
  if(rec->flags & FLAG_DO_RECORD)
    rec->flags &= FLAG_RECORDING;
    
  
  bg_player_threads_init(rec->th, NUM_THREADS);
  bg_player_threads_start(rec->th, NUM_THREADS);
  
  rec->flags |= FLAG_RUNNING;
  
  return 1;
  }

/* Encoders */

const bg_parameter_info_t *
bg_recorder_get_encoder_parameters(bg_recorder_t * rec)
  {
  if(!rec->encoder_parameters)
    rec->encoder_parameters =
      bg_plugin_registry_create_encoder_parameters(rec->plugin_reg,
                                                   stream_mask,
                                                   plugin_mask);
  return rec->encoder_parameters;
  }

void bg_recorder_set_encoder_section(bg_recorder_t * rec, bg_cfg_section_t * s)
  {
  rec->encoder_section = s;
  }

void bg_recorder_stop(bg_recorder_t * rec)
  {
  if(!(rec->flags & FLAG_RUNNING))
    return;
  bg_player_threads_join(rec->th, NUM_THREADS);
  bg_recorder_audio_cleanup(rec);
  bg_recorder_video_cleanup(rec);
  
  rec->flags &= ~(FLAG_RECORDING | FLAG_RUNNING);
  }

void bg_recorder_record(bg_recorder_t * rec, int record)
  {
  int was_running = !!(rec->flags & FLAG_RUNNING);

  if(was_running)
    bg_recorder_stop(rec);

  if(record)
    rec->flags |= FLAG_DO_RECORD;
  else
    rec->flags &= ~FLAG_DO_RECORD;

  if(was_running)
    bg_recorder_run(rec);
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

static const bg_parameter_info_t output_parameters[] =
  {
    {
      .name      = "output_directory",
      .long_name = TRS("Output directory"),
      .type      = BG_PARAMETER_DIRECTORY,
      .val_default = { .val_str = "." },
    },
    {
      .name      = "output_filename_mask",
      .long_name = TRS("Output filename mask"),
      .type      = BG_PARAMETER_STRING,
      .val_default = { .val_str = "%Y-%m-%d-%H:%M:%S" },
    },
    {
      .name      = "snapshot_directory",
      .long_name = TRS("Snapshot directory"),
      .type      = BG_PARAMETER_DIRECTORY,
      .val_default = { .val_str = "." },
    },
    {
      .name      = "snapshot_filename_mask",
      .long_name = TRS("Snapshot filename mask"),
      .type      = BG_PARAMETER_STRING,
      .val_default = { .val_str = "shot_%5n" },
    },
    { /* End */ }
  };

const bg_parameter_info_t *
bg_recorder_get_output_parameters(bg_recorder_t * rec)
  {
  return output_parameters;
  }

void
bg_recorder_set_output_parameter(void * data,
                                 const char * name,
                                 const bg_parameter_value_t * val)
  {

  }
