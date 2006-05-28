/*****************************************************************
 
  player_thread.c
 
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

/* Metadata */

static void msg_metadata(bg_msg_t * msg, const void * data)
  {
  bg_metadata_t * m = (bg_metadata_t *)data;
  bg_msg_set_id(msg, BG_PLAYER_MSG_METADATA);
  bg_msg_set_arg_metadata(msg, 0, m);
  }

static void msg_time(bg_msg_t * msg,
                     const void * data)
  {
  bg_msg_set_id(msg, BG_PLAYER_MSG_TIME_CHANGED);
  bg_msg_set_arg_time(msg, 0, *((const gavl_time_t*)data));
  }

static void msg_volume_changed(bg_msg_t * msg,
                               const void * data)
  {
  bg_msg_set_id(msg, BG_PLAYER_MSG_VOLUME_CHANGED);
  bg_msg_set_arg_float(msg, 0, *((float*)data));
  }

static void msg_audio_stream(bg_msg_t * msg,
                             const void * data)
  {
  const bg_player_t * player;
  player = (const bg_player_t*)data;
  //  fprintf(stderr, "msg_audio_stream\n");
  bg_msg_set_id(msg, BG_PLAYER_MSG_AUDIO_STREAM);
  bg_msg_set_arg_int(msg, 0, player->current_audio_stream);
  bg_msg_set_arg_audio_format(msg, 1,
                              &(player->audio_stream.input_format));
  bg_msg_set_arg_audio_format(msg, 2,
                              &(player->audio_stream.output_format));
  
  }

static void msg_video_stream(bg_msg_t * msg,
                             const void * data)
  {
  bg_player_t * player;
  player = (bg_player_t*)data;

  bg_msg_set_id(msg, BG_PLAYER_MSG_VIDEO_STREAM);
  bg_msg_set_arg_int(msg, 0, player->current_video_stream);
  bg_msg_set_arg_video_format(msg, 1,
                              &(player->video_stream.input_format));
  bg_msg_set_arg_video_format(msg, 2,
                              &(player->video_stream.output_format));
  
  }

static void msg_still_stream(bg_msg_t * msg,
                             const void * data)
  {
  bg_player_t * player;
  player = (bg_player_t*)data;

  bg_msg_set_id(msg, BG_PLAYER_MSG_STILL_STREAM);
  bg_msg_set_arg_int(msg, 0, player->current_still_stream);
  bg_msg_set_arg_video_format(msg, 1,
                              &(player->video_stream.input_format));
  bg_msg_set_arg_video_format(msg, 2,
                              &(player->video_stream.output_format));
  
  }

static void msg_subtitle_stream(bg_msg_t * msg,
                                const void * data)
  {
  bg_player_t * player;
  player = (bg_player_t*)data;

  bg_msg_set_id(msg, BG_PLAYER_MSG_SUBTITLE_STREAM);
  bg_msg_set_arg_int(msg, 0, player->current_subtitle_stream);
  }

static void msg_num_streams(bg_msg_t * msg,
                            const void * data)
  {
  bg_track_info_t * info;
  info = (bg_track_info_t *)data;
  bg_msg_set_id(msg, BG_PLAYER_MSG_TRACK_NUM_STREAMS);

  bg_msg_set_arg_int(msg, 0, info->num_audio_streams);
  bg_msg_set_arg_int(msg, 1, info->num_video_streams);
  bg_msg_set_arg_int(msg, 2, info->num_subtitle_streams);
  }

struct stream_info_s
  {
  bg_track_info_t * track;
  int index;
  };

static void msg_audio_stream_info(bg_msg_t * msg, const void * data)
  {
  const struct stream_info_s * si;
  si = (const struct stream_info_s*)data;
  bg_msg_set_id(msg, BG_PLAYER_MSG_AUDIO_STREAM_INFO);

  bg_msg_set_arg_int(msg,    0, si->index);
  bg_msg_set_arg_string(msg, 1, si->track->audio_streams[si->index].info);
  bg_msg_set_arg_string(msg, 2, si->track->audio_streams[si->index].language);
  }

static void msg_video_stream_info(bg_msg_t * msg, const void * data)
  {
  const struct stream_info_s * si;
  si = (const struct stream_info_s*)data;
  bg_msg_set_id(msg, BG_PLAYER_MSG_VIDEO_STREAM_INFO);

  bg_msg_set_arg_int(msg,    0, si->index);
  bg_msg_set_arg_string(msg, 1, si->track->video_streams[si->index].info);
  bg_msg_set_arg_string(msg, 2, si->track->video_streams[si->index].language);
  }

