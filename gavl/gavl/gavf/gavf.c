#include <stdlib.h>
#include <string.h>

#include <gavfprivate.h>

typedef enum
  {
    ENC_STARTING = 0,
    ENC_SYNCHRONOUS,
    ENC_INTERLEAVE
  } encoding_mode_t;

struct gavf_s
  {
  gavf_io_t * io;
  gavf_program_header_t ph;
  gavf_file_index_t     fi;
  gavf_sync_index_t     si;
  gavf_packet_index_t   pi;

  gavl_chapter_list_t * cl;
  
  gavf_packet_header_t  pkthdr;
  int have_pkt_header;
  
  gavf_stream_t * streams;

  int64_t * sync_pts;
  
  gavf_options_t opt;
  
  gavf_io_t pkt_io;
  gavf_buffer_t pkt_buf;
  
  uint64_t sync_pos;
  
  int wr;
  
  gavl_packet_t write_pkt;
  gavl_video_frame_t * write_vframe;
  gavl_audio_frame_t * write_aframe;

  /* Time of the last sync header */
  gavl_time_t last_sync_time;
  gavl_time_t sync_distance;

  encoding_mode_t encoding_mode;
  encoding_mode_t final_encoding_mode;

  
  };

gavf_options_t * gavf_get_options(gavf_t * g)
  {
  return &g->opt;
  }

/* Extensions */

int gavf_extension_header_read(gavf_io_t * io, gavl_extension_header_t * eh)
  {
  if(!gavf_io_read_uint32v(io, &eh->key) ||
     !gavf_io_read_uint32v(io, &eh->len))
    return 0;
  return 1;
  }

int gavf_extension_write(gavf_io_t * io, uint32_t key, uint32_t len,
                         uint8_t * data)
  {
  if(!gavf_io_write_uint32v(io, key) ||
     !gavf_io_write_uint32v(io, len) ||
     (gavf_io_write_data(io, data, len) < len))
    return 0;
  return 1;
  }

/* Connector callbacks */

