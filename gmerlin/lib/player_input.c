/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <gmerlin/player.h>
#include <playerprivate.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "player.input"

// #define DUMP_TIMESTAMPS

#define DEBUG_COUNTER

#if 0
struct bg_player_input_context_s
  {
  /* Plugin stuff */
  
  int audio_finished;
  int video_finished;
  int subtitle_finished;
  int send_silence;
    
  int64_t audio_samples_written;
  int64_t video_frames_written;

#ifdef DEBUG_COUNTER
  int64_t audio_sample_counter;
  int64_t video_frame_counter;
#endif
  
  // int has_first_audio_timestamp;
  
  /* Internal times */

  gavl_time_t audio_time;
  gavl_time_t video_time;
  
  /* For changing global stuff */
  
  bg_player_t * player;
  
  
  };
#endif

void bg_player_input_create(bg_player_t * player)
  {
  }

void bg_player_input_destroy(bg_player_t * player)
  {
  if(player->input_handle)
    bg_plugin_unref(player->input_handle);
  }

void bg_player_input_select_streams(bg_player_t * p)
  {
  int i, vis_enable;
  bg_player_audio_stream_t * as = &p->audio_stream;
  bg_player_video_stream_t * vs = &p->video_stream;
  bg_player_subtitle_stream_t * ss = &p->subtitle_stream;
  
  vs->do_still = 0;
  if(vs->still_frame_in)
    {
    gavl_video_frame_destroy(vs->still_frame_in);
    vs->still_frame_in = (gavl_video_frame_t*)0;
    }
  
  /* Adjust stream indices */
  if(p->current_audio_stream >= p->track_info->num_audio_streams)
    p->current_audio_stream = 0;

  if(p->current_video_stream >= p->track_info->num_video_streams)
    p->current_video_stream = 0;

  if(p->current_subtitle_stream >= p->track_info->num_subtitle_streams)
    p->current_subtitle_stream = 0;

  if(!p->audio_stream.plugin_handle)
    p->current_audio_stream = -1;

  if(!p->video_stream.plugin)
    {
    p->current_video_stream = -1;
    p->current_subtitle_stream = -1;
    }
  
  /* Check if the streams are actually there */
  p->flags &= 0xFFFF0000;
  
  as->eof = 1;
  vs->eof = 1;
  ss->eof = 1;
  
  if((p->current_audio_stream >= 0) &&
     (p->current_audio_stream < p->track_info->num_audio_streams))
    {
    as->eof = 0;
    p->flags |= PLAYER_DO_AUDIO;
    }
  if((p->current_video_stream >= 0) &&
     (p->current_video_stream < p->track_info->num_video_streams))
    {
    vs->eof = 0;
    p->flags |= PLAYER_DO_VIDEO;
    }

  if((p->current_subtitle_stream >= 0) &&
     (p->current_subtitle_stream < p->track_info->num_subtitle_streams))
    {
    p->flags |= PLAYER_DO_SUBTITLE;
    ss->eof = 0;
    }
  
  pthread_mutex_lock(&p->config_mutex);
  vis_enable = p->visualizer_enabled;
  pthread_mutex_unlock(&p->config_mutex);
  
  if(DO_AUDIO(p->flags) &&
     !DO_VIDEO(p->flags) &&
     !DO_SUBTITLE(p->flags) &&
     vis_enable)
    {
    p->flags |= PLAYER_DO_VISUALIZE;
    }
  else if(!DO_VIDEO(p->flags) &&
          DO_SUBTITLE(p->flags))
    {
    p->flags |= PLAYER_DO_VIDEO;
    vs->eof = 0;
    
    p->flags |= PLAYER_DO_SUBTITLE_ONLY;
    
    pthread_mutex_lock(&p->video_stream.config_mutex);
    /* Get background color */
    gavl_video_options_get_background_color(p->video_stream.options.opt,
                                            vs->bg_color);
    pthread_mutex_unlock(&p->video_stream.config_mutex);
    
    vs->bg_color[3] = 1.0;
    }
  
  /* En-/Disable streams at the input plugin */
  
  if(p->input_plugin->set_audio_stream)
    {
    for(i = 0; i < p->track_info->num_audio_streams; i++)
      {
      if(i == p->current_audio_stream) 
        p->input_plugin->set_audio_stream(p->input_priv, i,
                                          BG_STREAM_ACTION_DECODE);
      else
        p->input_plugin->set_audio_stream(p->input_priv, i,
                                          BG_STREAM_ACTION_OFF);
      }
    }


  if(p->input_plugin->set_video_stream)
    {
    for(i = 0; i < p->track_info->num_video_streams; i++)
      {
      if(i == p->current_video_stream) 
        p->input_plugin->set_video_stream(p->input_priv, i,
                                      BG_STREAM_ACTION_DECODE);
      else
        p->input_plugin->set_video_stream(p->input_priv, i,
                                      BG_STREAM_ACTION_OFF);
      }
    }

  if(p->input_plugin->set_subtitle_stream)
    {
    for(i = 0; i < p->track_info->num_subtitle_streams; i++)
      {
      if(i == p->current_subtitle_stream) 
        p->input_plugin->set_subtitle_stream(p->input_priv, i,
                                      BG_STREAM_ACTION_DECODE);
      else
        p->input_plugin->set_subtitle_stream(p->input_priv, i,
                                         BG_STREAM_ACTION_OFF);
      }
    }
  }

