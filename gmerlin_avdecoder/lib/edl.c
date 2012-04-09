/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

bgav_edl_stream_t * bgav_edl_add_audio_stream(bgav_edl_track_t * t)
  {
  t->audio_streams = realloc(t->audio_streams, (t->num_audio_streams+1)*sizeof(*t->audio_streams));
  memset(t->audio_streams + t->num_audio_streams, 0, sizeof(*t->audio_streams));
  t->num_audio_streams++;
  return t->audio_streams + (t->num_audio_streams-1);
  }

bgav_edl_stream_t * bgav_edl_add_video_stream(bgav_edl_track_t * t)
  {
  t->video_streams = realloc(t->video_streams, (t->num_video_streams+1)*sizeof(*t->video_streams));
  memset(t->video_streams + t->num_video_streams, 0, sizeof(*t->video_streams));
  t->num_video_streams++;
  return t->video_streams + (t->num_video_streams-1);
  
  }

bgav_edl_stream_t * bgav_edl_add_subtitle_text_stream(bgav_edl_track_t * t)
  {
  t->subtitle_text_streams = realloc(t->subtitle_text_streams, (t->num_subtitle_text_streams+1)*sizeof(*t->subtitle_text_streams));
  memset(t->subtitle_text_streams + t->num_subtitle_text_streams, 0, sizeof(*t->subtitle_text_streams));
  t->num_subtitle_text_streams++;
  return t->subtitle_text_streams + (t->num_subtitle_text_streams-1);
  }

bgav_edl_stream_t * bgav_edl_add_subtitle_overlay_stream(bgav_edl_track_t * t)
  {
  t->subtitle_overlay_streams = realloc(t->subtitle_overlay_streams, (t->num_subtitle_overlay_streams+1)*sizeof(*t->subtitle_overlay_streams));
  memset(t->subtitle_overlay_streams + t->num_subtitle_overlay_streams, 0, sizeof(*t->subtitle_overlay_streams));
  t->num_subtitle_overlay_streams++;
  return t->subtitle_overlay_streams + (t->num_subtitle_overlay_streams-1);
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
    if(src[i].name)
      ret[i].name = bgav_strdup(src[i].name);
    /* Copy pointers */
    if(src[i].metadata)
      {
      ret[i].metadata = calloc(1, sizeof(*ret[i].metadata));
      gavl_metadata_copy(ret[i].metadata,
                         src[i].metadata);
      }
    
    
    ret[i].audio_streams =
      copy_streams(src[i].audio_streams,
                   src[i].num_audio_streams);
    ret[i].video_streams =
      copy_streams(src[i].video_streams,
                   src[i].num_video_streams);
    ret[i].subtitle_text_streams =
      copy_streams(src[i].subtitle_text_streams,
                   src[i].num_subtitle_text_streams);
    ret[i].subtitle_overlay_streams =
      copy_streams(src[i].subtitle_overlay_streams,
                   src[i].num_subtitle_overlay_streams);
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
    if(s[i].metadata)
      {
      gavl_metadata_free(s[i].metadata);
      free(s[i].metadata);
      }
    if(s[i].name)
      free(s[i].name);
    
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

void bgav_edl_destroy(bgav_edl_t * e)
  {
  if(e->tracks)
    free_tracks(e->tracks, e->num_tracks);
  if(e->url)
    free(e->url);
  free(e);
  }

static void dump_stream(const bgav_edl_stream_t* s)
  {
  int i;
  bgav_edl_segment_t * seg;
  bgav_diprintf(8, "Timescale: %d\n", s->timescale);
  bgav_diprintf(8, "Segments:  %d\n", s->num_segments);
  for(i = 0; i < s->num_segments; i++)
    {
    seg = &s->segments[i];
    bgav_diprintf(8, "Segment\n");
    bgav_diprintf(10, "URL:                  %s\n", (seg->url ? seg->url : "(null)"));
    bgav_diprintf(10, "Track:                %d\n", seg->track);
    bgav_diprintf(10, "Stream index:         %d\n", seg->stream);
    bgav_diprintf(10, "Source timescale:     %d\n", seg->timescale);
    bgav_diprintf(10, "Source time:          %" PRId64 "\n", seg->src_time);
    bgav_diprintf(10, "Destination time:     %" PRId64 "\n", seg->dst_time);
    bgav_diprintf(10, "Destination duration: %" PRId64 "\n", seg->dst_duration);
    bgav_diprintf(10, "Playback speed:       %.3f [%d/%d]\n",
                  (float)(seg->speed_num) / (float)(seg->speed_den),
                  seg->speed_num, seg->speed_den);
    }
  }

static void dump_track(const bgav_edl_track_t * t)
  {
  int i;
  bgav_diprintf(2, "Track\n");
  bgav_diprintf(4, "Metadata\n");
  if(t->metadata)
    gavl_metadata_dump(t->metadata, 6);
  
  bgav_diprintf(4, "Audio streams: %d\n", t->num_audio_streams);
  for(i = 0; i < t->num_audio_streams; i++)
    {
    bgav_diprintf(6, "Audio stream\n");
    dump_stream(&t->audio_streams[i]);
    }
  
  bgav_diprintf(4, "Video streams: %d\n", t->num_video_streams);
  for(i = 0; i < t->num_video_streams; i++)
    {
    bgav_diprintf(6, "Video stream\n");
    dump_stream(&t->video_streams[i]);
    }

  bgav_diprintf(4, "Subtitle text streams: %d\n", t->num_subtitle_text_streams);
  for(i = 0; i < t->num_subtitle_text_streams; i++)
    {
    bgav_diprintf(6, "Subtitle text stream\n");
    dump_stream(&t->subtitle_text_streams[i]);
    }
  bgav_diprintf(4, "Subtitle overlay streams: %d\n", t->num_subtitle_overlay_streams);
  for(i = 0; i < t->num_subtitle_overlay_streams; i++)
    {
    bgav_diprintf(6, "Subtitle overlay stream\n");
    dump_stream(&t->subtitle_overlay_streams[i]);
    }
  }

void bgav_edl_dump(const bgav_edl_t * e)
  {
  int i;
  bgav_dprintf("EDL\n");
  bgav_diprintf(2, "URL:    %s\n", (e->url ? e->url : "(null)"));
  bgav_diprintf(2, "Tracks: %d\n", e->num_tracks);
  for(i = 0; i < e->num_tracks; i++)
    {
    dump_track(&e->tracks[i]);
    }
  }
