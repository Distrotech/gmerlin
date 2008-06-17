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

#include <gavl/gavl.h>
#include <edl.h>
#include <utils.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static void bg_dprintf(const char * format, ...)
  {
  va_list argp; /* arg ptr */
  va_start( argp, format);
  vfprintf(stderr, format, argp);
  va_end(argp);
  }

static void bg_diprintf(int indent, const char * format, ...)
  {
  int i;
  va_list argp; /* arg ptr */
  for(i = 0; i < indent; i++)
    bg_dprintf( " ");
  
  va_start( argp, format);
  vfprintf(stderr, format, argp);
  va_end(argp);
  }


bg_edl_t * bg_edl_create()
  {
  bg_edl_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

bg_edl_track_t * bg_edl_add_track(bg_edl_t * e)
  {
  e->tracks = realloc(e->tracks, (e->num_tracks+1)*sizeof(*e->tracks));
  memset(e->tracks + e->num_tracks, 0, sizeof(*e->tracks));
  e->num_tracks++;
  return e->tracks + (e->num_tracks-1);
  }

bg_edl_stream_t * bg_edl_add_audio_stream(bg_edl_track_t * t)
  {
  t->audio_streams = realloc(t->audio_streams, (t->num_audio_streams+1)*sizeof(*t->audio_streams));
  memset(t->audio_streams + t->num_audio_streams, 0, sizeof(*t->audio_streams));
  t->num_audio_streams++;
  return t->audio_streams + (t->num_audio_streams-1);
  }

bg_edl_stream_t * bg_edl_add_video_stream(bg_edl_track_t * t)
  {
  t->video_streams = realloc(t->video_streams, (t->num_video_streams+1)*sizeof(*t->video_streams));
  memset(t->video_streams + t->num_video_streams, 0, sizeof(*t->video_streams));
  t->num_video_streams++;
  return t->video_streams + (t->num_video_streams-1);
  
  }

bg_edl_stream_t * bg_edl_add_subtitle_text_stream(bg_edl_track_t * t)
  {
  t->subtitle_text_streams = realloc(t->subtitle_text_streams, (t->num_subtitle_text_streams+1)*sizeof(*t->subtitle_text_streams));
  memset(t->subtitle_text_streams + t->num_subtitle_text_streams, 0, sizeof(*t->subtitle_text_streams));
  t->num_subtitle_text_streams++;
  return t->subtitle_text_streams + (t->num_subtitle_text_streams-1);
  }

bg_edl_stream_t * bg_edl_add_subtitle_overlay_stream(bg_edl_track_t * t)
  {
  t->subtitle_overlay_streams = realloc(t->subtitle_overlay_streams, (t->num_subtitle_overlay_streams+1)*sizeof(*t->subtitle_overlay_streams));
  memset(t->subtitle_overlay_streams + t->num_subtitle_overlay_streams, 0, sizeof(*t->subtitle_overlay_streams));
  t->num_subtitle_overlay_streams++;
  return t->subtitle_overlay_streams + (t->num_subtitle_overlay_streams-1);
  }

bg_edl_segment_t * bg_edl_add_segment(bg_edl_stream_t * s)
  {
  s->segments = realloc(s->segments, (s->num_segments+1)*sizeof(*s->segments));
  memset(s->segments + s->num_segments, 0, sizeof(*s->segments));
  s->num_segments++;
  return s->segments + (s->num_segments-1);
  }

static bg_edl_segment_t * copy_segments(const bg_edl_segment_t * src, int len)
  {
  int i;
  bg_edl_segment_t * ret;
  ret = calloc(len, sizeof(*ret));

  /* Copy integers */
  memcpy(ret, src, len * sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */
    ret[i].url = bg_strdup((char*)0, src[i].url);
    }
  return ret;
  }


static bg_edl_stream_t * copy_streams(const bg_edl_stream_t * src, int len)
  {
  int i;
  bg_edl_stream_t * ret;
  ret = calloc(len, sizeof(*ret));
  
  /* Copy integers */
  memcpy(ret, src, len * sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */
    ret[i].segments = copy_segments(src[i].segments, src[i].num_segments);
    }
  return ret;
  }

static bg_edl_track_t * copy_tracks(const bg_edl_track_t * src, int len)
  {
  int i;
  bg_edl_track_t * ret;
  ret = calloc(len, sizeof(*ret));
  
  /* Copy integers */
  memcpy(ret, src, len * sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */

    ret[i].name = bg_strdup((char*)0, src[i].name);
    
    ret[i].audio_streams = copy_streams(src[i].audio_streams,
                                        src[i].num_audio_streams);
    ret[i].video_streams = copy_streams(src[i].video_streams,
                                        src[i].num_video_streams);
    ret[i].subtitle_text_streams = copy_streams(src[i].subtitle_text_streams,
                                           src[i].num_subtitle_text_streams);
    ret[i].subtitle_overlay_streams = copy_streams(src[i].subtitle_overlay_streams,
                                           src[i].num_subtitle_overlay_streams);
    }
  return ret;
  }

bg_edl_t * bg_edl_copy(const bg_edl_t * e)
  {
  bg_edl_t * ret;
  ret = calloc(1, sizeof(*ret));

  /* Copy integers */
  memcpy(ret, e, sizeof(*ret));
  
  /* Copy pointers */
  ret->tracks = copy_tracks(e->tracks, e->num_tracks);
  ret->url = bg_strdup((char*)0, e->url);
  return ret;
  }

static void free_segments(bg_edl_segment_t * s, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    if(s[i].url) free(s[i].url);
    }
  free(s);
  }

