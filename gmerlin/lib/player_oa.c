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
#include <gmerlin/player.h>
#include <playerprivate.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "player.audio_output"

struct bg_player_oa_context_s
  {
  bg_plugin_handle_t * plugin_handle;
  bg_oa_plugin_t     * plugin;
  void               * priv;
  
  bg_player_t * player;

  /*
   *  Sync mode, set ONLY in init function,
   *  read by various threads
   */
  
  bg_player_sync_mode_t sync_mode;
  
  /* Timing stuff */

  int output_open;
    
  pthread_mutex_t time_mutex;
  gavl_time_t     current_time;
  gavl_timer_t *  timer;

  int64_t samples_written;

  int have_first_timestamp;
  
  };

void * bg_player_oa_create_frame(void * data)
  {
  bg_player_oa_context_t * ctx = (bg_player_oa_context_t *)data;
  return gavl_audio_frame_create(&(ctx->player->audio_stream.fifo_format));
  }

int  bg_player_oa_has_plugin(bg_player_oa_context_t * ctx)
  {
  return (ctx->plugin_handle ? 1 : 0);
  }


void bg_player_oa_destroy_frame(void * data, void * frame)
  {
  gavl_audio_frame_t *  audio_frame = (gavl_audio_frame_t*)frame;
  gavl_audio_frame_destroy(audio_frame);
  }

void bg_player_oa_create(bg_player_t * player)
  {
  bg_player_oa_context_t * ctx;
  ctx = calloc(1, sizeof(*ctx));
  ctx->player = player;

  ctx->timer = gavl_timer_create();
  
  pthread_mutex_init(&(ctx->time_mutex),(pthread_mutexattr_t *)0);
  
  player->oa_context = ctx;
  }

void bg_player_oa_set_plugin(bg_player_t * player,
                             bg_plugin_handle_t * handle)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;

  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  
  ctx->plugin_handle = handle;
  
  ctx->plugin = (bg_oa_plugin_t*)(ctx->plugin_handle->plugin);
  ctx->priv = ctx->plugin_handle->priv;
  }

void bg_player_oa_destroy(bg_player_t * player)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;

  pthread_mutex_destroy(&(ctx->time_mutex));

  gavl_timer_destroy(ctx->timer);
  
  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  free(ctx);
  }

void bg_player_time_init(bg_player_t * player)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;

  if((player->input_handle->plugin->flags & BG_PLUGIN_INPUT_HAS_SYNC) &&
     player->do_bypass)
    ctx->sync_mode = SYNC_INPUT;
  else if(ctx->plugin && (ctx->plugin->get_delay) &&
          DO_AUDIO(ctx->player->flags))
    ctx->sync_mode = SYNC_SOUNDCARD;
  else
    ctx->sync_mode = SYNC_SOFTWARE;
  }

void bg_player_time_start(bg_player_t * player)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;
  
  /* Set timer */

  if(ctx->sync_mode == SYNC_SOFTWARE)
    {
    pthread_mutex_lock(&(ctx->time_mutex));
    gavl_timer_set(ctx->timer, ctx->current_time);
    gavl_timer_start(ctx->timer);
    pthread_mutex_unlock(&(ctx->time_mutex));
    }
  }

void bg_player_time_stop(bg_player_t * player)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;
  if(ctx->sync_mode == SYNC_SOFTWARE)
    {
    pthread_mutex_lock(&(ctx->time_mutex));
    gavl_timer_stop(ctx->timer);
    pthread_mutex_unlock(&(ctx->time_mutex));
    }
  }

void bg_player_time_reset(bg_player_t * player)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;
  if(ctx->sync_mode == SYNC_SOFTWARE)
    {
    pthread_mutex_lock(&(ctx->time_mutex));
    gavl_timer_stop(ctx->timer);
    ctx->current_time = 0;
    pthread_mutex_unlock(&(ctx->time_mutex));
    }
  else
    {
    pthread_mutex_lock(&(ctx->time_mutex));
    ctx->current_time = 0;
    pthread_mutex_unlock(&(ctx->time_mutex));
    }
  }

/* Get the current time */

void bg_player_time_get(bg_player_t * player, int exact,
                        gavl_time_t * ret)
  {
  bg_player_oa_context_t * ctx;
  int samples_in_soundcard;
  
  ctx = player->oa_context;
  if(!exact || (ctx->sync_mode == SYNC_INPUT))
    {
    pthread_mutex_lock(&(ctx->time_mutex));
    *ret = ctx->current_time;
    pthread_mutex_unlock(&(ctx->time_mutex));
    }
  else
    {
    if(ctx->sync_mode == SYNC_SOFTWARE)
      {
      pthread_mutex_lock(&(ctx->time_mutex));
      ctx->current_time = gavl_timer_get(ctx->timer);
      *ret = ctx->current_time;
      pthread_mutex_unlock(&(ctx->time_mutex));
      }
    else
      {
      samples_in_soundcard = 0;
      bg_plugin_lock(ctx->plugin_handle);
      if(ctx->output_open)
        samples_in_soundcard = ctx->plugin->get_delay(ctx->priv);
      bg_plugin_unlock(ctx->plugin_handle);

      pthread_mutex_lock(&(ctx->time_mutex));
      ctx->current_time =
        gavl_samples_to_time(ctx->player->audio_stream.output_format.samplerate,
                             ctx->samples_written-samples_in_soundcard);

      
      // ctx->current_time *= ctx->player->audio_stream.output_format.samplerate;
      // ctx->current_time /= ctx->player->audio_stream.input_format.samplerate;
      
      *ret = ctx->current_time;
      pthread_mutex_unlock(&(ctx->time_mutex));
      }
    }

  }

