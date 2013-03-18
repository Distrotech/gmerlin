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

#include <config.h>
#include <gavl/metatags.h>

#include <gmerlin/translation.h>


#include <gmerlin/pluginregistry.h>
#include <gmerlin/converters.h>
#include <gmerlin/utils.h>

#include <pluginreg_priv.h>


#include <gmerlin/log.h>
#define LOG_DOMAIN "edldec"

#define PTS_UNDEFINED 0x8000000000000000LL

/* Sources */

typedef struct
  {
  bg_plugin_handle_t * handle;
  bg_input_plugin_t * plugin;
  const char * url;
  int track;
  int stream;
  bg_track_info_t * ti;
  void * read_data;
  } source_common_t;

typedef struct
  {
  source_common_t com;
  gavl_audio_source_t * src;
  const gavl_audio_format_t * src_fmt;
  } audio_source_t;

typedef struct
  {
  source_common_t com;
  gavl_video_source_t * src;
  const gavl_video_format_t * src_fmt;
  } video_source_t;

typedef struct
  {
  source_common_t com;
  gavl_packet_source_t * src;
  int src_timescale;
  } text_source_t;

typedef struct
  {
  source_common_t com;
  gavl_video_source_t * src;
  const gavl_video_format_t * src_fmt;
  } overlay_source_t;

/* Segments */

typedef struct
  {
  source_common_t * src;
  const gavl_edl_segment_t * seg;
  int64_t mute_start;
  int64_t mute_duration;
  } segment_t;

/* Streams */

typedef struct
  {
  int num_segments;
  segment_t * segments;
  int current_segment;

  int64_t out_time;
  int64_t segment_end_time;
  bg_stream_action_t action;
  const gavl_edl_stream_t * es;
  } stream_common_t;

typedef struct
  {
  stream_common_t com;
  
  int num_sources;
  audio_source_t * sources;
  
  const gavl_audio_format_t * src_fmt;
  const gavl_audio_format_t * dst_fmt;
  
  gavl_audio_frame_t * frame;

  gavl_audio_source_t * src_ext;
  gavl_audio_source_t * src_int;
  const bg_audio_info_t * info;
  } audio_stream_t;

typedef struct
  {
  stream_common_t com;
  
  int num_sources;
  video_source_t * sources;
  
  const gavl_video_format_t * src_fmt;
  gavl_video_format_t * dst_fmt;

  gavl_video_source_t * src_ext;
  gavl_video_source_t * src_int;
  const bg_video_info_t * info;
  } video_stream_t;

typedef struct
  {
  stream_common_t com;
  
  int num_sources;
  text_source_t * sources;
  int64_t time_offset;      /* In output tics */
  int src_timescale;
  int dst_timescale;

  gavl_packet_source_t * src_ext;
  gavl_packet_source_t * src_int;
  const bg_text_info_t * info;
  } text_stream_t;

typedef struct
  {
  stream_common_t com;
  int num_sources;
  overlay_source_t * sources;
  const gavl_video_format_t * src_fmt;
  gavl_video_format_t * dst_fmt;
  int64_t time_offset;      /* In output tics */
  
  gavl_video_source_t * src_ext;
  gavl_video_source_t * src_int;
  const bg_overlay_info_t * info;
  } overlay_stream_t;

typedef struct
  {
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  text_stream_t * text_streams;
  overlay_stream_t * overlay_streams;
  } track_t;

typedef struct
  {
  int current_track;
  track_t * tracks;
  
  bg_track_info_t * track_info;
  const gavl_edl_t * edl;

  bg_plugin_registry_t * plugin_reg;
  bg_input_callbacks_t * callbacks;

  gavl_audio_options_t * a_opt;
  gavl_video_options_t * v_opt;
  
  } edl_dec_t;

/* Timing functions */

/*
  |---------------------------------> out_time
  |               |-----------------> in_time

  0             dst_time
*/
  
static int64_t get_source_offset(const gavl_edl_stream_t * st,
                                 segment_t * s, int64_t out_time, int64_t out_scale)
  {
  int64_t seg_offset;    /* in out timescale */
  int64_t source_offset; /* in out timescale */
  seg_offset = out_time - gavl_time_rescale(st->timescale,
                                            out_scale,
                                            s->seg->dst_time);
  
  source_offset = gavl_time_rescale(s->seg->timescale,
                                    out_scale, s->seg->src_time);
  return source_offset + seg_offset;
  }

static int seek_segment(const gavl_edl_stream_t * st,
                        segment_t * segs, int nsegs, int64_t out_time,
                        int out_scale)
  {
  /* Rescale to edit granularity */
  int64_t t = gavl_time_rescale(out_scale, st->timescale, out_time);
  int i;
  for(i = 0; i < nsegs; i++)
    {
    if(segs[i].src)
      {
      if((segs[i].seg->dst_time <= t) &&
         (segs[i].seg->dst_time + segs[i].seg->dst_duration > t))
        return i;
      }
    else
      {
      if((segs[i].mute_start <= t) &&
         (segs[i].mute_start + segs[i].mute_duration > t))
        return i;
      }
    }
  return -1;
  }

/* Stream initialization */

static const gavl_edl_segment_t *
find_same_source(const gavl_edl_segment_t * segments,
                 const gavl_edl_segment_t * seg)
  {
  while(segments < seg)
    {
    /* url mismatch */
    if((!segments->url && seg->url) ||
       (segments->url && !seg->url) ||
       (segments->url && seg->url &&
        strcmp(segments->url, seg->url)))
      {
      segments++;
      continue;
      }
    /* track/stream mismatch */
    if((segments->track != seg->track) ||
       (segments->stream != seg->stream))
      {
      segments++;
      continue;
      }
    return segments;
    }
  return NULL;
  }
                           