static void msg_subtitle_stream_info(bg_msg_t * msg, const void * data)
  {
  const struct stream_info_s * si;
  si = (const struct stream_info_s*)data;
  bg_msg_set_id(msg, BG_PLAYER_MSG_SUBTITLE_STREAM_INFO);

  bg_msg_set_arg_int(msg,    0, si->index);
  bg_msg_set_arg_string(msg, 1, si->track->subtitle_streams[si->index].info);
  bg_msg_set_arg_string(msg, 2, si->track->subtitle_streams[si->index].language);
  }

static void msg_video_description(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_PLAYER_MSG_VIDEO_DESCRIPTION);
  bg_msg_set_arg_string(msg, 0, (char*)data);
  }

static void msg_still_description(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_PLAYER_MSG_STILL_DESCRIPTION);
  bg_msg_set_arg_string(msg, 0, (char*)data);
  }

static void msg_audio_description(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_PLAYER_MSG_AUDIO_DESCRIPTION);
  bg_msg_set_arg_string(msg, 0, (char*)data);
  }

static void msg_stream_description(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_PLAYER_MSG_STREAM_DESCRIPTION);
  bg_msg_set_arg_string(msg, 0, (char*)data);
  }

#if 0 

static void msg_subpicture_description(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_PLAYER_MSG_SUBPICTURE_DESCRIPTION);
  bg_msg_set_arg_string(msg, 0, (char*)data);
  }

#endif


/*
 *  Interrupt playback so all plugin threads are waiting inside
 *  keep_going();
 */

static void interrupt_cmd(bg_player_t * p, int new_state)
  {
  int old_state;
  
  pthread_mutex_lock(&(p->stop_mutex));

  /* Get the old state */
  old_state = bg_player_get_state(p);
  
  /* Set the new state */
  bg_player_set_state(p, new_state, NULL, NULL);

  if(old_state == BG_PLAYER_STATE_PAUSED)
    {
    pthread_mutex_unlock(&p->stop_mutex);
    return;
    }
  /* Tell it to the fifos */

  if(p->do_audio)
    bg_fifo_set_state(p->audio_stream.fifo, BG_FIFO_PAUSED);
  if(p->do_video)
    bg_fifo_set_state(p->video_stream.fifo, BG_FIFO_PAUSED);
  
  //  if(p->waiting_plugin_threads < p->total_plugin_threads)
  pthread_cond_wait(&(p->stop_cond), &(p->stop_mutex));
  
  pthread_mutex_unlock(&p->stop_mutex);
  bg_player_time_stop(p);

  if(p->do_audio)
    bg_player_oa_stop(p->oa_context);
  }

/* Preload fifos */

static void preload(bg_player_t * p)
  {
  
  bg_player_input_preload(p->input_context);

  }

/* Start playback */

static void start_playback(bg_player_t * p, int new_state)
  {
  int want_new;
  //  fprintf(stderr, "start_playback...");
  if(new_state == BG_PLAYER_STATE_CHANGING)
    {
    want_new = 0;
    bg_player_set_state(p, new_state, &want_new, NULL);
    }
  else
    bg_player_set_state(p, new_state, NULL, NULL);
    
    //  fprintf(stderr, "start_playback\n");

  /* Start timer */
  
  bg_player_time_start(p);

  if(p->do_audio)
    bg_player_oa_start(p->oa_context);
  
  pthread_mutex_lock(&(p->start_mutex));
  pthread_cond_broadcast(&(p->start_cond));
  pthread_mutex_unlock(&(p->start_mutex));
  //  fprintf(stderr, "start_playback done\n");
  }

/* Pause command */

static void pause_cmd(bg_player_t * p)
  {
  int state;
  state = bg_player_get_state(p);

  if(state == BG_PLAYER_STATE_PLAYING)
    {
    interrupt_cmd(p, BG_PLAYER_STATE_PAUSED);

    if(p->do_bypass)
      bg_player_input_bypass_set_pause(p->input_context, 1);

    /* Now that all threads are stopped, we can go back to play mode */
    if(p->do_audio)
      bg_fifo_set_state(p->audio_stream.fifo, BG_FIFO_PLAYING);
    if(p->do_video)
      bg_fifo_set_state(p->video_stream.fifo, BG_FIFO_PLAYING);
    
    }
  else if(state == BG_PLAYER_STATE_PAUSED)
    {
    preload(p);
    if(p->do_bypass)
      bg_player_input_bypass_set_pause(p->input_context, 0);

    start_playback(p, BG_PLAYER_STATE_PLAYING);
    }
  }

/* Initialize playback pipelines */