static gavl_source_status_t
read_audio_func(void * priv, gavl_audio_frame_t ** frame)
  {
  gavl_packet_t * pp;
  gavl_source_status_t st;
  gavf_stream_t * s = priv;
  
  pp = &s->p;
  
  if((st = gavl_packet_source_read_packet(s->psrc, &pp)) != GAVL_SOURCE_OK)
    return st;

  if(!s->aframe)
    s->aframe = gavl_audio_frame_create(NULL);

  gavf_packet_to_audio_frame(pp, s->aframe, &s->h->format.audio);
  *frame = s->aframe;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_video_func(void * priv, gavl_video_frame_t ** frame)
  {
  gavl_packet_t * pp;
  gavl_source_status_t st;
  gavf_stream_t * s = priv;
  pp = &s->p;

  if((st = gavl_packet_source_read_packet(s->psrc, &pp)) != GAVL_SOURCE_OK)
    return st;

  if(!s->vframe)
    s->vframe = gavl_video_frame_create(NULL);
  
  gavf_packet_to_video_frame(pp, s->vframe, &s->h->format.video);
  *frame = s->vframe;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t
read_packet_func(void * priv, gavl_packet_t ** p)
  {
  gavf_stream_t * s = priv;

  if(!s->g->have_pkt_header && !gavf_packet_read_header(s->g))
    return GAVL_SOURCE_EOF;

  if(s->g->pkthdr.stream_id != s->h->id)
    return GAVL_SOURCE_AGAIN;
  
  s->g->have_pkt_header = 0;
  
  gavf_buffer_reset(&s->g->pkt_buf);
  
  if(!gavf_io_read_buffer(s->g->io, &s->g->pkt_buf) ||
     !gavf_read_gavl_packet(&s->g->pkt_io, s, *p, s->g->pkt_buf.len))
    return GAVL_SOURCE_EOF;
  return GAVL_SOURCE_OK;
  }

static gavl_audio_frame_t *
get_audio_frame_func(void * priv)
  {
  gavf_stream_t * s = priv;
  gavl_packet_reset(&s->p);
  gavl_packet_alloc(&s->p, s->h->ci.max_packet_size);
  if(!s->aframe)
    s->aframe = gavl_audio_frame_create(NULL);

  s->aframe->valid_samples = s->h->format.audio.samples_per_frame;
  
  gavl_audio_frame_set_channels(s->aframe,
                                &s->h->format.audio, s->p.data);
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
    gavf_audio_frame_to_packet_metadata(s->aframe, &s->p);
    s->p.data_len = s->h->ci.max_packet_size;
    return gavl_packet_sink_put_packet(s->psink, &s->p);
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

static gavl_video_frame_t *
get_video_frame_func(void * priv)
  {
  gavf_stream_t * s = priv;
  gavl_packet_reset(&s->p);
  gavl_packet_alloc(&s->p, s->h->ci.max_packet_size);

  if(!s->vframe)
    s->vframe = gavl_video_frame_create(NULL);
  s->vframe->strides[0] = 0;
  gavl_video_frame_set_planes(s->vframe,
                              &s->h->format.video, s->p.data);
  return s->vframe;
  }

static gavl_sink_status_t
put_video_func(void * priv, gavl_video_frame_t * frame)
  {
  gavf_stream_t * s = priv;
  gavf_video_frame_to_packet_metadata(frame, &s->p);
  s->p.data_len = s->h->ci.max_packet_size;
  return gavl_packet_sink_put_packet(s->psink, &s->p);
  }

static gavl_sink_status_t
put_packet_func(void * priv, gavl_packet_t * p)
  {
  gavf_stream_t * s = priv;
  return gavf_write_packet(s->g, (int)(s - s->g->streams), p) ?
    GAVL_SINK_OK : GAVL_SINK_ERROR;
  }

/* Streams */

static void gavf_stream_init_audio(gavf_t * g, gavf_stream_t * s)
  {
  int sample_size;
  s->timescale = s->h->format.audio.samplerate;

  /* Figure out the samples per frame */
  if(gavl_compression_constant_frame_samples(s->h->ci.id))
    s->packet_duration = s->h->format.audio.samples_per_frame;
  else if(s->h->ci.id == GAVL_CODEC_ID_NONE)
    s->block_align =
      gavl_bytes_per_sample(s->h->format.audio.sample_format) *
      s->h->format.audio.num_channels;
  else if((sample_size = gavl_compression_get_sample_size(s->h->ci.id)))
    s->block_align =
      sample_size * s->h->format.audio.num_channels;
  else
    s->flags |= STREAM_FLAG_HAS_DURATION;

  if(s->h->ci.id == GAVL_CODEC_ID_NONE)
    s->h->ci.max_packet_size =
      s->block_align *
      s->h->format.audio.samples_per_frame;
  
  if(g->wr)
    {
    /* Create audio sink */
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      s->asink = gavl_audio_sink_create(get_audio_frame_func,
                                        put_audio_func, s,
                                        &s->h->format.audio);
    /* Create packet sink */
    s->psink = gavl_packet_sink_create(NULL, put_packet_func, s);
    }
  else
    {
    /* Create audio source */
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      s->asrc = gavl_audio_source_create(read_audio_func, s,
                                         GAVL_SOURCE_SRC_ALLOC,
                                         &s->h->format.audio);
    /* Create packet source */
    s->psrc = gavl_packet_source_create(read_packet_func, s, 0,
                                        &s->h->ci, &s->h->format.audio,
                                        NULL);
    }
  }

static void gavf_stream_init_video(gavf_t * g, gavf_stream_t * s)
  {
  s->timescale = s->h->format.video.timescale;

  if(s->h->ci.flags & GAVL_COMPRESSION_HAS_B_FRAMES)
    s->flags |= STREAM_FLAG_HAS_PTS;
  
  if(s->h->ci.flags & GAVL_COMPRESSION_HAS_P_FRAMES)
    g->sync_distance = 0;
  
  if(s->h->format.video.framerate_mode == GAVL_FRAMERATE_CONSTANT)
    s->packet_duration = s->h->format.video.frame_duration;
  else
    s->flags |= STREAM_FLAG_HAS_DURATION;
  
  if(((s->h->format.video.interlace_mode == GAVL_INTERLACE_MIXED) ||
      (s->h->format.video.interlace_mode == GAVL_INTERLACE_MIXED_TOP) ||
      (s->h->format.video.interlace_mode == GAVL_INTERLACE_MIXED_BOTTOM)) &&
     (s->h->ci.id == GAVL_CODEC_ID_NONE))
    s->flags |= STREAM_FLAG_HAS_INTERLACE;
  
  if(s->h->ci.id == GAVL_CODEC_ID_NONE)
    s->h->ci.max_packet_size =
      gavl_video_format_get_image_size(&s->h->format.video);


  if(g->wr)
    {
    /* Create video sink */
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      s->vsink = gavl_video_sink_create(get_video_frame_func,
                                        put_video_func, s,
                                        &s->h->format.video);
    /* Create packet sink */
    s->psink = gavl_packet_sink_create(NULL, put_packet_func, s);
    }
  else
    {
    /* Create video source */
    if(s->h->ci.id == GAVL_CODEC_ID_NONE)
      s->vsrc = gavl_video_source_create(read_video_func, s,
                                         GAVL_SOURCE_SRC_ALLOC,
                                         &s->h->format.video);
    /* Create packet source */
    s->psrc = gavl_packet_source_create(read_packet_func, s, 0,
                                        &s->h->ci, NULL,
                                        &s->h->format.video);
    }
  }

static void gavf_stream_init_text(gavf_t * g, gavf_stream_t * s)
  {
  s->timescale = s->h->format.text.timescale;
  s->flags |=
    (STREAM_FLAG_HAS_PTS |
     STREAM_FLAG_HAS_DURATION |
     STREAM_FLAG_DISCONTINUOUS);

  if(g->wr)
    {
    /* Create packet sink */
    s->psink = gavl_packet_sink_create(NULL, put_packet_func, s);
    }
  else
    {
    /* Create packet source */
    s->psrc = gavl_packet_source_create(read_packet_func, s, 0,
                                        NULL, NULL, NULL);
    }
  }

static void init_streams(gavf_t * g)
  {
  int i;
  
  gavf_sync_index_init(&g->si, g->ph.num_streams);
  
  g->streams = calloc(g->ph.num_streams, sizeof(*g->streams));
  g->sync_pts = calloc(g->ph.num_streams, sizeof(*g->sync_pts));

  for(i = 0; i < g->ph.num_streams; i++)
    {
    g->sync_pts[i] = GAVL_TIME_UNDEFINED;

    g->streams[i].next_sync_pts = GAVL_TIME_UNDEFINED;
    g->streams[i].last_sync_pts = GAVL_TIME_UNDEFINED;
    
    g->streams[i].h = &g->ph.streams[i];
    g->streams[i].g = g;
    
    switch(g->streams[i].h->type)
      {
      case GAVF_STREAM_AUDIO:
        gavf_stream_init_audio(g, &g->streams[i]);
        break;
      case GAVF_STREAM_VIDEO:
        gavf_stream_init_video(g, &g->streams[i]);
        break;
      case GAVF_STREAM_TEXT:
        gavf_stream_init_text(g, &g->streams[i]);
        break;
      }
    if(g->wr)
      {
      g->streams[i].pb =
        gavf_packet_buffer_create(g->streams[i].timescale);

      }
    }
  }

static void gavf_stream_free(gavf_stream_t * s)
  {
  if(s->asrc)
    gavl_audio_source_destroy(s->asrc);
  if(s->vsrc)
    gavl_video_source_destroy(s->vsrc);
  if(s->psrc)
    gavl_packet_source_destroy(s->psrc);

  if(s->asink)
    gavl_audio_sink_destroy(s->asink);
  if(s->vsink)
    gavl_video_sink_destroy(s->vsink);
  if(s->psink)
    gavl_packet_sink_destroy(s->psink);

  if(s->aframe)
    {
    gavl_audio_frame_null(s->aframe);
    gavl_audio_frame_destroy(s->aframe);
    }
  if(s->vframe)
    {
    gavl_video_frame_null(s->vframe);
    gavl_video_frame_destroy(s->vframe);
    }
  gavl_packet_free(&s->p);
  }


gavf_t * gavf_create()
  {
  gavf_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

/* Read support */

static int handle_chunk(gavf_t * g, char * sig)
  {
  if(!strncmp(sig, GAVF_TAG_PROGRAM_HEADER, 8))
    {
    if(!gavf_program_header_read(g->io, &g->ph))
      return 0;

    if(g->opt.flags & GAVF_OPT_FLAG_DUMP_HEADERS)
      gavf_program_header_dump(&g->ph);
    
    init_streams(g);
    }
  else if(!strncmp(sig, GAVF_TAG_SYNC_HEADER, 8))
    {
    g->sync_pos = g->io->position;
    }  
  else if(!strncmp(sig, GAVF_TAG_SYNC_INDEX, 8))
    {
    if(gavf_sync_index_read(g->io, &g->si))
      g->opt.flags |= GAVF_OPT_FLAG_SYNC_INDEX;

    if(g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES)
      gavf_sync_index_dump(&g->si);
    }
  else if(!strncmp(sig, GAVF_TAG_PACKET_INDEX, 8))
    {
    if(gavf_packet_index_read(g->io, &g->pi))
      g->opt.flags |= GAVF_OPT_FLAG_PACKET_INDEX;
    if(g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES)
      gavf_packet_index_dump(&g->pi);
    }
  else if(!strncmp(sig, GAVF_TAG_CHAPTER_LIST, 8))
    {
    if(!(g->cl = gavf_read_chapter_list(g->io)))
      return 0;
    if(g->opt.flags & GAVF_OPT_FLAG_DUMP_HEADERS)
      gavl_chapter_list_dump(g->cl);
    }
  return 1;
  }

static int read_sync_header(gavf_t * g)
  {
  int i;
  /* Read sync header */

  fprintf(stderr, "Read sync header\n");
  
  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(!gavf_io_read_int64v(g->io, &g->sync_pts[i]))
      return 0;

    fprintf(stderr, "PTS[%d]: %ld\n", i, g->sync_pts[i]);
    
    if(g->sync_pts[i] != GAVL_TIME_UNDEFINED)
      {
      g->streams[i].last_sync_pts = g->sync_pts[i];
      g->streams[i].next_pts = g->sync_pts[i];
      }
    }
  return 1;
  }
  
int gavf_open_read(gavf_t * g, gavf_io_t * io)
  {
  int i;
  char sig[8];
  
  g->io = io;

  g->opt.flags &= ~(GAVF_OPT_FLAG_SYNC_INDEX|
                    GAVF_OPT_FLAG_PACKET_INDEX);
  
  /* Initialize packet buffer */
  gavf_io_init_buf_read(&g->pkt_io, &g->pkt_buf);

  /* Read up to the first sync header */

  while(1)
    {
    if(gavf_io_read_data(g->io, (uint8_t*)sig, 8) < 8)
      return 0;

    if(!strncmp(sig, GAVF_TAG_FILE_INDEX, 8))
      {
      if(!gavf_file_index_read(g->io, &g->fi))
        return 0;

      if(g->opt.flags & GAVF_OPT_FLAG_DUMP_HEADERS)
        gavf_file_index_dump(&g->fi);
      
      if(g->io->seek_func)
        {
        for(i = 0; i < g->fi.num_entries; i++)
          {
          gavf_io_seek(io, g->fi.entries[i].position, SEEK_SET);

          if(gavf_io_read_data(g->io, (uint8_t*)sig, 8) < 8)
            return 0;

          if(strncmp(sig, (char*)g->fi.entries[i].tag, 8))
            return 0;

          if(!handle_chunk(g, sig))
            return 0;
          }
        }
      }
    else
      {
      if(!handle_chunk(g, sig))
        return 0;
      }
    if(g->sync_pos > 0)
      break;
    }

  gavf_reset(g);
  
  return 1;
  }

int gavf_reset(gavf_t * g)
  {
  if(g->sync_pos != g->io->position)
    {
    if(g->io->seek_func)
      gavf_io_seek(g->io, g->sync_pos, SEEK_SET);
    else
      return 0;
    }
  
  g->have_pkt_header = 0;
  
  if(!read_sync_header(g))
    return 0;
  return 1;
  }

gavf_program_header_t * gavf_get_program_header(gavf_t * g)
  {
  return &g->ph;
  }

const gavl_chapter_list_t * gavf_get_chapter_list(gavf_t * g)
  {
  return g->cl;
  }


const gavf_packet_header_t * gavf_packet_read_header(gavf_t * g)
  {
  char c[8];

  while(1)
    {
    if(!gavf_io_read_data(g->io, (uint8_t*)c, 1))
      return NULL;
    
    if(c[0] == GAVF_TAG_PACKET_HEADER_C)
      {
      /* Got new packet */
      if(!gavf_io_read_uint32v(g->io, &g->pkthdr.stream_id))
        return 0;
      g->have_pkt_header = 1;
      return &g->pkthdr;
      }
    else
      {
      if(gavf_io_read_data(g->io, (uint8_t*)&c[1], 7) < 7)
        return 0;

      if(!strncmp(c, GAVF_TAG_SYNC_HEADER, 8))
        {
        if(!read_sync_header(g))
          return 0;
        }
      else
        return 0;
      }
    
    }
  return NULL;
  }

void gavf_packet_skip(gavf_t * g)
  {
  uint32_t len;
  if(!gavf_io_read_uint32v(g->io, &len))
    return;
  gavf_io_skip(g->io, len);
  }

int gavf_packet_read_packet(gavf_t * g, gavl_packet_t * p)
  {
  int i;
  gavf_stream_t * s = NULL;

  g->have_pkt_header = 0;
  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(g->pkthdr.stream_id == g->streams[i].h->id)
      {
      s = &g->streams[i];
      break;
      }
    }

  if(!s)
    return 0;
  
  gavf_buffer_reset(&g->pkt_buf);
  
  if(!gavf_io_read_buffer(g->io, &g->pkt_buf) ||
     !gavf_read_gavl_packet(&g->pkt_io, s, p, g->pkt_buf.len))
    return 0;
  return 1;
  }

const int64_t * gavf_first_pts(gavf_t * gavf)
  {
  if(gavf->opt.flags & GAVF_OPT_FLAG_SYNC_INDEX)
    return gavf->si.entries[0].pts;
  else
    return NULL;
  }

/* Get the last PTS of the streams */

const int64_t * gavf_end_pts(gavf_t * gavf)
  {
  if(gavf->opt.flags & GAVF_OPT_FLAG_SYNC_INDEX)
    return gavf->si.entries[gavf->si.num_entries - 1].pts;
  else
    return NULL;
  }

/* Seek to a specific time. Return the sync timestamps of
   all streams at the current position */

const int64_t * gavf_seek(gavf_t * g, int64_t time, int scale)
  {
  int stream = 0;
  int64_t index_position;
  int64_t time_scaled;
  int done = 0;
  
  if(!(g->opt.flags & GAVF_OPT_FLAG_SYNC_INDEX))
    return NULL;

  index_position = g->si.num_entries - 1;
  while(!done)
    {
    /* Find next continuous stream */
    while(g->streams[stream].flags & STREAM_FLAG_DISCONTINUOUS)
      {
      stream++;
      if(stream >= g->ph.num_streams)
        {
        done = 1;
        break;
        }
      }

    if(done)
      break;
    
    time_scaled = gavl_time_rescale(scale, g->streams[stream].timescale, time);
    
    /* Descrease index pointer until we are before this time */
    
    while(g->si.entries[index_position].pts[stream] > time_scaled)
      {
      if(!index_position)
        {
        done = 1;
        break;
        }
      index_position--;
      }
    stream++;
    }

  /* Seek to the positon */
  gavf_io_seek(g->io, g->si.entries[index_position].pos, SEEK_SET);
  return g->si.entries[index_position].pts;
  }


/* Write support */

int gavf_open_write(gavf_t * g, gavf_io_t * io,
                    const gavl_metadata_t * m,
                    const gavl_chapter_list_t * cl)
  {
  g->io = io;
  g->wr = 1;
  /* Initialize packet buffer */
  gavf_io_init_buf_write(&g->pkt_io, &g->pkt_buf);

  if(m)
    gavl_metadata_copy(&g->ph.m, m);

  if(cl)
    g->cl = gavl_chapter_list_copy(cl);
  
  if(g->io->seek_func)
    {
    gavf_file_index_init(&g->fi, 8);
    gavf_file_index_write(g->io, &g->fi);
    }
  return 1;
  }

int gavf_add_audio_stream(gavf_t * g,
                          const gavl_compression_info_t * ci,
                          const gavl_audio_format_t * format,
                          const gavl_metadata_t * m)
  {
  return gavf_program_header_add_audio_stream(&g->ph, ci, format, m);
  }

int gavf_add_video_stream(gavf_t * g,
                          const gavl_compression_info_t * ci,
                          const gavl_video_format_t * format,
                          const gavl_metadata_t * m)
  {
  return gavf_program_header_add_video_stream(&g->ph, ci, format, m);
  }

int gavf_add_text_stream(gavf_t * g,
                         uint32_t timescale,
                         const gavl_metadata_t * m)
  {
  return gavf_program_header_add_text_stream(&g->ph, timescale, m);
  }

int gavf_start(gavf_t * g)
  {
  if(!g->wr || g->streams)
    return 1;
  
  g->sync_distance = g->opt.sync_distance;
  
  init_streams(g);

  if(g->ph.num_streams == 1)
    {
    g->encoding_mode = ENC_SYNCHRONOUS;
    g->final_encoding_mode = ENC_SYNCHRONOUS;
    }
  else
    {
    g->encoding_mode = ENC_STARTING;
    
    if(g->opt.flags & GAVF_OPT_FLAG_INTERLEAVE)
      g->final_encoding_mode = ENC_INTERLEAVE;
    else
      g->final_encoding_mode = ENC_SYNCHRONOUS;
    }
  
  gavf_file_index_add(&g->fi, GAVF_TAG_PROGRAM_HEADER, g->io->position);

  if(g->opt.flags & GAVF_OPT_FLAG_DUMP_HEADERS)
    gavf_program_header_dump(&g->ph);
  
  if(!gavf_program_header_write(g->io, &g->ph))
    return 0;
  
  gavf_io_flush(g->io);
  
  return 1;
  }

static int write_sync_header(gavf_t * g, int stream, const gavl_packet_t * p)
  {
  int i;
  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(i == stream)
      g->sync_pts[i] = p->pts;
    else if((stream >= 0) &&
            (g->ph.streams[i].ci.flags & GAVL_COMPRESSION_HAS_B_FRAMES))
      g->sync_pts[i] = GAVL_TIME_UNDEFINED;

    else if((g->streams[i].flags & STREAM_FLAG_DISCONTINUOUS) &&
            (g->streams[i].next_sync_pts == GAVL_TIME_UNDEFINED))
      g->sync_pts[i] = 0;
    else
      g->sync_pts[i] = g->streams[i].next_sync_pts;
    
    }

  /* Update sync index */
  if(g->opt.flags & GAVF_OPT_FLAG_SYNC_INDEX)
    {
    gavf_sync_index_add(&g->si, g->io->position,
                        g->sync_pts);
    }

  /* If that's the first sync header, update file index */

  if(!g->sync_pos)
    {
    gavf_file_index_add(&g->fi, GAVF_TAG_SYNC_HEADER, g->io->position);
    g->sync_pos = g->io->position;
    }
  /* Write the sync header */
  if(gavf_io_write_data(g->io, (uint8_t*)GAVF_TAG_SYNC_HEADER, 8) < 8)
    return 0;

  fprintf(stderr, "Write sync header\n");
  
  for(i = 0; i < g->ph.num_streams; i++)
    {
    fprintf(stderr, "PTS[%d]: %ld\n", i, g->sync_pts[i]);
    if(!gavf_io_write_int64v(g->io, g->sync_pts[i]))
      return 0;
    }

  /* Update last sync pts */

  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(g->sync_pts[i] != GAVL_TIME_UNDEFINED)
      g->streams[i].last_sync_pts = g->sync_pts[i];
    }
  gavf_io_flush(g->io);
  return 1;
  }