int bg_player_input_start(bg_player_t * p)
  {
  gavl_video_format_t * video_format;
  
  /* Start input plugin, so we get the formats */
  if(p->input_plugin->start)
    {
    if(!p->input_plugin->start(p->input_priv))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Starting input plugin failed");

      if(p->input_handle)
        bg_plugin_unref(p->input_handle);
      p->input_handle = NULL;
      p->input_plugin = NULL;
      return 0;
      }
    }
  
  /* Subtitle type must be set here, because it's unknown before the
     start() call */

  if(DO_SUBTITLE(p->flags))
    {
    if(p->track_info->subtitle_streams[p->current_subtitle_stream].is_text)
      p->flags |= PLAYER_DO_SUBTITLE_TEXT;
    else
      p->flags |= PLAYER_DO_SUBTITLE_OVERLAY;
    }
  
  /* Check for still image mode */
  
  if(p->flags & PLAYER_DO_VIDEO)
    {
    video_format =
      &p->track_info->video_streams[p->current_video_stream].format;
    
    if(video_format->framerate_mode == GAVL_FRAMERATE_STILL)
      p->video_stream.do_still = 1;
    }
  p->audio_stream.has_first_timestamp_i = 0;
  return 1;
  }

void bg_player_input_stop(bg_player_t * p)
  {
  if(p->input_plugin->stop)
    p->input_plugin->stop(p->input_priv);
  }

int bg_player_input_set_track(bg_player_t * p)
  {
  if(p->input_plugin->set_track)
    {
    if(!p->input_plugin->set_track(p->input_priv, p->current_track))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Cannot select track, skipping");
      return 0;
      }
    }
  return 1;
  }

int bg_player_input_init(bg_player_t * p,
                         bg_plugin_handle_t * handle,
                         int track_index)
  {
  
  p->input_handle = handle;
  p->current_track = track_index;
  
  p->input_plugin = (bg_input_plugin_t*)(handle->plugin);
  p->input_priv = handle->priv;
  
  if(p->input_plugin->set_callbacks)
    {
    p->input_plugin->set_callbacks(p->input_priv,
                                   &p->input_callbacks);
    }
  
  p->track_info = p->input_plugin->get_track_info(p->input_priv,
                                            track_index);
  
  if(p->input_plugin->seek &&
     (p->track_info->flags & BG_TRACK_SEEKABLE) &&
     (p->track_info->duration > 0))
    p->can_seek = 1;
  else
    p->can_seek = 0;
  
  if(p->track_info->flags & BG_TRACK_PAUSABLE)
    p->can_pause = 1;
  else
    p->can_pause = 0;
  
  if(!p->track_info->num_audio_streams &&
     !p->track_info->num_video_streams)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
            "Track has neither audio nor video, skipping");
    return 0;
    }
  
  /* Set the track if neccesary */

  if(!bg_player_input_set_track(p))
    return 0;
  
  
  /* Select streams */
  bg_player_input_select_streams(p);
  
  /* Start input plugin, so we get the formats */
  
  if(!bg_player_input_start(p))
    {
    return 0;
    }
  return 1;
  }

