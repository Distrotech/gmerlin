/*****************************************************************
 
  transcoder.c
 
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

#include <stdlib.h>
#include <string.h>

#include <pluginregistry.h>
#include <msgqueue.h>
#include <transcoder.h>
#include <transcodermsg.h>
#include <utils.h>
#include <log.h>

#define LOG_DOMAIN "postprocessor"

struct bg_transcoder_pp_s
  {
  bg_plugin_handle_t * plugin;
  bg_encoder_pp_plugin_t * pp_plugin;
  
  bg_msg_queue_t      * msg_in;
  bg_msg_queue_list_t * msg_out;

  /* Data, we save for each track */
  int num_audio_streams, num_video_streams;
  char ** audio_filename;
  char ** video_filename;
  char * filename;
  bg_metadata_t metadata;
  
  pthread_t thread;

  bg_e_pp_callbacks_t callbacks;

  gavl_timer_t * timer;
  gavl_time_t last_time;

  char * output_directory;
  int cleanup;
  int num_tracks;
  };

static void action_callback(void * data, char * action)
  {
  bg_transcoder_pp_t * p;
  p = (bg_transcoder_pp_t*)data;
  bg_transcoder_send_msg_start(p->msg_out, action);
  p->last_time = 0;
  //  fprintf(stderr, "transcoder_pp: action %s\n", action);
  }

static void progress_callback(void * data, float perc)
  {
  bg_transcoder_pp_t * p;
  gavl_time_t current_time;
  gavl_time_t remaining_time;
  p = (bg_transcoder_pp_t*)data;

  //  fprintf(stderr, "transcoder_pp: progress %.1f\n", perc * 100.0);
  if(perc == 0.0)
    {
    gavl_timer_stop(p->timer);
    gavl_timer_set(p->timer, 0);
    gavl_timer_start(p->timer);
    }
  current_time = gavl_timer_get(p->timer);
  
  if(current_time - p->last_time < GAVL_TIME_SCALE)
    return;

  p->last_time = current_time;
  
  if(perc <= 0.0)
    remaining_time = GAVL_TIME_UNDEFINED;
  else
    remaining_time = (gavl_time_t)((double)current_time * (1.0 / perc - 1.0) + 0.5);

  bg_transcoder_send_msg_progress(p->msg_out,
                                  perc, remaining_time);
  }

bg_transcoder_pp_t * bg_transcoder_pp_create()
  {
  bg_transcoder_pp_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->callbacks.action_callback = action_callback;
  ret->callbacks.progress_callback = progress_callback;
  ret->callbacks.data = ret;
  ret->timer = gavl_timer_create();

  ret->msg_out = bg_msg_queue_list_create();
  ret->msg_in =  bg_msg_queue_create();
  
  return ret;
  }

void bg_transcoder_pp_destroy(bg_transcoder_pp_t * p)
  {
  bg_msg_queue_list_destroy(p->msg_out);
  bg_msg_queue_destroy(p->msg_in);
  bg_metadata_free(&p->metadata);
  gavl_timer_destroy(p->timer);
  if(p->plugin)
    bg_plugin_unref(p->plugin);
  free(p);
  }

void bg_transcoder_pp_set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  bg_transcoder_pp_t * w = (bg_transcoder_pp_t*)data;
  
  if(!name)
    return;

  if(!strcmp(name, "output_directory"))
    {
    w->output_directory = bg_strdup(w->output_directory, val->val_str);
    }
#if 0
  else if(!strcmp(name, "send_finished"))
    {
    w->send_finished = val->val_i;
    }
#endif
  else if(!strcmp(name, "cleanup_pp"))
    {
    w->cleanup = val->val_i;
    }

  }