static int write_packet(gavf_t * g, int stream,
                        const gavl_packet_t * p)
  {
  int write_sync = 0;
  gavf_stream_t * s = &g->streams[stream];
  
  /* Decide whether to write a sync header */
  if(!g->sync_pos)
    write_sync = 1;
  else if(g->sync_distance)
    {
    if(gavl_time_unscale(s->timescale, p->pts) - g->last_sync_time >
       g->sync_distance)
      write_sync = 1;
    }
  else
    {
    if((g->ph.streams[stream].type == GAVF_STREAM_VIDEO) &&
       (g->ph.streams[stream].ci.flags & GAVL_COMPRESSION_HAS_P_FRAMES) &&
       (p->flags & GAVL_PACKET_KEYFRAME))
      write_sync = 1;
    }

  if(write_sync)
    {
    if(!write_sync_header(g, stream, p))
      return 0;
    }
  
  // If a stream has B-frames, this won't be correct
  // for the next sync timestamp (it will be taken from the
  // packet pts in write_sync_header)
  // It will, however, be correct to get the duration
  
  if(s->next_sync_pts < p->pts + p->duration)
    s->next_sync_pts = p->pts + p->duration;

  /* Update packet index */
  
  if(g->opt.flags & GAVF_OPT_FLAG_PACKET_INDEX)
    {
    gavf_packet_index_add(&g->pi,
                          s->h->id, p->flags, g->io->position,
                          p->pts);
    }
  
  if((gavf_io_write_data(g->io,
                         (const uint8_t*)GAVF_TAG_PACKET_HEADER, 1) < 1) ||
     (!gavf_io_write_uint32v(g->io, s->h->id)))
    return 0;

  gavf_buffer_reset(&g->pkt_buf);

  if(!gavf_write_gavl_packet(&g->pkt_io, s, p) ||
     !gavf_io_write_buffer(g->io, &g->pkt_buf))
    return 0;

  gavf_io_flush(g->io);
  
  return 1;
  }

