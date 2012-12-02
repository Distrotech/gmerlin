// #include <stdlib.h>

#include <gavfprivate.h>

/*
 * Audio source
 */

static gavl_source_status_t
read_audio_func(void * priv, gavl_audio_frame_t ** frame)
  {
  gavl_packet_t * p = NULL;
  gavl_source_status_t st;
  gavf_stream_t * s = priv;
  
  if((st = gavl_packet_source_read_packet(s->psrc, &p)) != GAVL_SOURCE_OK)
    return st;

  if(!s->aframe)
    s->aframe = gavl_audio_frame_create(NULL);

  gavf_packet_to_audio_frame(p, s->aframe, &s->h->format.audio);
  *frame = s->aframe;
  return GAVL_SOURCE_OK;
  }

gavl_audio_source_t *
gavf_get_audio_source(gavf_t * g, int stream)
  {
  gavf_stream_t * s;
  if(g->wr)
    return NULL;
  s = g->streams + stream;
  if(s->h->ci.id != GAVL_CODEC_ID_NONE)
    return NULL;

  if(!s->asrc)
    s->asrc = gavl_audio_source_create(read_audio_func, s,
                                       GAVL_SOURCE_SRC_ALLOC,
                                       &s->h->format.audio);
  
  return s->asrc;
  }

/*
 * Video source
 */

static gavl_source_status_t
read_video_func(void * priv, gavl_video_frame_t ** frame)
  {
  gavl_packet_t * p = NULL;
  gavl_source_status_t st;
  gavf_stream_t * s = priv;

  if((st = gavl_packet_source_read_packet(s->psrc, &p)) != GAVL_SOURCE_OK)
    return st;

  if(!s->vframe)
    s->vframe = gavl_video_frame_create(NULL);
  
  gavf_packet_to_video_frame(p, s->vframe, &s->h->format.video);
  *frame = s->vframe;
  return GAVL_SOURCE_OK;
  }

gavl_video_source_t *
gavf_get_video_source(gavf_t * g, int stream)
  {
  gavf_stream_t * s;
  if(g->wr)
    return NULL;
  s = g->streams + stream;
  if(s->h->ci.id != GAVL_CODEC_ID_NONE)
    return NULL;

  if(!s->vsrc)
    s->vsrc = gavl_video_source_create(read_video_func, s,
                                       GAVL_SOURCE_SRC_ALLOC,
                                       &s->h->format.video);
  return s->vsrc;
  }

/*
 * Audio sink
 */

static gavl_audio_frame_t *
get_audio_func(void * priv)
  {
  gavf_stream_t * s = priv;

  s->p = gavl_packet_sink_get_packet(s->psink);
  
  gavl_packet_reset(s->p);
  gavl_packet_alloc(s->p, s->h->ci.max_packet_size);

  if(!s->aframe)
    s->aframe = gavl_audio_frame_create(NULL);
  
  s->aframe->valid_samples = s->h->format.audio.samples_per_frame;
  
  gavl_audio_frame_set_channels(s->aframe,
                                &s->h->format.audio, s->p->data);
  s->aframe->valid_samples = 0;
  return s->aframe;
  }

static gavl_sink_status_t
put_audio_func(void * priv, gavl_audio_frame_t * frame)
  {
  gavl_sink_status_t st;
  gavf_stream_t * s = priv;
  gavl_audio_frame_t * tmp_frame;
  gavl_packet_t tmp_packet;
  
  if(frame->valid_samples == s->h->format.audio.samples_per_frame)
    {
    gavf_audio_frame_to_packet_metadata(s->aframe, s->p);
    s->p->data_len = s->h->ci.max_packet_size;
    st = gavl_packet_sink_put_packet(s->psink, s->p);
    s->p = NULL;
    return st;
    }
 
  /* Incomplete (hopefully last) frame */
  gavl_packet_init(&tmp_packet);
  gavl_packet_alloc(&tmp_packet, frame->valid_samples * s->block_align);
  tmp_frame = gavl_audio_frame_create(NULL);
  tmp_frame->valid_samples = frame->valid_samples;

  gavl_audio_frame_set_channels(tmp_frame, &s->h->format.audio,
                                tmp_packet.data);
    
  gavl_audio_frame_copy(&s->h->format.audio,
                        tmp_frame,
                        frame,
                        0,
                        0,
                        frame->valid_samples,
                        tmp_frame->valid_samples);

  gavf_audio_frame_to_packet_metadata(frame, &tmp_packet);
  tmp_packet.data_len = frame->valid_samples * s->block_align;

  st = gavl_packet_sink_put_packet(s->psink, &tmp_packet);
  
  gavl_audio_frame_null(tmp_frame);
  gavl_audio_frame_destroy(tmp_frame);
  gavl_packet_free(&tmp_packet);
  
  return st;
  }

gavl_audio_sink_t *
gavf_get_audio_sink(gavf_t * g, int stream)
  {
  gavf_stream_t * s;
  if(!g->wr)
    return NULL;
  s = g->streams + stream;
  if(s->h->ci.id != GAVL_CODEC_ID_NONE)
    return NULL;

  if(!s->asink)
    s->asink = gavl_audio_sink_create(get_audio_func,
                                      put_audio_func, s,
                                      &s->h->format.audio);
  
  return s->asink;
  }

/*
 * Video sink
 */

static gavl_video_frame_t *
get_video_func(void * priv)
  {
  gavf_stream_t * s = priv;

  s->p = gavl_packet_sink_get_packet(s->psink);
  
  gavl_packet_reset(s->p);
  gavl_packet_alloc(s->p, s->h->ci.max_packet_size);

  if(!s->vframe)
    s->vframe = gavl_video_frame_create(NULL);
  s->vframe->strides[0] = 0;
  gavl_video_frame_set_planes(s->vframe,
                              &s->h->format.video, s->p->data);
  return s->vframe;
  }

static gavl_sink_status_t
put_video_func(void * priv, gavl_video_frame_t * frame)
  {
  gavl_sink_status_t st;
  gavf_stream_t * s = priv;
  gavf_video_frame_to_packet_metadata(frame, s->p);
  s->p->data_len = s->h->ci.max_packet_size;
  st = gavl_packet_sink_put_packet(s->psink, s->p);
  s->p = NULL;
  return st;
  }

gavl_video_sink_t *
gavf_get_video_sink(gavf_t * g, int stream)
  {
  gavf_stream_t * s;
  if(!g->wr)
    return NULL;
  s = g->streams + stream;
  if(s->h->ci.id != GAVL_CODEC_ID_NONE)
    return NULL;

  if(!s->vsink)
    s->vsink = gavl_video_sink_create(get_video_func,
                                      put_video_func, s,
                                      &s->h->format.video);

  return s->vsink;
  }
