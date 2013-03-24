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

typedef struct edldec_s edldec_t;

typedef struct
  {
  bg_plugin_handle_t * h;
  bg_input_plugin_t * plugin;
  int track;
  const char * location;
  int refcount;
  int stream;
  bg_stream_type_t type;  
  } source_t;

static int source_init(source_t * s, 
                       bg_plugin_registry_t * plugin_reg,
                       const gavl_edl_t * edl, 
                       const gavl_edl_segment_t * seg,
                       bg_stream_type_t type)
  {
  bg_track_info_t * ti;
  s->location = seg->url ? seg->url : edl->url;

  if(!bg_input_plugin_load(plugin_reg,
                           s->location,
                           NULL,
                           &s->h,
                           NULL, 0))
    return 0;
  
  s->track = seg->track;
  s->stream = seg->stream;
  s->type = type;

  s->plugin = (bg_input_plugin_t*)(s->h->plugin);
  if(s->plugin->set_track && !s->plugin->set_track(s->h->priv, s->track))
    return 0;
  
  ti = s->plugin->get_track_info(s->h->priv, s->track);

  if(!(ti->flags & BG_TRACK_SEEKABLE))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "EDL only works with seekable sources");
    return 0;
    }

  switch(type)
    {
    case BG_STREAM_AUDIO:
      s->plugin->set_audio_stream(s->h->priv, s->stream,
                                  BG_STREAM_ACTION_DECODE);
      break;
    case BG_STREAM_VIDEO:
      s->plugin->set_video_stream(s->h->priv, s->stream,
                                  BG_STREAM_ACTION_DECODE);
      break;
    case BG_STREAM_TEXT:
      s->plugin->set_text_stream(s->h->priv, s->stream,
                                 BG_STREAM_ACTION_DECODE);
      break;
    case BG_STREAM_OVERLAY:
      s->plugin->set_overlay_stream(s->h->priv, s->stream,
                                    BG_STREAM_ACTION_DECODE);
      break;
    }
  
  if(s->plugin->start && !s->plugin->start(s->h->priv))
    return 0;

  return 1;
  }

static void source_cleanup(source_t * s)
  {
  if(s->h)
    bg_plugin_unref(s->h);
  memset(s, 0, sizeof(*s));
  }

static void source_ref(source_t * s)
  {
  s->refcount++;
  }

static void source_unref(source_t * s)
  {
  s->refcount--;
  }

typedef struct
  {
  gavl_audio_source_t * asrc_ext;
  gavl_audio_source_t * asrc_int;

  gavl_video_source_t * vsrc_ext;
  gavl_video_source_t * vsrc_int;

  gavl_packet_source_t * psrc_ext;
  gavl_packet_source_t * psrc_int;

  bg_stream_action_t action;

  const gavl_edl_stream_t * s;
  const gavl_edl_segment_t * seg;

  source_t * src;
  
  int64_t mute_time;
  int64_t pts;

  struct edldec_s * dec;

  const gavl_audio_format_t * afmt;
  const gavl_video_format_t * vfmt;
  } stream_t;

static void stream_init(stream_t * s, gavl_edl_stream_t * es,
                        struct edldec_s * dec)
  {
  s->s = es;
  s->dec = dec;
  }

static void stream_cleanup(stream_t * s)
  {
  if(s->asrc_ext)
    gavl_audio_source_destroy(s->asrc_ext);
  if(s->vsrc_ext)
    gavl_video_source_destroy(s->vsrc_ext);
  if(s->psrc_ext)
    gavl_packet_source_destroy(s->psrc_ext);
  }

struct edldec_s
  {
  int num_sources;
  source_t * sources;

  stream_t * audio_streams;
  stream_t * video_streams; 
  stream_t * text_streams; 
  stream_t * overlay_streams; 

  const gavl_edl_t * edl;
  const gavl_edl_track_t * t;

  bg_track_info_t * ti;
  bg_track_info_t * ti_cur;

  bg_plugin_registry_t * plugin_reg;
  };