static int get_min_pts_stream(gavf_t * g, int flush_all,
                              gavl_time_t * min_time_p)
  {
  int i;
  int min_index;
  gavl_time_t min_time;
  gavl_time_t test_time;
  gavf_stream_t * s;
  
  min_index = -1;
  min_time = GAVL_TIME_UNDEFINED;
    
  for(i = 0; i < g->ph.num_streams; i++)
    {
    s = &g->streams[i];
    
    test_time =
      gavf_packet_buffer_get_min_pts(s->pb);

    if(test_time != GAVL_TIME_UNDEFINED)
      {
      if((min_time == GAVL_TIME_UNDEFINED) ||
         (test_time < min_time))
        {
        min_time = test_time;
        min_index = i;
        }
      }
    else
      {
      if(!(s->flags & STREAM_FLAG_DISCONTINUOUS) &&
         !flush_all)
        {
        /* Some streams without packets: stop here */
        return -1;
        }
      }
    }

  *min_time_p = min_time;
  return min_index;
  }

static int flush_packets(gavf_t * g, int flush_all)
  {
  gavl_packet_t * p;
  gavl_time_t min_time;
  int min_index;

  if(!g->streams)
    return 1;
  
  while(1)
    {
    min_index = get_min_pts_stream(g, flush_all, &min_time);
    if(min_index < 0)
      return 1;
    
    p = gavf_packet_buffer_get_read(g->streams[min_index].pb);
    
    if(!write_packet(g, min_index, p))
      return 0;
    }
  return 1;
  }