static void add_audio_segment(edl_dec_t * dec, int track, int stream,
                              const gavl_edl_segment_t * seg)
  {
  int i;
  audio_source_t * source = NULL;
  const gavl_edl_segment_t * same_source_seg;
  
  bg_audio_info_t * ai;
  audio_stream_t * s = &dec->tracks[track].audio_streams[stream];
  segment_t * as;
  ai = &dec->track_info[track].audio_streams[stream];

  if(seg->dst_time > ai->duration)
    {
    /* Insert mute segment */
    s->com.segments = realloc(s->com.segments, sizeof(*s->com.segments) * (s->com.num_segments+2));
    memset(s->com.segments + s->com.num_segments, 0, sizeof(*s->com.segments) * 2);
    s->com.num_segments += 2;
    as = s->com.segments + (s->com.num_segments-2);
    as->mute_start    = ai->duration;
    as->mute_duration = seg->dst_time - ai->duration;
    as++;
    }
  else
    {
    s->com.segments = realloc(s->com.segments, sizeof(*s->com.segments) * (s->com.num_segments+1));
    memset(s->com.segments + s->com.num_segments, 0, sizeof(*s->com.segments));
    s->com.num_segments++;
    as = s->com.segments + (s->com.num_segments-1);
    }
  same_source_seg =
    find_same_source(dec->edl->tracks[track].audio_streams[stream].segments, seg);
  
  if(same_source_seg)
    {
    for(i = 0; i < s->com.num_segments; i++)
      {
      if(same_source_seg == s->com.segments[i].seg)
        {
        source = (audio_source_t*)s->com.segments[i].src;
        break;
        }
      }
    }
  else
    {
    source = s->sources + s->num_sources;
    s->num_sources++;
    
    source->com.url   = seg->url ? seg->url : dec->edl->url;
    source->com.track = seg->track;
    source->com.stream = seg->stream;
    }
  as->src = (source_common_t*)source;
  as->seg = seg;
  ai->duration = seg->dst_time + seg->dst_duration;
  }

static int add_video_segment(edl_dec_t * dec, int track, int stream,
                             const gavl_edl_segment_t * seg)
  {
  int i;
  video_source_t * source = NULL;
  const gavl_edl_segment_t * same_source_seg;
  
  bg_video_info_t * vi;
  video_stream_t * s = &dec->tracks[track].video_streams[stream];
  segment_t * vs;
  vi = &dec->track_info[track].video_streams[stream];
  
  if(seg->dst_time > vi->duration)
    {
    /* Insert mute segment */
    s->com.segments = realloc(s->com.segments, sizeof(*s->com.segments) * (s->com.num_segments+2));
    memset(s->com.segments + s->com.num_segments, 0, sizeof(*s->com.segments) * 2);
    s->com.num_segments += 2;
    vs = s->com.segments + (s->com.num_segments-2);
    vs->mute_start    = vi->duration;
    vs->mute_duration = seg->dst_time - vi->duration;
    vs++;
    }
  else
    {
    s->com.segments = realloc(s->com.segments, sizeof(*s->com.segments) * (s->com.num_segments+1));
    memset(s->com.segments + s->com.num_segments, 0, sizeof(*s->com.segments));
    s->com.num_segments++;
    vs = s->com.segments + (s->com.num_segments-1);
    }
  same_source_seg =
    find_same_source(dec->edl->tracks[track].video_streams[stream].segments, seg);
  
  if(same_source_seg)
    {
    for(i = 0; i < s->com.num_segments; i++)
      {
      if(same_source_seg == s->com.segments[i].seg)
        {
        source = (video_source_t*)s->com.segments[i].src;
        break;
        }
      }
    }
  else
    {
    source = s->sources + s->num_sources;
    s->num_sources++;
    
    source->com.url   = seg->url ? seg->url : dec->edl->url;
    source->com.track = seg->track;
    source->com.stream = seg->stream;
    }
  vs->src = (source_common_t*)source;
  vs->seg = seg;
  vi->duration = seg->dst_time + seg->dst_duration;
  return 1;
  }

static int add_overlay_segment(edl_dec_t * dec, int track, int stream,
                                        const gavl_edl_segment_t * seg)
  {
  int i;
  overlay_source_t * source = NULL;
  const gavl_edl_segment_t * same_source_seg;
  
  bg_overlay_info_t * vi;
  overlay_stream_t * s = &dec->tracks[track].overlay_streams[stream];
  segment_t * vs;
  
  vi = &dec->track_info[track].overlay_streams[stream];
  
  s->com.segments = realloc(s->com.segments, sizeof(*s->com.segments) * (s->com.num_segments+1));
  memset(s->com.segments + s->com.num_segments, 0, sizeof(*s->com.segments));
  s->com.num_segments++;
  vs = s->com.segments + (s->com.num_segments-1);
  
  same_source_seg =
    find_same_source(dec->edl->tracks[track].overlay_streams[stream].segments, seg);
  
  if(same_source_seg)
    {
    for(i = 0; i < s->com.num_segments; i++)
      {
      if(same_source_seg == s->com.segments[i].seg)
        {
        source = (overlay_source_t*)s->com.segments[i].src;
        break;
        }
      }
    }
  else
    {
    source = s->sources + s->num_sources;
    s->num_sources++;
    
    source->com.url   = seg->url ? seg->url : dec->edl->url;
    source->com.track = seg->track;
    source->com.stream = seg->stream;
    }
  vs->src = (source_common_t*)source;
  vs->seg = seg;
  vi->duration = seg->dst_time + seg->dst_duration;
  return 1;
  }