static source_t * get_source(edldec_t * dec, bg_stream_type_t type,
                             const gavl_edl_segment_t * seg,
                             int64_t src_time)
  {
  int i;
  source_t * ret = NULL;
  const char * location = seg->url ? seg->url : dec->edl->url;

  /* Find a cached source */
  for(i = 0; i < dec->num_sources; i++)
    {
    source_t * s = dec->sources + i;

    if(s->location &&
       !strcmp(s->location, location) &&
       (s->track == seg->track) &&
       (s->type == type) &&
       (s->stream == seg->stream) &&
       !s->refcount)
      ret = s;
    }
  
  /* Find an empty slot */
  if(!ret)
    {
    for(i = 0; i < dec->num_sources; i++)
      {
      source_t * s = dec->sources + i;
      if(!s->location)
        {
        ret = s;
        break;
        }
      }
    }

  /* Free an existing slot */
  if(!ret)
    {
    for(i = 0; i < dec->num_sources; i++)
      {
      source_t * s = dec->sources + i;
      if(!s->refcount)
        {
        ret = s;
        source_cleanup(ret);
        break;
        }
      }
    }

  if(!ret->location)
    {
    if(!source_init(ret, dec->plugin_reg, dec->edl, seg, type))
      return NULL;
    }

  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Seeking to %"PRId64, src_time);
  ret->plugin->seek(ret->h->priv, &src_time, seg->timescale);
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Seeked to %"PRId64, src_time);
  
  return ret;  
  }

static void set_callbacks_edl(void * priv, bg_input_callbacks_t * cb)
  {

  }

static int get_num_tracks_edl(void * priv)
  {
  edldec_t * ed = priv;
  return ed->edl->num_tracks;
  }

static bg_track_info_t * get_track_info_edl(void * priv, int track)
  {
  edldec_t * ed = priv;
  return &ed->ti[track];
  }

static stream_t * streams_create(edldec_t * dec,
                                 gavl_edl_stream_t * es, int num)
  {
  int i;
  stream_t * ret;

  if(!num)
    return NULL;

  ret = calloc(num, sizeof(*ret));

  for(i = 0; i < num; i++)
    stream_init(&ret[i], &es[i], dec);

  return ret;
  }

static void streams_destroy(stream_t * s, int num)
  {
  int i;
  for(i = 0; i < num; i++)
    stream_cleanup(&s[i]);
  if(s)
    free(s);
  }

static int set_track_edl(void * priv, int track)
  {
  edldec_t * ed = priv;
  /* Clean up earlier streams */
  if(ed->t)
    {
    streams_destroy(ed->audio_streams, ed->t->num_audio_streams);
    streams_destroy(ed->video_streams, ed->t->num_video_streams);
    streams_destroy(ed->text_streams, ed->t->num_text_streams);
    streams_destroy(ed->overlay_streams, ed->t->num_overlay_streams);
    }

  ed->t = &ed->edl->tracks[track];
  ed->ti_cur = &ed->ti[track];
  /* Create streams */

  ed->audio_streams   =
    streams_create(ed, ed->t->audio_streams,   ed->t->num_audio_streams);
  ed->video_streams   =
    streams_create(ed, ed->t->video_streams,   ed->t->num_video_streams);
  ed->text_streams    =
    streams_create(ed, ed->t->text_streams,    ed->t->num_text_streams);
  ed->overlay_streams =
    streams_create(ed, ed->t->overlay_streams, ed->t->num_overlay_streams);
  return 1;
  }

static void destroy_edl(void * priv)
  {
  int i;
  edldec_t * ed = priv;
  if(ed->t)
    {
    streams_destroy(ed->audio_streams, ed->t->num_audio_streams);
    streams_destroy(ed->video_streams, ed->t->num_video_streams);
    streams_destroy(ed->text_streams, ed->t->num_text_streams);
    streams_destroy(ed->overlay_streams, ed->t->num_overlay_streams);
    }

  if(ed->sources)
    {
    for(i = 0; i < ed->num_sources; i++)
      source_cleanup(&ed->sources[i]);
    free(ed->sources);
    }

  if(ed->ti)
    {
    for(i = 0; i < ed->edl->num_tracks; i++)
      bg_track_info_free(ed->ti + i);
    free(ed->ti);
    }
  
  free(ed);
  }


static int set_audio_stream_edl(void * priv, int stream,
                                bg_stream_action_t action)
  {
  edldec_t * ed = priv;
  if((action != BG_STREAM_ACTION_OFF) && (action != BG_STREAM_ACTION_DECODE))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid action for audio stream");
    return 0;
    }
  ed->audio_streams[stream].action = action;
  return 1;
  }