int gavf_write_packet(gavf_t * g, int stream, const gavl_packet_t * p)
  {
  gavf_stream_t * s;
  gavl_packet_t * p1;
  gavl_time_t min_time;
  int min_index;
  
  s = &g->streams[stream];
  
  if(s->next_sync_pts == GAVL_TIME_UNDEFINED)
    s->next_sync_pts = p->pts;

  /* Decide whether to write a sync header */
  
  switch(g->encoding_mode)
    {
    case ENC_STARTING:
      /* Buffer packet */
      p1 = gavf_packet_buffer_get_write(s->pb);
      gavl_packet_copy(p1, p);

      /* Check if we are done */
      min_index = get_min_pts_stream(g, 0, &min_time);
      if(min_index >= 0)
        {
        g->encoding_mode = g->final_encoding_mode;
        if(g->final_encoding_mode == ENC_SYNCHRONOUS)
          return flush_packets(g, 1);
        else
          return flush_packets(g, 0);
        }
      else
        return 1;
      break;
    case ENC_SYNCHRONOUS:
      return write_packet(g, stream, p);
      break;
    case ENC_INTERLEAVE:
      p1 = gavf_packet_buffer_get_write(s->pb);
      gavl_packet_copy(p1, p);
      return flush_packets(g, 0);
      break;
    }
  return 0;
  }


