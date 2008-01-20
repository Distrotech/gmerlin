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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <player.h>
#include <playerprivate.h>
#include <log.h>

#define LOG_DOMAIN "player.input"

// #define DUMP_TIMESTAMPS

struct bg_player_input_context_s
  {
  /* Plugin stuff */
  
  bg_plugin_handle_t * plugin_handle;
  bg_input_plugin_t  * plugin;
  void * priv;

  int audio_finished;
  int video_finished;
  int subtitle_finished;
  int send_silence;
    
  int64_t audio_samples_written;
  int64_t video_frames_written;

  int has_first_audio_timestamp;
  
  /* Internal times */

  gavl_time_t audio_time;
  gavl_time_t video_time;
  
  /* For changing global stuff */
  
  bg_player_t * player;

  /* Callback structure */

  bg_input_callbacks_t callbacks;
  
  /* Config stuff */
  pthread_mutex_t config_mutex;
  int do_bypass;
  float still_framerate;
  
  int current_track;
  
  float bg_color[4];
  
  gavl_video_frame_t * still_frame;
  
  int do_still;
  };

static void track_changed(void * data, int track)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;


  bg_player_set_track(ctx->player, track);
  
  }

static void time_changed(void * data, gavl_time_t time)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;

  bg_player_time_set(ctx->player, time);
  }

static void duration_changed(void * data, gavl_time_t duration)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;


  bg_player_set_duration(ctx->player, duration, ctx->player->can_seek);


  //  bg_player_time_set(ctx->player, time);
  }

static void name_changed(void * data, const char * name)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;
  bg_player_set_track_name(ctx->player, name);
  }

static void metadata_changed(void * data, const bg_metadata_t * m)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;
  bg_player_set_metadata(ctx->player, m);
  }

static void buffer_notify(void * data, float percentage)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;
  bg_player_set_state(ctx->player, BG_PLAYER_STATE_BUFFERING,
                      &percentage, NULL);
  }

static void aspect_changed(void * data, int stream, int pixel_width,
                           int pixel_height)
  {
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)data;

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Aspect ratio changed");

  bg_player_ov_update_aspect(ctx->player->ov_context,
                             pixel_width, pixel_height);
  
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
  ctx->callbacks.aspect_changed   = aspect_changed;
  
  ctx->player = player;
  
  player->input_context = ctx;
  pthread_mutex_init(&(player->input_context->config_mutex),(pthread_mutexattr_t *)0);
  }

void bg_player_input_destroy(bg_player_t * player)
  {
  if(player->input_context->plugin_handle)
    bg_plugin_unref(player->input_context->plugin_handle);
  pthread_mutex_destroy(&(player->input_context->config_mutex));
  free(player->input_context);
  }