static int add_text_segment(edl_dec_t * dec, int track, int stream,
                                     const gavl_edl_segment_t * seg)
  {
  int i;
  text_source_t * source = NULL;
  const gavl_edl_segment_t * same_source_seg;
  
  bg_text_info_t * vi;
  text_stream_t * s = &dec->tracks[track].text_streams[stream];
  segment_t * vs;
  
  vi = &dec->track_info[track].text_streams[stream];
  
  s->com.segments = realloc(s->com.segments, sizeof(*s->com.segments) *
                        (s->com.num_segments+1));
  memset(s->com.segments + s->com.num_segments, 0, sizeof(*s->com.segments));
  s->com.num_segments++;
  vs = s->com.segments + (s->com.num_segments-1);
  
  same_source_seg =
    find_same_source(dec->edl->tracks[track].text_streams[stream].segments, seg);
  
  if(same_source_seg)
    {
    for(i = 0; i < s->com.num_segments; i++)
      {
      if(same_source_seg == s->com.segments[i].seg)
        {
        source = (text_source_t*)s->com.segments[i].src;
        break;
        }
      }
    }
  else
    {
    source = (text_source_t*)s->sources + s->num_sources;
    s->num_sources++;
    
    source->com.url   = seg->url ? seg->url : dec->edl->url;
    source->com.track = seg->track;
    source->com.stream = seg->stream;
    }
  vs->src = (source_common_t*)source;
  vs->seg = seg;
  vi->duration = seg->dst_time + seg->dst_duration;
  return 1;
  }

/* Initialize segments for immediate playback. out_time must be set before */

static void init_audio_segment(audio_stream_t * as)
  {
  segment_t * seg;
  int64_t t;
  seg = &as->com.segments[as->com.current_segment];
  if(seg->seg)
    {
    audio_source_t * src;
    src = (audio_source_t *)(seg->src);
    as->src_fmt = src->src_fmt;
    as->src_int = src->src;
    
    as->com.segment_end_time =
      gavl_time_rescale(as->com.es->timescale, as->dst_fmt->samplerate,
                        seg->seg->dst_time + seg->seg->dst_duration);
    t = get_source_offset(as->com.es, seg, as->com.out_time, as->src_fmt->samplerate);
    seg->src->plugin->seek(seg->src->handle->priv, &t, as->src_fmt->samplerate);
    }
  else
    as->com.segment_end_time =
      gavl_time_rescale(as->com.es->timescale, as->dst_fmt->samplerate,
                        seg->mute_start + seg->mute_duration);
  }

static void init_video_segment(video_stream_t * vs)
  {
  segment_t * seg;
  int64_t t;
  seg = &vs->com.segments[vs->com.current_segment];
  if(seg->seg)
    {
    video_source_t * src;
    src = (video_source_t *)(seg->src);
    vs->src_fmt = src->src_fmt;
    vs->com.segment_end_time =
      gavl_time_rescale(vs->com.es->timescale, vs->src_fmt->timescale,
                        seg->seg->dst_time + seg->seg->dst_duration);
    t = get_source_offset(vs->com.es, seg, vs->com.out_time, vs->src_fmt->timescale);
    seg->src->plugin->seek(seg->src->handle->priv, &t, vs->src_fmt->timescale);
    }
  else
    {
    vs->com.segment_end_time   =
      gavl_time_rescale(vs->com.es->timescale, vs->dst_fmt->timescale,
                        seg->mute_start + seg->mute_duration);
    }
  }

static void init_text_segment(text_stream_t * vs)
  {
  segment_t * seg;
  int64_t t1, t2;
  seg = &vs->com.segments[vs->com.current_segment];

  if(seg->seg)
    {
    vs->com.segment_end_time =
      gavl_time_rescale(vs->com.es->timescale, vs->src_timescale,
                        seg->seg->dst_time + seg->seg->dst_duration);

    /*
     *     |---------------|------------------------|---------------> 
     *     0   src_start/seg->timescale     *time_1 / src_scale
     *                     |
     *  |--|---------------|------------------------|--------------->  
     *  0  ?   dst_start/s->timescale       *time_2 / dst_scale
     *
     */
  
    t1 = gavl_time_rescale(seg->seg->timescale,
                           vs->src_timescale,
                           seg->seg->src_time);
    t2 = gavl_time_rescale(vs->com.es->timescale,
                           vs->src_timescale,
                           seg->seg->dst_time);
  
    vs->time_offset = t2 - t1;
  
    t1 = get_source_offset(vs->com.es, seg, vs->com.out_time, vs->src_timescale);
  
    seg->src->plugin->seek(seg->src->handle->priv, &t1, vs->src_timescale);
    }
  else
    {
    vs->com.segment_end_time   =
      gavl_time_rescale(vs->com.es->timescale, vs->dst_timescale,
                        seg->mute_start + seg->mute_duration);
    }
  }

static void init_overlay_segment(overlay_stream_t * vs)
  {
  segment_t * seg;
  int64_t t1, t2;
  seg = &vs->com.segments[vs->com.current_segment];

  if(seg->seg)
    {
    overlay_source_t * src;
    src = (overlay_source_t *)seg->src;
    vs->src_fmt = src->src_fmt;
    vs->com.segment_end_time =
      gavl_time_rescale(vs->com.es->timescale, vs->dst_fmt->timescale,
                        seg->seg->dst_time + seg->seg->dst_duration);
    
    
    /*
     *     |---------------|------------------------|---------------> 
     *     0   src_start/seg->timescale     *time_1 / src_scale
     *                     |
     *  |--|---------------|------------------------|--------------->  
     *  0  ?   dst_start/s->timescale       *time_2 / dst_scale
     *
     */
  
    t1 = gavl_time_rescale(seg->seg->timescale,
                           vs->dst_fmt->timescale,
                           seg->seg->src_time); // Src time scaled to 
    t2 = gavl_time_rescale(vs->com.es->timescale,
                           vs->dst_fmt->timescale,
                           seg->seg->dst_time);
  
    vs->time_offset = t2 - t1;
    t1 = get_source_offset(vs->com.es, seg, vs->com.out_time, vs->dst_fmt->timescale);
    seg->src->plugin->seek(seg->src->handle->priv, &t1, vs->dst_fmt->timescale);
    }
  }


static void set_callbacks_edl(void * priv, bg_input_callbacks_t * callbacks)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  }

static int get_num_tracks_edl(void * priv)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  return dec->edl->num_tracks;
  }

