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

static void msg_num_streams(bg_msg_t * msg,
                            const void * data)
  {
  bg_track_info_t * info;
  info = (bg_track_info_t *)data;
  bg_msg_set_id(msg, BG_PLAYER_MSG_TRACK_NUM_STREAMS);

  bg_msg_set_arg_int(msg, 0, info->num_audio_streams);
  bg_msg_set_arg_int(msg, 1, info->num_video_streams);
  bg_msg_set_arg_int(msg, 2, info->num_subpicture_streams);
  }

static void msg_video_description(bg_msg_t * msg, const void * data)
  {
  bg_msg_set_id(msg, BG_PLAYER_MSG_VIDEO_DESCRIPTION);
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



static void msg_duration(bg_msg_t * msg,
                         const void * data)
  {
  bg_track_info_t * info;
  info = (bg_track_info_t *)data;
  bg_msg_set_id(msg, BG_PLAYER_MSG_TRACK_DURATION);
  bg_msg_set_arg_time(msg, 0, info->duration);
  
  }

/*
 *  Interrupt playback so all plugin threads are waiting inside
 *  keep_going();
 */

static void interrupt_cmd(bg_player_t * p, int new_state)
  {
  pthread_mutex_lock(&(p->stop_mutex));
  /* Set the new state */

  bg_player_set_state(p, new_state, NULL, NULL);

  /* Tell it to the fifos */

  if(p->do_audio)
    bg_fifo_set_state(p->audio_stream.fifo, BG_FIFO_PAUSED);
  if(p->do_video)
    bg_fifo_set_state(p->video_stream.fifo, BG_FIFO_PAUSED);

  if(p->waiting_plugin_threads < p->total_plugin_threads)
    pthread_cond_wait(&(p->stop_cond), &(p->stop_mutex));

  pthread_mutex_unlock(&p->stop_mutex);
  bg_player_time_stop(p);

  if(p->do_audio)
    bg_player_oa_stop(p->oa_context);
  }

/* Preload fifos */

static void preload(bg_player_t * p)
  {
  if(p->do_audio)
    bg_fifo_set_state(p->audio_stream.fifo, BG_FIFO_NORMAL);
  if(p->do_video)
    bg_fifo_set_state(p->video_stream.fifo, BG_FIFO_NORMAL);
  
  bg_player_input_preload(p->input_context);

  }

/* Start playback */

static void start_playback(bg_player_t * p)
  {
  pthread_mutex_lock(&(p->start_mutex));
  bg_player_set_state(p, BG_PLAYER_STATE_PLAYING, &(p->can_seek), NULL);
  //  fprintf(stderr, "start_playback\n");

  /* Initialize timing */
  
  bg_player_time_start(p);

  if(p->do_audio)
    bg_player_oa_start(p->oa_context);
  
  pthread_cond_broadcast(&(p->start_cond));
  pthread_mutex_unlock(&(p->start_mutex));
  }

/* Pause command */

static void pause_cmd(bg_player_t * p)
  {
  int state;
  state = bg_player_get_state(p);

  if(state == BG_PLAYER_STATE_PLAYING)
    {
    interrupt_cmd(p, BG_PLAYER_STATE_PAUSED);

    
    }
  else if(state == BG_PLAYER_STATE_PAUSED)
    {
    preload(p);
    start_playback(p);
    }
  }


/* Play a file. This must be called ONLY if the player is in
   a defined stopped state
*/


static void play_cmd(bg_player_t * p,
                     bg_plugin_handle_t * handle,
                     int track_index)
  {
  int had_video;

  had_video = p->do_video;
  
  bg_player_set_state(p, BG_PLAYER_STATE_STARTING, NULL, NULL);
  
  bg_player_input_init(p->input_context,
                       handle, track_index);

  if(!bg_player_audio_init(p, 0))
    {
    bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                        "Cannot setup audio playback", NULL);
    return;
    }
  if(!bg_player_video_init(p, 0))
    {
    bg_player_set_state(p, BG_PLAYER_STATE_ERROR,
                        "Cannot setup video playback", NULL);
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
  bg_msg_queue_list_send(p->message_queues,
                         msg_duration,
                         p->track_info);

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
  else if(had_video)
    bg_player_ov_standby(p->ov_context);
  
  /* Count the threads */

  p->total_plugin_threads = 1;
  if(p->do_audio)
    p->total_plugin_threads++;
  if(p->do_video)
    p->total_plugin_threads++;

  /* Reset variables */

  p->waiting_plugin_threads = 0;
    
  /* Start playback */
  
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

  
  pthread_mutex_lock(&(p->stop_mutex));
  if(p->waiting_plugin_threads < p->total_plugin_threads)
    pthread_cond_wait(&(p->stop_cond), &(p->stop_mutex));
  pthread_mutex_unlock(&p->stop_mutex);
  
  bg_player_time_set(p, 0);
  preload(p);
  start_playback(p);
  }

static void player_cleanup(bg_player_t * player)
  {
  gavl_time_t t = 0;
  bg_player_input_cleanup(player->input_context);

  //  fprintf(stderr, "bg_player_oa_cleanup...");
  
  bg_player_oa_cleanup(player->oa_context);
  //  fprintf(stderr, "bg_player_oa_cleanup done\n");
  
  bg_player_ov_cleanup(player->ov_context);
  
  bg_player_time_stop(player);
  
  bg_player_video_cleanup(player);
  bg_player_audio_cleanup(player);
  bg_player_time_reset(player);

  bg_msg_queue_list_send(player->message_queues,
                         msg_time,
                         &t);
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
    want_new = 0;
    bg_player_set_state(player, new_state, &want_new, NULL);
    }
  else
    bg_player_set_state(player, new_state, NULL, NULL);

  if(old_state == BG_PLAYER_STATE_CHANGING)
    {
    if(new_state == BG_PLAYER_STATE_STOPPED)
      {
      if(player->do_video)
        {
        bg_player_ov_standby(player->ov_context);
        player->do_video = 0;
        }
      }
    return;
    }

  if(old_state == BG_PLAYER_STATE_PLAYING)
    {  
    /* Set the stop flag */
    if(player->do_audio)
      bg_fifo_set_state(player->audio_stream.fifo, BG_FIFO_STOPPED);
    if(player->do_video)
      bg_fifo_set_state(player->video_stream.fifo, BG_FIFO_STOPPED);
    
    fprintf(stderr, "Joining input thread...");
    pthread_join(player->input_thread, (void**)0);
    fprintf(stderr, "done\n");
    
    if(player->do_audio)
      {
      fprintf(stderr, "Joining audio thread...");
      pthread_join(player->oa_thread, (void**)0);
      fprintf(stderr, "done\n");
      }
    if(player->do_video)
      {
      fprintf(stderr, "Joining video thread...");
      pthread_join(player->ov_thread, (void**)0);
      fprintf(stderr, "done\n");
      }
    if(player->do_audio)
      bg_player_oa_stop(player->oa_context);
    player_cleanup(player);
    
    if(new_state == BG_PLAYER_STATE_STOPPED)
      {
      if(player->do_video)
        {
        bg_player_ov_standby(player->ov_context);
        player->do_video = 0;
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
  if(state == BG_PLAYER_STATE_PLAYING)
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
  if(state == BG_PLAYER_STATE_PLAYING)
    stop_cmd(player, BG_PLAYER_STATE_STOPPED);
  bg_player_oa_set_plugin(player, handle);
  }

static void seek_cmd(bg_player_t * player, gavl_time_t t)
  {
  gavl_video_frame_t * vf;
  //  fprintf(stderr, "Seek cmd\n");
  interrupt_cmd(player, BG_PLAYER_STATE_SEEKING);

  bg_player_input_seek(player->input_context, t);

  /* Clear fifos */

  if(player->do_audio)
    {
    bg_fifo_clear(player->audio_stream.fifo);
    }
  if(player->do_video)
    {
    bg_fifo_clear(player->video_stream.fifo);
    }
    
  /* Resync */
  //  fprintf(stderr, "Preload\n");
  preload(player);
  //  fprintf(stderr, "Preload done\n");
  
  if(player->do_video)
    {
    /* Ok, now we set the time from the video stream */
    vf = bg_fifo_get_read(player->video_stream.fifo);
    bg_player_time_set(player, vf->time);
    //    fprintf(stderr, "Time is now: %lld\n", vf->time);
    }
  else
    bg_player_time_set(player, t);
  start_playback(player);
  }


/* Process command, return FALSE if thread should be ended */

static int process_command(bg_player_t * player,
                            bg_msg_t * command)
  {
  int arg_i1;
  float arg_f1;
  int state;
  void * arg_ptr1;
  char * arg_str1;
  int next_track;
  int error_code;
  gavl_time_t time;
  gavl_time_t current_time;
  
  gavl_video_format_t logo_format;
  gavl_video_frame_t * logo_frame;
  
  //  fprintf(stderr, "process_command\n");
  
  switch(bg_msg_get_id(command))
    {
    case BG_PLAYER_CMD_QUIT:
      return 0;
      break;
    case BG_PLAYER_CMD_PLAY:
      //      fprintf(stderr, "Command play\n");
      arg_i1   = bg_msg_get_arg_int(command, 2);
      state = bg_player_get_state(player);
      
      if(arg_i1)
        {
        if((state == BG_PLAYER_STATE_PLAYING) &&
           (arg_i1 & BG_PLAYER_IGNORE_IF_PLAYING))
          return 1;
        else if((state == BG_PLAYER_STATE_STOPPED) &&
           (arg_i1 & BG_PLAYER_IGNORE_IF_STOPPED))
          return 1;
        }

      arg_ptr1 = bg_msg_get_arg_ptr_nocopy(command, 0);
      arg_i1   = bg_msg_get_arg_int(command, 1);

      if(state == BG_PLAYER_STATE_PLAYING)
        stop_cmd(player, BG_PLAYER_STATE_CHANGING);

      if(!arg_ptr1)
        {
        error_code = BG_PLAYER_ERROR_GENERAL;
        bg_player_set_state(player, BG_PLAYER_STATE_ERROR,
                            "No Track selected", &error_code);
        
        }
      else
        play_cmd(player, arg_ptr1, arg_i1);
      
      //      fprintf(stderr, "Command playfile %s\n", arg_str1);
                  
      break;
    case BG_PLAYER_CMD_STOP:
      state = bg_player_get_state(player);
      //      fprintf(stderr, "Command stop\n");
      switch(state)
        {
        case BG_PLAYER_STATE_PLAYING:
        case BG_PLAYER_STATE_CHANGING:
          stop_cmd(player, BG_PLAYER_STATE_STOPPED);
          break;
        }
      return 1;
      break;
    case BG_PLAYER_CMD_SEEK:
      if(!player->can_seek)
        {
        fprintf(stderr, "Cannot seek in this stream\n");
        }
      else
        {
        arg_f1 = bg_msg_get_arg_float(command, 0);
        time = (gavl_time_t)(player->track_info->duration*arg_f1);
        seek_cmd(player, time);
        }
      break;
    case BG_PLAYER_CMD_SEEK_REL:
      if(!player->can_seek)
        {
        fprintf(stderr, "Cannot seek in this stream\n");
        }
      else
        {
        bg_player_time_get(player, 1, &current_time);
        time = bg_msg_get_arg_time(command, 0);
        seek_cmd(player, current_time + time);
        }
      break;
      

    case BG_PLAYER_CMD_SETSTATE:
      arg_i1 = bg_msg_get_arg_int(command, 0);
      switch(arg_i1)
        {
        case BG_PLAYER_STATE_FINISHING:
          /* Close down everything */
          //          fprintf(stderr, "Finishing...\n");
          bg_player_set_state(player, BG_PLAYER_STATE_FINISHING, NULL, NULL);
          
          pthread_join(player->input_thread, (void**)0);

          if(player->do_audio)
            pthread_join(player->oa_thread, (void**)0);
          if(player->do_video)
            pthread_join(player->ov_thread, (void**)0);

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
    case BG_PLAYER_CMD_SETLOGO:
      //      fprintf(stderr, "***** Set Logo\n");
      bg_msg_get_arg_video_format(command, 0, &logo_format);
      (gavl_video_frame_t*)logo_frame = bg_msg_get_arg_ptr_nocopy(command, 1);
      bg_player_ov_set_logo(player->ov_context, &logo_format, logo_frame);
      if(!player->do_video)
        bg_player_ov_standby(player->ov_context);
      break;
    case BG_PLAYER_CMD_PAUSE:
      pause_cmd(player);
      break;
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
  
  player = (bg_player_t*)data;
  bg_msg_t * command;

  bg_player_set_state(player, BG_PLAYER_STATE_STOPPED, NULL, NULL);

  wait_time = 10000;
  do_exit = 0;
  while(1)
    {
    /* Process commands */

    //    fprintf(stderr, "Blupp 1\n");
    command = bg_msg_queue_try_lock_read(player->command_queue);
    if(command)
      {
      if(!process_command(player, command))
        do_exit = 1;
      bg_msg_queue_unlock_read(player->command_queue);
      }

    if(do_exit)
      break;

    //    fprintf(stderr, "Blupp 2\n");

    state = bg_player_get_state(player);

    switch(state)
      {
      case BG_PLAYER_STATE_PLAYING:
      case BG_PLAYER_STATE_FINISHING:
        bg_player_time_get(player, 1, &time);
        seconds = time / GAVL_TIME_SCALE;
        if(seconds != old_seconds)
          {
          old_seconds = seconds;
          bg_msg_queue_list_send(player->message_queues,
                                 msg_time,
                                 &time);
          //      fprintf(stderr, "%d\n", seconds);
          }
        break;
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