void bg_player_input_select_streams(bg_player_input_context_t * ctx)
  {
  int i, vis_enable;
  ctx->do_still = 0;
  if(ctx->still_frame)
    {
    gavl_video_frame_destroy(ctx->still_frame);
    ctx->still_frame = (gavl_video_frame_t*)0;
    }
  
  /* Adjust stream indices */
  if(ctx->player->current_audio_stream >= ctx->player->track_info->num_audio_streams)
    ctx->player->current_audio_stream = 0;

  if(ctx->player->current_video_stream >= ctx->player->track_info->num_video_streams)
    ctx->player->current_video_stream = 0;

  if(ctx->player->current_subtitle_stream >= ctx->player->track_info->num_subtitle_streams)
    ctx->player->current_subtitle_stream = 0;

  if(!bg_player_oa_has_plugin(ctx->player->oa_context))
    ctx->player->current_audio_stream = -1;

  if(!bg_player_ov_has_plugin(ctx->player->ov_context))
    {
    ctx->player->current_video_stream = -1;
    ctx->player->current_subtitle_stream = -1;
    }
  
  /* Check if the streams are actually there */
  ctx->player->flags = 0;
  
  ctx->audio_finished = 1;
  ctx->video_finished = 1;
  ctx->subtitle_finished = 1;

  if(!ctx->player->do_bypass)
    {
    if((ctx->player->current_audio_stream >= 0) &&
       (ctx->player->current_audio_stream < ctx->player->track_info->num_audio_streams))
      {
      ctx->audio_finished = 0;
      ctx->player->flags |= PLAYER_DO_AUDIO;
      }
    if((ctx->player->current_video_stream >= 0) &&
       (ctx->player->current_video_stream < ctx->player->track_info->num_video_streams))
      {
      ctx->video_finished = 0;
      ctx->player->flags |= PLAYER_DO_VIDEO;
      }

    if((ctx->player->current_subtitle_stream >= 0) &&
       (ctx->player->current_subtitle_stream < ctx->player->track_info->num_subtitle_streams))
      {
      ctx->player->flags |= PLAYER_DO_SUBTITLE;
      ctx->subtitle_finished = 0;
      }
    }
  
  pthread_mutex_lock(&ctx->player->config_mutex);
  vis_enable = ctx->player->visualizer_enabled;
  pthread_mutex_unlock(&ctx->player->config_mutex);
  
  
  if(DO_AUDIO(ctx->player->flags) &&
     !DO_VIDEO(ctx->player->flags) &&
     !DO_SUBTITLE(ctx->player->flags) &&
     vis_enable)
    {
    ctx->player->flags |= PLAYER_DO_VISUALIZE;
    }
  else if(!DO_VIDEO(ctx->player->flags) &&
          DO_SUBTITLE(ctx->player->flags))
    {
    ctx->player->flags |= PLAYER_DO_VIDEO;
    ctx->video_finished = 0;
    
    ctx->player->flags |= PLAYER_DO_SUBTITLE_ONLY;
    
    pthread_mutex_lock(&(ctx->player->video_stream.config_mutex));
    /* Get background color */
    gavl_video_options_get_background_color(ctx->player->video_stream.options.opt,
                                            ctx->bg_color);
    pthread_mutex_unlock(&(ctx->player->video_stream.config_mutex));
    
    ctx->bg_color[3] = 1.0;
    }
  
  /* En-/Disable streams at the input plugin */
  
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

  if(ctx->plugin->set_subtitle_stream)
    {
    for(i = 0; i < ctx->player->track_info->num_subtitle_streams; i++)
      {
      if(i == ctx->player->current_subtitle_stream) 
        ctx->plugin->set_subtitle_stream(ctx->priv, i,
                                      ctx->player->do_bypass ?
                                      BG_STREAM_ACTION_BYPASS :
                                      BG_STREAM_ACTION_DECODE);
      else
        ctx->plugin->set_subtitle_stream(ctx->priv, i,
                                         BG_STREAM_ACTION_OFF);
      }
    }
  }

int bg_player_input_start(bg_player_input_context_t * ctx)
  {
  gavl_video_format_t * video_format;
  
  /* Start input plugin, so we get the formats */
  if(ctx->plugin->start)
    {
    if(!ctx->plugin->start(ctx->priv))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Starting input plugin failed");
      return 0;
      }
    }

  /* Subtitle type must be set here, because it's unknown before the
     start() call */

  if(DO_SUBTITLE(ctx->player->flags))
    {
    if(ctx->player->track_info->subtitle_streams[ctx->player->current_subtitle_stream].is_text)
      ctx->player->flags |= PLAYER_DO_SUBTITLE_TEXT;
    else
      ctx->player->flags |= PLAYER_DO_SUBTITLE_OVERLAY;
    }
  
  /* Check for still image mode */
  
  if(ctx->player->flags & PLAYER_DO_VIDEO)
    {
    video_format =
      &ctx->player->track_info->video_streams[ctx->player->current_video_stream].format;
    
    if(video_format->framerate_mode == GAVL_FRAMERATE_STILL)
      ctx->do_still = 1;
    }
  ctx->has_first_audio_timestamp = 0;
  return 1;
  }