static bg_track_info_t * get_track_info_edl(void * priv, int track)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  return &dec->track_info[track];
  }

static int set_track_edl(void * priv, int track)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  dec->current_track = track;
  return 1;
  }

static int set_audio_stream_edl(void * priv, int stream, bg_stream_action_t action)
  {
  edl_dec_t * dec = priv;
  dec->tracks[dec->current_track].audio_streams[stream].com.action = action;
  return 1;
  }

static int set_video_stream_edl(void * priv, int stream, bg_stream_action_t action)
  {
  edl_dec_t * dec = priv;
  dec->tracks[dec->current_track].video_streams[stream].com.action = action;
  return 1;
  }

static int set_text_stream_edl(void * priv, int stream, bg_stream_action_t action)
  {
  edl_dec_t * dec = priv;
  dec->tracks[dec->current_track].text_streams[stream].com.action = action;
  return 1;
  }

static int set_overlay_stream_edl(void * priv, int stream, bg_stream_action_t action)
  {
  edl_dec_t * dec = priv;
  dec->tracks[dec->current_track].overlay_streams[stream].com.action = action;
  return 1;
  }

/* Start streams */

static int init_source_common(edl_dec_t * dec,
                              source_common_t * com)
  {
  if(!bg_input_plugin_load(dec->plugin_reg,
                           com->url,
                           NULL,
                           &com->handle,
                           dec->callbacks, 0))
    return 0;
  
  com->plugin = (bg_input_plugin_t*)(com->handle->plugin);
  if(com->plugin->set_track && !com->plugin->set_track(com->handle->priv, com->track))
    return 0;
  com->ti = com->plugin->get_track_info(com->handle->priv, com->track);

  if(!(com->ti->flags & BG_TRACK_SEEKABLE))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "EDL only works with seekable sources");
    return 0;
    }
  return 1;
  }

static void cleanup_source_common(source_common_t * com)
  {
  if(com->handle)
    bg_plugin_unref(com->handle);
  }

// static int has_edl(void * priv, int stream)
//  {
  //  edl_dec_t * dec;
  //  dec = (edl_dec_t*)priv;
//  return 1;
//  }

/* Read one audio frame. Frames might be incomplete at segment boundaries */

static gavl_source_status_t read_audio_edl(void * priv, gavl_audio_frame_t ** frame)
  {
  audio_stream_t * as;
  int num;
  segment_t * seg;
  gavl_source_status_t st;
  as = priv;

  if(as->com.current_segment >= as->com.num_segments)
    return GAVL_SOURCE_EOF;
  
  /* Check if the seqment is finished */
  if(as->com.out_time >= as->com.segment_end_time)
    {
    as->com.current_segment++;
    if(as->com.current_segment >= as->com.num_segments)
      return GAVL_SOURCE_EOF;
    init_audio_segment(as);
    }
  
  num = as->dst_fmt->samples_per_frame;

  /* Check for end of segment */
  if(as->com.out_time + num > as->com.segment_end_time)
    num = as->com.segment_end_time - as->com.out_time;

  seg = &as->com.segments[as->com.current_segment];

  gavl_audio_frame_mute(*frame, as->dst_fmt);

  if(seg->src)
    {
    audio_source_t * src;
    src = (audio_source_t *)seg->src;

    if((st = gavl_audio_source_read_frame(src->src, frame)) != GAVL_SOURCE_OK)
      return st;
    
    }
  
  (*frame)->valid_samples = num;
  (*frame)->timestamp = as->com.out_time;
  as->com.out_time += (*frame)->valid_samples;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_video_edl(void * priv, gavl_video_frame_t ** frame)
  {
  video_stream_t * vs;
  segment_t * seg;
  video_source_t * src;
  gavl_source_status_t st;
  vs = priv;
  
  if(vs->com.current_segment >= vs->com.num_segments)
    return 0;
  
  while(vs->com.out_time >= vs->com.segment_end_time)
    {
    vs->com.current_segment++;
    if(vs->com.current_segment >= vs->com.num_segments)
      return GAVL_SOURCE_EOF;
    init_video_segment(vs);
    }
  
  seg = &vs->com.segments[vs->com.current_segment];

  if(!seg->src)
    {
    gavl_video_frame_clear(*frame, vs->dst_fmt);
    
    (*frame)->duration = vs->dst_fmt->frame_duration;
    }
  else
    {
    src = (video_source_t *)seg->src;

    if((st = gavl_video_source_read_frame(src->src, frame)) != GAVL_SOURCE_OK)
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Unexpected EOF, clearing frame");
      gavl_video_frame_clear(*frame, vs->dst_fmt);
      return GAVL_SOURCE_OK;
      }
    }
  
  (*frame)->timestamp = vs->com.out_time;
  vs->com.out_time += (*frame)->duration;
  return 1;
  }


static int correct_timestamp(int64_t * time, int64_t * duration)
  {
  return 0;
  }

static gavl_source_status_t read_text(void * priv, gavl_packet_t ** p)
  {
  text_stream_t * s = priv;
  
  if(s->com.current_segment >= s->com.es->num_segments)
    return GAVL_SOURCE_EOF;
  
  }

static gavl_source_status_t read_overlay(void * priv, gavl_video_frame_t * f)
  {
  overlay_stream_t * s = priv;
  
  if(s->com.current_segment >= s->com.es->num_segments)
    return GAVL_SOURCE_EOF;
  
  }

#if 0
typedef struct
  {
  gavl_overlay_t * ovl;
  char ** text;
  int * text_alloc;
  int64_t * start_time;
  int64_t * duration;
  stream_t * s;
  } decode_data;

static int
decode_text(decode_data * d)
  {
  source_t * src;
  src = (source_t*)(d->s->com.segments[d->s->com.current_segment].src);
  return src->com.plugin->read_text(src->com.handle->priv,
                                             d->text, d->text_alloc,
                                             d->start_time,
                                             d->duration, src->com.stream);
  }

