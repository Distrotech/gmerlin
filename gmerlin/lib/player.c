/*****************************************************************
 
  player.c
 
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

int bg_player_keep_going(bg_player_t * p)
  {
  int state = bg_player_get_state(p);
  switch(state)
    {
    case BG_PLAYER_STATE_STOPPED:
    case BG_PLAYER_STATE_CHANGING:
      return 0;
    case BG_PLAYER_STATE_PLAYING:
    case BG_PLAYER_STATE_FINISHING:
      break;
    case BG_PLAYER_STATE_STARTING:
    case BG_PLAYER_STATE_PAUSED:
    case BG_PLAYER_STATE_SEEKING:
    case BG_PLAYER_STATE_BUFFERING:

      /* Suspend the thread until restart */

      pthread_mutex_lock(&(p->start_mutex));

      /* If we are the last thread to stop, tell the player
         to continue */
      pthread_mutex_lock(&p->stop_mutex);
      p->waiting_plugin_threads++;
      if(p->waiting_plugin_threads == p->total_plugin_threads)
        {
        pthread_cond_broadcast(&(p->stop_cond));
        }
      pthread_mutex_unlock(&p->stop_mutex);

      pthread_cond_wait(&(p->start_cond), &(p->start_mutex));
      p->waiting_plugin_threads--;
      pthread_mutex_unlock(&(p->start_mutex));
      break;
    }
  return 1;
  }

bg_player_t * bg_player_create()
  {
  bg_player_t * ret;
    
  ret = calloc(1, sizeof(*ret));
    
  /* Create contexts */

  bg_player_audio_create(ret);
  bg_player_video_create(ret);
  
  bg_player_input_create(ret);
  bg_player_oa_create(ret);
  bg_player_ov_create(ret);

  /* Create message queues */

  ret->command_queue  = bg_msg_queue_create();
  ret->to_oa_queue    = bg_msg_queue_create();
  ret->to_ov_queue    = bg_msg_queue_create();
  ret->to_input_queue = bg_msg_queue_create();
  
  ret->message_queues = bg_msg_queue_list_create();

  pthread_mutex_init(&(ret->state_mutex),(pthread_mutexattr_t *)0);
    
  return ret;
  }

void bg_player_destroy(bg_player_t * player)
  {
  bg_player_input_destroy(player);
  bg_player_oa_destroy(player);
  bg_player_ov_destroy(player);
  bg_player_audio_destroy(player);
  bg_player_video_destroy(player);
  
  bg_msg_queue_destroy(player->command_queue);
  bg_msg_queue_destroy(player->to_oa_queue);
  bg_msg_queue_destroy(player->to_ov_queue);
  bg_msg_queue_destroy(player->to_input_queue);
 
  bg_msg_queue_list_destroy(player->message_queues);

  pthread_mutex_destroy(&(player->state_mutex));
  
  free(player);
  }

bg_msg_queue_t * bg_player_get_command_queue(bg_player_t * player)
  {
  return player->command_queue;
  }

void bg_player_add_message_queue(bg_player_t * player,
                                 bg_msg_queue_t * message_queue)
  {
  bg_msg_queue_list_add(player->message_queues, message_queue);
  }

void bg_player_delete_message_queue(bg_player_t * player,
                                    bg_msg_queue_t * message_queue)
  {
  bg_msg_queue_list_remove(player->message_queues, message_queue);
  }


int  bg_player_get_state(bg_player_t * player)
  {
  int ret;
  pthread_mutex_lock(&(player->state_mutex));
  ret = player->state;
  pthread_mutex_unlock(&(player->state_mutex));
  return ret;
  }

struct state_struct
  {
  int state;
  float percentage;
  float can_seek;
  int want_new;
  const char * error_msg;
  };

static void msg_state(bg_msg_t * msg,
                      const void * data)
  {
  struct state_struct * s;
  s = (struct state_struct *)data;
  

  bg_msg_set_id(msg, BG_PLAYER_MSG_STATE_CHANGED);
  bg_msg_set_arg_int(msg, 0, s->state);

  if(s->state == BG_PLAYER_STATE_BUFFERING)
    {
    bg_msg_set_arg_float(msg, 1, s->percentage);
    }
  else if(s->state == BG_PLAYER_STATE_ERROR)
    {
    //    fprintf(stderr, "SET ERROR: %s\n", s->error_msg);
    bg_msg_set_arg_string(msg, 1, s->error_msg);
    }
  else if(s->state == BG_PLAYER_STATE_PLAYING)
    {
    bg_msg_set_arg_int(msg, 1, s->can_seek);
    }
  else if(s->state == BG_PLAYER_STATE_CHANGING)
    {
    bg_msg_set_arg_int(msg, 1, s->want_new);
    }
  }

void bg_player_set_state(bg_player_t * player, int state,
                         const void * arg1, const void * arg2)
  {
  struct state_struct s;
  pthread_mutex_lock(&(player->state_mutex));
  player->state = state;
  pthread_mutex_unlock(&(player->state_mutex));

  /* Broadcast this message */

  //  fprintf(stderr, "bg_player_set_state %d\n", state);
  
  //  memset(&state, 0, sizeof(state));
    
  s.state = state;

  if(state == BG_PLAYER_STATE_BUFFERING)
    s.percentage = *((const float*)arg1);
  else if(state == BG_PLAYER_STATE_ERROR)
    {
    s.error_msg =  (const char *)arg1;
    }
  
  else if(state == BG_PLAYER_STATE_PLAYING)
    s.can_seek = *((const int*)arg1);
  else if(state == BG_PLAYER_STATE_CHANGING)
    s.want_new = *((const int*)arg1);
  
  bg_msg_queue_list_send(player->message_queues,
                         msg_state,
                         &s);
  }