static int init_streams(bg_player_t * p)
  {
  if(!bg_player_audio_init(p, p->current_audio_stream))
    {
    //    bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
    //                    "Cannot setup audio playback", NULL);
    if(p->audio_stream.error_msg)
      bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                          p->audio_stream.error_msg, NULL);
    else
      bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                          "Cannot setup audio playback (unknown error)", NULL);
    return 0;
    }
  if(!bg_player_video_init(p, p->current_video_stream))
    {
    if(p->video_stream.error_msg)
      bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                          p->video_stream.error_msg, NULL);
    else
      bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                          "Cannot setup video playback (unknown error)", NULL);
    return 0;
    }
  if(!bg_player_subtitle_init(p, p->current_subtitle_stream))
    {
    if(p->subtitle_stream.error_msg)
      bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                          p->subtitle_stream.error_msg, NULL);
    else
      bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                          "Cannot setup subtitle playback (unknown error)", NULL);
    return 0;
    }
  return 1;
  }

/* Cleanup everything */

static void player_cleanup(bg_player_t * player)
  {
  gavl_time_t t = 0;

  //  fprintf(stderr, "Player cleanup\n");
  
  bg_player_input_cleanup(player->input_context);
  player->input_handle = (bg_plugin_handle_t*)0;
  
  //  fprintf(stderr, "bg_player_oa_cleanup...");
  
  bg_player_oa_cleanup(player->oa_context);
  //  fprintf(stderr, "bg_player_oa_cleanup done\n");
  
  bg_player_ov_cleanup(player->ov_context);
  
  bg_player_time_stop(player);
  
  bg_player_video_cleanup(player);
  bg_player_audio_cleanup(player);
  bg_player_subtitle_cleanup(player);
  bg_player_time_reset(player);

  bg_msg_queue_list_send(player->message_queues,
                         msg_time,
                         &t);
  }


/* Play a file. This must be called ONLY if the player is in
   a defined stopped state
*/