void bg_player_time_set(bg_player_t * player, gavl_time_t time)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;

  
  pthread_mutex_lock(&(ctx->time_mutex));

  if(ctx->sync_mode == SYNC_SOFTWARE)
    gavl_timer_set(ctx->timer, time);
  else if(ctx->sync_mode == SYNC_SOUNDCARD)
    {
    ctx->samples_written =
      gavl_time_to_samples(ctx->player->audio_stream.output_format.samplerate,
                           time);
    /* If time is set explicitely, we don't do that timestamp offset stuff */
    ctx->have_first_timestamp = 1;
    }
  ctx->current_time = time;
  pthread_mutex_unlock(&(ctx->time_mutex));
  }

#define CHECK_PEAK(id, pos) \
   index = gavl_channel_index(&ctx->player->audio_stream.fifo_format, id); \
   if(index >= 0) \
     { \
     if(d.peak_out[pos] < peak[index]) \
       d.peak_out[pos] = peak[index]; \
     }

#define CHECK_PEAK_2(id) \
   index = gavl_channel_index(&ctx->player->audio_stream.fifo_format, id); \
   if(index >= 0) \
     { \
     if(d.peak_out[0] < peak[index]) \
       d.peak_out[0] = peak[index]; \
     if(d.peak_out[1] < peak[index]) \
       d.peak_out[1] = peak[index]; \
     }

typedef struct
  {
  double peak_out[2];
  int num_samples;
  } peak_data_t;

static void msg_peak(bg_msg_t * msg,
                     const void * data)
  {
  const peak_data_t * d = data;
  bg_msg_set_id(msg, BG_PLAYER_MSG_AUDIO_PEAK);
  bg_msg_set_arg_int(msg, 0, d->num_samples);
  bg_msg_set_arg_float(msg, 1, d->peak_out[0]);
  bg_msg_set_arg_float(msg, 2, d->peak_out[1]);
  }

static void do_peak(bg_player_oa_context_t * ctx, gavl_audio_frame_t * frame)
  {
  peak_data_t d;
  int index;
  double peak[GAVL_MAX_CHANNELS];

  d.peak_out[0] = 0.0;
  d.peak_out[1] = 0.0;
  d.num_samples = frame->valid_samples;
  
  gavl_peak_detector_reset(ctx->player->audio_stream.peak_detector);
  gavl_peak_detector_update(ctx->player->audio_stream.peak_detector,
                            frame);

  gavl_peak_detector_get_peaks(ctx->player->audio_stream.peak_detector,
                               (double*)0,
                               (double*)0,
                               peak);
  
  /* Collect channels and merge into 2 */
  CHECK_PEAK(GAVL_CHID_FRONT_LEFT, 0);
  CHECK_PEAK(GAVL_CHID_FRONT_RIGHT, 1);

  CHECK_PEAK(GAVL_CHID_REAR_LEFT, 0);
  CHECK_PEAK(GAVL_CHID_REAR_RIGHT, 1);

  CHECK_PEAK(GAVL_CHID_SIDE_LEFT, 0);
  CHECK_PEAK(GAVL_CHID_SIDE_RIGHT, 1);

  CHECK_PEAK(GAVL_CHID_FRONT_CENTER_LEFT, 0);
  CHECK_PEAK(GAVL_CHID_FRONT_CENTER_RIGHT, 1);
  
  CHECK_PEAK_2(GAVL_CHID_FRONT_CENTER);
  CHECK_PEAK_2(GAVL_CHID_LFE);

  /* Broadcast */
  bg_msg_queue_list_send(ctx->player->message_queues, msg_peak, &d);
  }

/* Audio output thread */

