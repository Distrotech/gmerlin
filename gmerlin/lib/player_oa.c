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

void bg_player_set_oa_plugin_internal(bg_player_t * player,
                                      bg_plugin_handle_t * handle)
  {
  bg_player_audio_stream_t * s = &player->audio_stream;
  
  if(s->plugin_handle)
    bg_plugin_unref(s->plugin_handle);
  
  s->plugin_handle = handle;
  
  s->plugin = (bg_oa_plugin_t*)(s->plugin_handle->plugin);
  s->priv = s->plugin_handle->priv;
  }


#define CHECK_PEAK(id, pos) \
   index = gavl_channel_index(&s->fifo_format, id); \
   if(index >= 0) \
     { \
     if(d.peak_out[pos] < peak[index]) \
       d.peak_out[pos] = peak[index]; \
     }

#define CHECK_PEAK_2(id) \
   index = gavl_channel_index(&s->fifo_format, id); \
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

static void do_peak(bg_player_t * p, gavl_audio_frame_t * frame)
  {
  peak_data_t d;
  int index;
  bg_player_audio_stream_t * s;
  double peak[GAVL_MAX_CHANNELS];

  s = &p->audio_stream;
  
  d.peak_out[0] = 0.0;
  d.peak_out[1] = 0.0;
  d.num_samples = frame->valid_samples;
  
  gavl_peak_detector_reset(s->peak_detector);
  gavl_peak_detector_update(s->peak_detector,
                            frame);

  gavl_peak_detector_get_peaks(s->peak_detector,
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
  bg_msg_queue_list_send(p->message_queues, msg_peak, &d);
  }

/* Audio output thread */

static void process_frame(bg_player_t * p, gavl_audio_frame_t * frame)
  {
  int do_mute;
  bg_player_audio_stream_t * s;
  char tmp_string[128];
  s = &p->audio_stream;
  
  if(!s->has_first_timestamp_o)
    {
    if(frame->timestamp)
      {
      sprintf(tmp_string, "%" PRId64, frame->timestamp);
      bg_log(BG_LOG_INFO, LOG_DOMAIN,
             "Got initial audio timestamp: %s",
             tmp_string);
      
      pthread_mutex_lock(&(s->time_mutex));
      s->samples_written += frame->timestamp;
      pthread_mutex_unlock(&(s->time_mutex));
      }
    s->has_first_timestamp_o = 1;
    }

  if(frame->valid_samples)
    {
    pthread_mutex_lock(&(s->mute_mutex));
    do_mute = s->mute;
    pthread_mutex_unlock(&(s->mute_mutex));
    
    if(DO_VISUALIZE(p->flags))
      bg_visualizer_update(p->visualizer, frame);
    
    if(DO_PEAK(p->flags))
      do_peak(p, frame);
    
    if(do_mute)
      {
      gavl_audio_frame_mute(frame,
                            &(s->fifo_format));
      }
    else
      {
      pthread_mutex_lock(&(s->volume_mutex));
      gavl_volume_control_apply(s->volume,
                                frame);
      
      pthread_mutex_unlock(&(s->volume_mutex));
      }
    }
  
  }

static void read_audio_callback(void * priv,
                                gavl_audio_frame_t* frame,
                                int stream,
                                int num_samples)
  {
  
  }

void * bg_player_oa_thread(void * data)
  {
  bg_player_audio_stream_t * s;
  gavl_audio_frame_t * frame;
  gavl_time_t wait_time;

  int state;
  bg_player_t * p = data;
  
  s = &(p->audio_stream);
  
  /* Wait for playback */

  bg_player_thread_wait_for_start(s->th);
  
  while(1)
    {
    if(!bg_player_thread_check(s->th))
      break;
    
    if(!bg_player_read_audio(p, s->fifo_frame, &state))
      {
      if(state == BG_PLAYER_STATE_PLAYING)
        {
        /* EOF */
        }
      else
        {
        /* Wait for restart or quit */
        }
      }

    process_frame(p, s->fifo_frame);
    
#if 1

    //    
    
#endif
    if(frame->valid_samples)
      {
      if(s->do_convert_out)
        {
        gavl_audio_convert(s->cnv_out, frame,
                           s->output_frame);

        bg_plugin_lock(s->plugin_handle);
        s->plugin->write_audio(s->priv,
                                 s->output_frame);
        bg_plugin_unlock(s->plugin_handle);
        }
      else
        {
        bg_plugin_lock(s->plugin_handle);
        s->plugin->write_audio(s->priv, frame);
        bg_plugin_unlock(s->plugin_handle);
        }
      
      pthread_mutex_lock(&(s->time_mutex));
      s->samples_written += frame->valid_samples;
      pthread_mutex_unlock(&(s->time_mutex));
      
      /* Now, wait a while to give other threads a chance to access the
         player time */
      wait_time =
        gavl_samples_to_time(s->output_format.samplerate,
                             frame->valid_samples)/2;
      }
    
    if(wait_time != GAVL_TIME_UNDEFINED)
      gavl_time_delay(&wait_time);
    }
  return NULL;
  }

int bg_player_oa_init(bg_player_audio_stream_t * ctx)
  {
  int result;
  bg_plugin_lock(ctx->plugin_handle);
  result =
    ctx->plugin->open(ctx->priv, &(ctx->output_format));
  if(result)
    ctx->output_open = 1;
  else
    ctx->output_open = 0;
  
  ctx->has_first_timestamp_o = 0;

  bg_plugin_unlock(ctx->plugin_handle);

  
  ctx->samples_written = 0;
  return result;
  }



void bg_player_oa_cleanup(bg_player_audio_stream_t * ctx)
  {
  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->close(ctx->priv);
  ctx->output_open = 0;
  bg_plugin_unlock(ctx->plugin_handle);

  }

int bg_player_oa_start(bg_player_audio_stream_t * ctx)
  {
  int result = 1;
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->start)
    result = ctx->plugin->start(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  return result;
  }

void bg_player_oa_stop(bg_player_audio_stream_t * ctx)
  {
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->stop)
    ctx->plugin->stop(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  
  }

void bg_player_oa_set_volume(bg_player_audio_stream_t * ctx,
                             float volume)
  {
  pthread_mutex_lock(&(ctx->volume_mutex));
  gavl_volume_control_set_volume(ctx->volume, volume);
  pthread_mutex_unlock(&(ctx->volume_mutex));
  }

int bg_player_oa_get_latency(bg_player_audio_stream_t * ctx)
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

void bg_player_oa_set_plugin(bg_player_t * player,
                             bg_plugin_handle_t * handle)
  {
  bg_player_audio_stream_t * ctx;

  ctx = &player->audio_stream;

  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  
  ctx->plugin_handle = handle;
  
  ctx->plugin = (bg_oa_plugin_t*)(ctx->plugin_handle->plugin);
  ctx->priv = ctx->plugin_handle->priv;

#if 0  
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->set_callbacks)
    ctx->plugin->set_callbacks(ctx->priv, &(ctx->callbacks));
  bg_plugin_unlock(ctx->plugin_handle);
#endif
  }
