#include <stdlib.h>
#include <string.h>

#include <gavfprivate.h>

// #define DUMP_EOF

static struct
  {
  gavf_stream_type_t type;
  const char * name;
  }
stream_types[] =
  {
    { GAVF_STREAM_AUDIO, "audio" },
    { GAVF_STREAM_VIDEO, "video" },
    { GAVF_STREAM_TEXT,  "text"  },
    { GAVF_STREAM_OVERLAY,  "overlay" },
  };

GAVL_PUBLIC
const char * gavf_stream_type_name(gavf_stream_type_t t)
  {
  int i;
  for(i = 0; i < sizeof(stream_types)/sizeof(stream_types[0]); i++)
    {
    if(stream_types[i].type == t)
      return stream_types[i].name;
    }
  return NULL;
  }

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

/*
 */

static int write_sync_header(gavf_t * g, int stream, const gavl_packet_t * p)
  {
  int i;
  gavf_stream_t * s;
  for(i = 0; i < g->ph.num_streams; i++)
    {
    s = &g->streams[i];

    if(i == stream)
      {
      g->sync_pts[i] = p->pts;
      s->packets_since_sync = 0;
      }
    else if((stream >= 0) &&
            (s->h->ci.flags & GAVL_COMPRESSION_HAS_B_FRAMES))
      {
      gavl_packet_t * p;
      p = gavf_packet_buffer_peek_read(s->pb);
      if(!p && !(p->flags & GAVL_PACKET_KEYFRAME))
        g->sync_pts[i] = GAVL_TIME_UNDEFINED;
      else
        g->sync_pts[i] = p->pts;
      }
    else if((g->streams[i].flags & STREAM_FLAG_DISCONTINUOUS) &&
            (g->streams[i].next_sync_pts == GAVL_TIME_UNDEFINED))
      {
      g->sync_pts[i] = 0;
      s->packets_since_sync = 0;
      }
    else
      {
      g->sync_pts[i] = g->streams[i].next_sync_pts;
      s->packets_since_sync = 0;
      }
    }

  /* Update sync index */
  if(g->opt.flags & GAVF_OPT_FLAG_SYNC_INDEX)
    {
    gavf_sync_index_add(&g->si, g->io->position,
                        g->sync_pts);
    }

  /* If that's the first sync header, update file index */

  if(!g->first_sync_pos)
    g->first_sync_pos = g->io->position;
  
  /* Write the sync header */
  if(gavf_io_write_data(g->io, (uint8_t*)GAVF_TAG_SYNC_HEADER, 8) < 8)
    return 0;
#if 0
  fprintf(stderr, "Write sync header\n");
#endif
  
  for(i = 0; i < g->ph.num_streams; i++)
    {
#if 0
    fprintf(stderr, "PTS[%d]: %"PRId64"\n", i, g->sync_pts[i]);
#endif
    if(!gavf_io_write_int64v(g->io, g->sync_pts[i]))
      return 0;
    }
  /* Update last sync pts */

  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(g->sync_pts[i] != GAVL_TIME_UNDEFINED)
      g->streams[i].last_sync_pts = g->sync_pts[i];
    }
  return gavf_io_flush(g->io);
  }