void * bg_player_oa_thread(void * data)
  {
  bg_player_oa_context_t * ctx;
  bg_player_audio_stream_t * s;
  gavl_audio_frame_t * frame;
  gavl_time_t wait_time;
  int do_mute;
  bg_fifo_state_t state;
  char tmp_string[128];
  int interrupt = 0;
  
  ctx = (bg_player_oa_context_t *)data;
  
  s = &(ctx->player->audio_stream);

  
  
  /* Wait for playback */
    
  while(1)
    {
    if(!bg_player_keep_going(ctx->player, NULL, NULL, interrupt))
      break;

    interrupt = 0;
    
    if(!s->fifo) // Audio was switched off
      break;

    wait_time = GAVL_TIME_UNDEFINED;
    
    frame = bg_fifo_lock_read(s->fifo, &state);
    if(!frame)
      {
      if(state == BG_FIFO_STOPPED) 
        break;
      else if(state == BG_FIFO_PAUSED)
        {
        interrupt = 1;
        continue;
        }
      }

#if 1
    if(!ctx->have_first_timestamp)
      {
      if(frame->timestamp)
        {
        sprintf(tmp_string, "%" PRId64, frame->timestamp);
        bg_log(BG_LOG_INFO, LOG_DOMAIN,
               "Got initial audio timestamp: %s",
               tmp_string);
        
        pthread_mutex_lock(&(ctx->time_mutex));
        ctx->samples_written += frame->timestamp;
        pthread_mutex_unlock(&(ctx->time_mutex));
        }
      ctx->have_first_timestamp = 1;
      }
#endif
    if(frame->valid_samples)
      {
      pthread_mutex_lock(&(ctx->player->mute_mutex));
      do_mute = ctx->player->mute;
      pthread_mutex_unlock(&(ctx->player->mute_mutex));

      if(DO_VISUALIZE(ctx->player->flags))
        bg_visualizer_update(ctx->player->visualizer, frame);

      if(DO_PEAK(ctx->player->flags))
        do_peak(ctx, frame);
      
      if(do_mute)
        {
        gavl_audio_frame_mute(frame,
                              &(ctx->player->audio_stream.fifo_format));
        }
      else
        {
        pthread_mutex_lock(&(ctx->player->audio_stream.volume_mutex));
        gavl_volume_control_apply(ctx->player->audio_stream.volume,
                                  frame);
        
        pthread_mutex_unlock(&(ctx->player->audio_stream.volume_mutex));
        }
      
      if(ctx->player->audio_stream.do_convert_out)
        {
        gavl_audio_convert(ctx->player->audio_stream.cnv_out, frame,
                           ctx->player->audio_stream.frame_out);

        bg_plugin_lock(ctx->plugin_handle);
        ctx->plugin->write_audio(ctx->priv,
                                 ctx->player->audio_stream.frame_out);
        bg_plugin_unlock(ctx->plugin_handle);
        }
      else
        {
        bg_plugin_lock(ctx->plugin_handle);
        ctx->plugin->write_audio(ctx->priv, frame);
        bg_plugin_unlock(ctx->plugin_handle);
        }
      
      pthread_mutex_lock(&(ctx->time_mutex));
      ctx->samples_written += frame->valid_samples;
      pthread_mutex_unlock(&(ctx->time_mutex));

      /* Now, wait a while to give other threads a chance to access the
         player time */
      wait_time =
        gavl_samples_to_time(ctx->player->audio_stream.output_format.samplerate,
                             frame->valid_samples)/2;
      }
    bg_fifo_unlock_read(s->fifo);
    
    if(wait_time != GAVL_TIME_UNDEFINED)
      gavl_time_delay(&wait_time);
    }
  return NULL;
  }

int bg_player_oa_init(bg_player_oa_context_t * ctx)
  {
  int result;
  bg_plugin_lock(ctx->plugin_handle);
  result =
    ctx->plugin->open(ctx->priv, &(ctx->player->audio_stream.output_format));
  if(result)
    ctx->output_open = 1;
  else
    ctx->output_open = 0;
  
  ctx->have_first_timestamp = 0;

  bg_plugin_unlock(ctx->plugin_handle);

  
  ctx->samples_written = 0;
  return result;
  }



void bg_player_oa_cleanup(bg_player_oa_context_t * ctx)
  {
  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->close(ctx->priv);
  ctx->output_open = 0;
  bg_plugin_unlock(ctx->plugin_handle);

  }

int bg_player_oa_start(bg_player_oa_context_t * ctx)
  {
  int result = 1;
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->start)
    result = ctx->plugin->start(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  return result;
  }

void bg_player_oa_stop(bg_player_oa_context_t * ctx)
  {
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->stop)
    ctx->plugin->stop(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  
  }

void bg_player_oa_set_volume(bg_player_oa_context_t * ctx,
                             float volume)
  {
  pthread_mutex_lock(&(ctx->player->audio_stream.volume_mutex));
  gavl_volume_control_set_volume(ctx->player->audio_stream.volume, volume);
  pthread_mutex_unlock(&(ctx->player->audio_stream.volume_mutex));
  }

int bg_player_oa_get_latency(bg_player_oa_context_t * ctx)
  {
  int ret;
  if(!ctx->priv || !ctx->plugin || !ctx->plugin->get_delay ||
     !ctx->output_open)
    {
    return 0;
    }
  bg_plugin_lock(ctx->plugin_handle);
  ret = ctx->plugin->get_delay(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  return ret;
  }