static int set_video_stream_edl(void * priv, int stream,
                                bg_stream_action_t action)
  {
  edldec_t * ed = priv;
  if((action != BG_STREAM_ACTION_OFF) && (action != BG_STREAM_ACTION_DECODE))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid action for video stream");
    return 0;
    }
  ed->video_streams[stream].action = action;
  return 1;
  }

static int set_text_stream_edl(void * priv, int stream,
                               bg_stream_action_t action)
  {
  edldec_t * ed = priv;
  ed->text_streams[stream].action = action;
  return 1;
  }

static int set_overlay_stream_edl(void * priv, int stream,
                                  bg_stream_action_t action)
  {
  edldec_t * ed = priv;
  if((action != BG_STREAM_ACTION_OFF) && (action != BG_STREAM_ACTION_DECODE))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid action for overlay stream");
    return 0;
    }
  ed->overlay_streams[stream].action = action;
  return 1;
  }

static gavl_audio_source_t * get_audio_source(edldec_t * ed,
                                              const gavl_edl_segment_t * seg,
                                              source_t ** src,
                                              int64_t src_time)
  {
  if(*src)
    source_unref(*src);
  if(!(*src = get_source(ed, BG_STREAM_AUDIO, seg, src_time)))
    return NULL;
  source_ref(*src);
  return (*src)->plugin->get_audio_source((*src)->h->priv, seg->stream);
  }

static gavl_video_source_t * get_video_source(edldec_t * ed,
                                              const gavl_edl_segment_t * seg,
                                              source_t ** src,
                                              int64_t src_time)
  {
  if(*src)
    source_unref(*src);
  if(!(*src = get_source(ed, BG_STREAM_VIDEO, seg, src_time)))
    return NULL;
  source_ref(*src);
  return (*src)->plugin->get_video_source((*src)->h->priv, seg->stream);
  }

static gavl_packet_source_t * get_text_source(edldec_t * ed,
                                              const gavl_edl_segment_t * seg,
                                              source_t ** src,
                                              int64_t src_time)
  {
  if(*src)
    source_unref(*src);
  if(!(*src = get_source(ed, BG_STREAM_TEXT, seg, src_time)))
    return NULL;
  source_ref(*src);
  return (*src)->plugin->get_text_source((*src)->h->priv, seg->stream);
  }

static gavl_video_source_t * get_overlay_source(edldec_t * ed,
                                                const gavl_edl_segment_t * seg,
                                                source_t ** src,
                                                int64_t src_time)
  {
  if(*src)
    source_unref(*src);

  if(!(*src = get_source(ed, BG_STREAM_OVERLAY, seg, src_time)))
    return NULL;
  source_ref(*src);
  return (*src)->plugin->get_overlay_source((*src)->h->priv, seg->stream);
  }

static gavl_source_status_t read_audio(void * priv,
                                       gavl_audio_frame_t ** frame)
  {
  gavl_source_status_t st;
  stream_t * s = priv;

  /* Early return */
  if(!s->seg && !s->mute_time)
    return GAVL_SOURCE_EOF;
  
  /* Check for segment end */
  if(s->seg && (s->pts >= s->seg->dst_time + s->seg->dst_duration))
    {
    int64_t src_time;

    s->seg = gavl_edl_dst_time_to_src(s->dec->t, s->s, s->pts,
                                      &src_time, &s->mute_time);

    if(!s->seg && !s->mute_time)
      return GAVL_SOURCE_EOF;

    if(s->seg)
      {
      if(!(s->asrc_int = get_audio_source(s->dec, s->seg, &s->src, src_time)))
        return GAVL_SOURCE_EOF;
      gavl_audio_source_set_dst(s->asrc_int, 0, s->afmt);
      }
    else
      s->asrc_int = NULL;
    }
  
  /* Check for mute */
  if(s->mute_time)
    {
    gavl_audio_frame_mute(*frame, s->afmt);
    if((*frame)->valid_samples > s->mute_time)
      (*frame)->valid_samples = s->mute_time;
    s->mute_time -= (*frame)->valid_samples;
    (*frame)->timestamp = s->pts;
    s->pts += (*frame)->valid_samples;
    return GAVL_SOURCE_OK;
    }
  
  /* Read audio */
  st = gavl_audio_source_read_frame(s->asrc_int, frame);
  if(st != GAVL_SOURCE_OK)
    return st;
  
  if(s->pts + (*frame)->valid_samples >
     s->seg->dst_time + s->seg->dst_duration)
    {
    (*frame)->valid_samples =
      s->seg->dst_time + s->seg->dst_duration - s->pts;
    }
  (*frame)->timestamp = s->pts;
  s->pts += (*frame)->valid_samples;
  return GAVL_SOURCE_OK;
  }