void bg_player_input_stop(bg_player_input_context_t * ctx)
  {
  if(ctx->plugin->stop)
    ctx->plugin->stop(ctx->priv);
  }

int bg_player_input_set_track(bg_player_input_context_t * ctx)
  {
  if(ctx->plugin->set_track)
    {
    if(!ctx->plugin->set_track(ctx->priv, ctx->current_track))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Cannot select track, skipping");
      return 0;
      }
    }
  return 1;
  }

int bg_player_input_init(bg_player_input_context_t * ctx,
                         bg_plugin_handle_t * handle,
                         int track_index)
  {
  int do_bypass;

  pthread_mutex_lock(&(ctx->config_mutex));
  do_bypass = ctx->do_bypass;
  pthread_mutex_unlock(&(ctx->config_mutex));
  
  ctx->plugin_handle = handle;
  ctx->current_track = track_index;
  
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
     !ctx->player->track_info->num_video_streams)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
            "Stream has neither audio nor video, skipping");
    return 0;
    }
  
  /* Set the track if neccesary */

  if(!bg_player_input_set_track(ctx))
    return 0;
  
  /* Check for bypass mode */
  
  if(do_bypass && (ctx->plugin_handle->info->flags & BG_PLUGIN_BYPASS))
    {
    /* Initialize volume for bypass mode */
    bg_player_input_bypass_set_volume(ctx, ctx->player->volume);
    ctx->player->do_bypass = 1;
    }
  else
    ctx->player->do_bypass = 0;

  /* Select streams */
  bg_player_input_select_streams(ctx);
  
  /* Start input plugin, so we get the formats */
  
  if(!bg_player_input_start(ctx))
    {
    return 0;
    }
  return 1;
  }

void bg_player_input_cleanup(bg_player_input_context_t * ctx)
  {
  bg_player_input_stop(ctx);
  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  ctx->plugin_handle = NULL;
  ctx->plugin = NULL;

  if(ctx->still_frame)
    {
    gavl_video_frame_destroy(ctx->still_frame);
    ctx->still_frame = (gavl_video_frame_t*)0;
    }
  }

int bg_player_input_get_audio_format(bg_player_input_context_t * ctx)
  {
  gavl_audio_format_copy(&(ctx->player->audio_stream.input_format),
                         &(ctx->player->track_info->audio_streams[ctx->player->current_audio_stream].format));
  return 1;
  }

int bg_player_input_get_video_format(bg_player_input_context_t * ctx)
  {
  gavl_video_format_copy(&(ctx->player->video_stream.input_format),
                         &(ctx->player->track_info->video_streams[ctx->player->current_video_stream].format));

  if(ctx->do_still)
    {
    ctx->player->video_stream.input_format.timescale = GAVL_TIME_SCALE;
    pthread_mutex_lock(&ctx->config_mutex);
    ctx->player->video_stream.input_format.frame_duration =
      (int)((float)GAVL_TIME_SCALE / ctx->still_framerate);
    pthread_mutex_unlock(&ctx->config_mutex);
    }
  
  return 1;
  }

int
bg_player_input_get_subtitle_format(bg_player_input_context_t * ctx)
  {
  if(!ctx->player->track_info->subtitle_streams[ctx->player->current_subtitle_stream].is_text)
    {
    gavl_video_format_copy(&(ctx->player->subtitle_stream.input_format),
                           &(ctx->player->track_info->subtitle_streams[ctx->player->current_subtitle_stream].format));
    
    }
  return 1;
  }

int
bg_player_input_read_audio(void * priv, gavl_audio_frame_t * frame, int stream, int samples)
  {
  int result;
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)priv;
  
  bg_plugin_lock(ctx->plugin_handle);
  result = ctx->plugin->read_audio(ctx->priv, frame, stream, samples);
  bg_plugin_unlock(ctx->plugin_handle);
  
  if(!ctx->has_first_audio_timestamp)
    {
    ctx->audio_samples_written = frame->timestamp;
    ctx->has_first_audio_timestamp = 1;
    }
  ctx->audio_samples_written += frame->valid_samples;
  
  return result;
  }