static void play_cmd(bg_player_t * p,
                     bg_plugin_handle_t * handle,
                     int track_index, char * track_name, int flags)
  {
  int i;
  char * error_msg;
  const char * error_msg_input;
  struct stream_info_s si;
  
  int had_video;
  gavl_time_t time = 0;
    
  // fprintf(stderr, "Play_cmd %d\n", flags);
  
  /* Shut down from last playback if necessary */

  if(p->input_handle && !bg_plugin_equal(p->input_handle, handle))
    player_cleanup(p);

  
  had_video = p->do_video || p->do_still;
  
  bg_player_set_state(p, BG_PLAYER_STATE_STARTING, NULL, NULL);

  //  fprintf(stderr, "play_cmd %p\n", handle);
  
  bg_player_set_track_name(p, track_name);

  p->input_handle = handle;
  if(!bg_player_input_init(p->input_context,
                           handle, track_index))
    {
    error_msg_input = bg_player_input_get_error(p->input_context);

    if(error_msg_input)
      {
      error_msg = bg_sprintf("Cannot initialize input (%s)", error_msg_input);
      bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                          error_msg, NULL);
      free(error_msg);
      }
    else
      bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                          "Cannot initialize input (unknown error)", NULL);
    
    return;
    }

  /* Initialize audio and video streams if not in bypass mode */

  p->do_audio = 0;
  p->do_video = 0;
  p->do_still = 0;

  if(!p->do_bypass)
    {
    if(!init_streams(p))
      return;
    }
  
  /* Send messages about the stream */
  /*
    bg_msg_queue_list_send(p->message_queues,
    msg_name,
    p->track_info);
  */
  bg_msg_queue_list_send(p->message_queues,
                         msg_num_streams,
                         p->track_info);

  /* Send infos about the streams we have */
  si.track = p->track_info;
  for(i = 0; i < p->track_info->num_audio_streams; i++)
    {
    si.index = i;
    bg_msg_queue_list_send(p->message_queues,
                           msg_audio_stream_info,
                           &si);
    }
  for(i = 0; i < p->track_info->num_video_streams; i++)
    {
    si.index = i;
    bg_msg_queue_list_send(p->message_queues,
                           msg_video_stream_info,
                           &si);
    }
  for(i = 0; i < p->track_info->num_subtitle_streams; i++)
    {
    si.index = i;
    bg_msg_queue_list_send(p->message_queues,
                           msg_subtitle_stream_info,
                           &si);
    }
  
  bg_player_set_duration(p, p->track_info->duration, p->can_seek);
  
  /* Send metadata */

  bg_msg_queue_list_send(p->message_queues,
                         msg_metadata,
                         &(p->track_info->metadata));
  
  if(p->track_info->description)
    bg_msg_queue_list_send(p->message_queues,
                           msg_stream_description,
                           p->track_info->description);
  
  /* Send messages about formats */
  if(p->do_audio)
    {
    if(p->track_info->audio_streams[p->current_audio_stream].description)
      bg_msg_queue_list_send(p->message_queues,
                             msg_audio_description,
                             p->track_info->audio_streams[p->current_audio_stream].description);
    bg_msg_queue_list_send(p->message_queues,
                           msg_audio_stream,
                           p);
    }
  if(p->do_video)
    {
    if(p->track_info->video_streams[p->current_video_stream].description)
      bg_msg_queue_list_send(p->message_queues,
                             msg_video_description,
                             p->track_info->video_streams[p->current_video_stream].description);
    bg_msg_queue_list_send(p->message_queues,
                           msg_video_stream,
                           p);
    }
  else if(p->do_still)
    {
    if(p->track_info->still_streams[p->current_still_stream].description)
      bg_msg_queue_list_send(p->message_queues,
                             msg_still_description,
                             p->track_info->still_streams[p->current_still_stream].description);
    bg_msg_queue_list_send(p->message_queues,
                           msg_still_stream,
                           p);
    }
  else if(had_video)
    bg_player_ov_standby(p->ov_context);

  if(p->do_subtitle_overlay || p->do_subtitle_text)
    {
    bg_msg_queue_list_send(p->message_queues, msg_subtitle_stream, p);
    }
  
  /* Count the threads */

  p->total_plugin_threads = 1;
  if(p->do_audio)
    p->total_plugin_threads++;
  if(p->do_video || p->do_still)
    p->total_plugin_threads++;

  /* Reset variables */

  p->waiting_plugin_threads = 0;
    
  /* Start playback */
  
  pthread_mutex_lock(&(p->stop_mutex));

  if(p->do_bypass)
    pthread_create(&(p->input_thread),
                   (pthread_attr_t*)0,
                   bg_player_input_thread_bypass, p->input_context);
  else
    pthread_create(&(p->input_thread),
                   (pthread_attr_t*)0,
                   bg_player_input_thread, p->input_context);
    
  
  if(p->do_audio)
    pthread_create(&(p->oa_thread),
                   (pthread_attr_t*)0,
                   bg_player_oa_thread, p->oa_context);

  if(p->do_video)
    pthread_create(&(p->ov_thread),
                   (pthread_attr_t*)0,
                   bg_player_ov_thread, p->ov_context);
  else if(p->do_still)
    pthread_create(&(p->ov_thread),
                   (pthread_attr_t*)0,
                   bg_player_ov_still_thread, p->ov_context);

  //  fprintf(stderr, "BlaBla\n");
  
  //  if(p->waiting_plugin_threads < p->total_plugin_threads)
  pthread_cond_wait(&(p->stop_cond), &(p->stop_mutex));
  pthread_mutex_unlock(&p->stop_mutex);
  
  bg_player_time_init(p);
  bg_player_time_set(p, 0);

  /* Fire up the fifos */
  if(p->do_audio)
    bg_fifo_set_state(p->audio_stream.fifo, BG_FIFO_PLAYING);
  if(p->do_video)
    bg_fifo_set_state(p->video_stream.fifo, BG_FIFO_PLAYING);
  
  preload(p);

  if(flags & BG_PLAY_FLAG_INIT_THEN_PAUSE)
    {
    bg_player_set_state(p, BG_PLAYER_STATE_PAUSED, NULL, NULL);
    if(p->do_bypass)
      bg_player_input_bypass_set_pause(p->input_context, 1);
    if(p->do_video)
      bg_player_ov_update_still(p->ov_context);

    }
  else
    start_playback(p, BG_PLAYER_STATE_PLAYING);
  
  /* Set start time to zero */

  bg_msg_queue_list_send(p->message_queues,
                         msg_time,
                         &time);
  }