static int
decode_overlay(decode_data * d)
  {
  source_t * src;
  src = (source_t*)(d->s->com.segments[d->s->com.current_segment].src);
  return src->read_func(src->com.read_data, d->ovl, src->com.stream);
  }

/* Here, the actual magic happens */
static int
decode_subtitle(int (*decode_func)(decode_data*),
                decode_data * d)
  {
  if(d->s->com.current_segment >= d->s->com.es->com.num_segments)
    return 0;
  
  while(1)
    {
    /* Check is segment is finished */
    if(d->s->out_time >= d->s->com.segment_end_time)
      {
      d->s->com.current_segment++;
      if(d->s->com.current_segment >= d->s->es->com.num_segments)
        return 0;
      init_segment(d->s);
      }
    
    /* Decode one subtitle */
    if(!decode_func(d))
      {
      d->s->com.current_segment++;
      if(d->s->com.current_segment >= d->s->es->com.num_segments)
        return 0;
      init_segment(d->s);
      continue;
      }
    /* Correct timestamps */
    
    *d->start_time =
      gavl_time_rescale(((source_t*)(d->s->com.segments[d->s->com.current_segment].src))->format->timescale,
                        d->s->format->timescale, *d->start_time) + d->s->time_offset;
    *d->duration =
      gavl_time_rescale(((source_t*)(d->s->com.segments[d->s->com.current_segment].src))->format->timescale,
                        d->s->format->timescale, *d->duration);

    /* Still inside the segment */

    if(*d->start_time < d->s->com.segment_end_time)
      {
      if(*d->start_time + *d->duration > d->s->com.segment_end_time)
        *d->duration = d->s->com.segment_end_time - *d->start_time;
      
      d->s->out_time = *d->start_time + *d->duration;
      break;
      }
    /* Subtitle there but after segment */
    else
      {
      d->s->out_time = d->s->com.segment_end_time;
      }
    }
  return 1;
  }

static int read_overlay_edl(void * priv,
                                     gavl_overlay_t*ovl, int stream)
  {
  edl_dec_t * dec;
  decode_data d;
  dec = (edl_dec_t*)priv;
  
  d.ovl = ovl;
  d.start_time = &ovl->timestamp;
  d.duration   = &ovl->duration;

  d.s =
    &dec->tracks[dec->current_track].overlay_streams[stream -
                                                             dec->edl->tracks[dec->current_track].num_text_streams];
  return decode_subtitle(decode_overlay, &d);
  }

static int read_text_edl(void * priv,
                                  char ** text, int * text_alloc,
                                  int64_t * start_time,
                                  int64_t * duration, int stream)
  {
  edl_dec_t * dec;
  decode_data d;

  dec = (edl_dec_t*)priv;

  d.text = text;
  d.text_alloc = text_alloc;

  d.start_time = start_time;
  d.duration   = duration;

  d.s =
    &dec->tracks[dec->current_track].text_streams[stream];
  
  return decode_subtitle(decode_text, &d);
  }
#endif

/* Start */

static int start_audio(audio_stream_t * as, edl_dec_t * dec, bg_audio_info_t * info)
  {
  int j;
  audio_source_t * asrc;
  if(as->com.action != BG_STREAM_ACTION_DECODE)
    return 1;

  as->info = info;
  as->dst_fmt = &as->info->format;
  
  /* Fire up the sources */
  for(j = 0; j < as->num_sources; j++)
    {
    asrc = &as->sources[j];
    if(!init_source_common(dec, &asrc->com))
      return 0;

    if(asrc->com.plugin->set_audio_stream &&
       !asrc->com.plugin->set_audio_stream(asrc->com.handle->priv,
                                           asrc->com.stream,
                                           BG_STREAM_ACTION_DECODE))
      return 0;
    if(asrc->com.plugin->start &&
       !asrc->com.plugin->start(asrc->com.handle->priv))
      return 0;
    asrc->src = asrc->com.plugin->get_audio_source(asrc->com.handle->priv, asrc->com.stream);
    asrc->src_fmt = gavl_audio_source_get_src_format(asrc->src);
    
    if(!j)
      gavl_audio_format_copy(&info->format, asrc->src_fmt);
    gavl_audio_source_set_dst(asrc->src, 0, as->dst_fmt);
    }
  /* Initialize first segment */
  as->com.current_segment = 0;
  init_audio_segment(as);
  return 1;
  }

static int start_video(video_stream_t * vs, edl_dec_t * dec, bg_video_info_t * info)
  {
  int j;
  video_source_t * vsrc;

  if(vs->com.action != BG_STREAM_ACTION_DECODE)
    return 1;
    
  /* Fire up the sources */
  
  vs->dst_fmt = &info->format;
  for(j = 0; j < vs->num_sources; j++)
    {
    vsrc = &vs->sources[j];

    if(!init_source_common(dec, &vs->sources[j].com))
      return 0;

    if(vsrc->com.plugin->set_video_stream &&
       !vsrc->com.plugin->set_video_stream(vsrc->com.handle->priv,
                                           vsrc->com.stream,
                                           BG_STREAM_ACTION_DECODE))
      return 0;
    if(vsrc->com.plugin->start &&
       !vsrc->com.plugin->start(vsrc->com.handle->priv))
      return 0;

    vsrc->src = vsrc->com.plugin->get_video_source(vsrc->com.handle->priv, vsrc->com.stream);
    vsrc->src_fmt = gavl_video_source_get_src_format(vsrc->src);
      
    if(!j)
      gavl_video_format_copy(&info->format, vsrc->src_fmt);
    gavl_video_source_set_dst(vsrc->src, 0, vs->dst_fmt);
    }
  /* Initialize first segment */
  vs->com.current_segment = 0;
  init_video_segment(vs);
  return 1;
  }