int
bg_player_input_read_video(void * priv, gavl_video_frame_t * frame, int stream)
  {
  int result;
  bg_player_input_context_t * ctx;
  ctx = (bg_player_input_context_t *)priv;

  if(ctx->do_still)
    {
    gavl_video_format_t * format;
    format = &ctx->player->video_stream.input_format;
    if(!ctx->still_frame)
      {
      ctx->still_frame = gavl_video_frame_create(format);
      bg_plugin_lock(ctx->plugin_handle);
      result = ctx->plugin->read_video(ctx->priv, ctx->still_frame,
                                             stream);
      bg_plugin_unlock(ctx->plugin_handle);
      ctx->still_frame->timestamp = 0;
      if(!result)
        return 0;
      }
    
    gavl_video_frame_copy(format, frame, ctx->still_frame);
    frame->timestamp = ctx->still_frame->timestamp;
    ctx->still_frame->timestamp += format->frame_duration;
    result = 1;
    }
  else
    {
    bg_plugin_lock(ctx->plugin_handle);
    result = ctx->plugin->read_video(ctx->priv, frame, stream);
    bg_plugin_unlock(ctx->plugin_handle);
#ifdef DUMP_TIMESTAMPS
    bg_debug("Input timestamp: %"PRId64"\n",
             gavl_time_unscale(ctx->player->video_stream.input_format.timescale,
                               frame->timestamp));
#endif
    }
  return result;
  }

int
bg_player_input_read_video_subtitle_only(void * priv, gavl_video_frame_t * frame, int stream)
  {
  bg_player_input_context_t * ctx;
  bg_player_video_stream_t * s;
  ctx = (bg_player_input_context_t *)priv;
  s = &(ctx->player->video_stream);
  
  gavl_video_frame_fill(frame, &s->output_format, ctx->bg_color);
  
  frame->timestamp = (int64_t)ctx->video_frames_written *
    ctx->player->video_stream.output_format.frame_duration;
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

  if(ctx->send_silence)
    {
    if(preload)
      audio_frame = (gavl_audio_frame_t*)bg_fifo_try_lock_write(s->fifo, &state);
    else
      audio_frame = (gavl_audio_frame_t*)bg_fifo_lock_write(s->fifo, &state);
    if(!audio_frame)
      return 0;
    gavl_audio_frame_mute(audio_frame, &(ctx->player->audio_stream.fifo_format));
    audio_frame->valid_samples =
      ctx->player->audio_stream.fifo_format.samples_per_frame;
    ctx->audio_samples_written += audio_frame->valid_samples;
    ctx->audio_time =
      gavl_samples_to_time(ctx->player->audio_stream.input_format.samplerate,
                           ctx->audio_samples_written);
        
    bg_fifo_unlock_write(s->fifo, 0);
    
    return 1;
    }

  if(preload)
    audio_frame = (gavl_audio_frame_t*)bg_fifo_try_lock_write(s->fifo,
                                                              &state);
  else
    audio_frame = (gavl_audio_frame_t*)bg_fifo_lock_write(s->fifo, &state);
  
  if(!audio_frame)
    return 0;
  
  if(!s->in_func(s->in_data, audio_frame, s->in_stream,
                 s->fifo_format.samples_per_frame))
    ctx->audio_finished = 1;
  
  if(ctx->audio_finished &&
     (!ctx->video_finished || !ctx->subtitle_finished))
    ctx->send_silence = 1;

  bg_fifo_unlock_write(s->fifo,
                       ctx->audio_finished &&
                       ctx->video_finished &&
                       ctx->subtitle_finished);
  
  
  ctx->audio_time =
    gavl_samples_to_time(ctx->player->audio_stream.input_format.samplerate,
                         ctx->audio_samples_written);

  
  return !ctx->audio_finished;
  }

