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

#include <string.h>

#include <config.h>
#include <translation.h>

#include <pluginregistry.h>
#include <converters.h>

#include <log.h>
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

/* Segments */

typedef struct
  {
  source_common_t * src;
  const bg_edl_segment_t * seg;
  int64_t mute_start;
  int64_t mute_duration;
  } segment_t;

typedef struct
  {
  const bg_edl_segment_t * seg;
  
  int64_t mute_start;
  int64_t mute_duration;
  } subtitle_text_segment_t;

typedef struct
  {
  const bg_edl_segment_t * seg;
  
  int64_t mute_start;
  int64_t mute_duration;
  } subtitle_overlay_segment_t;

/* Streams */

typedef struct
  {
  int num_segments;
  segment_t * segments;

  int num_sources;
  audio_source_t * sources;

  gavl_audio_format_t * format;
  
  bg_stream_action_t action;

  int64_t out_time;
  int64_t segment_end_time;
  
  int current_segment;

  const bg_edl_stream_t * es;
  
  gavl_audio_frame_t * frame;
  } audio_stream_t;

typedef struct
  {
  int num_segments;
  segment_t * segments;

  gavl_video_format_t * format;
  
  int num_sources;
  video_source_t * sources;

  bg_stream_action_t action;
  
  int64_t out_time;
  int64_t segment_end_time;
  int64_t segment_start_time;
  int64_t first_pts;
  
  int current_segment;
  
  const bg_edl_stream_t * es;
  } video_stream_t;

typedef struct
  {
  int num_segments;
  subtitle_text_segment_t * segments;

  bg_stream_action_t action;
    
  bg_edl_stream_t * es;
  } subtitle_text_stream_t;

typedef struct
  {
  int num_segments;
  subtitle_overlay_segment_t * segments;
  
  gavl_video_format_t format;
  bg_stream_action_t action;
  bg_edl_stream_t * es;
  } subtitle_overlay_stream_t;

typedef struct
  {
  audio_stream_t * audio_streams;
  video_stream_t * video_streams;
  subtitle_text_stream_t * subtitle_text_streams;
  subtitle_overlay_stream_t * subtitle_overlay_streams;
  } track_t;

typedef struct
  {
  int current_track;
  track_t * tracks;
  
  bg_track_info_t * track_info;
  const bg_edl_t * edl;

  bg_plugin_registry_t * plugin_reg;
  const bg_plugin_info_t * plugin_info;
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
  seg_offset = out_time - gavl_time_rescale(st->timescale, out_scale, s->seg->dst_time);
  source_offset = gavl_time_rescale(s->seg->timescale, out_scale, s->seg->src_time);
  return source_offset + seg_offset;
  }

static int seek_segment(const bg_edl_stream_t * st,
                        segment_t * segs, int nsegs, int64_t out_time, int out_scale)
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

static const bg_edl_segment_t * find_same_source(const bg_edl_segment_t * segments,
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
  return (bg_edl_segment_t *)0;
  }
                           
static void add_audio_segment(edl_dec_t * dec, int track, int stream,
                              const bg_edl_segment_t * seg)
  {
  int i;
  audio_source_t * source = (audio_source_t*)0;
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
  video_source_t * source = (video_source_t*)0;
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
    
    seg->src->plugin->seek(seg->src->handle->priv,
                           &t, as->format->samplerate);
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
    vs->segment_start_time =
      gavl_time_rescale(vs->es->timescale, vs->format->timescale,
                        seg->seg->dst_time);
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
    vs->segment_start_time = gavl_time_rescale(vs->es->timescale, vs->format->timescale,
                                               seg->mute_start);
    vs->segment_end_time   = gavl_time_rescale(vs->es->timescale, vs->format->timescale,
                                               seg->mute_start + seg->mute_duration);
    }
  vs->first_pts = PTS_UNDEFINED;
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
  return 0;
  }

/* Start streams */

