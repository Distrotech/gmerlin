/*****************************************************************
 
  player_input.c
 
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "player.h"
#include "playerprivate.h"

struct bg_player_input_context_s
  {
  /* Plugin stuff */
  
  bg_plugin_handle_t * plugin_handle;
  bg_input_plugin_t  * plugin;
  void * priv;

  int audio_finished;
  int video_finished;
  int send_silence;
    
  int64_t audio_samples_written;
  int64_t video_frames_written;

  /* Internal times */

  gavl_time_t audio_time;
  gavl_time_t video_time;
  
  /* For changing global stuff */
  
  bg_player_t * player;

  /* Callback structure */

  bg_input_callbacks_t callbacks;
  };

static void track_changed(void * data, int track)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;

  //  fprintf(stderr, "Track changed callback, new track: %d\n", track);

  bg_player_set_track(ctx->player, track);
  
  }

static void time_changed(void * data, gavl_time_t time)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;

  //  fprintf(stderr, "Time changed callback, new track: %f\n", gavl_time_to_seconds(time));
  bg_player_time_set(ctx->player, time);
  }

static void duration_changed(void * data, gavl_time_t duration)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;

  //  fprintf(stderr, "Duration changed callback, new duration: %f\n",
  //          gavl_time_to_seconds(duration));

  bg_player_set_duration(ctx->player, duration, ctx->player->can_seek);


  //  bg_player_time_set(ctx->player, time);
  }

static void name_changed(void * data, const char * name)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;
  //  fprintf(stderr, "Name changed callback\n");
  bg_player_set_track_name(ctx->player, name);
  }

static void metadata_changed(void * data, const bg_metadata_t * m)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;
  bg_player_set_metadata(ctx->player, m);
  //  fprintf(stderr, "** Set metadata **\n");
  }

static void buffer_notify(void * data, float percentage)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;
  bg_player_set_state(ctx->player, BG_PLAYER_STATE_BUFFERING, &percentage, NULL);
  }

void bg_player_input_create(bg_player_t * player)
  {
  bg_player_input_context_t * ctx;
  ctx = calloc(1, sizeof(*ctx));

  /* Set up callbacks */

  ctx->callbacks.data = ctx;
  ctx->callbacks.track_changed    = track_changed;
  ctx->callbacks.time_changed     = time_changed;
  ctx->callbacks.name_changed     = name_changed;
  ctx->callbacks.duration_changed = duration_changed;
  ctx->callbacks.metadata_changed = metadata_changed;
  ctx->callbacks.buffer_notify    = buffer_notify;
  
  ctx->player = player;
  
  player->input_context = ctx;
  }

void bg_player_input_destroy(bg_player_t * player)
  {
  if(player->input_context->plugin_handle)
    bg_plugin_unref(player->input_context->plugin_handle);
  free(player->input_context);
  }