static int start_text(text_stream_t * s, edl_dec_t * dec, bg_text_info_t * info)
  {
  int j;
  text_source_t * src;
    
  if(s->com.action != BG_STREAM_ACTION_DECODE)
    return 1;
    
  /* Fire up the sources */
  for(j = 0; j < s->num_sources; j++)
    {
    src = (text_source_t *)&s->sources[j];
    
    if(!init_source_common(dec, &s->sources[j].com))
      return 0;
    if(src->com.plugin->set_text_stream &&
       !src->com.plugin->set_text_stream(src->com.handle->priv,
                                           src->com.stream,
                                           BG_STREAM_ACTION_DECODE))
      return 0;
    if(src->com.plugin->start &&
       !src->com.plugin->start(src->com.handle->priv))
      return 0;

    src->src = src->com.plugin->get_text_source(src->com.handle->priv, src->com.stream);
    src->src_timescale = src->com.ti->text_streams[src->com.stream].timescale;
      
    if(!j)
      {
      info->timescale = src->src_timescale;
      s->dst_timescale = src->src_timescale;
      }
    }
  /* Initialize first segment */
  s->com.current_segment = 0;
  init_text_segment(s);
  return 1;
  }

static int start_overlay(overlay_stream_t * s, edl_dec_t * dec, bg_overlay_info_t * info)
  {
  int j;
  overlay_source_t * src;
  
  if(s->com.action != BG_STREAM_ACTION_DECODE)
    return 1;
    
  s->dst_fmt = &info->format;
  
  /* Fire up the sources */
  for(j = 0; j < s->num_sources; j++)
    {
    src = &s->sources[j];
    if(!init_source_common(dec, &src->com))
      return 0;
      
    if(src->com.plugin->set_overlay_stream &&
       !src->com.plugin->set_overlay_stream(src->com.handle->priv,
                                            src->com.stream,
                                            BG_STREAM_ACTION_DECODE))
      return 0;
    if(src->com.plugin->start &&
       !src->com.plugin->start(src->com.handle->priv))
      return 0;
    src->src = src->com.plugin->get_overlay_source(src->com.handle->priv,
                                                   src->com.stream);
    
    src->src_fmt = gavl_video_source_get_src_format(src->src);
    
    if(!j)
      gavl_video_format_copy(&info->format, src->src_fmt);
    
    gavl_video_source_set_dst(src->src, 0, s->dst_fmt);
    }
  /* Initialize first segment */
  s->com.current_segment = 0;
  init_overlay_segment(s);
  return 1;
  }

static int start_edl(void * priv)
  {
  int i;
  edl_dec_t * dec;
  track_t * t;
  bg_track_info_t * ti;
  
  dec = (edl_dec_t*)priv;

  t = &dec->tracks[dec->current_track];
  ti = &dec->track_info[dec->current_track];
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_audio_streams; i++)
    {
    if(!start_audio(&t->audio_streams[i], dec, &ti->audio_streams[i]))
      return 0;
    }
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_video_streams; i++)
    {
    if(!start_video(&t->video_streams[i], dec, &ti->video_streams[i]))
      return 0;
    }
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_text_streams; i++)
    {
    if(!start_text(&t->text_streams[i], dec, &ti->text_streams[i]))
      return 0;
    }
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_overlay_streams; i++)
    {
    if(!start_overlay(&t->overlay_streams[i], dec, &ti->overlay_streams[i]))
      return 0;
    }
  
  return 1;
  }



static void seek_edl(void * priv, int64_t * time, int scale)
  {
  edl_dec_t * dec;
  int i;
  track_t * t;
  gavl_edl_track_t * et;
  int64_t time_scaled;
  dec = (edl_dec_t*)priv;
  t = &dec->tracks[dec->current_track];
  et = &dec->edl->tracks[dec->current_track];
  
  for(i = 0; i < et->num_audio_streams; i++)
    {
    if(t->audio_streams[i].com.action != BG_STREAM_ACTION_DECODE)
      continue;
    time_scaled = gavl_time_rescale(scale, t->audio_streams[i].dst_fmt->samplerate,
                                    *time);
    t->audio_streams[i].com.current_segment =
      seek_segment(t->audio_streams[i].com.es,
                   t->audio_streams[i].com.segments,
                   t->audio_streams[i].com.num_segments,
                   time_scaled, t->audio_streams[i].dst_fmt->samplerate);
    if(t->audio_streams[i].com.current_segment < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked audio out of range");
      return;
      }
    t->audio_streams[i].com.out_time = time_scaled;
    init_audio_segment(&t->audio_streams[i]);
    }
  for(i = 0; i < et->num_video_streams; i++)
    {
    if(t->video_streams[i].com.action != BG_STREAM_ACTION_DECODE)
      continue;

    time_scaled = gavl_time_rescale(scale, t->video_streams[i].dst_fmt->timescale,
                                    *time);
    t->video_streams[i].com.current_segment =
      seek_segment(t->video_streams[i].com.es,
                   t->video_streams[i].com.segments,
                   t->video_streams[i].com.num_segments,
                   time_scaled, t->video_streams[i].dst_fmt->timescale);
    if(t->video_streams[i].com.current_segment < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked video out of range");
      return;
      }
    t->video_streams[i].com.out_time = time_scaled;
    init_video_segment(&t->video_streams[i]);
    
    }
  for(i = 0; i < et->num_text_streams; i++)
    {
    if(t->text_streams[i].com.action != BG_STREAM_ACTION_DECODE)
      continue;

    time_scaled = gavl_time_rescale(scale, t->text_streams[i].dst_timescale,
                                    *time);
    
    t->text_streams[i].com.current_segment =
      seek_segment(t->text_streams[i].com.es,
                   t->text_streams[i].com.segments,
                   t->text_streams[i].com.num_segments,
                   time_scaled, t->text_streams[i].dst_timescale);
    if(t->text_streams[i].com.current_segment < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked text subtitles out of range");
      return;
      }
    t->text_streams[i].com.out_time = time_scaled;
    init_text_segment(&t->text_streams[i]);
    }
  for(i = 0; i < et->num_overlay_streams; i++)
    {
    if(t->overlay_streams[i].com.action != BG_STREAM_ACTION_DECODE)
      continue;

    time_scaled = gavl_time_rescale(scale, t->overlay_streams[i].dst_fmt->timescale,
                                    *time);
    
    t->overlay_streams[i].com.current_segment =
      seek_segment(t->overlay_streams[i].com.es,
                   t->overlay_streams[i].com.segments,
                   t->overlay_streams[i].com.num_segments,
                   time_scaled, t->overlay_streams[i].dst_fmt->timescale);
    if(t->overlay_streams[i].com.current_segment < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked overlay subtitles out of range");
      return;
      }
    t->overlay_streams[i].com.out_time = time_scaled;
    init_overlay_segment(&t->overlay_streams[i]);
    }
        
  }