static int init_source_common(edl_dec_t * dec,
                              source_common_t * com)
  {
  if(!bg_input_plugin_load(dec->plugin_reg,
                           com->url,
                           dec->plugin_info,
                           &com->handle,
                           dec->callbacks, 0))
    return 0;
  
  com->plugin = (bg_input_plugin_t*)(com->handle->plugin);
  if(com->plugin->set_track && !com->plugin->set_track(com->handle->priv, com->track))
    return 0;
  com->ti = com->plugin->get_track_info(com->handle->priv, com->track);

  if(!com->ti->seekable)
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

static int start_edl(void * priv)
  {
  int i, j;
  edl_dec_t * dec;
  audio_stream_t * as;
  video_stream_t * vs;
  audio_source_t * asrc;
  video_source_t * vsrc;
  bg_track_info_t * ti;
  
  dec = (edl_dec_t*)priv;

  ti = &dec->track_info[dec->current_track];
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_audio_streams; i++)
    {
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
          bg_audio_converter_connect_input(asrc->cnv, asrc->read_func, asrc->com.read_data,
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

    //    fprintf(stderr, "Audio format:\n");
    //    gavl_audio_format_dump(as->format);

    }
  
  for(i = 0; i < dec->edl->tracks[dec->current_track].num_video_streams; i++)
    {
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
          bg_video_converter_connect_input(vsrc->cnv, vsrc->read_func, vsrc->com.read_data,
                                           vsrc->com.read_stream);
          vsrc->read_func = bg_video_converter_read;
          vsrc->com.read_data = vsrc->cnv;
          vsrc->com.read_stream = 0;
          }
        }
      /* Initialize first segment */
      vs->current_segment = 0;
      init_video_segment(vs);
      }
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
      src->read_func(seg->src->read_data, as->frame, seg->src->read_stream,
                     num);
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

  //  fprintf(stderr, "read_video_edl: %ld %ld %ld\n",
  //          vs->first_pts, vs->out_time, vs->segment_end_time);
  
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
      

    if(vs->first_pts == PTS_UNDEFINED)
      vs->first_pts = frame->timestamp;
    
    //    frame->timestamp = vs->segment_start_time + frame->timestamp - vs->first_pts;
    frame->timestamp = vs->out_time;
    //    vs->out_time = frame->timestamp + frame->duration;
    vs->out_time += frame->duration;
    }
  return 1;
  }

static int has_subtitle_edl(void * priv, int stream)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  return 0;
  }
    
static int read_subtitle_overlay_edl(void * priv,
                                     gavl_overlay_t*ovl, int stream)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  return 0;
  }

static int read_subtitle_text_edl(void * priv,
                                  char ** text, int * text_alloc,
                                  int64_t * start_time,
                                  int64_t * duration, int stream)
  {
  edl_dec_t * dec;
  dec = (edl_dec_t*)priv;
  return 0;
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
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked out of range");
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
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Seeked out of range");
      return;
      }
    t->video_streams[i].out_time = time_scaled;
    init_video_segment(&t->video_streams[i]);
    
    }
  for(i = 0; i < et->num_subtitle_text_streams; i++)
    {
    if(t->subtitle_text_streams[i].action != BG_STREAM_ACTION_DECODE)
      continue;
    
    }
  for(i = 0; i < et->num_subtitle_overlay_streams; i++)
    {
    if(t->subtitle_overlay_streams[i].action != BG_STREAM_ACTION_DECODE)
      continue;
    
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
      
      }
    for(j = 0; j < dec->edl->tracks[i].num_subtitle_overlay_streams; j++)
      {
      
      }
    }
  if(dec->tracks)
    {
    free(dec->tracks);
    dec->tracks = (track_t*)0;
    }
  if(dec->track_info)
    {
    for(i = 0; i < dec->edl->num_tracks; i++)
      {
      bg_track_info_free(&dec->track_info[i]);
      }
    free(dec->track_info);
    dec->track_info = (bg_track_info_t*)0;
    
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
      .name =           "i_edl",
      .long_name =      TRS("EDL decoder"),
      .description =    TRS("This metaplugin decodes and EDL as if it was a single file."),
      .type =           BG_PLUGIN_INPUT,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
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


int bg_input_plugin_load_edl(bg_plugin_registry_t * reg,
                             const bg_edl_t * edl,
                             const bg_plugin_info_t * info,
                             bg_plugin_handle_t ** ret1,
                             bg_input_callbacks_t * callbacks)
  {
  int i, j, k;
  const bg_edl_track_t * track;
  const bg_edl_stream_t * stream;
  const bg_edl_segment_t * seg;
  bg_track_info_t * ti;
  track_t * t;
  edl_dec_t * priv;
  bg_plugin_handle_t * ret;
  gavl_time_t test_duration;
  if(*ret1)
    bg_plugin_unref(*ret1);
  
  ret = calloc(1, sizeof(*ret));
  *ret1 = ret;
    
  ret->plugin_reg = reg;
  ret->plugin = (bg_plugin_common_t*)(&edl_plugin);
  pthread_mutex_init(&(ret->mutex),(pthread_mutexattr_t *)0);

  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  ret->refcount = 1;
  
  priv->edl = edl;
  priv->plugin_reg = reg;
  priv->plugin_info = info;
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
    
    ti->seekable = 1;
    
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
    
    for(j = 0; j < track->num_subtitle_text_streams; j++)
      {
      stream = &track->subtitle_text_streams[j];
      for(k = 0; k < stream->num_segments; k++)
        {
        seg = &stream->segments[k];
        }
      test_duration =
        gavl_time_unscale(stream->timescale,
                          ti->subtitle_streams[j].duration);
      if(ti->duration < test_duration)
        ti->duration = test_duration;
      
      }
    /* Subtitle overlay streams */
    for(j = 0; j < track->num_subtitle_overlay_streams; j++)
      {
      stream = &track->subtitle_overlay_streams[j];
      for(k = 0; k < stream->num_segments; k++)
        {
        seg = &stream->segments[k];
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
