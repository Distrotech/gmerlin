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

#include <player.h>
#include <playerprivate.h>
#include <log.h>

#define LOG_DOMAIN "player.input"

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

  int current_track;
  
  float bg_color[4];
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
  int i;
  
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
      if(ctx->player->track_info->video_streams[ctx->player->current_video_stream].is_still)
        ctx->player->flags |= PLAYER_DO_STILL;
      else
        ctx->player->flags |= PLAYER_DO_VIDEO;
      ctx->video_finished = 0;
      }
    }
  
  /* En-/Disable strams at the input plugin */
  
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

  if((ctx->player->current_subtitle_stream >= 0) &&
     (ctx->player->current_subtitle_stream < ctx->player->track_info->num_subtitle_streams))
    {
    if(ctx->player->track_info->subtitle_streams[ctx->player->current_subtitle_stream].is_text)
      {
      ctx->player->flags |= PLAYER_DO_SUBTITLE_TEXT;
      }
    else
      {
      ctx->player->flags |= PLAYER_DO_SUBTITLE_OVERLAY;
      }
    ctx->subtitle_finished = 0;

    if(!(ctx->player->flags & PLAYER_DO_VIDEO))
      {
        
      ctx->player->flags |= PLAYER_DO_SUBTITLE_ONLY;
      ctx->player->flags |= PLAYER_DO_VIDEO;
      ctx->video_finished = 0;

      pthread_mutex_lock(&(ctx->player->video_stream.config_mutex));
      /* Get background color */
      gavl_video_options_get_background_color(ctx->player->video_stream.options.opt,
                                              ctx->bg_color);
      pthread_mutex_unlock(&(ctx->player->video_stream.config_mutex));

      ctx->bg_color[3] = 1.0;
      }
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
  return 1;
  }

int
bg_player_input_get_subtitle_format(bg_player_input_context_t * ctx)
  {
  if(!ctx->player->track_info->subtitle_streams[ctx->player->current_subtitle_stream].is_text)
    {
    gavl_video_format_copy(&(ctx->player->subtitle_stream.format),
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
  result = ctx->plugin->read_audio_samples(ctx->priv, frame, stream, samples);
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
  
  bg_plugin_lock(ctx->plugin_handle);
  result = ctx->plugin->read_video_frame(ctx->priv, frame, stream);
  bg_plugin_unlock(ctx->plugin_handle);
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
    gavl_samples_to_time(ctx->player->audio_stream.fifo_format.samplerate,
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
    if(DO_SUBTITLE_TEXT(ctx->player))
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
      bg_text_renderer_render(s->renderer, s->buffer, ovl);
      ovl->frame->timestamp = start;
      ovl->frame->duration = duration;
      //      return 1;
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

  if(preload)
    video_frame = (gavl_video_frame_t*)bg_fifo_try_lock_write(s->fifo,
                                                              &state);
  else
    video_frame = (gavl_video_frame_t*)bg_fifo_lock_write(s->fifo, &state);
  
  if(!video_frame)
    return 0;

  result = s->in_func(s->in_data, video_frame, s->in_stream);
  
  if(!result || (DO_SUBTITLE_ONLY(ctx->player) && ctx->subtitle_finished && ctx->audio_finished))
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
  //  int do_audio;
  //  int do_video;
  //  int do_subtitle;
  
  int read_audio;
  
  ctx = (bg_player_input_context_t*)data;

#if 0  
  do_audio = ctx->player->do_audio;
  do_video = ctx->player->do_video || ctx->player->do_still;
  do_subtitle =
    ctx->player->do_subtitle_overlay ||
    ctx->player->do_subtitle_text;
  
  ctx->audio_finished = !do_audio;
  ctx->video_finished = !do_video;
  ctx->subtitle_finished = !do_subtitle;
#endif  
  ctx->send_silence = 0;
  ctx->audio_samples_written = 0;
  ctx->video_frames_written = 0;
  
  ctx->audio_time = 0;
  ctx->video_time = 0;

  read_audio = 0;

  if(DO_AUDIO(ctx->player) &&
     !(DO_VIDEO(ctx->player) || DO_STILL(ctx->player)))
    read_audio = 1;

  
  while(1)
    {
    if(!bg_player_keep_going(ctx->player, NULL, NULL))
      {
      return NULL;
      }

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
    if(DO_AUDIO(ctx->player) &&
       (DO_VIDEO(ctx->player) || DO_STILL(ctx->player)))
      {
            
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
    
    /* If we sent silence before, we must tell the audio fifo EOF */
    
    if(ctx->send_silence && ctx->audio_finished && ctx->video_finished)
      {
      bg_fifo_lock_write(ctx->player->audio_stream.fifo, &state);
      bg_fifo_unlock_write(ctx->player->audio_stream.fifo, 1);
      }
    }
  msg = bg_msg_queue_lock_write(ctx->player->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_SETSTATE);
  
  if(DO_STILL(ctx->player) && !DO_AUDIO(ctx->player))
    bg_msg_set_arg_int(msg, 0, BG_PLAYER_STATE_STILL);
  else
    bg_msg_set_arg_int(msg, 0, BG_PLAYER_STATE_FINISHING);
  
  bg_msg_queue_unlock_write(ctx->player->command_queue);

  if(DO_AUDIO(ctx->player))
    {
    sprintf(tmp_string, "%" PRId64, ctx->audio_samples_written);
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Processed %s audio samples",
           tmp_string);
    }
  if(DO_VIDEO(ctx->player) || DO_STILL(ctx->player))
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
  int do_still;
  int do_subtitle;

  do_audio = !!DO_AUDIO(ctx->player);
  do_video = !!DO_VIDEO(ctx->player);
  do_still = !!DO_STILL(ctx->player);
  do_subtitle = !!DO_SUBTITLE(ctx->player);
  
  if(do_still)
    {
    process_video(ctx, 1);
    ctx->video_finished = 1;
    }
  
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
  
  if(DO_SUBTITLE_ONLY(ctx->player))
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
  do_audio = DO_AUDIO(ctx->player);
  do_video = DO_VIDEO(ctx->player) || DO_STILL(ctx->player);
  do_subtitle =
    DO_SUBTITLE_OVERLAY(ctx->player) ||
    DO_SUBTITLE_TEXT(ctx->player);
  
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

static bg_parameter_info_t parameters[] =
  {
#if 0
    {
      name:      "input",
      long_name: TRS("Input options"),
      type:      BG_PARAMETER_SECTION,
    },
#endif
    {
      name:      "do_bypass",
      long_name: TRS("Enable bypass mode"),
      type:      BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
      help_string: TRS("Use input plugins in bypass mode if they support it (Currently only the audio CD player).\
 This dramatically decreases CPU usage but doesn't work on all hardware setups.")
    },
    { /* End of parameters */ }
  };


bg_parameter_info_t * bg_player_get_input_parameters(bg_player_t * p)
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
