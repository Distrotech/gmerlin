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

#include <config.h>
#include <stdlib.h>

#include <gmerlin/utils.h> 

#include <gmerlin/recorder.h>
#include <recorder_private.h>

bg_recorder_t * bg_recorder_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_recorder_t * ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = plugin_reg;
  ret->tc = bg_player_thread_common_create();
  
  bg_recorder_create_audio(ret);
  bg_recorder_create_video(ret);

  ret->th[0] = ret->as.th;
  ret->th[1] = ret->vs.th;
  
  return ret;
  }

void bg_recorder_destroy(bg_recorder_t * rec)
  {
  bg_recorder_destroy_audio(rec);
  bg_recorder_destroy_video(rec);

  bg_player_thread_common_destroy(rec->tc);

  free(rec->display_string);
  
  free(rec);
  }

int bg_recorder_run(bg_recorder_t * rec)
  {
  if(rec->as.active)
    {
    if(!bg_recorder_audio_init(rec))
      rec->as.active = 0;
    }
  
  if(rec->vs.active)
    {
    if(!bg_recorder_video_init(rec))
      rec->vs.active = 0;
    }

  
  if(rec->vs.active)
    bg_player_thread_set_func(rec->vs.th, bg_recorder_video_thread, rec);
  else
    bg_player_thread_set_func(rec->vs.th, NULL, NULL);

  
  if(rec->as.active)
    bg_player_thread_set_func(rec->as.th, bg_recorder_audio_thread, rec);
  else
    bg_player_thread_set_func(rec->as.th, NULL, NULL);
    
  
  bg_player_threads_init(rec->th, NUM_THREADS);
  bg_player_threads_start(rec->th, NUM_THREADS);

  rec->running = 1;
  
  return 1;
  }

void bg_recorder_stop(bg_recorder_t * rec)
  {
  bg_player_threads_join(rec->th, NUM_THREADS);
  rec->running = 0;
  }

void bg_recorder_set_display_string(bg_recorder_t * rec, const char * str)
  {
  rec->display_string = bg_strdup(rec->display_string, str);
  }