static void close_stream_common(stream_common_t * com)
  {
  if(com->segments)
    free(com->segments);
  }

static void close_edl(void * priv)
  {
  int i, j, k;
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;

  for(i = 0; i < dec->edl->num_tracks; i++)
    {
    for(j = 0; j < dec->edl->tracks[i].num_audio_streams; j++)
      {
      audio_stream_t * s = &dec->tracks[i].audio_streams[j];
      
      /* Close sources */
      for(k = 0; k < s->num_sources; k++)
        cleanup_source_common(&s->sources[k].com);

      if(s->sources)
        free(s->sources);

      if(s->src_ext)
        gavl_audio_source_destroy(s->src_ext);

      close_stream_common(&s->com);
      }
    if(dec->tracks[i].audio_streams)
      free(dec->tracks[i].audio_streams);
    
    for(j = 0; j < dec->edl->tracks[i].num_video_streams; j++)
      {
      video_stream_t * s = &dec->tracks[i].video_streams[j];
      
      /* Close sources */
      for(k = 0; k < s->num_sources; k++)
        cleanup_source_common(&s->sources[k].com);
      
      if(s->sources)
        free(s->sources);
      
      if(s->src_ext)
        gavl_video_source_destroy(s->src_ext);
      close_stream_common(&s->com);
      }
    if(dec->tracks[i].video_streams)
      free(dec->tracks[i].video_streams);

    for(j = 0; j < dec->edl->tracks[i].num_text_streams; j++)
      {
      text_stream_t * s = &dec->tracks[i].text_streams[j];
      /* Close sources */
      for(k = 0; k < s->num_sources; k++)
        cleanup_source_common(&s->sources[k].com);

      if(s->sources)
        free(dec->tracks[i].text_streams[j].sources);

      if(s->src_ext)
        gavl_packet_source_destroy(s->src_ext);
      close_stream_common(&s->com);
      }
    if(dec->tracks[i].text_streams)
      free(dec->tracks[i].text_streams);

    for(j = 0; j < dec->edl->tracks[i].num_overlay_streams; j++)
      {
      overlay_stream_t * s = &dec->tracks[i].overlay_streams[j];

      /* Close sources */
      for(k = 0; k < s->num_sources; k++)
        cleanup_source_common(&s->sources[k].com);
      
      if(s->sources)
        free(s->sources);

      if(s->src_ext)
        gavl_video_source_destroy(s->src_ext);
      
      close_stream_common(&s->com);
      }
    if(dec->tracks[i].overlay_streams)
      free(dec->tracks[i].overlay_streams);
    }
  if(dec->tracks)
    {
    free(dec->tracks);
    dec->tracks = NULL;
    }
  if(dec->track_info)
    {
    for(i = 0; i < dec->edl->num_tracks; i++)
      {
      bg_track_info_free(&dec->track_info[i]);
      }
    free(dec->track_info);
    dec->track_info = NULL;
    
    }
  
  }

static gavl_audio_source_t * get_audio_source_edl(void * priv, int stream)
  {
  edl_dec_t * dec = priv;
  track_t * t = &dec->tracks[dec->current_track];
  return t->audio_streams[stream].src_ext;
  }

static gavl_video_source_t * get_video_source_edl(void * priv, int stream)
  {
  edl_dec_t * dec = priv;
  track_t * t = &dec->tracks[dec->current_track];
  return t->video_streams[stream].src_ext;
  
  }

static gavl_packet_source_t * get_text_source_edl(void * priv, int stream)
  {
  edl_dec_t * dec = priv;
  track_t * t = &dec->tracks[dec->current_track];
  return t->text_streams[stream].src_ext;
  }

static gavl_video_source_t * get_overlay_source_edl(void * priv, int stream)
  {
  edl_dec_t * dec = priv;
  track_t * t = &dec->tracks[dec->current_track];
  return t->overlay_streams[stream].src_ext;
  }

static void destroy_edl(void * priv)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  if(dec->tracks)
    close_edl(priv);

  gavl_audio_options_destroy(dec->a_opt);
  gavl_video_options_destroy(dec->v_opt);

  free(priv);
  }

static const bg_input_plugin_t edl_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "i_edldec",
      .long_name =      TRS("EDL decoder"),
      .description =    TRS("This metaplugin decodes an EDL as if it was a single file."),
      .type =           BG_PLUGIN_INPUT,
      .flags =          0,
      .priority =       1,
      .create =         NULL,
      .destroy =        destroy_edl,
      //      .get_parameters = get_parameters_edl,
      //      .set_parameter =  set_parameter_edl
    },
    //    .open =          open_input,
    .set_callbacks = set_callbacks_edl,
    .get_num_tracks = get_num_tracks_edl,
    .get_track_info = get_track_info_edl,
    .set_track      = set_track_edl,
    /* Set streams */
    .set_video_stream =         set_video_stream_edl,
    .set_audio_stream =         set_audio_stream_edl,
    .set_text_stream =      set_text_stream_edl,
    .set_overlay_stream =      set_overlay_stream_edl,
    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =                 start_edl,
    /* Read one video frame (returns FALSE on EOF) */
    .get_audio_source = get_audio_source_edl,
    .get_video_source = get_video_source_edl,
    .get_text_source = get_text_source_edl,
    .get_overlay_source = get_overlay_source_edl,
    
    /*
     *  Do percentage seeking (can be NULL)
     *  Media streams are supposed to be seekable, if this
     *  function is non-NULL AND the duration field of the track info
     *  is > 0
     */
    .seek = seek_edl,
    /* Stop playback, close all decoders */
    //    .stop = stop_edl,
    .close = close_edl,
  };

