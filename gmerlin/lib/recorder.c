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
#include <string.h>
#include <time.h>


#include <gmerlin/utils.h> 
#include <gmerlin/translation.h> 

#include <gmerlin/recorder.h>
#include <recorder_private.h>

const uint32_t bg_recorder_stream_mask =
BG_STREAM_AUDIO | BG_STREAM_VIDEO;                                   

const uint32_t bg_recorder_plugin_mask = BG_PLUGIN_FILE;

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
  pthread_mutex_init(&ret->enc_mutex, NULL);
  pthread_mutex_init(&ret->time_mutex, NULL);
  
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

  if(rec->output_directory)       free(rec->output_directory);
  if(rec->output_filename_mask)   free(rec->output_filename_mask);
  if(rec->snapshot_directory)     free(rec->snapshot_directory);
  if(rec->snapshot_filename_mask) free(rec->snapshot_filename_mask);

  bg_metadata_free(&rec->m);
  pthread_mutex_destroy(&rec->enc_mutex);
  pthread_mutex_destroy(&rec->time_mutex);
  
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

static void init_encoding(bg_recorder_t * rec)
  {
  struct tm brokentime;
  time_t t;
  char time_string[512];
  char * filename_base;
  
  time(&t);
  localtime_r(&t, &brokentime);
  strftime(time_string, 511, rec->output_filename_mask, &brokentime);

  filename_base =
    bg_sprintf("%s/%s", rec->output_directory, time_string);
  
  rec->as.flags |= STREAM_ENCODE;
  rec->vs.flags |= STREAM_ENCODE;
  
  rec->enc = bg_encoder_create(rec->plugin_reg,
                               rec->encoder_section,
                               NULL,
                               bg_recorder_stream_mask, bg_recorder_plugin_mask);
  
  bg_encoder_open(rec->enc, filename_base, NULL, NULL);
  free(filename_base);
  }

static int finalize_encoding(bg_recorder_t * rec)
  {
  if(!bg_encoder_start(rec->enc))
    return 0;
  
  if(rec->as.flags & STREAM_ACTIVE)
    bg_recorder_audio_finalize_encode(rec);
  if(rec->vs.flags & STREAM_ACTIVE)
    bg_recorder_video_finalize_encode(rec);
  
  return 1;
  }

int bg_recorder_run(bg_recorder_t * rec)
  {
  if(rec->flags & FLAG_DO_RECORD)
    {
    init_encoding(rec);
    rec->recording_time = 0;
    rec->last_recording_time = - 2 * GAVL_TIME_SCALE;
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

  if(rec->flags & FLAG_DO_RECORD)
    finalize_encoding(rec);
  
  
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
                                                   bg_recorder_stream_mask,
                                                   bg_recorder_plugin_mask);
  return rec->encoder_parameters;
  }

void bg_recorder_set_encoder_section(bg_recorder_t * rec,
                                     bg_cfg_section_t * s)
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

  if(rec->enc)
    {
    bg_encoder_destroy(rec->enc, 0);
    rec->enc = NULL;
    bg_recorder_msg_time(rec,
                         GAVL_TIME_UNDEFINED);
    }
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

static void msg_time(bg_msg_t * msg,
                     const void * data)
  {
  const gavl_time_t * t = data;
  bg_msg_set_id(msg, BG_RECORDER_MSG_TIME);
  bg_msg_set_arg_time(msg, 0, *t);
  }
                       
void bg_recorder_msg_time(bg_recorder_t * rec,
                          gavl_time_t t)
  {
  bg_msg_queue_list_send(rec->msg_queues,
                         msg_time, &t);
  }


/* Parameter stuff */



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
      .val_default = { .val_str = "%Y-%m-%d-%H-%M-%S" },
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
  bg_recorder_t * rec;
  if(!name)
    return;

  rec = data;
  
  if(!strcmp(name, "output_directory"))
    rec->output_directory = bg_strdup(rec->output_directory, val->val_str);
  else if(!strcmp(name, "output_filename_mask"))
    rec->output_filename_mask = bg_strdup(rec->output_filename_mask, val->val_str);
  else if(!strcmp(name, "snapshot_directory"))
    rec->snapshot_directory = bg_strdup(rec->snapshot_directory, val->val_str);
  else if(!strcmp(name, "snapshot_filename_mask"))
    rec->snapshot_filename_mask = bg_strdup(rec->snapshot_filename_mask, val->val_str);
  }

const bg_parameter_info_t *
bg_recorder_get_metadata_parameters(bg_recorder_t * rec)
  {
  if(!rec->metadata_parameters)
    rec->metadata_parameters = bg_metadata_get_parameters(&rec->m);
  return rec->metadata_parameters;
  }

void
bg_recorder_set_metadata_parameter(void * data,
                                   const char * name,
                                   const bg_parameter_value_t * val)
  {
  bg_recorder_t * rec = data;
  bg_metadata_set_parameter(&rec->m, name, val);
  }

void bg_recorder_update_time(bg_recorder_t * rec, gavl_time_t t)
  {
  pthread_mutex_lock(&rec->time_mutex);

  if(rec->recording_time < t)
    rec->recording_time = t;

  if(rec->recording_time - rec->last_recording_time >
     GAVL_TIME_SCALE)
    {
    bg_recorder_msg_time(rec,
                         rec->recording_time);
    rec->last_recording_time = rec->recording_time;
    }
  pthread_mutex_unlock(&rec->time_mutex);
  }