static void stop_cmd(bg_player_t * player, int new_state)
  {
  int want_new;
  int old_state;
  
  old_state = bg_player_get_state(player);
  //  fprintf(stderr, "STOP CMD\n");
  
  if(old_state == BG_PLAYER_STATE_STOPPED)
    return;
  
  if(new_state == BG_PLAYER_STATE_CHANGING)
    {
    //    if(old_state == BG_PLAYER_STATE_STILL)
    //      want_new = 1;
    //    else
    want_new = 0;
    bg_player_set_state(player, new_state, &want_new, NULL);
    //    fprintf(stderr, "Changing, want_new: %d\n", want_new);
    }
  else
    bg_player_set_state(player, new_state, NULL, NULL);

  switch(old_state)
    {
    case BG_PLAYER_STATE_CHANGING:
      if(new_state == BG_PLAYER_STATE_STOPPED)
        {
        if(player->do_video || player->do_still)
          {
          bg_player_ov_standby(player->ov_context);
          player->do_video = 0;
          player->do_still = 0;
          }
        }
      return;
      break;
    case BG_PLAYER_STATE_STARTING:
    case BG_PLAYER_STATE_PAUSED:
    case BG_PLAYER_STATE_SEEKING:
    case BG_PLAYER_STATE_BUFFERING:
      /* If the threads are sleeping, wake them up now so they'll end */
      start_playback(player, new_state);
    }
  
  if((old_state == BG_PLAYER_STATE_PLAYING) ||
     (old_state == BG_PLAYER_STATE_PAUSED) ||
     (old_state == BG_PLAYER_STATE_STILL))
    {
    /* Set the stop flag */
    if(player->do_audio)
      bg_fifo_set_state(player->audio_stream.fifo, BG_FIFO_STOPPED);
    if(player->do_video || player->do_still)
      bg_fifo_set_state(player->video_stream.fifo, BG_FIFO_STOPPED);

    /* Input thread is joined before entering still mode */
    if(old_state != BG_PLAYER_STATE_STILL)
      {
      fprintf(stderr, "Joining input thread...");
      pthread_join(player->input_thread, (void**)0);
      fprintf(stderr, "done\n");
      }

    if(player->do_audio)
      {
      fprintf(stderr, "Joining audio thread...");
      pthread_join(player->oa_thread, (void**)0);
      fprintf(stderr, "done\n");
      }
    if(player->do_video || player->do_still)
      {
      fprintf(stderr, "Joining video thread...");
      pthread_join(player->ov_thread, (void**)0);
      fprintf(stderr, "done\n");
      }
    if(player->do_audio)
      bg_player_oa_stop(player->oa_context);

    if((new_state == BG_PLAYER_STATE_STOPPED) ||
       !(player->input_handle->info->flags & BG_PLUGIN_KEEP_RUNNING))
      player_cleanup(player);
    
    if(new_state == BG_PLAYER_STATE_STOPPED)
      {
      if(player->do_video || player->do_still)
        {
        bg_player_ov_standby(player->ov_context);
        player->do_video = 0;
        player->do_still = 0;
        }
      }
    }
  }

static void set_ov_plugin_cmd(bg_player_t * player,
                              bg_plugin_handle_t * handle)
  {
  int state;
  //  fprintf(stderr, "Setting video output plugin: %s\n", handle->info->name);

  state = bg_player_get_state(player);
  if((state == BG_PLAYER_STATE_PLAYING) || (state == BG_PLAYER_STATE_STILL))
    stop_cmd(player, BG_PLAYER_STATE_STOPPED);
  
  bg_player_ov_set_plugin(player, handle);
  bg_player_ov_standby(player->ov_context);
  }

static void set_oa_plugin_cmd(bg_player_t * player,
                              bg_plugin_handle_t * handle)
  {
  int state;
  //  fprintf(stderr, "Setting audio output plugin: %s\n", handle->info->name);
  state = bg_player_get_state(player);
  if((state == BG_PLAYER_STATE_PLAYING) || (state == BG_PLAYER_STATE_STILL))
    stop_cmd(player, BG_PLAYER_STATE_STOPPED);
  bg_player_oa_set_plugin(player, handle);
  }

static void seek_cmd(bg_player_t * player, gavl_time_t t)
  {
  int old_state;
  gavl_time_t sync_time = t;

  old_state = bg_player_get_state(player);
  
  //  gavl_video_frame_t * vf;
  //  fprintf(stderr, "Seek cmd\n");
  interrupt_cmd(player, BG_PLAYER_STATE_SEEKING);

  bg_player_input_seek(player->input_context, &sync_time);

  //  fprintf(stderr, "Player seeked: %f %f\n", gavl_time_to_seconds(t), gavl_time_to_seconds(sync_time));

  
  
  /* Clear fifos */

  if(player->do_audio)
    {
    bg_fifo_clear(player->audio_stream.fifo);
    }
  if(player->do_video)
    {
    bg_fifo_clear(player->video_stream.fifo);
    }
  if(player->do_subtitle_text || player->do_subtitle_overlay)
    {
    bg_fifo_clear(player->subtitle_stream.fifo);
    }
  
  /* Resync */
  //  fprintf(stderr, "Preload\n");

  /* Fire up the fifos */
  if(player->do_audio)
    bg_fifo_set_state(player->audio_stream.fifo, BG_FIFO_PLAYING);
  if(player->do_video)
    bg_fifo_set_state(player->video_stream.fifo, BG_FIFO_PLAYING);
  
  preload(player);
  //  fprintf(stderr, "Preload done\n");
  
  bg_player_time_set(player, sync_time);

  bg_player_ov_reset(player);
  
  if(old_state == BG_PLAYER_STATE_PAUSED)
    {
    bg_player_set_state(player, BG_PLAYER_STATE_PAUSED, NULL, NULL);

    /* Need to update slider and time for seeking case */

    bg_msg_queue_list_send(player->message_queues,
                           msg_time,
                           &sync_time);
    
    if(player->do_video)
      bg_player_ov_update_still(player->ov_context);
    
    // if(p->do_bypass)
    // bg_player_input_bypass_set_pause(p->input_context, 1);
    }
  else
    start_playback(player, BG_PLAYER_STATE_PLAYING);
  }