int bg_player_input_init(bg_player_input_context_t * ctx,
                         bg_plugin_handle_t * handle,
                         int track_index)
  {
  int i;
  
  ctx->plugin_handle = handle;
  //  bg_plugin_ref(ctx->plugin_handle);
  
  ctx->plugin = (bg_input_plugin_t*)(handle->plugin);
  ctx->priv = handle->priv;

  if(ctx->plugin->set_callbacks)
    {
    ctx->plugin->set_callbacks(ctx->priv, &(ctx->callbacks));
    }

  ctx->player->track_info = ctx->plugin->get_track_info(ctx->priv,
                                                        track_index);
  
  if(ctx->plugin->seek && ctx->player->track_info->seekable &&
     (ctx->player->track_info->duration > 0))
    ctx->player->can_seek = 1;
  else
    ctx->player->can_seek = 0;
  
  if(!ctx->player->track_info->num_audio_streams &&
     !ctx->player->track_info->num_video_streams &&
     !ctx->player->track_info->num_still_streams)
    {
    fprintf(stderr,
            "Stream has neither audio nor video, skipping\n");
    return 0;
    }
  
  /* Set the track if neccesary */
  
  if(ctx->plugin->set_track)
    ctx->plugin->set_track(ctx->priv, track_index);

  /* Check for bypass mode */
  
  if(ctx->plugin_handle->info->flags & BG_PLUGIN_BYPASS)
    {
    /* Initialize volume for bypass mode */
    bg_player_input_bypass_set_volume(ctx, ctx->player->volume);
    ctx->player->do_bypass = 1;
    }
  else
    ctx->player->do_bypass = 0;
  //  fprintf(stderr, "Bypass mode: %d\n", ctx->player->do_bypass);
  /* Select streams */

  /* Adjust stream indices */
  if(ctx->player->current_audio_stream >= ctx->player->track_info->num_audio_streams)
    ctx->player->current_audio_stream = 0;
  if(ctx->player->current_video_stream >= ctx->player->track_info->num_video_streams)
    ctx->player->current_video_stream = 0;
  if(ctx->player->current_subtitle_stream >= ctx->player->track_info->num_subtitle_streams)
    ctx->player->current_subtitle_stream = 0;
  
  if(ctx->plugin->set_audio_stream)
    {
    for(i = 0; i < ctx->player->track_info->num_audio_streams; i++)
      {
      if(i == ctx->player->current_audio_stream) 
        ctx->plugin->set_audio_stream(ctx->priv, i,
                                      ctx->player->do_bypass ?
                                      BG_STREAM_ACTION_BYPASS :
                                      BG_STREAM_ACTION_DECODE);
      else
        ctx->plugin->set_audio_stream(ctx->priv, i,
                                      BG_STREAM_ACTION_OFF);
      }
    }
  if(ctx->plugin->set_video_stream)
    {
    for(i = 0; i < ctx->player->track_info->num_video_streams; i++)
      {
      if(i == ctx->player->current_video_stream) 
        ctx->plugin->set_video_stream(ctx->priv, i,
                                      ctx->player->do_bypass ?
                                      BG_STREAM_ACTION_BYPASS :
                                      BG_STREAM_ACTION_DECODE);
      else
        ctx->plugin->set_video_stream(ctx->priv, i,
                                      BG_STREAM_ACTION_OFF);
      }
    }
  if(ctx->plugin->set_still_stream)
    {
    for(i = 0; i < ctx->player->track_info->num_still_streams; i++)
      {
      if(i == ctx->player->current_still_stream) 
        ctx->plugin->set_still_stream(ctx->priv, i,
                                      ctx->player->do_bypass ?
                                      BG_STREAM_ACTION_BYPASS :
                                      BG_STREAM_ACTION_DECODE);
      else
        ctx->plugin->set_still_stream(ctx->priv, i,
                                      BG_STREAM_ACTION_OFF);
      }
    }
  
  /* Start input plugin, so we get the formats */
  
  if(ctx->plugin->start)
    {
    if(!ctx->plugin->start(ctx->priv))
      {
      fprintf(stderr, "start() failed\n");
      return 0;
      }
    }
  return 1;
  }

void bg_player_input_cleanup(bg_player_input_context_t * ctx)
  {

  if(ctx->plugin->stop)
    ctx->plugin->stop(ctx->priv);
  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  ctx->plugin_handle = NULL;
  ctx->plugin = NULL;
  
  //  if(!(ctx->plugin_handle->info->flags & BG_PLUGIN_REMOVABLE))
  //    ctx->plugin->close(ctx->priv);
  }

int bg_player_input_set_audio_stream(bg_player_input_context_t * ctx,
                                     int audio_stream)
  {
  if((audio_stream < 0) ||
     (audio_stream > ctx->player->track_info->num_audio_streams-1))
    {
    return 0;
    }
  gavl_audio_format_copy(&(ctx->player->audio_stream.input_format),
                         &(ctx->player->track_info->audio_streams[audio_stream].format));
  return 1;
  }

int bg_player_input_set_video_stream(bg_player_input_context_t * ctx,
                                     int video_stream)
  {
  if((video_stream < 0) ||
     (video_stream > ctx->player->track_info->num_video_streams-1))
    {
    return 0;
    }

  gavl_video_format_copy(&(ctx->player->video_stream.input_format),
                         &(ctx->player->track_info->video_streams[video_stream].format));
  return 1;
  }

