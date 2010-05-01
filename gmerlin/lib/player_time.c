/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

void bg_player_time_init(bg_player_t * player)
  {
  bg_player_audio_stream_t * s = &player->audio_stream;
  
  if(s->plugin && (s->plugin->get_delay) &&
     DO_AUDIO(player->flags))
    s->sync_mode = SYNC_SOUNDCARD;
  else
    s->sync_mode = SYNC_SOFTWARE;
  }

void bg_player_time_start(bg_player_t * player)
  {
  bg_player_audio_stream_t * ctx = &player->audio_stream;
 
  /* Set timer */

  if(ctx->sync_mode == SYNC_SOFTWARE)
    {
    pthread_mutex_lock(&ctx->time_mutex);
    gavl_timer_set(ctx->timer, ctx->current_time);
    gavl_timer_start(ctx->timer);
    pthread_mutex_unlock(&ctx->time_mutex);
    }
  }

void bg_player_time_stop(bg_player_t * player)
  {
  bg_player_audio_stream_t * ctx = &player->audio_stream;

  if(ctx->sync_mode == SYNC_SOFTWARE)
    {
    pthread_mutex_lock(&ctx->time_mutex);
    gavl_timer_stop(ctx->timer);
    pthread_mutex_unlock(&ctx->time_mutex);
    }
  }

void bg_player_time_reset(bg_player_t * player)
  {
  bg_player_audio_stream_t * ctx = &player->audio_stream;
  if(ctx->sync_mode == SYNC_SOFTWARE)
    {
    pthread_mutex_lock(&ctx->time_mutex);
    gavl_timer_stop(ctx->timer);
    ctx->current_time = 0;
    pthread_mutex_unlock(&ctx->time_mutex);
    }
  else
    {
    pthread_mutex_lock(&ctx->time_mutex);
    ctx->current_time = 0;
    pthread_mutex_unlock(&ctx->time_mutex);
    }
  }

/* Get the current time */

void bg_player_time_get(bg_player_t * player, int exact,
                        gavl_time_t * ret)
  {
  bg_player_audio_stream_t * ctx = &player->audio_stream;

  int samples_in_soundcard;
  
  if(!exact)
    {
    pthread_mutex_lock(&ctx->time_mutex);
    *ret = ctx->current_time;
    pthread_mutex_unlock(&ctx->time_mutex);
    }
  else
    {
    if(ctx->sync_mode == SYNC_SOFTWARE)
      {
      pthread_mutex_lock(&ctx->time_mutex);
      ctx->current_time = gavl_timer_get(ctx->timer);
      *ret = ctx->current_time;
      pthread_mutex_unlock(&ctx->time_mutex);
      }
    else
      {
      samples_in_soundcard = 0;
      bg_plugin_lock(ctx->plugin_handle);
      if(ctx->output_open)
        samples_in_soundcard = ctx->plugin->get_delay(ctx->priv);
      bg_plugin_unlock(ctx->plugin_handle);

      pthread_mutex_lock(&ctx->time_mutex);
      ctx->current_time =
        gavl_samples_to_time(ctx->output_format.samplerate,
                             ctx->samples_written-samples_in_soundcard);

      
      // ctx->current_time *= ctx->player->audio_stream.output_format.samplerate;
      // ctx->current_time /= ctx->player->audio_stream.input_format.samplerate;
      
      *ret = ctx->current_time;
      pthread_mutex_unlock(&ctx->time_mutex);
      }
    }

  }

void bg_player_time_set(bg_player_t * player, gavl_time_t time)
  {
  bg_player_audio_stream_t * ctx = &player->audio_stream;
 
  pthread_mutex_lock(&ctx->time_mutex);

  if(ctx->sync_mode == SYNC_SOFTWARE)
    gavl_timer_set(ctx->timer, time);
  else if(ctx->sync_mode == SYNC_SOUNDCARD)
    {
    ctx->samples_written =
      gavl_time_to_samples(ctx->output_format.samplerate,
                           time);
    /* If time is set explicitely, we don't do that timestamp offset stuff */
    ctx->has_first_timestamp_o = 1;
    }
  ctx->current_time = time;
  pthread_mutex_unlock(&ctx->time_mutex);
  }
