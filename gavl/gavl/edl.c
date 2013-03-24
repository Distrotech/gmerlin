/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <gavl/gavl.h>
#include <gavl/metadata.h>

#include <gavl/edl.h>
#include <gavl/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char * my_strdup(char * s)
  {
  char * ret;
  if(!s)
    return NULL;
  ret = malloc(strlen(s)+1);
  strcpy(ret, s);
  return ret;
  }

gavl_edl_t * gavl_edl_create()
  {
  gavl_edl_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

gavl_edl_track_t * gavl_edl_add_track(gavl_edl_t * e)
  {
  e->tracks = realloc(e->tracks, (e->num_tracks+1)*sizeof(*e->tracks));
  memset(e->tracks + e->num_tracks, 0, sizeof(*e->tracks));
  e->num_tracks++;
  return e->tracks + (e->num_tracks-1);
  }

gavl_edl_stream_t * gavl_edl_add_audio_stream(gavl_edl_track_t * t)
  {
  t->audio_streams = realloc(t->audio_streams, (t->num_audio_streams+1)*sizeof(*t->audio_streams));
  memset(t->audio_streams + t->num_audio_streams, 0, sizeof(*t->audio_streams));
  t->num_audio_streams++;
  return t->audio_streams + (t->num_audio_streams-1);
  }

gavl_edl_stream_t * gavl_edl_add_video_stream(gavl_edl_track_t * t)
  {
  t->video_streams = realloc(t->video_streams, (t->num_video_streams+1)*sizeof(*t->video_streams));
  memset(t->video_streams + t->num_video_streams, 0, sizeof(*t->video_streams));
  t->num_video_streams++;
  return t->video_streams + (t->num_video_streams-1);
  
  }

gavl_edl_stream_t * gavl_edl_add_text_stream(gavl_edl_track_t * t)
  {
  t->text_streams = realloc(t->text_streams, (t->num_text_streams+1)*sizeof(*t->text_streams));
  memset(t->text_streams + t->num_text_streams, 0, sizeof(*t->text_streams));
  t->num_text_streams++;
  return t->text_streams + (t->num_text_streams-1);
  }

gavl_edl_stream_t * gavl_edl_add_overlay_stream(gavl_edl_track_t * t)
  {
  t->overlay_streams = realloc(t->overlay_streams, (t->num_overlay_streams+1)*sizeof(*t->overlay_streams));
  memset(t->overlay_streams + t->num_overlay_streams, 0, sizeof(*t->overlay_streams));
  t->num_overlay_streams++;
  return t->overlay_streams + (t->num_overlay_streams-1);
  }

gavl_edl_segment_t * gavl_edl_add_segment(gavl_edl_stream_t * s)
  {
  s->segments = realloc(s->segments, (s->num_segments+1)*sizeof(*s->segments));
  memset(s->segments + s->num_segments, 0, sizeof(*s->segments));
  s->num_segments++;
  return s->segments + (s->num_segments-1);
  }

static gavl_edl_segment_t * copy_segments(const gavl_edl_segment_t * src, int len)
  {
  int i;
  gavl_edl_segment_t * ret;
  ret = calloc(len, sizeof(*ret));

  /* Copy integers */
  memcpy(ret, src, len * sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */
    ret[i].url = my_strdup(src[i].url);
    }
  return ret;
  }


static gavl_edl_stream_t * copy_streams(const gavl_edl_stream_t * src, int len)
  {
  int i;
  gavl_edl_stream_t * ret;
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

static gavl_edl_track_t * copy_tracks(const gavl_edl_track_t * src, int len)
  {
  int i;
  gavl_edl_track_t * ret;
  ret = calloc(len, sizeof(*ret));
  
  /* Copy integers */
  memcpy(ret, src, len * sizeof(*ret));
  
  for(i = 0; i < len; i++)
    {
    /* Copy pointers */
    gavl_metadata_init(&ret[i].metadata);
    
    gavl_metadata_copy(&ret[i].metadata, &src[i].metadata);
    
    ret[i].audio_streams =
      copy_streams(src[i].audio_streams,
                   src[i].num_audio_streams);
    ret[i].video_streams =
      copy_streams(src[i].video_streams,
                   src[i].num_video_streams);
    ret[i].text_streams =
      copy_streams(src[i].text_streams,
                   src[i].num_text_streams);
    ret[i].overlay_streams =
      copy_streams(src[i].overlay_streams,
                   src[i].num_overlay_streams);
    }
  return ret;
  }

gavl_edl_t * gavl_edl_copy(const gavl_edl_t * e)
  {
  gavl_edl_t * ret;
  ret = calloc(1, sizeof(*ret));

  /* Copy integers */
  memcpy(ret, e, sizeof(*ret));
  
  /* Copy pointers */
  ret->tracks = copy_tracks(e->tracks, e->num_tracks);
  ret->url = my_strdup(e->url);
  return ret;
  }

static void free_segments(gavl_edl_segment_t * s, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    if(s[i].url) free(s[i].url);
    }
  free(s);
  }

static void free_streams(gavl_edl_stream_t * s, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    if(s[i].segments) free_segments(s[i].segments, s[i].num_segments);
    }
  free(s);
  }

