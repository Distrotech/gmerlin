/*****************************************************************
 
  player_oa.c
 
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
#include "player.h"
#include "playerprivate.h"

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

  int64_t audio_samples_written;

  /* Volume control */

  gavl_volume_control_t * volume;
  pthread_mutex_t volume_mutex;
  
  };

void * bg_player_oa_create_frame(void * data)
  {
  bg_player_oa_context_t * ctx = (bg_player_oa_context_t *)data;
  return gavl_audio_frame_create(&(ctx->player->audio_stream.output_format));
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

  ctx->volume = gavl_volume_control_create();
  ctx->timer = gavl_timer_create();

  pthread_mutex_init(&(ctx->time_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ctx->volume_mutex),(pthread_mutexattr_t *)0);

    
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
  bg_plugin_ref(ctx->plugin_handle);
  
  ctx->plugin = (bg_oa_plugin_t*)(ctx->plugin_handle->plugin);
  ctx->priv = ctx->plugin_handle->priv;
  }

void bg_player_oa_destroy(bg_player_t * player)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;

  pthread_mutex_destroy(&(ctx->time_mutex));

  gavl_timer_destroy(ctx->timer);
  gavl_volume_control_destroy(ctx->volume);
  
  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  free(ctx);
  }

/* Get the current time */

void bg_player_time_start(bg_player_t * player)
  {
  bg_player_oa_context_t * ctx;
  ctx = player->oa_context;
  
  if((ctx->plugin->get_delay) && (ctx->player->do_audio))
    ctx->sync_mode = SYNC_SOUNDCARD;
  else
    ctx->sync_mode = SYNC_SOFTWARE;
  
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
  //  fprintf(stderr, "time reset\n");
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

void bg_player_time_get(bg_player_t * player, int exact, gavl_time_t * ret)
  {
  bg_player_oa_context_t * ctx;
  int samples_in_soundcard;
  ctx = player->oa_context;
  if(!exact)
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
                             ctx->audio_samples_written-samples_in_soundcard);
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
  else
    ctx->audio_samples_written =
      gavl_time_to_samples(ctx->player->audio_stream.output_format.samplerate,
                           time);
  ctx->current_time = time;
  pthread_mutex_unlock(&(ctx->time_mutex));
  }

/* Audio output thread */

void * bg_player_oa_thread(void * data)
  {
  bg_player_oa_context_t * ctx;
  bg_player_audio_stream_t * s;
  gavl_audio_frame_t * frame;
  gavl_time_t wait_time;
  
  bg_fifo_state_t state;
  
  //  fprintf(stderr, "oa thread started\n");
    
  ctx = (bg_player_oa_context_t *)data;
  
  s = &(ctx->player->audio_stream);
  
  /* Wait for playback */
    
  while(1)
    {
    if(!bg_player_keep_going(ctx->player))
      break;

    wait_time = GAVL_TIME_UNDEFINED;
    
    frame = bg_fifo_lock_read(s->fifo, &state);
    if(!frame)
      {
      if(state == BG_FIFO_STOPPED) 
        {
        //        fprintf(stderr, "oa thread finisheded\n");
        break;
        }
      else if(state == BG_FIFO_PAUSED)
        {
        continue;
        }
      }
    if(frame->valid_samples)
      {
      pthread_mutex_lock(&(ctx->volume_mutex));
      gavl_volume_control_apply(ctx->volume, frame);
      pthread_mutex_unlock(&(ctx->volume_mutex));
      
      bg_plugin_lock(ctx->plugin_handle);
      ctx->plugin->write_frame(ctx->priv, frame);
      bg_plugin_unlock(ctx->plugin_handle);

      pthread_mutex_lock(&(ctx->time_mutex));
      ctx->audio_samples_written += frame->valid_samples;
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
  bg_plugin_unlock(ctx->plugin_handle);

  gavl_volume_control_set_format(ctx->volume,
                                 &(ctx->player->audio_stream.output_format));
  
  ctx->audio_samples_written = 0;
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
  pthread_mutex_lock(&(ctx->volume_mutex));
  gavl_volume_control_set_volume(ctx->volume, volume);
  pthread_mutex_unlock(&(ctx->volume_mutex));
  }


int bg_player_oa_get_latency(bg_player_oa_context_t * ctx)
  {
  int ret;
  if(!ctx->priv || !ctx->plugin || !ctx->plugin->get_delay)
    {
    return 0;
    }
  bg_plugin_lock(ctx->plugin_handle);
  ret = ctx->plugin->get_delay(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  return ret;
  }