int bg_player_input_set_still_stream(bg_player_input_context_t * ctx,
                                     int still_stream)
  {
  if((still_stream < 0) ||
     (still_stream > ctx->player->track_info->num_still_streams-1))
    {
    return 0;
    }

  gavl_video_format_copy(&(ctx->player->video_stream.input_format),
                         &(ctx->player->track_info->still_streams[still_stream].format));
  return 1;
  }


/*
 *  Read audio frames from the input plugin until one frame for
 *  the output plugin is ready
 */

static int process_audio(bg_player_input_context_t * ctx, int preload)
  {
  gavl_audio_frame_t * audio_frame;
  bg_player_audio_stream_t * s;
  bg_fifo_state_t state;
  s = &(ctx->player->audio_stream);
  
  //  fprintf(stderr, "Process audio\n");

  if(ctx->send_silence)
    {
    if(preload)
      audio_frame = (gavl_audio_frame_t*)bg_fifo_try_lock_write(s->fifo, &state);
    else
      audio_frame = (gavl_audio_frame_t*)bg_fifo_lock_write(s->fifo, &state);
    if(!audio_frame)
      return 0;
    gavl_audio_frame_mute(audio_frame, &(ctx->player->audio_stream.output_format));
    audio_frame->valid_samples =
      ctx->player->audio_stream.output_format.samples_per_frame;
    ctx->audio_samples_written += audio_frame->valid_samples;
#if 0
    fprintf(stderr, "Rate: %d\n",
            ctx->player->audio_stream.input_format.samplerate);
#endif
    ctx->audio_time =
      gavl_samples_to_time(ctx->player->audio_stream.input_format.samplerate,
                           ctx->audio_samples_written);
        
    bg_fifo_unlock_write(s->fifo, 0);
    
    return 1;
    }
  if(s->do_convert)
    {
    if(preload)
      audio_frame = (gavl_audio_frame_t*)bg_fifo_try_lock_write(s->fifo,
                                                                &state);
    else
      audio_frame = (gavl_audio_frame_t*)bg_fifo_lock_write(s->fifo, &state);
    
    if(!audio_frame)
      return 0;

    bg_plugin_lock(ctx->plugin_handle);
    if(!ctx->plugin->read_audio_samples(ctx->priv, s->frame,
                                        ctx->player->current_audio_stream,
                                        ctx->player->audio_stream.input_format.samples_per_frame))
      ctx->audio_finished = 1;
    ctx->audio_samples_written += s->frame->valid_samples;
    bg_plugin_unlock(ctx->plugin_handle);
    //    fprintf(stderr, "Convert audio %d\n", s->frame->valid_samples);
    
    gavl_audio_convert(s->cnv, s->frame, audio_frame);
    }
  else
    {
    if(preload)
      audio_frame = (gavl_audio_frame_t*)bg_fifo_try_lock_write(s->fifo,
                                                                &state);
    else
      audio_frame = (gavl_audio_frame_t*)bg_fifo_lock_write(s->fifo, &state);
    
    if(!audio_frame)
      return 0;

    if(!ctx->plugin->read_audio_samples(ctx->priv, audio_frame, 0,
                                        ctx->player->audio_stream.input_format.samples_per_frame))
      ctx->audio_finished = 1;
    ctx->audio_samples_written += audio_frame->valid_samples;
    }
  bg_fifo_unlock_write(s->fifo, (ctx->audio_finished && ctx->video_finished));
#if 0
  fprintf(stderr, "Rate: %d\n",
          ctx->player->audio_stream.input_format.samplerate);
#endif
  ctx->audio_time =
    gavl_samples_to_time(ctx->player->audio_stream.input_format.samplerate,
                         ctx->audio_samples_written);
  //  if(ctx->audio_finished)
  //    fprintf(stderr, "ctx->audio_finished\n");

  //  fprintf(stderr, "Process audio done\n");

  return !ctx->audio_finished;
  }