static void set_audio_stream_cmd(bg_player_t * player, int stream)
  {
  int state;
  fprintf(stderr, "SET_AUDIO_STREAM %d\n", stream);
  
  if(stream == player->current_audio_stream)
    return;

  state = bg_player_get_state(player);
  if(state != BG_PLAYER_STATE_STOPPED)
    stop_cmd(player, BG_PLAYER_STATE_STOPPED);
  player->current_audio_stream = stream;
  }

static void set_video_stream_cmd(bg_player_t * player, int stream)
  {
  int state;
  fprintf(stderr, "SET_VIDEO_STREAM %d\n", stream);

  if(stream == player->current_video_stream)
    return;

  state = bg_player_get_state(player);
  if(state != BG_PLAYER_STATE_STOPPED)
    stop_cmd(player, BG_PLAYER_STATE_STOPPED);
  player->current_video_stream = stream;

  }


static void set_subtitle_stream_cmd(bg_player_t * player, int stream)
  {
  int state;
  fprintf(stderr, "SET_SUBTITLE_STREAM %d\n", stream);

  if(stream == player->current_subtitle_stream)
    return;

  state = bg_player_get_state(player);
  if(state != BG_PLAYER_STATE_STOPPED)
    stop_cmd(player, BG_PLAYER_STATE_STOPPED);
  player->current_subtitle_stream = stream;
    
  }


/* Process command, return FALSE if thread should be ended */