static int start_audio_stream(edldec_t * ed, stream_t * s, bg_audio_info_t * ai)
  {
  int64_t src_time;
  if(s->action == BG_STREAM_ACTION_OFF)
    return 1;
  if(!(s->seg = gavl_edl_dst_time_to_src(ed->t, s->s, 0,
                                         &src_time, &s->mute_time)))
    return 0;

  if(!(s->asrc_int = get_audio_source(ed, s->seg, &s->src, src_time)))
    return 0;

  /* Get format */
  gavl_audio_format_copy(&ai->format,
                         gavl_audio_source_get_src_format(s->asrc_int));
  s->afmt = &ai->format;
  
  /* Adjust format */
  ai->format.samplerate = s->s->timescale;
  ai->format.samples_per_frame = 1024;
  
  /* Set destination format */
  gavl_audio_source_set_dst(s->asrc_int, 0, &ai->format);

  /* Create external source */
  s->asrc_ext = gavl_audio_source_create(read_audio, s,
                                         GAVL_SOURCE_SRC_FRAMESIZE_MAX,
                                         &ai->format);
  
  return 1;
  }

static gavl_source_status_t read_video(void * priv,
                                       gavl_video_frame_t ** frame)
  {
  gavl_source_status_t st;
  stream_t * s = priv;

  /* Early return */
  if(!s->seg && !s->mute_time)
    return GAVL_SOURCE_EOF;

  /* Check for segment end */
  if(s->seg && (s->pts >= s->seg->dst_time + s->seg->dst_duration))
    {
    int64_t src_time;

    s->seg = gavl_edl_dst_time_to_src(s->dec->t, s->s, s->pts,
                                      &src_time, &s->mute_time);

    if(!s->seg && !s->mute_time)
      return GAVL_SOURCE_EOF;
    
    if(s->seg)
      {
      if(!(s->vsrc_int = get_video_source(s->dec, s->seg, &s->src, src_time)))
        return GAVL_SOURCE_EOF;
      gavl_video_source_set_dst(s->vsrc_int, 0, s->vfmt);
      }
    else
      s->vsrc_int = NULL;
    }

  /* Check for mute */

  if(s->mute_time > 0)
    {
    gavl_video_frame_clear(*frame, s->vfmt);
    (*frame)->timestamp = s->pts;
    (*frame)->duration = s->mute_time;

    /* TODO: Limit frame duration */
    (*frame)->timestamp = s->pts;
    s->pts += (*frame)->duration;
    s->mute_time -= (*frame)->duration;
    return GAVL_SOURCE_OK;
    }
  
  /* Read video */
  st = gavl_video_source_read_frame(s->vsrc_int, frame);
  if(st != GAVL_SOURCE_OK)
    return st;
  
  if(s->pts + (*frame)->duration >
     s->seg->dst_time + s->seg->dst_duration)
    {
    (*frame)->duration =
      s->seg->dst_time + s->seg->dst_duration - s->pts;
    }
  s->pts += (*frame)->duration;
  return GAVL_SOURCE_OK;
  
  
  }

static gavl_source_status_t read_text(void * priv,
                                      gavl_packet_t ** p)
  {
  gavl_source_status_t st;
  stream_t * s = priv;

  /* Early return */
  if(!s->seg && !s->mute_time)
    return GAVL_SOURCE_EOF;
  }

static int start_video_stream(edldec_t * ed, stream_t * s, bg_video_info_t * vi)
  {
  int64_t src_time;
  if(s->action == BG_STREAM_ACTION_OFF)
    return 1;

  if(!(s->seg = gavl_edl_dst_time_to_src(ed->t, s->s, 0,
                                         &src_time, &s->mute_time)))
    return 0;
  
  if(!(s->vsrc_int = get_video_source(ed, s->seg, &s->src, src_time)))
    return 0;

  /* Get format */
  gavl_video_format_copy(&vi->format,
                         gavl_video_source_get_src_format(s->vsrc_int));
  s->vfmt = &vi->format;

  /* Adjust format */
  vi->format.timescale = s->s->timescale;
  vi->format.frame_duration = 0;
  vi->format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
  
  /* Set destination format */
  gavl_video_source_set_dst(s->vsrc_int, 0, &vi->format);

  /* Create external source */
  s->vsrc_ext = gavl_video_source_create(read_video, s, 0, &vi->format);
  
  return 1;
  }

