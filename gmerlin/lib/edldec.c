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
  
  int read_stream;
  void * read_data;

  } source_common_t;

typedef struct
  {
  source_common_t com;
  bg_audio_converter_t * cnv;

  bg_read_audio_func_t read_func;

  gavl_audio_format_t * format;

  } audio_source_t;

typedef struct
  {
  source_common_t com;
  bg_video_converter_t * cnv;

  bg_read_video_func_t read_func;

  gavl_video_format_t * format;
  
  } video_source_t;

typedef struct
  {
  source_common_t com;
  /* Rest is only for overlays */
  gavl_video_converter_t * cnv;
  gavl_overlay_t ovl;
  int (*read_func)(void * priv, gavl_overlay_t*ovl, int stream);
  gavl_video_format_t * format;
  } subtitle_source_t;

/* Segments */

typedef struct
  {
  source_common_t * src;
  const bg_edl_segment_t * seg;
  int64_t mute_start;
  int64_t mute_duration;
  } segment_t;

/* Streams */

typedef struct
  {
  int num_segments;
  segment_t * segments;
  int current_segment;

  int num_sources;
  audio_source_t * sources;

  gavl_audio_format_t * format;
  
  bg_stream_action_t action;

  int64_t out_time;
  int64_t segment_end_time;
  

  const bg_edl_stream_t * es;
  
  gavl_audio_frame_t * frame;
  } audio_stream_t;

typedef struct
  {
  int num_segments;
  segment_t * segments;
  int current_segment;

  gavl_video_format_t * format;
  
  int num_sources;
  video_source_t * sources;

  bg_stream_action_t action;
  
  int64_t out_time;
  int64_t segment_end_time;
  
  const bg_edl_stream_t * es;
  } video_stream_t;

typedef struct
  {
  int num_segments;
  segment_t * segments;
  int current_segment;

  int num_sources;
  subtitle_source_t * sources;
  
  gavl_video_format_t * format;
  bg_stream_action_t action;
  int64_t out_time;
  int64_t segment_end_time;
  int64_t time_offset;      /* In output tics */
  
  const bg_edl_stream_t * es;
  } subtitle_stream_t;

typedef struct
  {
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  subtitle_stream_t * subtitle_text_streams;
  subtitle_stream_t * subtitle_overlay_streams;
  } track_t;