static int process_commands(bg_player_t * player)
  {
  int play_flags;
  int arg_i1;
  float arg_f1;
  int state;
  void * arg_ptr1;
  char * arg_str1;
  int next_track;
  int error_code;
  gavl_time_t time;
  gavl_time_t current_time;
  bg_msg_t * command;
  int queue_locked = 0;
  uint32_t id;
  
  while(1)
    {
    command = bg_msg_queue_try_lock_read(player->command_queue);
    if(!command)
      return 1;
    
  /* Process commands */

    queue_locked = 1;
    
    switch(bg_msg_get_id(command))
      {
      case BG_PLAYER_CMD_QUIT:
        state = bg_player_get_state(player);
        //      fprintf(stderr, "Command quit\n");
        switch(state)
          {
          case BG_PLAYER_STATE_PLAYING:
          case BG_PLAYER_STATE_CHANGING:
          case BG_PLAYER_STATE_PAUSED:
          case BG_PLAYER_STATE_STILL:
            stop_cmd(player, BG_PLAYER_STATE_STOPPED);
            break;
          }
        bg_msg_queue_unlock_read(player->command_queue);
        queue_locked = 0;
        return 0;
        break;
      case BG_PLAYER_CMD_PLAY:
        //      fprintf(stderr, "Command play\n");
        play_flags = bg_msg_get_arg_int(command, 2);
        state = bg_player_get_state(player);

        if(play_flags)
          {
          if(((state == BG_PLAYER_STATE_PLAYING) || (state == BG_PLAYER_STATE_STILL)) &&
             (play_flags & BG_PLAY_FLAG_IGNORE_IF_PLAYING))
            break;
          else if((state == BG_PLAYER_STATE_STOPPED) &&
                  (play_flags & BG_PLAY_FLAG_IGNORE_IF_STOPPED))
            break;
          }
        if(state == BG_PLAYER_STATE_PAUSED)
          {
          if(play_flags & BG_PLAY_FLAG_RESUME)
            {
            pause_cmd(player);
            break;
            }
          else
            play_flags |= BG_PLAY_FLAG_INIT_THEN_PAUSE;
          }
        
        arg_ptr1 = bg_msg_get_arg_ptr_nocopy(command, 0);
        arg_i1   = bg_msg_get_arg_int(command, 1);
        arg_str1 = bg_msg_get_arg_string(command, 3);
      
        if((state == BG_PLAYER_STATE_PLAYING) ||
           (state == BG_PLAYER_STATE_PAUSED) ||
           (state == BG_PLAYER_STATE_STILL))
          {
          stop_cmd(player, BG_PLAYER_STATE_CHANGING);
          }
        if(!arg_ptr1)
          {
          error_code = BG_PLAYER_ERROR_GENERAL;
          bg_player_set_state(player, BG_PLAYER_STATE_ERROR,
                              "No Track selected", &error_code);
          }
        else
          play_cmd(player, arg_ptr1, arg_i1, arg_str1, play_flags);

        if(arg_str1)
          free(arg_str1);
      
        //      fprintf(stderr, "Command playfile %s\n", arg_str1);
                  
        break;
      case BG_PLAYER_CMD_STOP:
        state = bg_player_get_state(player);
        //      fprintf(stderr, "Command stop\n");
        switch(state)
          {
          case BG_PLAYER_STATE_PLAYING:
          case BG_PLAYER_STATE_PAUSED:
          case BG_PLAYER_STATE_STILL:
          case BG_PLAYER_STATE_CHANGING:
            stop_cmd(player, BG_PLAYER_STATE_STOPPED);
            break;
          }
        break;
      case BG_PLAYER_CMD_SEEK:
        if(!player->can_seek)
          {
          //        fprintf(stderr, "Cannot seek in this stream\n");
          }
        else
          {
          time = bg_msg_get_arg_time(command, 0);
          seek_cmd(player, time);
          }
        break;
      case BG_PLAYER_CMD_SEEK_REL:
        if(!player->can_seek)
          break;
        
        time = bg_msg_get_arg_time(command, 0);

        bg_msg_queue_unlock_read(player->command_queue);
        queue_locked = 0;
        
        /* Check if there are more messages */
        while(bg_msg_queue_peek(player->command_queue, &id) && 
              (id == BG_PLAYER_CMD_SEEK_REL))
          {
          command = bg_msg_queue_lock_read(player->command_queue);
          queue_locked = 1;
          time += bg_msg_get_arg_time(command, 0);
          //          fprintf(stderr, "Accumulating relative seek\n");
          bg_msg_queue_unlock_read(player->command_queue);
          queue_locked = 0;
          }
        
        bg_player_time_get(player, 1, &current_time);
        time += current_time;
        if(time < 0)
          time = 0;
        else if(time > player->track_info->duration)
          {
          /* Seeked beyond end -> finish track */
          stop_cmd(player, BG_PLAYER_STATE_CHANGING);
          break;
          }
        seek_cmd(player, time);
        break;
      case BG_PLAYER_CMD_SET_VOLUME:
        arg_f1 = bg_msg_get_arg_float(command, 0);
        player->volume = arg_f1;
        if(player->do_bypass)
          bg_player_input_bypass_set_volume(player->input_context,
                                            player->volume);
        else
          bg_player_oa_set_volume(player->oa_context, player->volume);

        bg_msg_queue_list_send(player->message_queues,
                               msg_volume_changed,
                               &(player->volume));

        break;
      case BG_PLAYER_CMD_SET_VOLUME_REL:

        arg_f1 = bg_msg_get_arg_float(command, 0);

        player->volume += arg_f1;
        if(player->volume > 1.0)
          player->volume = 1.0;
        if(player->volume < BG_PLAYER_VOLUME_MIN)
          player->volume = BG_PLAYER_VOLUME_MIN;
        if(player->do_bypass)
          bg_player_input_bypass_set_volume(player->input_context,
                                            player->volume);
        else
          bg_player_oa_set_volume(player->oa_context, player->volume);

        bg_msg_queue_list_send(player->message_queues,
                               msg_volume_changed,
                               &(player->volume));
        break;
      case BG_PLAYER_CMD_SETSTATE:
        arg_i1 = bg_msg_get_arg_int(command, 0);
        switch(arg_i1)
          {
          case BG_PLAYER_STATE_STILL:
            /* Close down input plugin */
            fprintf(stderr, "Joining input thread...");
            pthread_join(player->input_thread, (void**)0);
            fprintf(stderr, "done\n");
            bg_player_set_state(player, BG_PLAYER_STATE_STILL, NULL, NULL);
            break;
          case BG_PLAYER_STATE_FINISHING:
            state = bg_player_get_state(player);
          
            /* Close down everything */
            //          fprintf(stderr, "Finishing...\n");
            bg_player_set_state(player, BG_PLAYER_STATE_FINISHING, NULL, NULL);
          
            if(state != BG_PLAYER_STATE_STILL)
              {
              fprintf(stderr, "Joining input thread...");
              pthread_join(player->input_thread, (void**)0);
              fprintf(stderr, "done\n");
              }
            if(player->do_audio)
              {
              fprintf(stderr, "Joining audio thread...");
              pthread_join(player->oa_thread, (void**)0);
              fprintf(stderr, "done\n");
              }
            if(player->do_video || player->do_still)
              {
              fprintf(stderr, "Joining video thread...");
              pthread_join(player->ov_thread, (void**)0);
              fprintf(stderr, "done\n");
              }
            player_cleanup(player);
            next_track = 1;
            bg_player_set_state(player, BG_PLAYER_STATE_CHANGING, &next_track, NULL);
            //          fprintf(stderr, "Finishing done\n");
            break;
          case BG_PLAYER_STATE_ERROR:
            arg_str1 = bg_msg_get_arg_string(command, 1);
            stop_cmd(player, BG_PLAYER_STATE_STOPPED);
            bg_player_set_state(player, BG_PLAYER_STATE_ERROR,
                                arg_str1, NULL);
          }
        break;
      case BG_PLAYER_CMD_SET_OA_PLUGIN:
        arg_ptr1 = bg_msg_get_arg_ptr_nocopy(command, 0);
        set_oa_plugin_cmd(player, arg_ptr1);
        break;
      case BG_PLAYER_CMD_SET_OV_PLUGIN:
        //      fprintf(stderr, "***** Set OV Plugin\n");
        arg_ptr1 = bg_msg_get_arg_ptr_nocopy(command, 0);
        set_ov_plugin_cmd(player, arg_ptr1);
        break;

      case BG_PLAYER_CMD_SET_AUDIO_STREAM:
        arg_i1 = bg_msg_get_arg_int(command, 0);
        set_audio_stream_cmd(player, arg_i1);
        break;
      case BG_PLAYER_CMD_SET_VIDEO_STREAM:
        arg_i1 = bg_msg_get_arg_int(command, 0);
        set_video_stream_cmd(player, arg_i1);
        break;
      case BG_PLAYER_CMD_SET_SUBTITLE_STREAM:
        arg_i1 = bg_msg_get_arg_int(command, 0);
        set_subtitle_stream_cmd(player, arg_i1);
        break;
      case BG_PLAYER_CMD_PAUSE:
        pause_cmd(player);
        break;
      }
    if(queue_locked)
      bg_msg_queue_unlock_read(player->command_queue);
    }
  
  return 1;
  }