static int start_text_stream(edldec_t * ed, stream_t * s, bg_text_info_t * ti)
  {
  int64_t src_time;
  if(s->action == BG_STREAM_ACTION_OFF)
    return 1;
  s->seg = gavl_edl_dst_time_to_src(ed->t, s->s, 0, &src_time, &s->mute_time);
  if(!s->seg)
    return 0;
  s->psrc_int = get_text_source(ed, s->seg, &s->src, src_time);
  if(s->psrc_int)
    return 0;

  /* Create external source */
  s->psrc_ext = gavl_packet_source_create_text(read_text, s, 0,
                                               s->s->timescale);

  }

static int start_overlay_stream(edldec_t * ed, stream_t * s,
                                bg_overlay_info_t * oi)
  {
  int64_t src_time;
  if(s->action == BG_STREAM_ACTION_OFF)
    return 1;
  s->seg = gavl_edl_dst_time_to_src(ed->t, s->s, 0, &src_time, &s->mute_time);
  if(!s->seg)
    return 0;
  s->vsrc_int = get_overlay_source(ed, s->seg, &s->src, src_time);
  if(s->vsrc_int)
    return 0;

  /* Get format */
  gavl_video_format_copy(&oi->format,
                         gavl_video_source_get_src_format(s->vsrc_int));
  s->vfmt = &oi->format;
  
  }

static int start_edl(void * priv)
  {
  int i;
  edldec_t * ed = priv;

  for(i = 0; i < ed->t->num_audio_streams; i++)
    {
    if(!start_audio_stream(ed, &ed->audio_streams[i],
                           &ed->ti_cur->audio_streams[i]))
      return 0;
    }
  for(i = 0; i < ed->t->num_video_streams; i++)
    {
    if(!start_video_stream(ed, &ed->video_streams[i],
                           &ed->ti_cur->video_streams[i]))
      return 0;
    }
  for(i = 0; i < ed->t->num_text_streams; i++)
    {
    if(!start_text_stream(ed, &ed->text_streams[i],
                          &ed->ti_cur->text_streams[i]))
      return 0;
    }
  for(i = 0; i < ed->t->num_overlay_streams; i++)
    {
    if(!start_overlay_stream(ed, &ed->overlay_streams[i],
                             &ed->ti_cur->overlay_streams[i]))
      return 0;
    }
  return 1;
  }

static gavl_audio_source_t * get_audio_source_edl(void * priv, int stream)
  {
  edldec_t * ed = priv;
  return ed->audio_streams[stream].asrc_ext;
  }

static gavl_video_source_t * get_video_source_edl(void * priv, int stream)
  {
  edldec_t * ed = priv;
  return ed->video_streams[stream].vsrc_ext;
  }

static gavl_packet_source_t * get_text_source_edl(void * priv, int stream)
  {
  edldec_t * ed = priv;
  return ed->text_streams[stream].psrc_ext;
  }

static gavl_video_source_t * get_overlay_source_edl(void * priv, int stream)
  {
  edldec_t * ed = priv;
  return ed->overlay_streams[stream].vsrc_ext;
  }