void bg_player_input_cleanup(bg_player_t * p)
  {
#ifdef DEBUG_COUNTER
  char tmp_string[128];
#endif  
  bg_player_input_stop(p);
  if(p->input_handle)
    bg_plugin_unref(p->input_handle);
  p->input_handle = NULL;
  p->input_plugin = NULL;

  if(p->video_stream.still_frame_in)
    {
    gavl_video_frame_destroy(p->video_stream.still_frame_in);
    p->video_stream.still_frame_in = (gavl_video_frame_t*)0;
    }
#ifdef DEBUG_COUNTER
  sprintf(tmp_string, "%" PRId64, p->audio_stream.samples_read);
  
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Audio sample counter: %s",
         tmp_string);

  sprintf(tmp_string, "%" PRId64, p->video_stream.frames_read);
  
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Video frame counter: %s",
         tmp_string);
#endif
  }

int bg_player_input_get_audio_format(bg_player_t * p)
  {
  gavl_audio_format_copy(&p->audio_stream.input_format,
                         &p->track_info->audio_streams[p->current_audio_stream].format);
  return 1;
  }

int bg_player_input_get_video_format(bg_player_t * p)
  {
  gavl_video_format_copy(&p->video_stream.input_format,
                         &p->track_info->video_streams[p->current_video_stream].format);

  if(p->video_stream.do_still)
    {
    p->video_stream.input_format.timescale = GAVL_TIME_SCALE;
    pthread_mutex_lock(&p->config_mutex);
    p->video_stream.input_format.frame_duration =
      (int)((float)GAVL_TIME_SCALE / p->still_framerate);
    pthread_mutex_unlock(&p->config_mutex);
    }
  
  return 1;
  }

int
bg_player_input_get_subtitle_format(bg_player_t * p)
  {
  gavl_video_format_copy(&p->subtitle_stream.input_format,
                         &p->track_info->subtitle_streams[p->current_subtitle_stream].format);
  return 1;
  }

int
bg_player_input_read_audio(void * priv, gavl_audio_frame_t * frame, int stream, int samples)
  {
  int result;
  bg_player_t * p = priv;
  bg_player_audio_stream_t * as = &p->audio_stream;
  
  bg_plugin_lock(p->input_handle);
  result = p->input_plugin->read_audio(p->input_priv, frame, stream, samples);
  bg_plugin_unlock(p->input_handle);
  
  if(!as->has_first_timestamp_i)
    {
    as->samples_read = frame->timestamp;
    as->has_first_timestamp_i = 1;
    }
  as->samples_read += frame->valid_samples;
  return result;
  }

int
bg_player_input_read_video_still(void * priv,
                                 gavl_video_frame_t * frame, int stream)
  {
  int result;
  bg_player_t * p = priv;
  bg_player_video_stream_t * vs = &p->video_stream;

  gavl_video_format_t * format;
  format = &vs->input_format;

  if(!vs->still_frame_in)
    {
    vs->still_frame_in = gavl_video_frame_create(&vs->input_format);
    vs->still_frame_in->timestamp = 0;
    }
  
  bg_plugin_lock(p->input_handle);

  if(p->input_plugin->has_still(p->input_priv, stream))
    result = p->input_plugin->read_video(p->input_priv, frame,
                                         stream);
    
  else
    result = 0;
    
  bg_plugin_unlock(p->input_handle);
  
  if(result)
    gavl_video_frame_copy(&vs->input_format, vs->still_frame_in, frame);
  else
    {
    if(!DO_AUDIO(p->flags) &&
       (p->track_info->duration != GAVL_TIME_UNDEFINED) &&
       gavl_time_unscale(vs->input_format.timescale,
                         vs->still_frame_in->timestamp) > p->track_info->duration)
      result = 0;
    else
      {
      gavl_video_frame_copy(&vs->input_format, frame, vs->still_frame_in);
      frame->timestamp = vs->still_frame_in->timestamp;
      result = 1;
      }
    }
  vs->still_frame_in->timestamp += vs->input_format.frame_duration;
#ifdef DUMP_TIMESTAMPS
  bg_debug("Input timestamp: %"PRId64" (timescale: %d)\n",
           gavl_time_unscale(vs->input_format.timescale,
                             frame->timestamp), vs->input_format.timescale);
#endif
  return result;
    
  }

