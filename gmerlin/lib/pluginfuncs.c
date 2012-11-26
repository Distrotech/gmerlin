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

#include <string.h>
#include <math.h>


#include <config.h>

#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>



int bg_encoder_cb_create_output_file(bg_encoder_callbacks_t * cb,
                                     const char * filename)
  {
  if(cb && cb->create_output_file)
    return cb->create_output_file(cb->data, filename);
  else
    return 1;
  }

int bg_encoder_cb_create_temp_file(bg_encoder_callbacks_t * cb,
                                   const char * filename)
  {
  if(cb && cb->create_temp_file)
    return cb->create_temp_file(cb->data, filename);
  else
    return 1;
  
  }

int bg_iw_cb_create_output_file(bg_iw_callbacks_t * cb,
                                const char * filename)
  {
  if(cb && cb->create_output_file)
    return cb->create_output_file(cb->data, filename);
  else
    return 1;
  }

int bg_encoder_set_framerate_parameter(bg_encoder_framerate_t * f,
                                       const char * name,
                                       const bg_parameter_value_t * val)
  {
  if(!strcmp(name, "default_timescale"))
    {
    f->timescale = val->val_i;
    return 1;
    }
  else if(!strcmp(name, "default_frame_duration"))
    {
    f->frame_duration = val->val_i;
    return 1;
    }
  return 0;
  }

void bg_encoder_set_framerate(const bg_encoder_framerate_t * f,
                              gavl_video_format_t * format)
  {
  if(format->framerate_mode != GAVL_FRAMERATE_CONSTANT)
    {
    format->framerate_mode = GAVL_FRAMERATE_CONSTANT;
    format->timescale      = f->timescale;
    format->frame_duration = f->frame_duration;
    }
  }

void
bg_encoder_set_framerate_nearest(const bg_encoder_framerate_t * rate_default,
                                 const bg_encoder_framerate_t * rates_supported,
                                 gavl_video_format_t * format)
  {
  int i, min_index = 0;
  double diff = 0.0, diff_min = 0.0;
  double rate_d;
  
  /* Set default is we don't already have one */
  bg_encoder_set_framerate(rate_default, format);
  
  /* Set to nearest value */

  rate_d =
    (double)(format->timescale) /
    (double)(format->frame_duration);

  i = 0;
  while(rates_supported[i].timescale)
    {
    diff = fabs(rate_d - (double)(rates_supported[i].timescale) /
                (double)(rates_supported[i].frame_duration));

    if(!i || (diff < diff_min))
      {
      diff_min = diff;
      min_index = i;
      }
    i++;
    }
  
  format->timescale      = rates_supported[min_index].timescale;
  format->frame_duration = rates_supported[min_index].frame_duration;
  }

/* PTS cache for encoders, which reorder frames */

#define PTS_CACHE_MAX 64

struct bg_encoder_pts_cache_s
  {
  int64_t frame_counter;
  int num_frames;
  
  struct
    {
    int64_t pts;
    int64_t duration;
    gavl_interlace_mode_t il;
    gavl_timecode_t tc;
    int64_t frame_num;
    } entries[PTS_CACHE_MAX];
  
  };

bg_encoder_pts_cache_t * bg_encoder_pts_cache_create(void)
  {
  bg_encoder_pts_cache_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_encoder_pts_cache_destroy(bg_encoder_pts_cache_t * c)
  {
  free(c);
  }

int bg_encoder_pts_cache_push_frame(bg_encoder_pts_cache_t * c, gavl_video_frame_t * f)
  {
  if(c->num_frames >= PTS_CACHE_MAX)
    return 0;

  c->entries[c->num_frames].pts       = f->timestamp;
  c->entries[c->num_frames].duration  = f->duration;
  c->entries[c->num_frames].il        = f->interlace_mode;
  c->entries[c->num_frames].tc        = f->timecode;
  c->entries[c->num_frames].frame_num = c->frame_counter++;
  
  c->num_frames++;
  return 1;
  }

int bg_encoder_pts_cache_pop_packet(bg_encoder_pts_cache_t * c, gavl_packet_t * p,
                                    int64_t frame_num, int64_t pts)
  {
  int i, idx = -1;

  if(pts != GAVL_TIME_UNDEFINED)
    {
    for(i = 0; i < c->num_frames; i++)
      {
      if(c->entries[i].pts == pts)
        {
        idx = i;
        break;
        }
      }
    }
  else
    {
    for(i = 0; i < c->num_frames; i++)
      {
      if(c->entries[i].frame_num == frame_num)
        {
        idx = i;
        break;
        }
      }
    }

  if(idx < 0)
    return 0;

  p->pts = c->entries[idx].pts;
  p->timecode = c->entries[idx].tc;
  p->duration = c->entries[idx].duration;
  p->interlace_mode = c->entries[idx].il;

  if(idx < c->num_frames-1)
    {
    memmove(&c->entries[idx], &c->entries[idx+1],
            sizeof(c->entries[idx]) * (c->num_frames-1-idx));
    }
  c->num_frames--;
  return 1;
  }