void gavf_video_frame_to_packet_metadata(const gavl_video_frame_t * frame,
                                         gavl_packet_t * pkt)
  {
  pkt->pts = frame->timestamp;
  pkt->duration = frame->duration;
  pkt->timecode = frame->timecode;
  pkt->interlace_mode = frame->interlace_mode;
  }

/* LEGACY */
int gavf_write_video_frame(gavf_t * g,
                           int stream, gavl_video_frame_t * frame)
  {
  gavf_stream_t * s = &g->streams[stream];
  if(!s->vsink)
    return 0;
  return (gavl_video_sink_put_frame(s->vsink, frame) == GAVL_SINK_OK);
  }

void gavf_packet_to_video_frame(gavl_packet_t * p, gavl_video_frame_t * frame,
                                const gavl_video_format_t * format)
  {
  frame->timecode  = p->timecode;
  frame->timestamp = p->pts;
  frame->interlace_mode = p->interlace_mode;
  frame->duration = p->duration;
  
  frame->strides[0] = 0;
  gavl_video_frame_set_planes(frame, format, p->data);
  }

void gavf_audio_frame_to_packet_metadata(const gavl_audio_frame_t * frame,
                                         gavl_packet_t * pkt)
  {
  pkt->pts = frame->timestamp;
  pkt->duration = frame->valid_samples;
  }