static void free_streams(bg_edl_stream_t * s, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    if(s[i].segments) free_segments(s[i].segments, s[i].num_segments);
    }
  free(s);
  }

static void free_tracks(bg_edl_track_t * s, int len)
  {
  int i;
  if(s->name)
    free(s->name);
  for(i = 0; i < len; i++)
    {
    if(s[i].audio_streams)
      free_streams(s[i].audio_streams, s[i].num_audio_streams);
    if(s[i].video_streams)
      free_streams(s[i].video_streams, s[i].num_video_streams);
    if(s[i].subtitle_text_streams)
      free_streams(s[i].subtitle_text_streams, s[i].num_subtitle_text_streams);
    if(s[i].subtitle_overlay_streams)
      free_streams(s[i].subtitle_overlay_streams, s[i].num_subtitle_overlay_streams);
    }
  free(s);
  }

void bg_edl_destroy(bg_edl_t * e)
  {
  if(e->tracks) free_tracks(e->tracks, e->num_tracks);
  if(e->url) free(e->url);
  free(e);
  }

static void dump_stream(const bg_edl_stream_t* s)
  {
  int i;
  bg_edl_segment_t * seg;
  bg_diprintf(8, "Timescale: %d\n", s->timescale);
  bg_diprintf(8, "Segments:  %d\n", s->num_segments);
  for(i = 0; i < s->num_segments; i++)
    {
    seg = &s->segments[i];
    bg_diprintf(8, "Segment\n");
    bg_diprintf(10, "URL:                  %s\n", (seg->url ? seg->url : "(null)"));
    bg_diprintf(10, "Track:                %d\n", seg->track);
    bg_diprintf(10, "Stream index:         %d\n", seg->stream);
    bg_diprintf(10, "Source timescale:     %d\n", seg->timescale);
    bg_diprintf(10, "Source time:          %" PRId64 "\n", seg->src_time);
    bg_diprintf(10, "Destination time:     %" PRId64 "\n", seg->dst_time);
    bg_diprintf(10, "Destination duration: %" PRId64 "\n", seg->dst_duration);
    bg_diprintf(10, "Playback speed:       %.3f [%d/%d]\n",
                  (float)(seg->speed_num) / (float)(seg->speed_den),
                  seg->speed_num, seg->speed_den);
    }
  }