static void free_tracks(gavl_edl_track_t * s, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    gavl_metadata_free(&s[i].metadata);
    
    if(s[i].audio_streams)
      free_streams(s[i].audio_streams, s[i].num_audio_streams);
    if(s[i].video_streams)
      free_streams(s[i].video_streams, s[i].num_video_streams);
    if(s[i].text_streams)
      free_streams(s[i].text_streams, s[i].num_text_streams);
    if(s[i].overlay_streams)
      free_streams(s[i].overlay_streams, s[i].num_overlay_streams);
    }
  free(s);
  }

void gavl_edl_destroy(gavl_edl_t * e)
  {
  if(e->tracks)
    free_tracks(e->tracks, e->num_tracks);
  if(e->url)
    free(e->url);
  free(e);
  }

static void dump_stream(const gavl_edl_stream_t* s)
  {
  int i;
  gavl_edl_segment_t * seg;
  gavl_diprintf(8, "Timescale: %d\n", s->timescale);
  gavl_diprintf(8, "Segments:  %d\n", s->num_segments);
  for(i = 0; i < s->num_segments; i++)
    {
    seg = &s->segments[i];
    gavl_diprintf(8, "Segment\n");
    gavl_diprintf(10, "URL:                  %s\n", (seg->url ? seg->url : "(null)"));
    gavl_diprintf(10, "Track:                %d\n", seg->track);
    gavl_diprintf(10, "Stream index:         %d\n", seg->stream);
    gavl_diprintf(10, "Source timescale:     %d\n", seg->timescale);
    gavl_diprintf(10, "Source time:          %" PRId64 "\n", seg->src_time);
    gavl_diprintf(10, "Destination time:     %" PRId64 "\n", seg->dst_time);
    gavl_diprintf(10, "Destination duration: %" PRId64 "\n", seg->dst_duration);
    gavl_diprintf(10, "Playback speed:       %.3f [%d/%d]\n",
                  (float)(seg->speed_num) / (float)(seg->speed_den),
                  seg->speed_num, seg->speed_den);
    }
  }

static void dump_track(const gavl_edl_track_t * t)
  {
  int i;
  gavl_diprintf(2, "Track\n");
  gavl_diprintf(4, "Metadata\n");
  gavl_metadata_dump(&t->metadata, 6);
  
  gavl_diprintf(4, "Audio streams: %d\n", t->num_audio_streams);
  for(i = 0; i < t->num_audio_streams; i++)
    {
    gavl_diprintf(6, "Audio stream\n");
    dump_stream(&t->audio_streams[i]);
    }
  
  gavl_diprintf(4, "Video streams: %d\n", t->num_video_streams);
  for(i = 0; i < t->num_video_streams; i++)
    {
    gavl_diprintf(6, "Video stream\n");
    dump_stream(&t->video_streams[i]);
    }

  gavl_diprintf(4, "Text streams: %d\n", t->num_text_streams);
  for(i = 0; i < t->num_text_streams; i++)
    {
    gavl_diprintf(6, "Text stream\n");
    dump_stream(&t->text_streams[i]);
    }
  gavl_diprintf(4, "Overlay streams: %d\n", t->num_overlay_streams);
  for(i = 0; i < t->num_overlay_streams; i++)
    {
    gavl_diprintf(6, "Overlay stream\n");
    dump_stream(&t->overlay_streams[i]);
    }
  }