static gavl_sink_status_t
write_packet(gavf_t * g, int stream, const gavl_packet_t * p)
  {
  int write_sync = 0;
  gavf_stream_t * s = &g->streams[stream];
#if 1
  if(p->data_len == 0)
    {
    fprintf(stderr, "write_packet %p\n", p);
    gavl_packet_dump(p);
    }
#endif
  /* Decide whether to write a sync header */
  if(!g->first_sync_pos)
    write_sync = 1;
  else if(g->sync_distance)
    {
    if(gavl_time_unscale(s->timescale, p->pts) - g->last_sync_time >
       g->sync_distance)
      write_sync = 1;
    }
  else
    {
    if((s->h->type == GAVF_STREAM_VIDEO) &&
       (s->h->ci.flags & GAVL_COMPRESSION_HAS_P_FRAMES) &&
       (p->flags & GAVL_PACKET_KEYFRAME) &&
       s->packets_since_sync)
      write_sync = 1;
    }

  if(write_sync)
    {
    if(!write_sync_header(g, stream, p))
      return GAVL_SINK_ERROR;
    if(g->sync_distance)
      g->last_sync_time = gavl_time_unscale(s->timescale, p->pts);
    }

  /* Write stored metadata if there is one */
  if(g->meta_buf.len)
    {
    if((gavf_io_write_data(g->io,
                           (const uint8_t*)GAVF_TAG_METADATA_HEADER, 1) < 1) ||
       !gavf_io_write_buffer(g->io, &g->meta_buf))
      return GAVL_SINK_ERROR;
    gavf_buffer_reset(&g->meta_buf);
    }
  
  // If a stream has B-frames, this won't be correct
  // for the next sync timestamp (it will be taken from the
  // packet pts in write_sync_header)
  
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
    return GAVL_SINK_ERROR;

  gavf_buffer_reset(&g->pkt_buf);
  
  if(!gavf_write_gavl_packet_header(&g->pkt_io, s, p) ||
     !gavf_io_write_uint32v(g->io, g->pkt_buf.len + p->data_len) ||
     (gavf_io_write_data(g->io, g->pkt_buf.buf, g->pkt_buf.len) < g->pkt_buf.len) ||
     (gavf_io_write_data(g->io, p->data, p->data_len) < p->data_len))
    return GAVL_SINK_ERROR;

  if(g->opt.flags & GAVF_OPT_FLAG_DUMP_PACKETS)
    {
    fprintf(stderr, "ID: %d ", s->h->id);
    gavl_packet_dump(p);
    }
  
  if(!gavf_io_flush(g->io))
    return GAVL_SINK_ERROR;

  
  s->packets_since_sync++;

  if(g->io->cb && !g->io->cb(g->io->cb_priv, GAVF_IO_CB_PACKET, p))
    return GAVL_SINK_ERROR;
  
  return GAVL_SINK_OK;
  }


/*
 *  Flush packets. s is the stream of the last packet written.
 *  If s is NULL, flush all streams
 */

gavl_sink_status_t gavf_flush_packets(gavf_t * g, gavf_stream_t * s)
  {
  int i;
  gavl_packet_t * p;
  int min_index;
  gavl_time_t min_time;
  gavl_time_t test_time;
  gavf_stream_t * ws;
  gavl_sink_status_t st;

  //  fprintf(stderr, "flush_packets\n");
  
  if(!g->first_sync_pos)
    {
    for(i = 0; i < g->ph.num_streams; i++)
      {
      if(!(g->streams[i].flags & STREAM_FLAG_DISCONTINUOUS) &&
         (g->streams[i].h->foot.pts_start == GAVL_TIME_UNDEFINED))
        return GAVL_SINK_OK;
      }
    }

  /* Flush discontinuous streams */
  for(i = 0; i < g->ph.num_streams; i++)
    {
    ws = &g->streams[i];
    if(ws->flags & STREAM_FLAG_DISCONTINUOUS)
      {
      while((p = gavf_packet_buffer_get_read(ws->pb)))
        {
        if((st = write_packet(g, i, p)) != GAVL_SINK_OK)
          return st;
        }
      }
    }
  
  /* Flush as many packets as possible */
  while(1)
    {
    /*
     *  1. Find stream we need to output now
     */

    min_index = -1;
    min_time = GAVL_TIME_UNDEFINED;
    for(i = 0; i < g->ph.num_streams; i++)
      {
      ws = &g->streams[i];

      if(ws->flags & STREAM_FLAG_DISCONTINUOUS)
        continue;
      
      test_time =
        gavf_packet_buffer_get_min_pts(ws->pb);
      
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
        if(!(ws->flags & STREAM_FLAG_DISCONTINUOUS) &&
           s && (g->encoding_mode == ENC_INTERLEAVE))
          {
          /* Some streams without packets: stop here */
          return GAVL_SINK_OK;
          }
        }
      }

    /*
     *  2. Exit if we are done
     */
    if(min_index < 0)
      return GAVL_SINK_OK;

    /*
     *  3. Output packet
     */

    ws = &g->streams[min_index];
    p = gavf_packet_buffer_get_read(ws->pb);

    if((st = write_packet(g, min_index, p)) != GAVL_SINK_OK)
      return st;
    
    /*
     *  4. If we have B-frames, output the complete Mini-GOP
     */
    
    if(ws->h->type == GAVF_STREAM_VIDEO)
      {
      while(1)
        {
        p = gavf_packet_buffer_peek_read(ws->pb);
        if(p && ((p->flags & GAVL_PACKET_TYPE_MASK) == GAVL_PACKET_TYPE_B))
          {
          if((st = write_packet(g, min_index, p)) != GAVL_SINK_OK)
            return st;
          }
        else
          break;
        }
      }

    /* Continue */
    
    }
  
  }