static int process_subtitle(bg_player_input_context_t * ctx)
  {
  //  char time_string[GAVL_TIME_STRING_LEN];
  gavl_overlay_t * ovl;
  gavl_time_t start, duration;
  bg_player_subtitle_stream_t * s;
  bg_fifo_state_t state;
  
  s = &(ctx->player->subtitle_stream);


  
  if(ctx->plugin->has_subtitle(ctx->priv,
                               ctx->player->current_subtitle_stream))
    {
    /* Try to get an overlay */
    ovl = (gavl_overlay_t*)bg_fifo_try_lock_write(s->fifo, &state);
    
    if(!ovl)
      {
      return 0;
      }
    if(DO_SUBTITLE_TEXT(ctx->player->flags))
      {
      if(!ctx->plugin->read_subtitle_text(ctx->priv,
                                          &(s->buffer),
                                          &(s->buffer_alloc),
                                          &start, &duration,
                                          ctx->player->current_subtitle_stream))
        {
        ctx->subtitle_finished = 1;
        return 0;
        }
      if(s->do_convert)
        {
        bg_text_renderer_render(s->renderer, s->buffer, &s->in_ovl);
        gavl_video_convert(s->cnv, s->in_ovl.frame, ovl->frame);
        gavl_rectangle_i_copy(&ovl->ovl_rect, &s->in_ovl.ovl_rect);
        ovl->dst_x = s->in_ovl.dst_x;
        ovl->dst_y = s->in_ovl.dst_y;
        }
      else
        {
        bg_text_renderer_render(s->renderer, s->buffer, ovl);
        }
      ovl->frame->timestamp = start;
      ovl->frame->duration = duration;
      //      return 1;
      }
    else
      {
      if(s->do_convert)
        {
        if(!ctx->plugin->read_subtitle_overlay(ctx->priv, &s->in_ovl,
                                               ctx->player->current_subtitle_stream))
          {
          ctx->subtitle_finished = 1;
          return 0;
          }
        gavl_video_convert(s->cnv, s->in_ovl.frame, ovl->frame);
        gavl_rectangle_i_copy(&ovl->ovl_rect, &s->in_ovl.ovl_rect);
        ovl->dst_x = s->in_ovl.dst_x;
        ovl->dst_y = s->in_ovl.dst_y;
        }
      else
        {
        if(!ctx->plugin->read_subtitle_overlay(ctx->priv, ovl,
                                               ctx->player->current_subtitle_stream))
          {
          ctx->subtitle_finished = 1;
          return 0;
          }
        }
      }
    bg_fifo_unlock_write(s->fifo, 0); /* Subtitle streams never signal eof! */
    }
  
  return 1;
  }

static int process_video(bg_player_input_context_t * ctx, int preload)
  {
  int result;
  bg_fifo_state_t state;
  gavl_video_frame_t * video_frame;
  bg_player_video_stream_t * s;
  s = &(ctx->player->video_stream);

  if(preload || DO_SUBTITLE_ONLY(ctx->player->flags))
    video_frame = (gavl_video_frame_t*)bg_fifo_try_lock_write(s->fifo,
                                                              &state);
  else
    video_frame = (gavl_video_frame_t*)bg_fifo_lock_write(s->fifo, &state);
  
  if(!video_frame)
    return 0;
  
  result = s->in_func(s->in_data, video_frame, s->in_stream);
  
  if(!result || (DO_SUBTITLE_ONLY(ctx->player->flags) &&
                 ctx->subtitle_finished && ctx->audio_finished))
    ctx->video_finished = 1;
  else
    ctx->video_frames_written++;
  
  if(ctx->player->video_stream.input_format.framerate_mode != GAVL_FRAMERATE_STILL)
    ctx->video_time = gavl_time_unscale(ctx->player->video_stream.input_format.timescale,
                                        video_frame->timestamp);
  
  bg_fifo_unlock_write(s->fifo, ctx->video_finished);
  
  return !ctx->video_finished;
  }