typedef struct
  {
  int current_track;
  track_t * tracks;
  
  bg_track_info_t * track_info;
  const bg_edl_t * edl;

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
  
static int64_t get_source_offset(const bg_edl_stream_t * st,
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

static int seek_segment(const bg_edl_stream_t * st,
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

static const bg_edl_segment_t *
find_same_source(const bg_edl_segment_t * segments,
                 const bg_edl_segment_t * seg)
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
                              const bg_edl_segment_t * seg)
  {
  int i;
  audio_source_t * source = NULL;
  const bg_edl_segment_t * same_source_seg;
  
  bg_audio_info_t * ai;
  audio_stream_t * s = &dec->tracks[track].audio_streams[stream];
  segment_t * as;
  ai = &dec->track_info[track].audio_streams[stream];

  if(seg->dst_time > ai->duration)
    {
    /* Insert mute segment */
    s->segments = realloc(s->segments, sizeof(*s->segments) * (s->num_segments+2));
    memset(s->segments + s->num_segments, 0, sizeof(*s->segments) * 2);
    s->num_segments += 2;
    as = s->segments + (s->num_segments-2);
    as->mute_start    = ai->duration;
    as->mute_duration = seg->dst_time - ai->duration;
    as++;
    }
  else
    {
    s->segments = realloc(s->segments, sizeof(*s->segments) * (s->num_segments+1));
    memset(s->segments + s->num_segments, 0, sizeof(*s->segments));
    s->num_segments++;
    as = s->segments + (s->num_segments-1);
    }
  same_source_seg =
    find_same_source(dec->edl->tracks[track].audio_streams[stream].segments, seg);
  
  if(same_source_seg)
    {
    for(i = 0; i < s->num_segments; i++)
      {
      if(same_source_seg == s->segments[i].seg)
        {
        source = (audio_source_t*)s->segments[i].src;
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
    source->cnv = bg_audio_converter_create(dec->a_opt);
    }
  as->src = (source_common_t*)source;
  as->seg = seg;
  ai->duration = seg->dst_time + seg->dst_duration;
  }

static int add_video_segment(edl_dec_t * dec, int track, int stream,
                             const bg_edl_segment_t * seg)
  {
  int i;
  video_source_t * source = NULL;
  const bg_edl_segment_t * same_source_seg;
  
  bg_video_info_t * vi;
  video_stream_t * s = &dec->tracks[track].video_streams[stream];
  segment_t * vs;
  vi = &dec->track_info[track].video_streams[stream];
  
  if(seg->dst_time > vi->duration)
    {
    /* Insert mute segment */
    s->segments = realloc(s->segments, sizeof(*s->segments) * (s->num_segments+2));
    memset(s->segments + s->num_segments, 0, sizeof(*s->segments) * 2);
    s->num_segments += 2;
    vs = s->segments + (s->num_segments-2);
    vs->mute_start    = vi->duration;
    vs->mute_duration = seg->dst_time - vi->duration;
    vs++;
    }
  else
    {
    s->segments = realloc(s->segments, sizeof(*s->segments) * (s->num_segments+1));
    memset(s->segments + s->num_segments, 0, sizeof(*s->segments));
    s->num_segments++;
    vs = s->segments + (s->num_segments-1);
    }
  same_source_seg =
    find_same_source(dec->edl->tracks[track].video_streams[stream].segments, seg);
  
  if(same_source_seg)
    {
    for(i = 0; i < s->num_segments; i++)
      {
      if(same_source_seg == s->segments[i].seg)
        {
        source = (video_source_t*)s->segments[i].src;
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
    source->cnv = bg_video_converter_create(dec->v_opt);
    }
  vs->src = (source_common_t*)source;
  vs->seg = seg;
  vi->duration = seg->dst_time + seg->dst_duration;
  return 1;
  }

static int add_subtitle_overlay_segment(edl_dec_t * dec, int track, int stream,
                                        const bg_edl_segment_t * seg)
  {
  int i;
  subtitle_source_t * source = NULL;
  const bg_edl_segment_t * same_source_seg;
  
  bg_subtitle_info_t * vi;
  subtitle_stream_t * s = &dec->tracks[track].subtitle_overlay_streams[stream];
  segment_t * vs;
  
  vi =
    &dec->track_info[track].subtitle_streams[stream +
                                             dec->edl->tracks[track].num_subtitle_text_streams];
  
  s->segments = realloc(s->segments, sizeof(*s->segments) * (s->num_segments+1));
  memset(s->segments + s->num_segments, 0, sizeof(*s->segments));
  s->num_segments++;
  vs = s->segments + (s->num_segments-1);
  
  same_source_seg =
    find_same_source(dec->edl->tracks[track].subtitle_overlay_streams[stream].segments, seg);
  
  if(same_source_seg)
    {
    for(i = 0; i < s->num_segments; i++)
      {
      if(same_source_seg == s->segments[i].seg)
        {
        source = (subtitle_source_t*)s->segments[i].src;
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
    source->cnv = gavl_video_converter_create();
    }
  vs->src = (source_common_t*)source;
  vs->seg = seg;
  vi->duration = seg->dst_time + seg->dst_duration;
  return 1;
  }

static int add_subtitle_text_segment(edl_dec_t * dec, int track, int stream,
                                     const bg_edl_segment_t * seg)
  {
  int i;
  subtitle_source_t * source = NULL;
  const bg_edl_segment_t * same_source_seg;
  
  bg_subtitle_info_t * vi;
  subtitle_stream_t * s = &dec->tracks[track].subtitle_text_streams[stream];
  segment_t * vs;
  
  vi = &dec->track_info[track].subtitle_streams[stream];
  
  s->segments = realloc(s->segments, sizeof(*s->segments) *
                        (s->num_segments+1));
  memset(s->segments + s->num_segments, 0, sizeof(*s->segments));
  s->num_segments++;
  vs = s->segments + (s->num_segments-1);
  
  same_source_seg =
    find_same_source(dec->edl->tracks[track].subtitle_text_streams[stream].segments, seg);
  
  if(same_source_seg)
    {
    for(i = 0; i < s->num_segments; i++)
      {
      if(same_source_seg == s->segments[i].seg)
        {
        source = (subtitle_source_t*)s->segments[i].src;
        break;
        }
      }
    }
  else
    {
    source = (subtitle_source_t*)s->sources + s->num_sources;
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
  audio_source_t * src;
  seg = &as->segments[as->current_segment];
  if(seg->seg)
    {
    as->segment_end_time =
      gavl_time_rescale(as->es->timescale, as->format->samplerate,
                        seg->seg->dst_time + seg->seg->dst_duration);
    t = get_source_offset(as->es, seg, as->out_time, as->format->samplerate);
    
    seg->src->plugin->seek(seg->src->handle->priv, &t, as->format->samplerate);
    src = (audio_source_t *)(seg->src);
    if(src->cnv)
      bg_audio_converter_reset(src->cnv);
    }
  else
    as->segment_end_time =
      gavl_time_rescale(as->es->timescale, as->format->samplerate,
                        seg->mute_start + seg->mute_duration);
  }

static void init_video_segment(video_stream_t * vs)
  {
  segment_t * seg;
  int64_t t;
  video_source_t * src;
  seg = &vs->segments[vs->current_segment];
  if(seg->seg)
    {
    vs->segment_end_time =
      gavl_time_rescale(vs->es->timescale, vs->format->timescale,
                        seg->seg->dst_time + seg->seg->dst_duration);
    t = get_source_offset(vs->es, seg, vs->out_time, vs->format->timescale);
    
    seg->src->plugin->seek(seg->src->handle->priv, &t, vs->format->timescale);

    src = (video_source_t *)(seg->src);
    if(src->cnv)
      bg_video_converter_reset(src->cnv);
    }
  else
    {
    vs->segment_end_time   =
      gavl_time_rescale(vs->es->timescale, vs->format->timescale,
                        seg->mute_start + seg->mute_duration);
    }
  }

static void init_subtitle_segment(subtitle_stream_t * vs)
  {
  segment_t * seg;
  int64_t t1, t2;
  seg = &vs->segments[vs->current_segment];

  vs->segment_end_time =
    gavl_time_rescale(vs->es->timescale, vs->format->timescale,
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
                         vs->format->timescale,
                         seg->seg->src_time);
  t2 = gavl_time_rescale(vs->es->timescale,
                         vs->format->timescale,
                         seg->seg->dst_time);
  
  vs->time_offset = t2 - t1;
  
  t1 = get_source_offset(vs->es, seg, vs->out_time, vs->format->timescale);
  
  seg->src->plugin->seek(seg->src->handle->priv, &t1, vs->format->timescale);
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
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  dec->tracks[dec->current_track].audio_streams[stream].action = action;
  return 1;
  }

static int set_video_stream_edl(void * priv, int stream, bg_stream_action_t action)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  dec->tracks[dec->current_track].video_streams[stream].action = action;
  return 1;
  }
  
static int set_subtitle_stream_edl(void * priv, int stream, bg_stream_action_t action)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;

  if(stream >= dec->edl->tracks[dec->current_track].num_subtitle_text_streams)
    {
    dec->tracks[dec->current_track].subtitle_overlay_streams[stream -
                                                             dec->edl->tracks[dec->current_track].num_subtitle_text_streams].action = action;
    }
  else
    {
    dec->tracks[dec->current_track].subtitle_text_streams[stream].action = action;
    }
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
  com->read_stream = com->stream;
  com->read_data = com->handle->priv;
  return 1;
  }

static void cleanup_source_common(source_common_t * com)
  {
  if(com->handle)
    bg_plugin_unref(com->handle);
  }

static int read_subtitle_overlay_convert(void * priv, gavl_overlay_t*ovl, int stream)
  {
  subtitle_source_t * sosrc;
  sosrc = (subtitle_source_t*)priv;

  if(!sosrc->ovl.frame)
    sosrc->ovl.frame = gavl_video_frame_create(sosrc->format);
  
  if(!sosrc->com.plugin->read_subtitle_overlay(sosrc->com.handle->priv,
                                               &sosrc->ovl,
                                               sosrc->com.stream))
    return 0;
  
  gavl_video_convert(sosrc->cnv, sosrc->ovl.frame, ovl->frame);

  /* TODO: Scale coordinates if source- and destination  sizes are different */
  gavl_rectangle_i_copy(&ovl->ovl_rect, &sosrc->ovl.ovl_rect);
  ovl->dst_x = sosrc->ovl.dst_x;
  ovl->dst_y = sosrc->ovl.dst_y;

  /* Need to undo the timescale and duration scaling */
  ovl->frame->timestamp = sosrc->ovl.frame->timestamp;
  ovl->frame->duration  = sosrc->ovl.frame->duration;
  
  return 1;
  }

static int start_edl(void * priv)
  {
  int i, j;
  edl_dec_t * dec;
  
  bg_track_info_t * ti;
  
  dec = (edl_dec_t*)priv;

  ti = &dec->track_info[dec->current_track];
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_audio_streams; i++)
    {
    audio_source_t * asrc;
    audio_stream_t * as;
    
    as = &dec->tracks[dec->current_track].audio_streams[i];
    if(as->action != BG_STREAM_ACTION_DECODE)
      continue;

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
      asrc->read_func = asrc->com.plugin->read_audio;
      asrc->format    =
        &asrc->com.ti->audio_streams[asrc->com.stream].format;
      
      if(!j)
        {
        gavl_audio_format_copy(&ti->audio_streams[i].format, asrc->format);
        as->format = &ti->audio_streams[i].format;
        }
      else
        {
        if(bg_audio_converter_init(asrc->cnv, asrc->format, as->format))
          {
          bg_audio_converter_connect_input(asrc->cnv, asrc->read_func,
                                           asrc->com.read_data,
                                           asrc->com.read_stream);
          asrc->read_func = bg_audio_converter_read;
          asrc->com.read_data = asrc->cnv;
          asrc->com.read_stream = 0;
          }
        }
      as->frame = gavl_audio_frame_create(as->format);
      }
    /* Initialize first segment */
    as->current_segment = 0;
    init_audio_segment(as);

    }
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_video_streams; i++)
    {
    video_stream_t * vs;
    video_source_t * vsrc;
    
    vs = &dec->tracks[dec->current_track].video_streams[i];
    if(vs->action != BG_STREAM_ACTION_DECODE)
      continue;
    
    /* Fire up the sources */
    
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
      vsrc->read_func = vsrc->com.plugin->read_video;
      vsrc->format    =
        &vsrc->com.ti->video_streams[vsrc->com.stream].format;

      if(!j)
        {
        gavl_video_format_copy(&ti->video_streams[i].format, vsrc->format);
        vs->format = &ti->video_streams[i].format;
        }
      else
        {
        if(bg_video_converter_init(vsrc->cnv, vsrc->format, vs->format))
          {
          bg_video_converter_connect_input(vsrc->cnv, vsrc->read_func,
                                           vsrc->com.read_data,
                                           vsrc->com.read_stream);
          vsrc->read_func = bg_video_converter_read;
          vsrc->com.read_data = vsrc->cnv;
          vsrc->com.read_stream = 0;
          }
        }
      }
    /* Initialize first segment */
    vs->current_segment = 0;
    init_video_segment(vs);
    }

  for(i = 0; i < dec->edl->tracks[dec->current_track].num_subtitle_text_streams; i++)
    {
    subtitle_stream_t * sts;
    subtitle_source_t * stsrc;
    bg_subtitle_info_t * si;
    
    si = &ti->subtitle_streams[i];
    
    sts = &dec->tracks[dec->current_track].subtitle_text_streams[i];
    if(sts->action != BG_STREAM_ACTION_DECODE)
      continue;
    
    /* Fire up the sources */
    for(j = 0; j < sts->num_sources; j++)
      {
      stsrc = &sts->sources[j];
      
      if(!init_source_common(dec, &sts->sources[j].com))
        return 0;
      if(stsrc->com.plugin->set_subtitle_stream &&
         !stsrc->com.plugin->set_subtitle_stream(stsrc->com.handle->priv,
                                                 stsrc->com.stream,
                                                 BG_STREAM_ACTION_DECODE))
        return 0;
      if(stsrc->com.plugin->start &&
         !stsrc->com.plugin->start(stsrc->com.handle->priv))
        return 0;

      stsrc->format    =
        &stsrc->com.ti->subtitle_streams[stsrc->com.stream].format;
      if(!j)
        {
        gavl_video_format_copy(&si->format, stsrc->format);
        sts->format = &si->format;
        }
      }
    /* Initialize first segment */
    sts->current_segment = 0;
    init_subtitle_segment(sts);
    }
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_subtitle_overlay_streams; i++)
    {
    subtitle_stream_t * sos;
    subtitle_source_t * sosrc;
    bg_subtitle_info_t * si;
    
    sos = &dec->tracks[dec->current_track].subtitle_overlay_streams[i];
    if(sos->action != BG_STREAM_ACTION_DECODE)
      continue;
    
    si = &ti->subtitle_streams[i + dec->edl->tracks[dec->current_track].num_subtitle_text_streams];
    
    /* Fire up the sources */
    for(j = 0; j < sos->num_sources; j++)
      {
      sosrc = &sos->sources[j];
      
      if(!init_source_common(dec, &sos->sources[j].com))
        return 0;
      
      if(sosrc->com.plugin->set_subtitle_stream &&
         !sosrc->com.plugin->set_subtitle_stream(sosrc->com.handle->priv,
                                                 sosrc->com.stream,
                                                 BG_STREAM_ACTION_DECODE))
        return 0;
      if(sosrc->com.plugin->start &&
         !sosrc->com.plugin->start(sosrc->com.handle->priv))
        return 0;
      sosrc->read_func = sosrc->com.plugin->read_subtitle_overlay;
      sosrc->format    =
        &sosrc->com.ti->subtitle_streams[sosrc->com.stream].format;
      
      if(!j)
        {
        gavl_video_format_copy(&si->format, sosrc->format);
        sos->format = &si->format;
        }
      else
        {
        if(gavl_video_converter_init(sosrc->cnv, sosrc->format, sos->format))
          {
          sosrc->read_func = read_subtitle_overlay_convert;
          sosrc->com.read_data = sosrc;
          // sosrc->com.read_stream must be unchanged
          }
        }
      }
    /* Initialize first segment */
    sos->current_segment = 0;
    init_subtitle_segment(sos);
    }
  
  return 1;
  }

static int read_audio_edl(void * priv, gavl_audio_frame_t* frame, int stream,
                          int num_samples)
  {
  int samples_decoded = 0;
  edl_dec_t * dec;
  audio_stream_t * as;
  int num;
  segment_t * seg;
  audio_source_t * src;
  dec = (edl_dec_t*)priv;
  as = &dec->tracks[dec->current_track].audio_streams[stream];

  if(as->current_segment >= as->num_segments)
    return 0;
  
  while(samples_decoded < num_samples)
    {
    num = as->format->samples_per_frame;
    
    /* Check for end of frame */
    if(samples_decoded + num > num_samples)
      num = num_samples - samples_decoded;

    /* Check for end of segment */
    if(as->out_time + samples_decoded + num > as->segment_end_time)
      num = as->segment_end_time - (as->out_time + samples_decoded);
    
    while(!num)
      {
      /* End of segment */
      as->current_segment++;
      if(as->current_segment >= as->num_segments)
        {
        /* End of stream */
        break;
        }
      else /* New segment (skip empty ones) */
        {
        init_audio_segment(as);

        num = as->format->samples_per_frame;
        
        /* Check for end of frame */
        if(samples_decoded + num > num_samples)
          num = num_samples - samples_decoded;
        
        /* Check for end of segment */
        if(as->out_time + samples_decoded + num > as->segment_end_time)
          num = as->segment_end_time - (as->out_time + samples_decoded);
        
        }
      }

    if(!num)
      break;
    
    seg = &as->segments[as->current_segment];
    if(!seg->src)
      {
      gavl_audio_frame_mute_samples(as->frame, as->format, num);
      }
    else
      {
      src = (audio_source_t *)(seg->src);
      if(!src->read_func(seg->src->read_data,
                         as->frame,
                         seg->src->read_stream,
                         num))
        return samples_decoded;
      }
    num = gavl_audio_frame_copy(as->format,
                                frame,
                                as->frame,
                                samples_decoded /* dst_pos */,
                                0 /* 	src_pos */ ,
                                num_samples - samples_decoded,
                                as->frame->valid_samples);
    samples_decoded += num;
    }
  frame->timestamp = as->out_time;
  frame->valid_samples = samples_decoded;
  as->out_time += samples_decoded;
  return samples_decoded;
  }

static int read_video_edl(void * priv, gavl_video_frame_t* frame, int stream)
  {
  edl_dec_t * dec;
  video_stream_t * vs;
  segment_t * seg;
  video_source_t * src;
  dec = (edl_dec_t*)priv;
  vs = &dec->tracks[dec->current_track].video_streams[stream];

  if(vs->current_segment >= vs->num_segments)
    return 0;
  
  while(vs->out_time >= vs->segment_end_time)
    {
    vs->current_segment++;

    if(vs->current_segment >= vs->num_segments)
      return 0;
    init_video_segment(vs);
    }
  
  seg = &vs->segments[vs->current_segment];
  if(!seg->src)
    {
    gavl_video_frame_clear(frame, vs->format);
    
    frame->timestamp = vs->out_time;
    frame->duration = vs->format->frame_duration;
    vs->out_time += vs->format->frame_duration;
    }
  else
    {
    src = (video_source_t *)seg->src;
    
    if(!src->read_func(src->com.read_data, frame, src->com.read_stream))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Unexpected EOF, clearing frame");
      gavl_video_frame_clear(frame, vs->format);
      }
    
    frame->timestamp = vs->out_time;
    //    vs->out_time = frame->timestamp + frame->duration;
    vs->out_time += frame->duration;
    }
  return 1;
  }

static int has_subtitle_edl(void * priv, int stream)
  {
  //  edl_dec_t * dec;
  //  dec = (edl_dec_t*)priv;
  return 1;
  }


typedef struct
  {
  gavl_overlay_t * ovl;
  char ** text;
  int * text_alloc;
  int64_t * start_time;
  int64_t * duration;
  subtitle_stream_t * s;
  } decode_subtitle_data;

static int
decode_subtitle_text(decode_subtitle_data * d)
  {
  subtitle_source_t * src;
  src = (subtitle_source_t*)(d->s->segments[d->s->current_segment].src);
  return src->com.plugin->read_subtitle_text(src->com.handle->priv,
                                             d->text, d->text_alloc,
                                             d->start_time,
                                             d->duration, src->com.stream);
  }

static int
decode_subtitle_overlay(decode_subtitle_data * d)
  {
  subtitle_source_t * src;
  src = (subtitle_source_t*)(d->s->segments[d->s->current_segment].src);
  return src->read_func(src->com.read_data, d->ovl, src->com.stream);
  }

/* Here, the actual magic happens */
static int
decode_subtitle(int (*decode_func)(decode_subtitle_data*),
                decode_subtitle_data * d)
  {
  if(d->s->current_segment >= d->s->es->num_segments)
    return 0;
  
  while(1)
    {
    /* Check is segment is finished */
    if(d->s->out_time >= d->s->segment_end_time)
      {
      d->s->current_segment++;
      if(d->s->current_segment >= d->s->es->num_segments)
        return 0;
      init_subtitle_segment(d->s);
      }
    
    /* Decode one subtitle */
    if(!decode_func(d))
      {
      d->s->current_segment++;
      if(d->s->current_segment >= d->s->es->num_segments)
        return 0;
      init_subtitle_segment(d->s);
      continue;
      }
    /* Correct timestamps */
    
    *d->start_time =
      gavl_time_rescale(((subtitle_source_t*)(d->s->segments[d->s->current_segment].src))->format->timescale,
                        d->s->format->timescale, *d->start_time) + d->s->time_offset;
    *d->duration =
      gavl_time_rescale(((subtitle_source_t*)(d->s->segments[d->s->current_segment].src))->format->timescale,
                        d->s->format->timescale, *d->duration);

    /* Still inside the segment */

    if(*d->start_time < d->s->segment_end_time)
      {
      if(*d->start_time + *d->duration > d->s->segment_end_time)
        *d->duration = d->s->segment_end_time - *d->start_time;
      
      d->s->out_time = *d->start_time + *d->duration;
      break;
      }
    /* Subtitle there but after segment */
    else
      {
      d->s->out_time = d->s->segment_end_time;
      }
    }
  return 1;
  }

static int read_subtitle_overlay_edl(void * priv,
                                     gavl_overlay_t*ovl, int stream)
  {
  edl_dec_t * dec;
  decode_subtitle_data d;
  dec = (edl_dec_t*)priv;
  
  d.ovl = ovl;
  d.start_time = &ovl->frame->timestamp;
  d.duration   = &ovl->frame->duration;

  d.s =
    &dec->tracks[dec->current_track].subtitle_overlay_streams[stream -
                                                             dec->edl->tracks[dec->current_track].num_subtitle_text_streams];
  return decode_subtitle(decode_subtitle_overlay, &d);
  }

static int read_subtitle_text_edl(void * priv,
                                  char ** text, int * text_alloc,
                                  int64_t * start_time,
                                  int64_t * duration, int stream)
  {
  edl_dec_t * dec;
  decode_subtitle_data d;

  dec = (edl_dec_t*)priv;

  d.text = text;
  d.text_alloc = text_alloc;

  d.start_time = start_time;
  d.duration   = duration;

  d.s =
    &dec->tracks[dec->current_track].subtitle_text_streams[stream];
  
  return decode_subtitle(decode_subtitle_text, &d);
  }

static void seek_edl(void * priv, int64_t * time, int scale)
  {
  edl_dec_t * dec;
  int i;
  track_t * t;
  bg_edl_track_t * et;
  int64_t time_scaled;
  dec = (edl_dec_t*)priv;
  t = &dec->tracks[dec->current_track];
  et = &dec->edl->tracks[dec->current_track];
  
  for(i = 0; i < et->num_audio_streams; i++)
    {
    if(t->audio_streams[i].action != BG_STREAM_ACTION_DECODE)
      continue;
    time_scaled = gavl_time_rescale(scale, t->audio_streams[i].format->samplerate,
                                    *time);
    t->audio_streams[i].current_segment =
      seek_segment(t->audio_streams[i].es,
                   t->audio_streams[i].segments,
                   t->audio_streams[i].num_segments,
                   time_scaled, t->audio_streams[i].format->samplerate);
    if(t->audio_streams[i].current_segment < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked audio out of range");
      return;
      }
    t->audio_streams[i].out_time = time_scaled;
    init_audio_segment(&t->audio_streams[i]);
    }
  for(i = 0; i < et->num_video_streams; i++)
    {
    if(t->video_streams[i].action != BG_STREAM_ACTION_DECODE)
      continue;

    time_scaled = gavl_time_rescale(scale, t->video_streams[i].format->timescale,
                                    *time);
    t->video_streams[i].current_segment =
      seek_segment(t->video_streams[i].es,
                   t->video_streams[i].segments,
                   t->video_streams[i].num_segments,
                   time_scaled, t->video_streams[i].format->timescale);
    if(t->video_streams[i].current_segment < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked video out of range");
      return;
      }
    t->video_streams[i].out_time = time_scaled;
    init_video_segment(&t->video_streams[i]);
    
    }
  for(i = 0; i < et->num_subtitle_text_streams; i++)
    {
    if(t->subtitle_text_streams[i].action != BG_STREAM_ACTION_DECODE)
      continue;

    time_scaled = gavl_time_rescale(scale, t->subtitle_text_streams[i].format->timescale,
                                    *time);
    
    t->subtitle_text_streams[i].current_segment =
      seek_segment(t->subtitle_text_streams[i].es,
                   t->subtitle_text_streams[i].segments,
                   t->subtitle_text_streams[i].num_segments,
                   time_scaled, t->subtitle_text_streams[i].format->timescale);
    if(t->subtitle_text_streams[i].current_segment < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked text subtitles out of range");
      return;
      }
    t->subtitle_text_streams[i].out_time = time_scaled;
    init_subtitle_segment(&t->subtitle_text_streams[i]);
    }
  for(i = 0; i < et->num_subtitle_overlay_streams; i++)
    {
    if(t->subtitle_overlay_streams[i].action != BG_STREAM_ACTION_DECODE)
      continue;

    time_scaled = gavl_time_rescale(scale, t->subtitle_overlay_streams[i].format->timescale,
                                    *time);
    
    t->subtitle_overlay_streams[i].current_segment =
      seek_segment(t->subtitle_overlay_streams[i].es,
                   t->subtitle_overlay_streams[i].segments,
                   t->subtitle_overlay_streams[i].num_segments,
                   time_scaled, t->subtitle_overlay_streams[i].format->timescale);
    if(t->subtitle_overlay_streams[i].current_segment < 0)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked text subtitles out of range");
      return;
      }
    t->subtitle_overlay_streams[i].out_time = time_scaled;
    init_subtitle_segment(&t->subtitle_overlay_streams[i]);
    }
        
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
      /* Close sources */
      for(k = 0; k < dec->tracks[i].audio_streams[j].num_sources; k++)
        {
        cleanup_source_common(&dec->tracks[i].audio_streams[j].sources[k].com);
        if(dec->tracks[i].audio_streams[j].sources[k].cnv)
          bg_audio_converter_destroy(dec->tracks[i].audio_streams[j].sources[k].cnv);
        }
      if(dec->tracks[i].audio_streams[j].sources)
        free(dec->tracks[i].audio_streams[j].sources);
      
      if(dec->tracks[i].audio_streams[j].segments)
        free(dec->tracks[i].audio_streams[j].segments);
      if(dec->tracks[i].audio_streams[j].frame)
        gavl_audio_frame_destroy(dec->tracks[i].audio_streams[j].frame);
      }
    if(dec->tracks[i].audio_streams)
      free(dec->tracks[i].audio_streams);
    
    for(j = 0; j < dec->edl->tracks[i].num_video_streams; j++)
      {
      /* Close sources */
      for(k = 0; k < dec->tracks[i].video_streams[j].num_sources; k++)
        {
        cleanup_source_common(&dec->tracks[i].video_streams[j].sources[k].com);
        if(dec->tracks[i].video_streams[j].sources[k].cnv)
          bg_video_converter_destroy(dec->tracks[i].video_streams[j].sources[k].cnv);
        }
      if(dec->tracks[i].video_streams[j].sources)
        free(dec->tracks[i].video_streams[j].sources);

      if(dec->tracks[i].video_streams[j].segments)
        free(dec->tracks[i].video_streams[j].segments);
      }
    if(dec->tracks[i].video_streams)
      free(dec->tracks[i].video_streams);

    for(j = 0; j < dec->edl->tracks[i].num_subtitle_text_streams; j++)
      {
      /* Close sources */
      for(k = 0; k < dec->tracks[i].subtitle_text_streams[j].num_sources; k++)
        cleanup_source_common(&dec->tracks[i].subtitle_text_streams[j].sources[k].com);
      if(dec->tracks[i].subtitle_text_streams[j].sources)
        free(dec->tracks[i].subtitle_text_streams[j].sources);
      if(dec->tracks[i].subtitle_text_streams[j].segments)
        free(dec->tracks[i].subtitle_text_streams[j].segments);
      }
    if(dec->tracks[i].subtitle_text_streams)
      free(dec->tracks[i].subtitle_text_streams);

    for(j = 0; j < dec->edl->tracks[i].num_subtitle_overlay_streams; j++)
      {
      /* Close sources */
      for(k = 0; k < dec->tracks[i].subtitle_overlay_streams[j].num_sources; k++)
        {
        cleanup_source_common(&dec->tracks[i].subtitle_overlay_streams[j].sources[k].com);
        if(dec->tracks[i].subtitle_overlay_streams[j].sources[k].cnv)
          gavl_video_converter_destroy(dec->tracks[i].subtitle_overlay_streams[j].sources[k].cnv);
        }
      if(dec->tracks[i].subtitle_overlay_streams[j].sources)
        free(dec->tracks[i].subtitle_overlay_streams[j].sources);

      if(dec->tracks[i].subtitle_overlay_streams[j].segments)
        free(dec->tracks[i].subtitle_overlay_streams[j].segments);
      
      }
    if(dec->tracks[i].subtitle_overlay_streams)
      free(dec->tracks[i].subtitle_overlay_streams);
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
    .set_subtitle_stream =      set_subtitle_stream_edl,
    /*
     *  Start decoding.
     *  Track info is the track, which should be played.
     *  The plugin must take care of the "active" fields
     *  in the stream infos to check out, which streams are to be decoded
     */
    .start =                 start_edl,
    /* Read one video frame (returns FALSE on EOF) */
    .read_video =      read_video_edl,
    .read_audio =      read_audio_edl,
    .has_subtitle = has_subtitle_edl,
    .read_subtitle_text =      read_subtitle_text_edl,
    .read_subtitle_overlay =      read_subtitle_overlay_edl,

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
                             const bg_edl_t * edl,
                             const bg_plugin_info_t * info,
                             bg_plugin_handle_t ** ret1,
                             bg_input_callbacks_t * callbacks)
  {
  int i, j, k;
  const bg_edl_track_t * track;
  const bg_edl_stream_t * stream;
  bg_track_info_t * ti;
  track_t * t;
  edl_dec_t * priv;
  bg_plugin_handle_t * ret;
  gavl_time_t test_duration;
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
    
    ti->name = bg_strdup(ti->name, track->name);

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
      
      t->audio_streams[j].es = stream;
      for(k = 0; k < stream->num_segments; k++)
        {
        add_audio_segment(priv, i, j, &stream->segments[k]);
        }
      test_duration = gavl_time_unscale(stream->timescale, ti->audio_streams[j].duration);
      if(ti->duration < test_duration)
        ti->duration = test_duration;
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
      
      t->video_streams[j].es = stream;
      for(k = 0; k < stream->num_segments; k++)
        {
        add_video_segment(priv, i, j, &stream->segments[k]);
        }
      test_duration =
        gavl_time_unscale(stream->timescale, ti->video_streams[j].duration);
      if(ti->duration < test_duration)
        ti->duration = test_duration;

      }
    /* Subtitle text streams */

    ti->num_subtitle_streams = track->num_subtitle_text_streams +
      track->num_subtitle_overlay_streams;

    ti->subtitle_streams = calloc(ti->num_subtitle_streams,
                                  sizeof(*ti->subtitle_streams));

    t->subtitle_text_streams = calloc(track->num_subtitle_text_streams,
                                      sizeof(*t->subtitle_text_streams));
    t->subtitle_overlay_streams = calloc(track->num_subtitle_overlay_streams,
                                      sizeof(*t->subtitle_overlay_streams));
    
    for(j = 0; j < track->num_subtitle_text_streams; j++)
      {
      stream = &track->subtitle_text_streams[j];

      t->subtitle_text_streams[j].sources =
        calloc(stream->num_segments,
               sizeof(*t->subtitle_text_streams[j].sources));

      t->subtitle_text_streams[j].es = stream;

      for(k = 0; k < stream->num_segments; k++)
        {
        add_subtitle_text_segment(priv, i, j, &stream->segments[k]);
        }
      test_duration =
        gavl_time_unscale(stream->timescale,
                          ti->subtitle_streams[j].duration);
      if(ti->duration < test_duration)
        ti->duration = test_duration;
      ti->subtitle_streams[j].is_text = 1;
      }
    /* Subtitle overlay streams */
    for(j = 0; j < track->num_subtitle_overlay_streams; j++)
      {
      stream = &track->subtitle_overlay_streams[j];
      
      t->subtitle_overlay_streams[j].sources =
        calloc(stream->num_segments,
               sizeof(*t->subtitle_overlay_streams[j].sources));

      t->subtitle_overlay_streams[j].es = stream;


      for(k = 0; k < stream->num_segments; k++)
        {
        add_subtitle_overlay_segment(priv, i, j, &stream->segments[k]);
        }
      test_duration =
        gavl_time_unscale(stream->timescale,
                          ti->subtitle_streams[j+track->num_subtitle_text_streams].duration);
      if(ti->duration < test_duration)
        ti->duration = test_duration;
      }
    }
  return 1;
  }
