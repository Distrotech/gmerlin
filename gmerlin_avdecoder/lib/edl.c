/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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

#include <avdec_private.h>
// #include <bgavedl.h>
#include <stdlib.h>
#include <string.h>

bgav_edl_t * bgav_edl_create()
  {
  bgav_edl_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

bgav_edl_track_t * bgav_edl_add_track(bgav_edl_t * e)
  {
  e->tracks = realloc(e->tracks, (e->num_tracks+1)*sizeof(*e->tracks));
  memset(e->tracks + e->num_tracks, 0, sizeof(*e->tracks));
  e->num_tracks++;
  return e->tracks + (e->num_tracks-1);
  }

bgav_edl_stream_t * bgav_edl_add_audio_stram(bgav_edl_track_t * t)
  {
  t->audio_streams = realloc(t->audio_streams, (t->num_audio_streams+1)*sizeof(*t->audio_streams));
  memset(t->audio_streams + t->num_audio_streams, 0, sizeof(*t->audio_streams));
  t->num_audio_streams++;
  return t->audio_streams + (t->num_audio_streams-1);
  }

bgav_edl_stream_t * bgav_edl_add_video_stram(bgav_edl_track_t * t)
  {
  t->video_streams = realloc(t->video_streams, (t->num_video_streams+1)*sizeof(*t->video_streams));
  memset(t->video_streams + t->num_video_streams, 0, sizeof(*t->video_streams));
  t->num_video_streams++;
  return t->video_streams + (t->num_video_streams-1);
  
  }

bgav_edl_stream_t * bgav_edl_add_subtitle_stram(bgav_edl_track_t * t)
  {
  t->subtitle_streams = realloc(t->subtitle_streams, (t->num_subtitle_streams+1)*sizeof(*t->subtitle_streams));
  memset(t->subtitle_streams + t->num_subtitle_streams, 0, sizeof(*t->subtitle_streams));
  t->num_subtitle_streams++;
  return t->subtitle_streams + (t->num_subtitle_streams-1);
  }

bgav_edl_segment_t * bgav_edl_add_segment(bgav_edl_stream_t * s)
  {
  s->segments = realloc(s->segments, (s->num_segments+1)*sizeof(*s->segments));
  memset(s->segments + s->num_segments, 0, sizeof(*s->segments));
  s->num_segments++;
  return s->segments + (s->num_segments-1);
  }

static bgav_edl_segment_t * copy_segments(const bgav_edl_segment_t * src, int len)
  {
  int i;
  bgav_edl_segment_t * ret;
  ret = calloc(len, sizeof(*ret));

  /* Copy integers */
  memcpy(ret, src, len * sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */
    ret[i].url = bgav_strdup(src[i].url);
    }
  return ret;
  }


static bgav_edl_stream_t * copy_streams(const bgav_edl_stream_t * src, int len)
  {
  int i;
  bgav_edl_stream_t * ret;
  ret = calloc(len, sizeof(*ret));
  
  /* Copy integers */
  memcpy(ret, src, len * sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */
    ret[i].segments = copy_segments(src[i].segments, len);
    }
  return ret;
  }

static bgav_edl_track_t * copy_tracks(const bgav_edl_track_t * src, int len)
  {
  int i;
  bgav_edl_track_t * ret;
  ret = calloc(len, sizeof(*ret));
  
  /* Copy integers */
  memcpy(ret, src, len * sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */
    ret[i].audio_streams = copy_streams(src[i].audio_streams,
                                        src[i].num_audio_streams);
    ret[i].video_streams = copy_streams(src[i].video_streams,
                                        src[i].num_video_streams);
    ret[i].subtitle_streams = copy_streams(src[i].subtitle_streams,
                                           src[i].num_subtitle_streams);
    }
  return ret;
  }

bgav_edl_t * bgav_edl_copy(const bgav_edl_t * e)
  {
  bgav_edl_t * ret;
  ret = calloc(1, sizeof(*ret));

  /* Copy integers */
  memcpy(ret, e, sizeof(*ret));
  
  /* Copy pointers */
  ret->tracks = copy_tracks(e->tracks, e->num_tracks);
  return ret;
  }

static void free_segments(bgav_edl_segment_t * s, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    if(s[i].url) free(s[i].url);
    }
  free(s);
  }

static void free_streams(bgav_edl_stream_t * s, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    if(s[i].segments) free_segments(s[i].segments, s[i].num_segments);
    }
  free(s);
  }

static void free_tracks(bgav_edl_track_t * s, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    if(s[i].audio_streams)
      free_streams(s[i].audio_streams, s[i].num_audio_streams);
    if(s[i].video_streams)
      free_streams(s[i].video_streams, s[i].num_video_streams);
    if(s[i].subtitle_streams)
      free_streams(s[i].subtitle_streams, s[i].num_subtitle_streams);
    }
  free(s);
  }

void bgav_edl_destroy(bgav_edl_t * e)
  {
  if(e->tracks) free_tracks(e->tracks, e->num_tracks);
  free(e);
  }