int
bg_player_input_read_video(void * priv,
                           gavl_video_frame_t * frame, int stream)
  {
  int result;
  bg_player_t * p = priv;
  bg_player_video_stream_t * vs = &p->video_stream;
  
  bg_plugin_lock(p->input_handle);

  /* Skip */

  if(vs->skip && p->input_plugin->skip_video)
    {
    gavl_time_t skip_time = 
      vs->frame_time + vs->skip;
    
    p->input_plugin->skip_video(p->input_priv, stream,
                                &skip_time, GAVL_TIME_SCALE, 0);
    }
  
  result = p->input_plugin->read_video(p->input_priv, frame, stream);
  bg_plugin_unlock(p->input_handle);

#ifdef DUMP_TIMESTAMPS
  bg_debug("Input timestamp: %"PRId64"\n",
           gavl_time_unscale(vs->input_format.timescale,
                             frame->timestamp));
#endif
  p->video_stream.frames_read++;
  
  //  if(!result)
  //    fprintf(stderr, "Read video failed\n");
  return result;
  }

int
bg_player_input_read_video_subtitle_only(void * priv,
                                         gavl_video_frame_t * frame,
                                         int stream)
  {
  bg_player_t * p = priv;
  bg_player_video_stream_t * vs = &p->video_stream;
  
  gavl_video_frame_fill(frame, &vs->output_format, vs->bg_color);
  
  frame->timestamp = (int64_t)vs->frames_read * vs->output_format.frame_duration;
  return 1;
  }


void bg_player_input_seek(bg_player_t * p,
                          gavl_time_t * time, int scale)
  {
  int do_audio, do_video, do_subtitle;

  bg_player_audio_stream_t * as;
  bg_player_video_stream_t * vs;
  bg_player_subtitle_stream_t * ss;

  as = &p->audio_stream;
  vs = &p->video_stream;
  ss = &p->subtitle_stream;
  
  bg_plugin_lock(p->input_handle);
  p->input_plugin->seek(p->input_priv, time, scale);
  bg_plugin_unlock(p->input_handle);
  
  as->samples_read =
    gavl_time_to_samples(as->input_format.samplerate, *time);

  // This wasn't set before if we switch streams or replug filters
  as->has_first_timestamp_i = 1;
  
  if(DO_SUBTITLE_ONLY(p->flags))
    vs->frames_read =
      gavl_time_to_frames(vs->output_format.timescale, vs->output_format.frame_duration,
                          *time);
  else
    {
    vs->frames_written =
      gavl_time_to_frames(vs->input_format.timescale, vs->input_format.frame_duration,
                          *time);
    if(vs->still_frame_in)
      vs->still_frame_in->timestamp =
        gavl_time_scale(vs->input_format.timescale, *time);
    }
  // Clear EOF states
  do_audio = DO_AUDIO(p->flags);
  do_video = DO_VIDEO(p->flags);
  do_subtitle = DO_SUBTITLE(p->flags);
  
  // fprintf(stderr, "Seek, do video: %d\n", do_video);
  
  ss->eof = !do_subtitle;
  as->eof = !do_audio;
  vs->eof = !do_video;

  as->send_silence = 0;
  vs->skip = 0;
  }


/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "still_framerate",
      .long_name =   "Still image repitition rate",
      .type =        BG_PARAMETER_FLOAT,
      .val_default = { .val_f = 10.0 },
      .val_min =     { .val_f = 0.5 },
      .val_max =     { .val_f = 100.0 },
      .help_string = TRS("When showing still images, gmerlin repeats them periodically to make realtime filter tweaking work."),
    },
    {
      .name =        "sync_offset",
      .long_name =   "Sync offset [ms]",
      .type      =   BG_PARAMETER_SLIDER_INT,
      .flags     =   BG_PARAMETER_SYNC,
      .val_default = { .val_i = 0 },
      .val_min =     { .val_i = -1000 },
      .val_max =     { .val_i =  1000 },
      .help_string = TRS("Use this for playing buggy files, which have a constant offset between audio and video. Use positive values if the video is ahead of audio"),
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

  pthread_mutex_lock(&player->config_mutex);
  if(!strcmp(name, "still_framerate"))
    player->still_framerate = val->val_f;
  else if(!strcmp(name, "sync_offset"))
    player->sync_offset = gavl_time_unscale(1000, val->val_i);

  pthread_mutex_unlock(&player->config_mutex);
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
    bg_msg_set_arg_string(msg, 0, d->handle->plugin->long_name);
    bg_msg_set_arg_string(msg, 1, d->handle->location);
    bg_msg_set_arg_int(msg, 2, d->track);
    }
  }

void bg_player_input_send_messages(bg_player_t * p)
  {
  struct msg_input_data d;
  d.handle = p->input_handle;
  d.track =  p->current_track;
  bg_msg_queue_list_send(p->message_queues, msg_input, &d);
  }
