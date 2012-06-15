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
  
  gavf_stream_t * streams;

  int64_t * sync_pts;
  
  gavf_options_t opt;
  
  gavf_io_t pkt_io;
  gavf_buffer_t pkt_buf;
  
  uint64_t sync_pos;
  
  int wr;
  
  gavl_packet_t write_pkt;
  gavl_video_frame_t * write_frame;

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

/* Streams */

static void gavf_stream_init_audio(gavf_stream_t * s)
  {
  s->timescale = s->h->format.audio.samplerate;

  /* Figure out the samples per frame */
  if(gavl_compression_constant_frame_samples(s->h->ci.id))
    s->packet_duration = s->h->format.audio.samples_per_frame;
  }

static void gavf_stream_init_video(gavf_t * g, gavf_stream_t * s)
  {
  s->timescale = s->h->format.video.timescale;

  if(s->h->ci.flags & GAVL_COMPRESSION_HAS_B_FRAMES)
    s->has_pts = 1;

  if(s->h->ci.flags & GAVL_COMPRESSION_HAS_P_FRAMES)
    g->sync_distance = 0;
  
  if(s->h->format.video.framerate_mode == GAVL_FRAMERATE_CONSTANT)
    s->packet_duration = s->h->format.video.frame_duration;

  if(s->h->ci.id == GAVL_CODEC_ID_NONE)
    s->image_size = gavl_video_format_get_image_size(&s->h->format.video);
  }

static void gavf_stream_init_text(gavf_stream_t * s)
  {
  s->timescale = s->h->format.text.timescale;
  s->has_pts = 1;
  s->discontinuous = 1;
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

    switch(g->streams[i].h->type)
      {
      case GAVF_STREAM_AUDIO:
        gavf_stream_init_audio(&g->streams[i]);
        break;
      case GAVF_STREAM_VIDEO:
        gavf_stream_init_video(g, &g->streams[i]);
        break;
      case GAVF_STREAM_TEXT:
        gavf_stream_init_text(&g->streams[i]);
        break;
      }
    if(g->wr)
      {
      g->streams[i].pb =
        gavf_packet_buffer_create(g->streams[i].timescale);

      if(g->streams[i].h->ci.global_header_len)
        {
        gavf_buffer_alloc(&g->streams[i].last_global_header,
                          g->streams[i].h->ci.global_header_len);
        memcpy(g->streams[i].last_global_header.buf,
               g->streams[i].h->ci.global_header,
               g->streams[i].h->ci.global_header_len);
        g->streams[i].last_global_header.len =
          g->streams[i].h->ci.global_header_len;
        }
      
      }
    }
  }

static void gavf_stream_free(gavf_stream_t * s)
  {
  gavf_buffer_free(&s->last_global_header);
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
  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(!gavf_io_read_int64v(g->io, &g->sync_pts[i]))
      return 0;
    
    if(g->sync_pts[i] != GAVL_TIME_UNDEFINED)
      {
      g->streams[i].last_sync_pts = g->sync_pts[i];
      if(!g->streams[i].has_pts)
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

  if(g->sync_pos != g->io->position)
    {
    if(g->io->seek_func)
      gavf_io_seek(io, g->sync_pos, SEEK_SET);
    else
      return 0;
    }

  if(!read_sync_header(g))
    return 0;
  
  return 1;
  }

const gavf_program_header_t * gavf_get_program_header(gavf_t * g)
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
     !gavf_read_gavl_packet(&g->pkt_io, s, p))
    return 0;
  return 1;
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
                          gavl_video_format_t * format,
                          const gavl_metadata_t * m)
  {
  if(ci->id == GAVL_CODEC_ID_NONE)
    gavl_video_format_set_frame_size(format, 0, 0);
  return gavf_program_header_add_video_stream(&g->ph, ci, format, m);
  }

int gavf_add_text_stream(gavf_t * g,
                         uint32_t timescale,
                         const gavl_metadata_t * m)
  {
  return gavf_program_header_add_text_stream(&g->ph, timescale, m);
  }

static int start_encoding(gavf_t * g)
  {
  if(g->streams)
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

    else if((g->streams[i].discontinuous) &&
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

  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(!gavf_io_write_int64v(g->io, g->sync_pts[i]))
      return 0;
    }

  /* Update last sync pts */

  for(i = 0; i < g->ph.num_streams; i++)
    {
    if(g->sync_pts[i] != GAVL_TIME_UNDEFINED)
      g->streams[i].last_sync_pts = g->sync_pts[i];
    }
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
  
  return 1;
  }

static int get_min_pts_stream(gavf_t * g, int flush_all,
                              gavl_time_t * min_time_p)
  {
  int i;
  int min_index;
  gavl_time_t min_time;
  gavl_time_t test_time;
  
  min_index = -1;
  min_time = GAVL_TIME_UNDEFINED;
    
  for(i = 0; i < g->ph.num_streams; i++)
    {
    test_time =
      gavf_packet_buffer_get_min_pts(g->streams[i].pb);

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
      if((!g->streams[i].discontinuous) &&
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
  
  if(!start_encoding(g))
    return 0;
  
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

static void video_frame_2_pkt(const gavl_video_frame_t * frame,
                              gavl_packet_t * pkt)
  {
  pkt->pts = frame->timestamp;
  pkt->duration = frame->duration;
  pkt->interlace_mode = frame->interlace_mode;
  pkt->timecode = frame->timecode;
  }

int gavf_write_video_frame(gavf_t * g, int stream, gavl_video_frame_t * frame)
  {
  gavf_stream_t * s;

  if(!start_encoding(g))
    return 0;

  s = &g->streams[stream];

  if((s->h->type != GAVF_STREAM_VIDEO) || (s->h->ci.id != GAVL_CODEC_ID_NONE))
    return 0;

  /* Check if we need to copy the frame or can take it directly */
  if(gavl_video_frame_continuous(&s->h->format.video, frame))
    {
    gavl_packet_t p;
    gavl_packet_init(&p);
    p.data_len = s->image_size;
    p.data = frame->planes[0];
    video_frame_2_pkt(frame, &p);

    return gavf_write_packet(g, stream, &p);
    }
  else
    {
    gavl_packet_alloc(&g->write_pkt, s->image_size);
    if(!g->write_frame)
      g->write_frame = gavl_video_frame_create(NULL);
    gavl_video_frame_set_strides(g->write_frame, &s->h->format.video);
    gavl_video_frame_set_planes(g->write_frame, &s->h->format.video, g->write_pkt.data);

    gavl_video_frame_copy(&s->h->format.video, g->write_frame, frame);
    video_frame_2_pkt(frame, &g->write_pkt);
    return gavf_write_packet(g, stream, &g->write_pkt);
    }
  }

/* Close */

void gavf_close(gavf_t * g)
  {
  int i;
  if(g->wr)
    {
    /* Flush packets if any */
    flush_packets(g, 1);
    
    /* Append final sync header */
    write_sync_header(g, -1, NULL);
    
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
  
  if(g->write_frame)
    {
    gavl_video_frame_null(g->write_frame);
    gavl_video_frame_destroy(g->write_frame);
    }

  if(g->sync_pts)
    free(g->sync_pts);
  
  gavl_packet_free(&g->write_pkt);

  gavf_buffer_free(&g->pkt_buf);
  
  free(g);
  }