int gavf_write_audio_frame(gavf_t * g, int stream, gavl_audio_frame_t * frame)
  {
  gavf_stream_t * s;
  gavl_audio_frame_t * frame_internal;  
  
  s = &g->streams[stream];
  if(!s->asink)
    return 0;

  frame_internal = gavl_audio_sink_get_frame(s->asink);
  //  gavl_video_frame_copy(&s->h->format.video, frame_internal, frame);

  frame_internal->valid_samples =
    gavl_audio_frame_copy(&s->h->format.audio,
                          frame_internal,
                          frame,
                          0,
                          0,
                          frame->valid_samples,
                          s->h->format.audio.samples_per_frame);
  
  frame_internal->timestamp = frame->timestamp;
  return (gavl_audio_sink_put_frame(s->asink, frame_internal) == GAVL_SINK_OK);
  }

void gavf_packet_to_audio_frame(gavl_packet_t * p, gavl_audio_frame_t * frame,
                                const gavl_audio_format_t * format)
  {
  frame->valid_samples = p->duration;
  frame->timestamp = p->pts;
  gavl_audio_frame_set_channels(frame, format, p->data);
  }


/* Close */

void gavf_close(gavf_t * g)
  {
  int i;
  if(g->wr)
    {
    if(g->streams)
      {
      /* Flush packets if any */
      flush_packets(g, 1);
    
      /* Append final sync header */
      write_sync_header(g, -1, NULL);
      }
    
    /* Write indices */

    if(g->opt.flags & GAVF_OPT_FLAG_SYNC_INDEX)
      {
      gavf_file_index_add(&g->fi, GAVF_TAG_SYNC_INDEX, g->io->position);
      if(g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES)
        gavf_sync_index_dump(&g->si);
      if(!gavf_sync_index_write(g->io, &g->si))
        return;
      }
    
    if(g->opt.flags & GAVF_OPT_FLAG_PACKET_INDEX)
      {
      gavf_file_index_add(&g->fi, GAVF_TAG_PACKET_INDEX, g->io->position);
      if(g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES)
        gavf_packet_index_dump(&g->pi);
      if(!gavf_packet_index_write(g->io, &g->pi))
        return;
      }

    if(g->cl)
      {
      gavf_file_index_add(&g->fi, GAVF_TAG_CHAPTER_LIST, g->io->position);
      if(!gavf_write_chapter_list(g->io, g->cl))
        return;
      }
    
    /* Rewrite file index */
    if(g->io->seek_func)
      {
      gavf_io_seek(g->io, 0, SEEK_SET);
      gavf_file_index_write(g->io, &g->fi);
      }
    
    }

  /* Free stuff */

  if(g->streams)
    {
    for(i = 0; i < g->ph.num_streams; i++)
      gavf_stream_free(&g->streams[i]);
    free(g->streams);
    }
  gavf_file_index_free(&g->fi);
  gavf_sync_index_free(&g->si);
  gavf_packet_index_free(&g->pi);
  gavf_program_header_free(&g->ph);

  if(g->cl)
    gavl_chapter_list_destroy(g->cl);
  
  if(g->write_vframe)
    {
    gavl_video_frame_null(g->write_vframe);
    gavl_video_frame_destroy(g->write_vframe);
    }

  if(g->write_aframe)
    {
    gavl_audio_frame_null(g->write_aframe);
    gavl_audio_frame_destroy(g->write_aframe);
    }

  if(g->sync_pts)
    free(g->sync_pts);
  
  gavl_packet_free(&g->write_pkt);

  gavf_buffer_free(&g->pkt_buf);
  
  free(g);
  }

gavl_packet_sink_t *
gavf_get_packet_sink(gavf_t * g, int stream)
  {
  return g->streams[stream].psink;
  }

gavl_audio_sink_t *
gavf_get_audio_sink(gavf_t * g, int stream)
  {
  return g->streams[stream].asink;
  }

gavl_video_sink_t *
gavf_get_video_sink(gavf_t * g, int stream)
  {
  return g->streams[stream].vsink;
  }

gavl_packet_source_t *
gavf_get_packet_source(gavf_t * g, int stream)
  {
  return g->streams[stream].psrc;
  }

gavl_audio_source_t *
gavf_get_audio_source(gavf_t * g, int stream)
  {
  return g->streams[stream].asrc;
  }

gavl_video_source_t *
gavf_get_video_source(gavf_t * g, int stream)
  {
  return g->streams[stream].vsrc;
  }