static void seek_edl(void * priv, int64_t * time, int scale)
  {
  int i;
  int64_t src_time;
  int time_scaled;
  stream_t * s;
  edldec_t * ed = priv;
  
  for(i = 0; i < ed->t->num_audio_streams; i++)
    {
    s = ed->audio_streams + i;
    time_scaled = gavl_time_rescale(scale, s->s->timescale, *time);

    s->seg = gavl_edl_dst_time_to_src(ed->t, s->s, time_scaled,
                                      &src_time, &s->mute_time);
    
    if(s->seg)
      {
      if((s->asrc_int = get_audio_source(s->dec, s->seg, &s->src, src_time)))
        gavl_audio_source_set_dst(s->asrc_int, 0, s->afmt);
      }
    else
      s->asrc_int = NULL;
    s->pts = time_scaled;
    }
  for(i = 0; i < ed->t->num_video_streams; i++)
    {
    s = ed->video_streams + i;
    time_scaled = gavl_time_rescale(scale, s->s->timescale, *time);

    s->seg = gavl_edl_dst_time_to_src(ed->t, s->s, time_scaled,
                                      &src_time, &s->mute_time);
    
    if(s->seg)
      {
      if((s->vsrc_int = get_video_source(s->dec, s->seg, &s->src, src_time)))
        gavl_video_source_set_dst(s->vsrc_int, 0, s->vfmt);
      }
    else
      s->vsrc_int = NULL;
    s->pts = time_scaled;
    }
  for(i = 0; i < ed->t->num_text_streams; i++)
    {
    s = ed->text_streams + i;
    time_scaled = gavl_time_rescale(scale, s->s->timescale, *time);

    s->seg = gavl_edl_dst_time_to_src(ed->t, s->s, time_scaled,
                                      &src_time, &s->mute_time);
    
    if(s->seg)
      s->psrc_int = get_text_source(s->dec, s->seg, &s->src, src_time);
    else
      s->psrc_int = NULL;
    s->pts = time_scaled;
    
    }
  for(i = 0; i < ed->t->num_overlay_streams; i++)
    {
    s = ed->overlay_streams + i;
    time_scaled = gavl_time_rescale(scale, s->s->timescale, *time);

    s->seg = gavl_edl_dst_time_to_src(ed->t, s->s, time_scaled,
                                      &src_time, &s->mute_time);
    
    if(s->seg)
      {
      if((s->vsrc_int = get_overlay_source(s->dec, s->seg, &s->src, src_time)))
        gavl_video_source_set_dst(s->vsrc_int, 0, s->vfmt);
      }
    else
      s->vsrc_int = NULL;
    s->pts = time_scaled;
    }
  
  }

static void close_edl(void * priv)
  {

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
  int i;
  int max_streams = 0;
  bg_plugin_handle_t * ret;
  edldec_t * priv;

  if(*ret1)
    bg_plugin_unref(*ret1);
  
  ret = calloc(1, sizeof(*ret));
  *ret1 = ret;
    
  ret->plugin = (bg_plugin_common_t*)(&edl_plugin);
  ret->info = info;
  
  pthread_mutex_init(&ret->mutex, NULL);

  priv = calloc(1, sizeof(*priv));
  ret->priv = priv;
  ret->refcount = 1;
  priv->edl = edl;
  priv->plugin_reg = reg;
  
  /* Create track info */
  
  priv->ti = calloc(priv->edl->num_tracks, sizeof(*priv->ti));
  
  for(i = 0; i < priv->edl->num_tracks; i++)
    {
    gavl_edl_track_t * track;
    bg_track_info_t * ti;
    int total_streams;

    track = priv->edl->tracks + i;
    ti = priv->ti + i;

    gavl_metadata_copy(&ti->metadata, &track->metadata);

    ti->num_audio_streams   = track->num_audio_streams;
    ti->num_video_streams   = track->num_video_streams;
    ti->num_text_streams    = track->num_text_streams;
    ti->num_overlay_streams = track->num_overlay_streams;
    
    ti->audio_streams = calloc(ti->num_audio_streams, sizeof(*ti->audio_streams));
    ti->video_streams = calloc(ti->num_video_streams, sizeof(*ti->video_streams));
    ti->text_streams  = calloc(ti->num_text_streams,  sizeof(*ti->text_streams));
    ti->overlay_streams = calloc(ti->num_overlay_streams, sizeof(*ti->overlay_streams));

    total_streams = 
      ti->num_audio_streams +
      ti->num_video_streams +
      ti->num_text_streams +
      ti->num_overlay_streams;

    if(total_streams > max_streams)
      max_streams = total_streams;

    gavl_metadata_set_long(&ti->metadata, GAVL_META_APPROX_DURATION,
                           gavl_edl_track_get_duration(track));

    ti->flags |= (BG_TRACK_SEEKABLE|BG_TRACK_PAUSABLE);
    }

  priv->num_sources = max_streams * 2; // We can keep twice as many files open than we have streams
  priv->sources = calloc(priv->num_sources, sizeof(*priv->sources));
  return 1;
  }