static int process_video(bg_player_input_context_t * ctx, int preload)
  {
  int result;
  bg_fifo_state_t state;
  gavl_video_frame_t * video_frame;
  bg_player_video_stream_t * s;
  //  fprintf(stderr, "Process video %d...", preload);
  s = &(ctx->player->video_stream);
  
  if(s->do_convert)
    {
    if(preload)
      video_frame = (gavl_video_frame_t*)bg_fifo_try_lock_write(s->fifo,
                                                                &state);
    else
      video_frame = (gavl_video_frame_t*)bg_fifo_lock_write(s->fifo, &state);
    
    if(!video_frame)
      {
      return 0;
      }
    bg_plugin_lock(ctx->plugin_handle);
    result = ctx->plugin->read_video_frame(ctx->priv,
                                           ctx->player->video_stream.frame,
                                           0);
    bg_plugin_unlock(ctx->plugin_handle);
    if(!result)
      ctx->video_finished = 1;
    if(ctx->player->video_stream.input_format.framerate_mode != GAVL_FRAMERATE_STILL)
      ctx->video_time = gavl_time_unscale(ctx->player->video_stream.input_format.timescale,
                                          ctx->player->video_stream.frame->time_scaled);
    gavl_video_convert(s->cnv, ctx->player->video_stream.frame, video_frame);
#if 0
    fprintf(stderr, "Video Frame time: %lld %lld\n",
            ctx->player->video_stream.frame->time,
            video_frame->time);
#endif
    }
  else
    {
    if(preload)
      video_frame = (gavl_video_frame_t*)bg_fifo_try_lock_write(s->fifo,
                                                                &state);
    else
      video_frame = (gavl_video_frame_t*)bg_fifo_lock_write(s->fifo, &state);
    
    if(!video_frame)
      return 0;
    //    fprintf(stderr, "Got frame\n");
    bg_plugin_lock(ctx->plugin_handle);
    result = ctx->plugin->read_video_frame(ctx->priv, video_frame, 0);
    bg_plugin_unlock(ctx->plugin_handle);
    if(!result)
      ctx->video_finished = 1;
    ctx->video_time = gavl_time_unscale(ctx->player->video_stream.input_format.timescale, video_frame->time_scaled);
    ctx->video_frames_written++;
    }
  bg_fifo_unlock_write(s->fifo, ctx->video_finished);
  //  fprintf(stderr, "done %d\n", ctx->video_finished);
  return !ctx->video_finished;
  }

void * bg_player_input_thread(void * data)
  {
  bg_player_input_context_t * ctx;
  bg_msg_t * msg;
  bg_fifo_state_t state;
  int do_audio;
  int do_video;
  
  int read_audio;
  
  ctx = (bg_player_input_context_t*)data;

  do_audio = ctx->player->do_audio;
  do_video = ((ctx->player->do_video) || (ctx->player->do_still));
  
  ctx->audio_finished = !do_audio;
  ctx->video_finished = !do_video;
  ctx->send_silence = 0;
  ctx->audio_samples_written = 0;
  
  ctx->audio_time = 0;
  ctx->video_time = 0;

  read_audio = 0;

  if(do_audio && !do_video)
    read_audio = 1;

  //  fprintf(stderr, "input thread started\n");
  
  while(1)
    {
    if(!bg_player_keep_going(ctx->player, NULL, NULL))
      {
      return NULL;
      }
    
    /* Check for EOF here */
    if(ctx->audio_finished && ctx->video_finished)
      break;
    
    if(do_audio && do_video)
      {
#if 0
      fprintf(stderr, "A: %f, V: %f\n",
              gavl_time_to_seconds(ctx->audio_time),
              gavl_time_to_seconds(ctx->video_time));
#endif        
            
      //      if(ctx->audio_finished)
      //        read_audio = 0;
      //      else
        if(ctx->video_finished)
          read_audio = 1;
      else if(ctx->audio_time > ctx->video_time)
        {
        /* Read video */
        read_audio = 0;
        }
      else
        {
        read_audio = 1;
        }
      }
    if(read_audio)
      {
      process_audio(ctx, 0);
      }
    else
      {
      process_video(ctx, 0);
      }
    if(do_audio && do_video && ctx->audio_finished && !ctx->video_finished)
      ctx->send_silence = 1;

    /* If we sent silence before, we must tell the audio fifo EOF */
        
    if(ctx->send_silence && ctx->audio_finished && ctx->video_finished)
      {
      bg_fifo_lock_write(ctx->player->audio_stream.fifo, &state);
      bg_fifo_unlock_write(ctx->player->audio_stream.fifo, 1);
      }
    }
  msg = bg_msg_queue_lock_write(ctx->player->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_SETSTATE);

  if(ctx->player->do_still && !ctx->player->do_audio)
    bg_msg_set_arg_int(msg, 0, BG_PLAYER_STATE_STILL);
  else
    bg_msg_set_arg_int(msg, 0, BG_PLAYER_STATE_FINISHING);
  
  bg_msg_queue_unlock_write(ctx->player->command_queue);
  
  fprintf(stderr, "input thread finished\n");
  return NULL;
  }