int bg_transcoder_pp_init(bg_transcoder_pp_t* p, bg_plugin_handle_t * plugin)
  {
  if(p->plugin)
    bg_plugin_unref(p->plugin);
  p->num_tracks = 0;
  p->plugin = plugin;
  bg_plugin_ref(p->plugin);
  p->pp_plugin = (bg_encoder_pp_plugin_t*)(p->plugin->plugin);

  if(p->pp_plugin->set_callbacks)
    p->pp_plugin->set_callbacks(p->plugin->priv, &(p->callbacks));
  
  if(!p->pp_plugin->init(p->plugin->priv))
    return 0;

  gavl_timer_start(p->timer);
  return 1;
  }

void bg_transcoder_pp_connect(bg_transcoder_pp_t * p, bg_transcoder_t * t)
  {
  bg_transcoder_add_message_queue(t, p->msg_in);
  }

void bg_transcoder_pp_update(bg_transcoder_pp_t * p)
  {
  bg_msg_t *msg;
  char * str = (char*)0;
  char * ext;
  int pp_only;
  //  fprintf(stderr, "bg_transcoder_pp_update\n");

  while((msg = bg_msg_queue_try_lock_read(p->msg_in)))
    {
    //    fprintf(stderr, "Got message\n");
    switch(bg_msg_get_id(msg))
      {
      case BG_TRANSCODER_MSG_START:
        break;
      case BG_TRANSCODER_MSG_FINISHED:
        break;
      case BG_TRANSCODER_MSG_METADATA:
        bg_metadata_free(&p->metadata);
        bg_msg_get_arg_metadata(msg, 0, &p->metadata);
        break;
      case BG_TRANSCODER_MSG_VIDEO_FILE:
        str = bg_msg_get_arg_string(msg, 1);
        pp_only = bg_msg_get_arg_int(msg, 2);
        break;
      case BG_TRANSCODER_MSG_AUDIO_FILE:
        str = bg_msg_get_arg_string(msg, 1);
        pp_only = bg_msg_get_arg_int(msg, 2);
        break;
      case BG_TRANSCODER_MSG_FILE:
        str = bg_msg_get_arg_string(msg, 0);
        pp_only = bg_msg_get_arg_int(msg, 1);
        break;
      case BG_TRANSCODER_MSG_PROGRESS:
        break;
      }
    if(str)
      {
      if(p->pp_plugin->common.extensions)
        {
        ext = strrchr(str, '.');
        if(!ext || !bg_string_match(ext+1, p->pp_plugin->common.extensions))
          {
          bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Not adding %s: Unsupported filename", str);
          free(str);
          str = (char*)0;
          }
        }
      if(str)
        {
        p->pp_plugin->add_track(p->plugin->priv, str, &p->metadata, pp_only);
        p->num_tracks++;
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Scheduling %s for postprocessing", str);
        //        fprintf(stderr, "Scheduling %s for postprocessing", str);
        free(str);
        str = (char*)0;
        }
      }
    bg_msg_queue_unlock_read(p->msg_in);
    }
  }



static void * thread_func(void * data)
  {
  bg_transcoder_pp_t * p;
  p = (bg_transcoder_pp_t *)data;
  if(p->num_tracks)
    p->pp_plugin->run(p->plugin->priv, p->output_directory, p->cleanup);
  else
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Skipping postprocessing (no suitable files found)");
  bg_transcoder_send_msg_finished(p->msg_out);
  return NULL;
  }

void bg_transcoder_pp_stop(bg_transcoder_pp_t * p)
  {
  if(p->pp_plugin && p->pp_plugin->stop)
    p->pp_plugin->stop(p->plugin->priv);
  }

void bg_transcoder_pp_run(bg_transcoder_pp_t * p)
  {
  pthread_create(&(p->thread),
                 (pthread_attr_t*)0, thread_func, p);
  }

void bg_transcoder_pp_finish(bg_transcoder_pp_t * p)
  {
  pthread_join(p->thread, NULL);
  }

void bg_transcoder_pp_add_message_queue(bg_transcoder_pp_t * p,
                                        bg_msg_queue_t * message_queue)
  {
  bg_msg_queue_list_add(p->msg_out, message_queue);
  }