static int get_audio_sample_size(const gavl_audio_format_t * fmt,
                                 const gavl_compression_info_t * ci)
  {
  if(ci->id == GAVL_CODEC_ID_NONE)
    return gavl_bytes_per_sample(fmt->sample_format);
  else
    return gavl_compression_get_sample_size(ci->id);
  }

int gavf_get_max_audio_packet_size(const gavl_audio_format_t * fmt,
                                   const gavl_compression_info_t * ci)
  {
  int sample_size = 0;

  if(ci->max_packet_size)
    return ci->max_packet_size;

  sample_size =
    get_audio_sample_size(fmt, ci);
  
  return fmt->samples_per_frame * fmt->num_channels * sample_size;
  }

/* Streams */

static void gavf_stream_init_audio(gavf_t * g, gavf_stream_t * s)
  {
  int sample_size;
  s->timescale = s->h->format.audio.samplerate;
  
  s->h->ci.max_packet_size =
    gavf_get_max_audio_packet_size(&s->h->format.audio, &s->h->ci);

  sample_size =
    get_audio_sample_size(&s->h->format.audio, &s->h->ci);
  
  /* Figure out the packet duration */
  if(gavl_compression_constant_frame_samples(s->h->ci.id) ||
     sample_size)
    s->packet_duration = s->h->format.audio.samples_per_frame;
  else
    s->flags |= STREAM_FLAG_HAS_DURATION;
  
  
  if(g->wr)
    {
    /* Create packet sink */
    gavf_stream_create_packet_sink(g, s);
    }
  else
    {
    /* Create packet source */
    gavf_stream_create_packet_src(g, s);
    }
  }

GAVL_PUBLIC 
int gavf_get_max_video_packet_size(const gavl_video_format_t * fmt,
                                   const gavl_compression_info_t * ci)
  {
  if(ci->max_packet_size)
    return ci->max_packet_size;
  if(ci->id == GAVL_CODEC_ID_NONE)
    return gavl_video_format_get_image_size(fmt);
  return 0;
  }


