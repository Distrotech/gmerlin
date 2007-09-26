/*****************************************************************
 
  player_video.c
 
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
#include <stdio.h>

#include "player.h"
#include "playerprivate.h"

#include <log.h>
#define LOG_DOMAIN "player.video"

void bg_player_video_create(bg_player_t * p, bg_plugin_registry_t * plugin_reg)
  {
  bg_gavl_video_options_init(&(p->video_stream.options));
  p->video_stream.fc =
    bg_video_filter_chain_create(&p->video_stream.options,
                                 plugin_reg);
  
  pthread_mutex_init(&(p->video_stream.config_mutex),(pthread_mutexattr_t *)0);
  }

void bg_player_video_destroy(bg_player_t * p)
  {
  pthread_mutex_destroy(&(p->video_stream.config_mutex));
  bg_gavl_video_options_free(&(p->video_stream.options));
  }

int bg_player_video_init(bg_player_t * player, int video_stream)
  {
  bg_player_video_stream_t * s;
  s = &(player->video_stream);

  s->in_func = bg_player_input_read_video;
  s->in_data = player->input_context;
  s->in_stream = player->current_video_stream;
  
  if(!DO_VIDEO(player) && !DO_STILL(player))
    return 1;

  if(!DO_SUBTITLE_ONLY(player))
    {
    bg_player_input_get_video_format(player->input_context);
    
    bg_video_filter_chain_connect_input(s->fc, s->in_func,
                                        s->in_data, s->in_stream);
    
    bg_video_filter_chain_init(s->fc, &s->input_format, &s->output_format);
    
    s->in_func = bg_video_filter_chain_read;
    s->in_data = s->fc;
    s->in_stream = 0;
    }
  
  if(!bg_player_ov_init(player->ov_context))
    return 0;

  if(!DO_SUBTITLE_ONLY(player))
    bg_video_filter_chain_set_out_format(s->fc, &(s->output_format));
  
  /* Initialize video fifo */

  if(DO_VIDEO(player))
    s->fifo = bg_fifo_create(NUM_VIDEO_FRAMES,
                             bg_player_ov_create_frame,
                             (void*)(player->ov_context));
  else if(DO_STILL(player))
    s->fifo = bg_fifo_create(1,
                             bg_player_ov_create_frame,
                             (void*)(player->ov_context));
  
  if(DO_SUBTITLE_ONLY(player))
    {
    /* Video output already initialized */
    bg_player_ov_set_subtitle_format(player->ov_context,
                                     &(player->subtitle_stream.format));

    s->in_func = bg_player_input_read_video_subtitle_only;
    s->in_data = player->input_context;
    s->in_stream = 0;
    }
  
  return 1;
  }

void bg_player_video_cleanup(bg_player_t * player)
  {
  if(player->video_stream.fifo)
    {
    bg_fifo_destroy(player->video_stream.fifo, bg_player_ov_destroy_frame,
                    (void*)(player->ov_context));
    player->video_stream.fifo = (bg_fifo_t*)0;
    }
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
#if 0
    {
      name:      "video",
      long_name: TRS("Video"),
      type:      BG_PARAMETER_SECTION,
    },
#endif
    BG_GAVL_PARAM_CONVERSION_QUALITY,
    BG_GAVL_PARAM_ALPHA,
    BG_GAVL_PARAM_RESAMPLE_CHROMA,
    {
      name: "still_framerate",
      long_name: TRS("Still image framerate"),
      type:      BG_PARAMETER_FLOAT,
      val_min:     { val_f:   1.0 },
      val_max:     { val_f: 100.0 },
      val_default: { val_f:  10.0 },
      num_digits:  2,
      help_string: TRS("Set framerate width which still images will be redisplayed periodically"),
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t * bg_player_get_video_parameters(bg_player_t * p)
  {
  return parameters;
  }

#if 0
void bg_player_set_video_parameter(void * data, char * name,
                                   bg_parameter_value_t * val)
  {
  bg_player_t * player = (bg_player_t*)data;

  if(!name)
    return;

  pthread_mutex_lock(&(player->video_stream.config_mutex));

  bg_gavl_video_set_parameter(&(player->video_stream.options),
                              name, val);
  
  pthread_mutex_unlock(&(player->video_stream.config_mutex));

  }
#endif

void bg_player_set_video_parameter(void * data, const char * name,
                                   const bg_parameter_value_t * val)
  {
  bg_player_t * p = (bg_player_t*)data;
  int need_restart = 0;
  int is_interrupted;
  int do_init;
  int check_restart;
  
  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);
  
  pthread_mutex_lock(&(p->video_stream.config_mutex));

  is_interrupted = p->video_stream.interrupted;
  
  bg_gavl_video_set_parameter(&(p->video_stream.options),
                              name, val);

  if(!do_init && !is_interrupted)
    check_restart = 1;
  else
    check_restart = 0;
  
  if(check_restart)
    need_restart = p->video_stream.options.options_changed;
  
  pthread_mutex_unlock(&(p->video_stream.config_mutex));

  if(!need_restart && check_restart)
    {
    bg_video_filter_chain_lock(p->video_stream.fc);
    need_restart =
        bg_video_filter_chain_need_restart(p->video_stream.fc);
    bg_video_filter_chain_unlock(p->video_stream.fc);
    }

  if(need_restart)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Restarting playback due to changed video options");
    bg_player_interrupt(p);
    
    pthread_mutex_lock(&(p->video_stream.config_mutex));
    p->video_stream.interrupted = 1;
    pthread_mutex_unlock(&(p->video_stream.config_mutex));
    }
  
  if(!name && is_interrupted)
    {
    bg_player_interrupt_resume(p);
    pthread_mutex_lock(&(p->video_stream.config_mutex));
    p->video_stream.interrupted = 0;
    pthread_mutex_unlock(&(p->video_stream.config_mutex));
    }
  }



bg_parameter_info_t *
bg_player_get_video_filter_parameters(bg_player_t * p)
  {
  return bg_video_filter_chain_get_parameters(p->video_stream.fc);
  }

void bg_player_set_video_filter_parameter(void * data, const char * name,
                                          const bg_parameter_value_t * val)
  {
  int need_rebuild = 0, need_restart = 0;
  int is_interrupted;
  int do_init;
  bg_player_t * p = (bg_player_t*)data;
  
  do_init = (bg_player_get_state(p) == BG_PLAYER_STATE_INIT);
  
  pthread_mutex_lock(&(p->video_stream.config_mutex));
  is_interrupted = p->video_stream.interrupted;
  pthread_mutex_unlock(&(p->video_stream.config_mutex));
  
  bg_video_filter_chain_lock(p->video_stream.fc);
  bg_video_filter_chain_set_parameter(p->video_stream.fc, name, val);
  
  need_rebuild =
    bg_video_filter_chain_need_rebuild(p->video_stream.fc);
  need_restart =
    bg_video_filter_chain_need_restart(p->video_stream.fc);
  
  bg_video_filter_chain_unlock(p->video_stream.fc);
  
  if(!do_init && (need_rebuild || need_restart) && !is_interrupted)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Restarting playback due to changed video filters");
    bg_player_interrupt(p);
    
    pthread_mutex_lock(&(p->video_stream.config_mutex));
    p->video_stream.interrupted = 1;
    pthread_mutex_unlock(&(p->video_stream.config_mutex));
    }
  if(!name && is_interrupted)
    {
    bg_player_interrupt_resume(p);
    pthread_mutex_lock(&(p->video_stream.config_mutex));
    p->video_stream.interrupted = 0;
    pthread_mutex_unlock(&(p->video_stream.config_mutex));
    }

  
  }