void gavl_edl_dump(const gavl_edl_t * e)
  {
  int i;
  gavl_dprintf("EDL\n");
  gavl_diprintf(2, "URL:    %s\n", (e->url ? e->url : "(null)"));
  gavl_diprintf(2, "Tracks: %d\n", e->num_tracks);
  for(i = 0; i < e->num_tracks; i++)
    {
    dump_track(&e->tracks[i]);
    }
  }

int64_t gavl_edl_src_time_to_dst(const gavl_edl_stream_t * st,
                                 const gavl_edl_segment_t * seg,
                                 int64_t src_time)
  {
  int64_t ret;
  /* Offset from the segment start in src scale */
  ret = src_time - seg->src_time; 
  
  /* Src scale -> dst_scale */
  ret = gavl_time_rescale(seg->timescale, st->timescale,
                          ret);

  /* Add offset of the segment start in dst scale */
  ret += seg->dst_time;
  
  return ret;
  }

static gavl_time_t get_streams_duration(const gavl_edl_stream_t * s,
                                        int num)
  {
  int i;
  gavl_time_t ret = 0;
  gavl_time_t test_time;
  const gavl_edl_segment_t * seg;
  
  for(i = 0; i < num; i++)
    {
    if(!s[i].num_segments)
      continue;

    seg = &s[i].segments[s[i].num_segments-1];
    
    test_time = gavl_time_unscale(s[i].timescale,
                                  seg->dst_time + seg->dst_duration);
    if(test_time > ret)
      ret = test_time;
    }
  return ret;
  }

gavl_time_t
gavl_edl_track_get_duration(const gavl_edl_track_t * t)
  {
  gavl_time_t test_time;
  gavl_time_t ret = 0;

  test_time = get_streams_duration(t->audio_streams,
                                   t->num_audio_streams);
  if(test_time > ret)
    ret = test_time;

  test_time = get_streams_duration(t->video_streams,
                                   t->num_video_streams);
  if(test_time > ret)
    ret = test_time;

  test_time = get_streams_duration(t->text_streams,
                                   t->num_text_streams);
  if(test_time > ret)
    ret = test_time;

  test_time = get_streams_duration(t->overlay_streams,
                                   t->num_overlay_streams);
  if(test_time > ret)
    ret = test_time;
  
  return ret;
  }

GAVL_PUBLIC
const gavl_edl_segment_t *
gavl_edl_dst_time_to_src(const gavl_edl_track_t * t,
                         const gavl_edl_stream_t * st,
                         int64_t dst_time,
                         int64_t * src_time,
                         int64_t * mute_time)
  {
  int i;
  const gavl_edl_segment_t * ret = NULL;
  
  for(i = 0; i < st->num_segments; i++)
    {
    if(st->segments[i].dst_time + st->segments[i].dst_duration > dst_time)
      {
      ret = &st->segments[i];
      break;
      }
    }

  if(!ret) // After the last segment
    {
    gavl_time_t duration = gavl_edl_track_get_duration(t);

    *mute_time = gavl_time_scale(st->timescale, duration) - dst_time;
    if(*mute_time < 0)
      *mute_time = 0;

    return NULL;
    }

  /* Get the next segment */

  *src_time = ret->src_time;
  
  if(ret->dst_time > dst_time)
    *mute_time = ret->dst_time - dst_time;

  if(dst_time > ret->dst_time)
    {
    *src_time += gavl_time_rescale(st->timescale,
                                   ret->timescale,
                                   dst_time - ret->dst_time);
    }
  return ret;
  }