static void gavf_stream_init_video(gavf_t * g, gavf_stream_t * s)
  {
  s->timescale = s->h->format.video.timescale;

  if((s->h->ci.flags & GAVL_COMPRESSION_HAS_B_FRAMES) ||
     (s->h->type == GAVF_STREAM_OVERLAY))
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

  if((s->h->format.video.framerate_mode == GAVL_FRAMERATE_STILL) ||
     (s->h->type == GAVF_STREAM_OVERLAY))
    s->flags |= STREAM_FLAG_DISCONTINUOUS;
  
  s->h->ci.max_packet_size =
    gavf_get_max_video_packet_size(&s->h->format.video,
                                   &s->h->ci);
  
  if(g->wr)
    {
    /* Create packet sink */
    gavf_stream_create_packet_sink(g, s);
    }
  else
    {
    /* Create packet source */
    gavf_stream_create_packet_src(g, s);
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
    gavf_stream_create_packet_sink(g, s);
    }
  else
    {
    /* Create packet source */
    gavf_stream_create_packet_src(g, s);
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
      case GAVF_STREAM_OVERLAY:
        gavf_stream_init_video(g, &g->streams[i]);
        break;
      case GAVF_STREAM_TEXT:
        gavf_stream_init_text(g, &g->streams[i]);
        break;
      }
    g->streams[i].pb =
      gavf_packet_buffer_create(g->streams[i].timescale);
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
  if(s->pb)
    gavf_packet_buffer_destroy(s->pb);
  
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
    
    init_streams(g);
    }
  else if(!strncmp(sig, GAVF_TAG_SYNC_HEADER, 8))
    {
    g->first_sync_pos = g->io->position;
    }  
  return 1;
  }

static int read_sync_header(gavf_t * g)
  {
  int i;
  /* Read sync header */
#if 0
  fprintf(stderr, "Read sync header\n");
#endif
  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(!gavf_io_read_int64v(g->io, &g->sync_pts[i]))
      return 0;
#if 0
    fprintf(stderr, "PTS[%d]: %ld\n", i, g->sync_pts[i]);
#endif
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
  char sig[8];
  
  g->io = io;

  g->opt.flags &= ~(GAVF_OPT_FLAG_SYNC_INDEX|
                    GAVF_OPT_FLAG_PACKET_INDEX);
  
  /* Initialize packet buffer */
  gavf_io_init_buf_read(&g->pkt_io, &g->pkt_buf);
  gavf_io_init_buf_read(&g->meta_io, &g->meta_buf);
  
  /* Read up to the first sync header */

  while(1)
    {
    if(gavf_io_read_data(g->io, (uint8_t*)sig, 8) < 8)
      return 0;
    
    if(!handle_chunk(g, sig))
      return 0;
    
    if(g->first_sync_pos > 0)
      break;
    }

  gavf_footer_check(g);

  /* Dump stuff */
  
  if(g->opt.flags & GAVF_OPT_FLAG_DUMP_HEADERS)
    {
    gavf_program_header_dump(&g->ph);

    if(g->cl)
      gavl_chapter_list_dump(g->cl);
    }

  if(g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES)
    {
    if((g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES) ||
       (g->opt.flags & GAVF_OPT_FLAG_SYNC_INDEX))
      gavf_sync_index_dump(&g->si);
    if((g->opt.flags & GAVF_OPT_FLAG_DUMP_INDICES) ||
       (g->opt.flags & GAVF_OPT_FLAG_PACKET_INDEX))
      gavf_packet_index_dump(&g->pi);
    }
  
  gavf_reset(g);
  
  return 1;
  }