void * bg_player_input_thread(void * data)
  {
  char tmp_string[128];
  
  bg_player_input_context_t * ctx;
  bg_msg_t * msg;
  bg_fifo_state_t state;
  
  int read_audio;
  
  ctx = (bg_player_input_context_t*)data;
  
  ctx->send_silence = 0;
  ctx->audio_samples_written = 0;
  ctx->video_frames_written = 0;
  
  ctx->audio_time = 0;
  ctx->video_time = 0;

  if(!DO_AUDIO(ctx->player->flags) && !DO_VIDEO(ctx->player->flags))
    read_audio = 0;
  else if(DO_AUDIO(ctx->player->flags) && !DO_VIDEO(ctx->player->flags))
    read_audio = 1;
  else
    read_audio = 0; // Decide inside loop
  
  while(1)
    {
    if(!bg_player_keep_going(ctx->player, NULL, NULL))
      return NULL;
    
    /* Read subtitles early */
    if(!ctx->subtitle_finished)
      process_subtitle(ctx);
    
    /* Check for EOF here */
    if(ctx->audio_finished && ctx->video_finished)
      {
      if(ctx->send_silence)
        {
        bg_fifo_lock_write(ctx->player->audio_stream.fifo, &state);
        bg_fifo_unlock_write(ctx->player->audio_stream.fifo, 1);
        }
      break;
      }

    if(!DO_SUBTITLE_ONLY(ctx->player->flags))
      {
      if(DO_AUDIO(ctx->player->flags) &&
         DO_VIDEO(ctx->player->flags))
        {
        if(ctx->video_finished)
          read_audio = 1;
        else if(ctx->audio_time > ctx->video_time)
          read_audio = 0;
        else
          read_audio = 1;
        }
      if(read_audio)
        process_audio(ctx, 0);
      else
        process_video(ctx, 0);
      }
    else
      {
      process_video(ctx, 0);
      process_audio(ctx, 0);
      }
    /* If we sent silence before, we must tell the audio fifo EOF */
    
    if(ctx->send_silence && ctx->audio_finished && ctx->video_finished)
      {
      bg_fifo_lock_write(ctx->player->audio_stream.fifo, &state);
      bg_fifo_unlock_write(ctx->player->audio_stream.fifo, 1);
      }
    }
  msg = bg_msg_queue_lock_write(ctx->player->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_SETSTATE);
  
  bg_msg_set_arg_int(msg, 0, BG_PLAYER_STATE_FINISHING);
  
  bg_msg_queue_unlock_write(ctx->player->command_queue);

  if(DO_AUDIO(ctx->player->flags))
    {
    sprintf(tmp_string, "%" PRId64, ctx->audio_samples_written);
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Processed %s audio samples",
           tmp_string);
    }
  if(DO_VIDEO(ctx->player->flags))
    {
    sprintf(tmp_string, "%" PRId64, ctx->video_frames_written);
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Processed %s video frames",
           tmp_string);
    }
  return NULL;
  }

void * bg_player_input_thread_bypass(void * data)
  {
  bg_msg_t * msg;
  bg_player_input_context_t * ctx;
  gavl_time_t delay_time = GAVL_TIME_SCALE / 20;

  ctx = (bg_player_input_context_t*)data;

    
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
  int do_subtitle;
  
  do_audio = !!DO_AUDIO(ctx->player->flags);
  do_video = !!DO_VIDEO(ctx->player->flags);
  do_subtitle = !!DO_SUBTITLE(ctx->player->flags);
  
  while(do_audio || do_video)
    {
    if(do_subtitle)
      do_subtitle = process_subtitle(ctx);
    if(do_audio)
      do_audio = process_audio(ctx, 1);
    if(do_video)
      do_video = process_video(ctx, 1);
    }
  }