static void dump_track(const bg_edl_track_t * t)
  {
  int i;
  bg_diprintf(2, "Track: %s\n", t->name);
  bg_diprintf(4, "Audio streams: %d\n", t->num_audio_streams);
  for(i = 0; i < t->num_audio_streams; i++)
    {
    bg_diprintf(6, "Audio stream\n");
    dump_stream(&t->audio_streams[i]);
    }
  
  bg_diprintf(4, "Video streams: %d\n", t->num_video_streams);
  for(i = 0; i < t->num_video_streams; i++)
    {
    bg_diprintf(6, "Video stream\n");
    dump_stream(&t->video_streams[i]);
    }

  bg_diprintf(4, "Subtitle text streams: %d\n", t->num_subtitle_text_streams);
  for(i = 0; i < t->num_subtitle_text_streams; i++)
    {
    bg_diprintf(6, "Subtitle text stream\n");
    dump_stream(&t->subtitle_text_streams[i]);
    }
  bg_diprintf(4, "Subtitle overlay streams: %d\n", t->num_subtitle_overlay_streams);
  for(i = 0; i < t->num_subtitle_overlay_streams; i++)
    {
    bg_diprintf(6, "Subtitle overlay stream\n");
    dump_stream(&t->subtitle_overlay_streams[i]);
    }
  }

void bg_edl_dump(const bg_edl_t * e)
  {
  int i;
  bg_dprintf("EDL\n");
  bg_diprintf(2, "URL:    %s\n", (e->url ? e->url : "(null)"));
  bg_diprintf(2, "Tracks: %d\n", e->num_tracks);
  for(i = 0; i < e->num_tracks; i++)
    {
    dump_track(&e->tracks[i]);
    }
  }

void bg_edl_append_track_info(bg_edl_t * e, const bg_track_info_t * info,
                              const char * url, int index, int total_tracks)
  {
  bg_edl_track_t * t;
  bg_edl_stream_t * s;
  bg_edl_segment_t * seg;
  int i;
  
  t = bg_edl_add_track(e);

  t->name = bg_get_track_name_default(url, index, total_tracks);
  
  for(i = 0; i < info->num_audio_streams; i++)
    {
    s = bg_edl_add_audio_stream(t);
    seg = bg_edl_add_segment(s);
    s->timescale = info->audio_streams[i].format.samplerate;
    seg->timescale = s->timescale;
    
    if(info->audio_streams[i].duration)
      seg->dst_duration = info->audio_streams[i].duration;
    else
      seg->dst_duration = gavl_time_rescale(GAVL_TIME_SCALE, s->timescale, info->duration);
    seg->speed_num = 1;
    seg->speed_den = 1;
    seg->url = bg_strdup((char*)0, url);
    seg->track = index;
    seg->stream = i;
    }
  for(i = 0; i < info->num_video_streams; i++)
    {
    s = bg_edl_add_video_stream(t);
    seg = bg_edl_add_segment(s);
    s->timescale = info->video_streams[i].format.timescale;
    seg->timescale = s->timescale;

    if(info->video_streams[i].duration)
      seg->dst_duration = info->video_streams[i].duration;
    else
      seg->dst_duration = gavl_time_rescale(GAVL_TIME_SCALE, s->timescale, info->duration);
    seg->speed_num = 1;
    seg->speed_den = 1;
    seg->url = bg_strdup((char*)0, url);
    seg->track = index;
    seg->stream = i;
    }
  for(i = 0; i < info->num_subtitle_streams; i++)
    {
    if(info->subtitle_streams[i].is_text)
      s = bg_edl_add_subtitle_text_stream(t);
    else
      s = bg_edl_add_subtitle_overlay_stream(t);
    seg = bg_edl_add_segment(s);
    s->timescale = info->subtitle_streams[i].format.timescale;
    seg->timescale = s->timescale;

    if(info->subtitle_streams[i].duration)
      seg->dst_duration = info->subtitle_streams[i].duration;
    else
      seg->dst_duration = gavl_time_rescale(GAVL_TIME_SCALE, s->timescale, info->duration);
    seg->speed_num = 1;
    seg->speed_den = 1;
    seg->url = bg_strdup((char*)0, url);
    seg->track = index;
    seg->stream = i;
    }
  }
