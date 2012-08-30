/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <gavl/gavl.h>
#include <gmerlin/frametimer.h>

#define TIME_SCALE 10000

struct bg_frame_timer_s
  {
  int frame_duration;
  
  gavl_timer_t * timer;
  int64_t next_pts;
  int64_t last_pts;
  
  gavl_time_t capture_start_time;
  gavl_time_t last_capture_duration;
  
  int limit_fps;
  };

bg_frame_timer_t * bg_frame_timer_create(float framerate,
                                         uint32_t * timescale)
  {
  bg_frame_timer_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  ret->timer = gavl_timer_create();
  ret->next_pts = GAVL_TIME_UNDEFINED;
  
  ret->frame_duration = (int)(TIME_SCALE / framerate);
  return ret;
  }

void bg_frame_timer_destroy(bg_frame_timer_t * t)
  {
  gavl_timer_destroy(t->timer);
  free(t);
  }

void bg_frame_timer_update(bg_frame_timer_t * t,
                           gavl_video_frame_t * frame)
  {
  int64_t diff;
  gavl_time_t current_time;

  current_time = gavl_timer_get(t->timer);
  if(t->limit_fps)
    t->last_capture_duration = current_time - t->capture_start_time;
  
  if(t->next_pts == GAVL_TIME_UNDEFINED)
    {
    frame->timestamp = 0;
    frame->duration = t->frame_duration;
    gavl_timer_start(t->timer);
    t->next_pts = frame->duration;
    return;
    }
  
  frame->timestamp = t->next_pts;
  
  /*
   * Diff > 0 -> frame too early
   * Diff < 0 -> frame too late
   */
  
  diff = t->next_pts - gavl_time_scale(TIME_SCALE, current_time);
  
  if(t->limit_fps)
    {
    //    f->duration = 
    }
  else
    {
    
    }
  t->next_pts += frame->duration;
  
  }

void bg_frame_timer_wait(bg_frame_timer_t * t)
  {
  gavl_time_t cur, diff;
  
  t->limit_fps = 1;
  if(t->next_pts == GAVL_TIME_UNDEFINED)
    return;

  cur = gavl_timer_get(t->timer);
  diff = gavl_time_unscale(TIME_SCALE, t->next_pts) - cur -
    t->last_capture_duration;
  
  if(diff > 0)
    gavl_time_delay(&diff);
  
  t->capture_start_time = gavl_timer_get(t->timer);
  }