static void * player_thread(void * data)
  {
  bg_player_t * player;
  gavl_time_t wait_time;
  int old_seconds = -1;
  int seconds;
  int do_exit;
  int state;
  gavl_time_t time;
  bg_msg_t * command;
  
  player = (bg_player_t*)data;

  bg_player_set_state(player, BG_PLAYER_STATE_STOPPED, NULL, NULL);

  wait_time = 10000;
  do_exit = 0;
  while(1)
    {
    /* Process commands */
    if(!process_commands(player))
      do_exit = 1;

    if(do_exit)
      break;

    state = bg_player_get_state(player);
    switch(state)
      {
      case BG_PLAYER_STATE_PLAYING:
      case BG_PLAYER_STATE_FINISHING:
      case BG_PLAYER_STATE_STILL:
        //        fprintf(stderr, "bg_player_time_get...");
        bg_player_time_get(player, 1, &time);
        //        fprintf(stderr, "done\n");
        seconds = time / GAVL_TIME_SCALE;
        if(seconds != old_seconds)
          {
          old_seconds = seconds;
          bg_msg_queue_list_send(player->message_queues,
                                 msg_time,
                                 &time);
          //          fprintf(stderr, "%d\n", seconds);
          }
        break;
      }
    /* Exit when still time is exceeded */
    if((state == BG_PLAYER_STATE_STILL) &&
       (player->track_info->duration != GAVL_TIME_UNDEFINED) &&
       (player->track_info->duration <= time))
      {
      //      stop_cmd(player, BG_PLAYER_STATE_CHANGING);
      command = bg_msg_queue_lock_write(player->command_queue);
      bg_msg_set_id(command, BG_PLAYER_CMD_SETSTATE);
      bg_msg_set_arg_int(command, 0, BG_PLAYER_STATE_FINISHING);
      bg_msg_queue_unlock_write(player->command_queue);
      }
    
    gavl_time_delay(&wait_time);
    }
  return (void*)0;
  }

void bg_player_run(bg_player_t * player)
  {
  pthread_create(&(player->player_thread),
                 (pthread_attr_t*)0,
                 player_thread, player);

  }

void bg_player_quit(bg_player_t *player)
  {
  bg_msg_t * msg;
  msg = bg_msg_queue_lock_write(player->command_queue);
  bg_msg_set_id(msg, BG_PLAYER_CMD_QUIT);
  bg_msg_queue_unlock_write(player->command_queue);
  
  //  pthread_cancel(player->player_thread);
  pthread_join(player->player_thread, (void**)0);
  }