void * bg_player_input_thread_bypass(void * data)
  {
  bg_msg_t * msg;
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t*)data;

  gavl_time_t delay_time = GAVL_TIME_SCALE / 20;
    
  while(1)
    {
    if(!bg_player_keep_going(ctx->player, NULL, NULL))
      {
      return NULL;
      }
    bg_plugin_lock(ctx->plugin_handle);

    if(ctx->plugin->bypass && !ctx->plugin->bypass(ctx->priv))
      {
      bg_plugin_unlock(ctx->plugin_handle);
      break;
      }
    bg_plugin_unlock(ctx->plugin_handle);
    
    gavl_time_delay(&delay_time);
    }

  msg = bg_msg_queue_lock_write(ctx->player->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_SETSTATE);
  bg_msg_set_arg_int(msg, 0, BG_PLAYER_STATE_FINISHING);
  bg_msg_queue_unlock_write(ctx->player->command_queue);
  return NULL;
  }


void bg_player_input_preload(bg_player_input_context_t * ctx)
  {
  int do_audio;
  int do_video;
  int do_still;

  do_audio = ctx->player->do_audio;
  do_video = ctx->player->do_video;
  do_still = ctx->player->do_still;

  if(do_still)
    {
    process_video(ctx, 1);
    ctx->video_finished = 1;
    }
  
  while(do_audio || do_video)
    {
    if(do_audio)
      do_audio = process_audio(ctx, 1);
    if(do_video)
      do_video = process_video(ctx, 1);
    }
  }

void bg_player_input_seek(bg_player_input_context_t * ctx,
                          gavl_time_t * time)
  {
  bg_plugin_lock(ctx->plugin_handle);
  //  fprintf(stderr, "bg_player_input_seek\n");
  ctx->plugin->seek(ctx->priv, time);
  bg_plugin_unlock(ctx->plugin_handle);

  ctx->audio_time = *time;
  ctx->video_time = *time;
  
  ctx->audio_samples_written =
    gavl_time_to_samples(ctx->player->audio_stream.input_format.samplerate,
                         ctx->audio_time);
  
  }

const char * bg_player_input_get_error(bg_player_input_context_t * ctx)
  {
  if(ctx->plugin->common.get_error)
    return ctx->plugin->common.get_error(ctx->priv);
  return (char*)0;
  }


void bg_player_input_bypass_set_volume(bg_player_input_context_t * ctx,
                                       float volume)
  {
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->bypass_set_volume)
    ctx->plugin->bypass_set_volume(ctx->priv, volume);
  bg_plugin_unlock(ctx->plugin_handle);
  }

void bg_player_input_bypass_set_pause(bg_player_input_context_t * ctx,
                                      int pause)
  {
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->bypass_set_pause)
    ctx->plugin->bypass_set_pause(ctx->priv, pause);
  bg_plugin_unlock(ctx->plugin_handle);
  }

