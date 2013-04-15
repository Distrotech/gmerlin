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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <gavl/gavl.h>
#include <gavl/metadata.h>
#include <gavl/metatags.h>
#include <gavl/edl.h>

#include <gmerlin/edl.h>
#include <gmerlin/utils.h>

void bg_edl_append_track_info(gavl_edl_t * e, const bg_track_info_t * info,
                              const char * url, int index, int total_tracks,
                              const char * name)
  {
  gavl_edl_track_t * t;
  gavl_edl_stream_t * s;
  gavl_edl_segment_t * seg;
  int i;
  char * real_name;
  
  t = gavl_edl_add_track(e);

  if(name)
    real_name = gavl_strdup(name);
  else
    real_name = bg_get_track_name_default(url, index, total_tracks);
  
  gavl_metadata_copy(&t->metadata, &info->metadata);
  gavl_metadata_set_nocpy(&t->metadata, GAVL_META_LABEL, real_name);
  
  for(i = 0; i < info->num_audio_streams; i++)
    {
    s = gavl_edl_add_audio_stream(t);
    seg = gavl_edl_add_segment(s);
    s->timescale = info->audio_streams[i].format.samplerate;
    seg->timescale = s->timescale;
    
    if(info->audio_streams[i].duration)
      seg->dst_duration = info->audio_streams[i].duration;
    else
      {
      gavl_time_t duration;

      if(gavl_metadata_get_long(&info->metadata, GAVL_META_APPROX_DURATION, &duration))
        seg->dst_duration = gavl_time_rescale(GAVL_TIME_SCALE, s->timescale, duration);
      }
    seg->speed_num = 1;
    seg->speed_den = 1;
    seg->url = gavl_strdup(url);
    seg->track = index;
    seg->stream = i;
    }
  for(i = 0; i < info->num_video_streams; i++)
    {
    s = gavl_edl_add_video_stream(t);
    seg = gavl_edl_add_segment(s);
    s->timescale = info->video_streams[i].format.timescale;
    seg->timescale = s->timescale;

    if(info->video_streams[i].duration)
      seg->dst_duration = info->video_streams[i].duration;
    else
      {
      gavl_time_t duration;

      if(gavl_metadata_get_long(&info->metadata, GAVL_META_APPROX_DURATION, &duration))
        seg->dst_duration = gavl_time_rescale(GAVL_TIME_SCALE, s->timescale, duration);
      }
    seg->speed_num = 1;
    seg->speed_den = 1;
    seg->url = gavl_strdup(url);
    seg->track = index;
    seg->stream = i;
    }
  
  for(i = 0; i < info->num_text_streams; i++)
    {
    s = gavl_edl_add_text_stream(t);
    seg = gavl_edl_add_segment(s);
    s->timescale = info->text_streams[i].timescale;
    seg->timescale = s->timescale;

    if(info->text_streams[i].duration)
      seg->dst_duration = info->text_streams[i].duration;
    else
      {
      gavl_time_t duration;
      if(gavl_metadata_get_long(&info->metadata, GAVL_META_APPROX_DURATION, &duration))
        seg->dst_duration = gavl_time_rescale(GAVL_TIME_SCALE,
                                              s->timescale,
                                              duration);
      }
    seg->speed_num = 1;
    seg->speed_den = 1;
    seg->url = gavl_strdup(url);
    seg->track = index;
    seg->stream = i;
    }

  for(i = 0; i < info->num_overlay_streams; i++)
    {
    s = gavl_edl_add_overlay_stream(t);
    seg = gavl_edl_add_segment(s);
    s->timescale = info->overlay_streams[i].format.timescale;
    seg->timescale = s->timescale;

    if(info->overlay_streams[i].duration)
      seg->dst_duration = info->overlay_streams[i].duration;
    else
      {
      gavl_time_t duration;
      if(gavl_metadata_get_long(&info->metadata, GAVL_META_APPROX_DURATION, &duration))
        seg->dst_duration = gavl_time_rescale(GAVL_TIME_SCALE,
                                              s->timescale,
                                              duration);
      }
    seg->speed_num = 1;
    seg->speed_den = 1;
    seg->url = gavl_strdup(url);
    seg->track = index;
    seg->stream = i;
    }

  }