int gavf_reset(gavf_t * g)
  {
  if(g->first_sync_pos != g->io->position)
    {
    if(g->io->seek_func)
      gavf_io_seek(g->io, g->first_sync_pos, SEEK_SET);
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
  uint32_t len;

#ifdef DUMP_EOF
  fprintf(stderr, "gavf_packet_read_header\n");
#endif
  
  if(g->eof)
    {
#ifdef DUMP_EOF
    fprintf(stderr, "EOF 0\n");
#endif
    return NULL;
    }
  
  while(1)
    {
    if(!gavf_io_read_data(g->io, (uint8_t*)c, 1))
      {
#ifdef DUMP_EOF
      fprintf(stderr, "EOF 1\n");
#endif
      goto got_eof;
      }
    if(c[0] == GAVF_TAG_PACKET_HEADER_C)
      {
      gavf_stream_t * s;
      /* Got new packet */
      if(!gavf_io_read_uint32v(g->io, &g->pkthdr.stream_id))
        {
#ifdef DUMP_EOF
        fprintf(stderr, "EOF 2\n");
#endif
        goto got_eof;
        }
      
      /* Check whether to skip this stream */
      if((s = gavf_find_stream_by_id(g, g->pkthdr.stream_id)) &&
         (s->flags & STREAM_FLAG_SKIP))
        {
        if(s->skip_func)
          s->skip_func(g, s->h, s->skip_priv);
        else
          {
          gavf_packet_skip(g);
          if(g->io->cb && !g->io->cb(g->io->cb_priv, GAVF_IO_CB_PACKET, NULL))
            {
#ifdef DUMP_EOF
            fprintf(stderr, "EOF 3\n");
#endif
            goto got_eof;
            }
          }
        }
      else
        {
        g->have_pkt_header = 1;
        return &g->pkthdr;
        }
      }
    else if(c[0] == GAVF_TAG_METADATA_HEADER_C)
      {
      /* Inline metadata */
      if(g->opt.metadata_cb ||
         (g->opt.flags & GAVF_OPT_FLAG_DUMP_METADATA))
        {
        gavl_metadata_t m;
        gavl_metadata_init(&m);
        
        gavf_buffer_reset(&g->meta_buf);
        
        if(!gavf_io_read_buffer(g->io, &g->meta_buf) ||
           !gavf_read_metadata(&g->meta_io, &m))
          {
#ifdef DUMP_EOF
          fprintf(stderr, "EOF 4\n");
#endif
          goto got_eof;
          }
        if(!gavl_metadata_equal(&g->metadata, &m))
          {
          gavl_metadata_free(&g->metadata);
          memcpy(&g->metadata, &m, sizeof(m));

          if(g->opt.metadata_cb)
            g->opt.metadata_cb(g->opt.metadata_cb_priv,
                               &g->metadata);
          
          if(g->opt.flags & GAVF_OPT_FLAG_DUMP_METADATA)
            {
            fprintf(stderr, "Got inline metadata\n");
            gavl_metadata_dump(&g->metadata, 2);
            }
          }
        else
          gavl_metadata_free(&m);
        }
      else
        {
        if(!gavf_io_read_uint32v(g->io, &len))
          {
#ifdef DUMP_EOF
          fprintf(stderr, "EOF 5\n");
#endif
          goto got_eof;
          }
        gavf_io_skip(g->io, len);
        }
      }
    else
      {
      if(gavf_io_read_data(g->io, (uint8_t*)&c[1], 7) < 7)
        {
#ifdef DUMP_EOF
        fprintf(stderr, "EOF 6\n");
#endif
        goto got_eof;
        }

      if(!strncmp(c, GAVF_TAG_SYNC_HEADER, 8))
        {
        if(!read_sync_header(g))
          {
#ifdef DUMP_EOF
          fprintf(stderr, "EOF 7\n");
#endif
          goto got_eof;
          }
        }
      else
        {
#ifdef DUMP_EOF
       fprintf(stderr, "EOF 8 %8s\n", c);
#endif
        goto got_eof;
        }
      }
    }
  
  got_eof:
  g->eof = 1;
  return NULL;
  }

int gavf_update_metadata(gavf_t * g, const gavl_metadata_t * m)
  {
  if(gavl_metadata_equal(&g->metadata, m))
    return 1;

  if(g->opt.flags & GAVF_OPT_FLAG_DUMP_METADATA)
    {
    fprintf(stderr, "Got inline metadata\n");
    gavl_metadata_dump(m, 2);
    }
  
  gavl_metadata_free(&g->metadata);
  gavl_metadata_init(&g->metadata);
  gavl_metadata_copy(&g->metadata, m);
  gavf_buffer_reset(&g->meta_buf);

  /*
   *  Write metadata to buffer.
   *  Will be flushed after the next sync header.
   */

  if(!gavf_write_metadata(&g->meta_io, &g->metadata))
    return 0;
  
  return 1;
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
  gavf_stream_t * s = NULL;

  g->have_pkt_header = 0;

  if(!(s = gavf_find_stream_by_id(g, g->pkthdr.stream_id)))
    return 0;
  
  gavf_buffer_reset(&g->pkt_buf);
  
  if(!gavf_read_gavl_packet(g->io, s, p))
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

  g->eof = 0;
  
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

  //  fprintf(stderr, "Index position: %ld, file position: %ld\n", index_position,
  //          g->si.entries[index_position].pos);

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
  gavf_io_init_buf_write(&g->meta_io, &g->meta_buf);

  if(m)
    gavl_metadata_copy(&g->ph.m, m);

  if(cl)
    g->cl = gavl_chapter_list_copy(cl);
  
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


int gavf_add_overlay_stream(gavf_t * g,
                            const gavl_compression_info_t * ci,
                            const gavl_video_format_t * format,
                            const gavl_metadata_t * m)
  {
  return gavf_program_header_add_overlay_stream(&g->ph, ci, format, m);
  }


int gavf_start(gavf_t * g)
  {
  if(!g->wr || g->streams)
    return 1;
  
  g->sync_distance = g->opt.sync_distance;
  
  init_streams(g);
  
  gavf_footer_init(g);
  
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
  
  if(g->opt.flags & GAVF_OPT_FLAG_DUMP_HEADERS)
    gavf_program_header_dump(&g->ph);
  
  if(!gavf_program_header_write(g->io, &g->ph))
    return 0;
  
  return 1;
  }


void gavf_video_frame_to_packet_metadata(const gavl_video_frame_t * frame,
                                         gavl_packet_t * pkt)
  {
  pkt->pts = frame->timestamp;
  pkt->duration = frame->duration;
  pkt->timecode = frame->timecode;
  pkt->interlace_mode = frame->interlace_mode;

  gavl_rectangle_i_copy(&pkt->src_rect, &frame->src_rect);
  pkt->dst_x = frame->dst_x;
  pkt->dst_y = frame->dst_y;
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
  frame->timestamp = p->pts;
  frame->duration = p->duration;

  frame->interlace_mode = p->interlace_mode;
  frame->timecode  = p->timecode;

  gavl_rectangle_i_copy(&frame->src_rect, &p->src_rect);
  frame->dst_x = p->dst_x;
  frame->dst_y = p->dst_y;
  
  frame->strides[0] = 0;
  gavl_video_frame_set_planes(frame, format, p->data);
  }

static void get_overlay_format(const gavl_video_format_t * src,
                               gavl_video_format_t * dst,
                               const gavl_rectangle_i_t * src_rect)
  {
  gavl_video_format_copy(dst, src);
  dst->image_width  = src_rect->w + src_rect->x;
  dst->image_height = src_rect->h + src_rect->y;
  gavl_video_format_set_frame_size(dst, 0, 0);
  }

void gavf_packet_to_overlay(gavl_packet_t * p, gavl_video_frame_t * frame,
                            const gavl_video_format_t * format)
  {
  gavl_video_format_t copy_format;
  gavl_video_frame_t tmp_frame_src;
  
  frame->timestamp = p->pts;
  frame->duration  = p->duration;
  
  memset(&tmp_frame_src, 0, sizeof(tmp_frame_src));

  get_overlay_format(format, &copy_format, &p->src_rect);
  gavl_video_frame_set_planes(&tmp_frame_src, &copy_format, p->data);
  
  gavl_video_frame_copy(&copy_format, frame, &tmp_frame_src);

  gavl_rectangle_i_copy(&frame->src_rect, &p->src_rect);
  frame->dst_x = p->dst_x;
  frame->dst_y = p->dst_y;
  }

void gavf_overlay_to_packet(gavl_video_frame_t * frame, 
                            gavl_packet_t * p,
                            const gavl_video_format_t * format)
  {
  gavl_video_format_t copy_format;
  gavl_video_frame_t tmp_frame_src;
  gavl_video_frame_t tmp_frame_dst;
  int sub_h, sub_v; // Not necessary yet but well....
  gavl_rectangle_i_t rect;
  p->pts      = frame->timestamp;
  p->duration = frame->duration;
  
  memset(&tmp_frame_src, 0, sizeof(tmp_frame_src));
  memset(&tmp_frame_dst, 0, sizeof(tmp_frame_dst));
  gavl_pixelformat_chroma_sub(format->pixelformat, &sub_h, &sub_v);
  
  gavl_rectangle_i_copy(&p->src_rect, &frame->src_rect);

  /* Shift rectangles */

  p->src_rect.x = frame->src_rect.x % sub_h;
  p->src_rect.y = frame->src_rect.y % sub_v;
  
  get_overlay_format(format, &copy_format, &p->src_rect);

  rect.x = frame->src_rect.x - p->src_rect.x;
  rect.y = frame->src_rect.y - p->src_rect.y;
  rect.w = copy_format.image_width;
  rect.h = copy_format.image_height;
  
  gavl_video_frame_get_subframe(format->pixelformat,
                                frame,
                                &tmp_frame_src,
                                &rect);

  /* p->data is assumed to have the proper allocation already!! */
  gavl_video_frame_set_planes(&tmp_frame_dst, &copy_format, p->data);

  gavl_video_frame_copy(&copy_format, &tmp_frame_dst, &tmp_frame_src);
  
  p->dst_x = frame->dst_x;
  p->dst_y = frame->dst_y;
  p->data_len = gavl_video_format_get_image_size(&copy_format);
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
      gavf_flush_packets(g, NULL);
    
      /* Append final sync header */
      write_sync_header(g, -1, NULL);
      }
    
    /* Write footer */
    
    if(!gavf_footer_write(g))
      return;
    }
  
  /* Free stuff */

  if(g->streams)
    {
    for(i = 0; i < g->ph.num_streams; i++)
      gavf_stream_free(&g->streams[i]);
    free(g->streams);
    }
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
  gavf_buffer_free(&g->meta_buf);
  gavl_metadata_free(&g->metadata);
  
  free(g);
  }