void bg_player_input_seek(bg_player_input_context_t * ctx,
                          gavl_time_t * time)
  {
  int do_audio, do_video, do_subtitle;
  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->seek(ctx->priv, time);
  bg_plugin_unlock(ctx->plugin_handle);

  ctx->audio_time = *time;
  ctx->video_time = *time;
  
  ctx->audio_samples_written =
    gavl_time_to_samples(ctx->player->audio_stream.input_format.samplerate,
                         ctx->audio_time);

  // This wasn't set before if we switch streams or replug filters
  ctx->has_first_audio_timestamp = 1;
  
  if(DO_SUBTITLE_ONLY(ctx->player->flags))
    ctx->video_frames_written =
      gavl_time_to_frames(ctx->player->video_stream.output_format.timescale,
                          ctx->player->video_stream.output_format.frame_duration,
                          ctx->video_time);
  else
    ctx->video_frames_written =
      gavl_time_to_frames(ctx->player->video_stream.input_format.timescale,
                          ctx->player->video_stream.input_format.frame_duration,
                          ctx->video_time);
  
  // Clear EOF states
  do_audio = DO_AUDIO(ctx->player->flags);
  do_video = DO_VIDEO(ctx->player->flags);
  do_subtitle = DO_SUBTITLE(ctx->player->flags);
  
  ctx->subtitle_finished = !do_subtitle;
  ctx->audio_finished = !do_audio;
  ctx->video_finished = !do_video;
  ctx->send_silence = 0;
  
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

/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
#if 0
    {
      .name =      "input",
      .long_name = TRS("Input options"),
      .type =      BG_PARAMETER_SECTION,
    },
#endif
    {
      .name =      "do_bypass",
      .long_name = TRS("Enable bypass mode"),
      .type =      BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Use input plugins in bypass mode if they support it (Currently only the audio CD player).\
 This dramatically decreases CPU usage but doesn't work on all hardware setups.")
    },
    {
      .name =        "still_framerate",
      .long_name =   "Still image repitition rate",
      .type =        BG_PARAMETER_FLOAT,
      .val_default = { .val_f = 10.0 },
      .val_min =     { .val_f = 0.5 },
      .val_max =     { .val_f = 100.0 },
      .help_string = TRS("When showing still images, gmerlin repeats them periodically to make realtime filter tweaking work."),
    },
    { /* End of parameters */ }
  };


const bg_parameter_info_t * bg_player_get_input_parameters(bg_player_t * p)
  {
  return parameters;
  }

void bg_player_set_input_parameter(void * data, const char * name,
                                   const bg_parameter_value_t * val)
  {
  bg_player_t * player = (bg_player_t*)data;

  if(!name)
    return;

  pthread_mutex_lock(&(player->input_context->config_mutex));

  if(!strcmp(name, "do_bypass"))
    player->input_context->do_bypass = val->val_i;
  else if(!strcmp(name, "still_framerate"))
    player->input_context->still_framerate = val->val_f;
  
  pthread_mutex_unlock(&(player->input_context->config_mutex));
  }

struct msg_input_data
  {
  bg_plugin_handle_t * handle;
  int track;
  };

static void msg_input(bg_msg_t * msg, const void * data)
  {
  struct msg_input_data * d = (struct msg_input_data *)data;
  
  bg_msg_set_id(msg, BG_PLAYER_MSG_INPUT);
  
  if(d->handle)
    {
    bg_msg_set_arg_string(msg, 0, d->handle->info->long_name);
    bg_msg_set_arg_string(msg, 1, d->handle->location);
    bg_msg_set_arg_int(msg, 2, d->track);
    }
  }

void bg_player_input_send_messages(bg_player_input_context_t * ctx)
  {
  struct msg_input_data d;
  d.handle = ctx->plugin_handle;
  d.track = ctx->current_track;

  bg_msg_queue_list_send(ctx->player->message_queues, msg_input, &d);
  }