bg_plugin_info_t * bg_edldec_get_info()
  {
  return bg_plugin_info_create(&edl_plugin.common);
  }

int bg_input_plugin_load_edl(bg_plugin_registry_t * reg,
                             const gavl_edl_t * edl,
                             const bg_plugin_info_t * info,
                             bg_plugin_handle_t ** ret1,
                             bg_input_callbacks_t * callbacks)
  {
  int i, j, k;
  const gavl_edl_track_t * track;
  const gavl_edl_stream_t * stream;
  bg_track_info_t * ti;
  track_t * t;
  edl_dec_t * priv;
  bg_plugin_handle_t * ret;
  gavl_time_t test_duration, duration = 0;
  if(*ret1)
    bg_plugin_unref(*ret1);
  
  ret = calloc(1, sizeof(*ret));
  *ret1 = ret;
    
  //  ret->plugin_reg = reg;
  ret->plugin = (bg_plugin_common_t*)(&edl_plugin);

  ret->info = info;
  
  pthread_mutex_init(&ret->mutex, NULL);

  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  ret->refcount = 1;
  
  priv->edl = edl;
  priv->plugin_reg = reg;
  priv->callbacks = callbacks;
  
  priv->track_info = calloc(edl->num_tracks, sizeof(*priv->track_info));
  priv->tracks = calloc(edl->num_tracks, sizeof(*priv->tracks));

  priv->a_opt = gavl_audio_options_create();
  priv->v_opt = gavl_video_options_create();
  
  for(i = 0; i < edl->num_tracks; i++)
    {
    track = &edl->tracks[i];
    ti    = &priv->track_info[i];
    t     = &priv->tracks[i];
    
    gavl_metadata_copy(&ti->metadata, &track->metadata);
    
    ti->flags |= (BG_TRACK_SEEKABLE|BG_TRACK_PAUSABLE);
    
    ti->num_audio_streams = track->num_audio_streams;
    
    ti->audio_streams = calloc(ti->num_audio_streams,
                               sizeof(*ti->audio_streams));
    t->audio_streams = calloc(ti->num_audio_streams,
                              sizeof(*t->audio_streams));
    
    /* Audio streams */
    for(j = 0; j < track->num_audio_streams; j++)
      {
      stream = &track->audio_streams[j];

      t->audio_streams[j].sources = calloc(stream->num_segments,
                                           sizeof(*t->audio_streams[j].sources));
      
      t->audio_streams[j].com.es = stream;
      for(k = 0; k < stream->num_segments; k++)
        {
        add_audio_segment(priv, i, j, &stream->segments[k]);
        }
      test_duration = gavl_time_unscale(stream->timescale, ti->audio_streams[j].duration);
      if(duration < test_duration)
        duration = test_duration;
      }
    
    /* Video streams */

    ti->num_video_streams = track->num_video_streams;
    
    ti->video_streams = calloc(ti->num_video_streams,
                               sizeof(*ti->video_streams));
    t->video_streams = calloc(ti->num_video_streams,
                              sizeof(*t->video_streams));

    for(j = 0; j < track->num_video_streams; j++)
      {
      stream = &track->video_streams[j];

      t->video_streams[j].sources =
        calloc(stream->num_segments,
               sizeof(*t->video_streams[j].sources));
      
      t->video_streams[j].com.es = stream;
      for(k = 0; k < stream->num_segments; k++)
        {
        add_video_segment(priv, i, j, &stream->segments[k]);
        }
      test_duration =
        gavl_time_unscale(stream->timescale, ti->video_streams[j].duration);
      if(duration < test_duration)
        duration = test_duration;

      }
    /* Subtitle text streams */

    ti->text_streams = calloc(ti->num_text_streams,
                              sizeof(*ti->text_streams));
    
    t->text_streams = calloc(track->num_text_streams,
                                      sizeof(*t->text_streams));
    t->overlay_streams = calloc(track->num_overlay_streams,
                                      sizeof(*t->overlay_streams));
    
    for(j = 0; j < track->num_text_streams; j++)
      {
      stream = &track->text_streams[j];

      t->text_streams[j].sources =
        calloc(stream->num_segments,
               sizeof(*t->text_streams[j].sources));

      t->text_streams[j].com.es = stream;

      for(k = 0; k < stream->num_segments; k++)
        {
        add_text_segment(priv, i, j, &stream->segments[k]);
        }
      test_duration =
        gavl_time_unscale(stream->timescale,
                          ti->text_streams[j].duration);
      if(duration < test_duration)
        duration = test_duration;
      }
    /* Subtitle overlay streams */
    for(j = 0; j < track->num_overlay_streams; j++)
      {
      stream = &track->overlay_streams[j];
      
      t->overlay_streams[j].sources =
        calloc(stream->num_segments,
               sizeof(*t->overlay_streams[j].sources));

      t->overlay_streams[j].com.es = stream;


      for(k = 0; k < stream->num_segments; k++)
        {
        add_overlay_segment(priv, i, j, &stream->segments[k]);
        }
      test_duration =
        gavl_time_unscale(stream->timescale,
                          ti->overlay_streams[j].duration);
      if(duration < test_duration)
        duration = test_duration;
      }
    gavl_metadata_set_long(&ti->metadata, GAVL_META_APPROX_DURATION, duration);
    }
  return 1;
  }