gavl_packet_sink_t *
gavf_get_packet_sink(gavf_t * g, uint32_t id)
  {
  gavf_stream_t * s;
  if(!(s = gavf_find_stream_by_id(g, id)))
    return NULL;
  return s->psink;
  }

gavl_packet_source_t *
gavf_get_packet_source(gavf_t * g, uint32_t id)
  {
  gavf_stream_t * s;
  if(!(s = gavf_find_stream_by_id(g, id)))
    return NULL;
  return s->psrc;
  }

void gavf_stream_set_skip(gavf_t * g, uint32_t id,
                          gavf_stream_skip_func func, void * priv)
  {
  gavf_stream_t * s;
  if((s = gavf_find_stream_by_id(g, id)))
    {
    s->flags |= STREAM_FLAG_SKIP;
    s->skip_func = func;
    s->skip_priv = priv;
    }
  }

gavf_stream_t * gavf_find_stream_by_id(gavf_t * g, uint32_t id)
  {
  int i;
  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(g->streams[i].h->id == id)
      return &g->streams[i];
    }
  return NULL;
  }


int gavf_get_num_streams(gavf_t * g, int type)
  {
  int i;
  int ret = 0;

  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(g->ph.streams[i].type == type)
      ret++;
    }
  return ret;
  }

const gavf_stream_header_t * gavf_get_stream(gavf_t * g, int index,
                                             int type)
  {
  int i;
  int idx = 0;
  
  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(g->ph.streams[i].type == type)
      {
      if(idx == index)
        return &g->ph.streams[i];
      idx++;
      }
    }
  return NULL;
  }

