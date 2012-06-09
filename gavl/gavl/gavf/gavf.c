#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gavfprivate.h>

struct gavf_s
  {
  gavf_io_t * io;
  gavf_program_header_t ph;
  gavf_file_index_t     fi;
  gavf_packet_header_t  pkthdr;

  gavf_stream_t * streams;
  
  int header_written;
  
  gavf_options_t opt;
  
  gavf_io_t pkt_io;
  gavf_buffer_t pkt_buf;
  };

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

static void gavf_stream_init_video(gavf_stream_t * s)
  {
  s->timescale = s->h->format.video.timescale;

  if(s->h->ci.flags & GAVL_COMPRESSION_HAS_B_FRAMES)
    s->has_pts = 1;
  
  if(s->h->format.video.framerate_mode == GAVL_FRAMERATE_CONSTANT)
    s->packet_duration = s->h->format.video.frame_duration;
  }

static void gavf_stream_init_text(gavf_stream_t * s)
  {
  s->timescale = s->h->format.text.timescale;
  s->has_pts = 1;
  }

static void init_streams(gavf_t * g)
  {
  int i;
  g->streams = calloc(g->ph.num_streams, sizeof(*g->streams));

  for(i = 0; i < g->ph.num_streams; i++)
    {
    g->streams[i].h = &g->ph.streams[i];

    switch(g->streams[i].h->type)
      {
      case GAVF_STREAM_AUDIO:
        gavf_stream_init_audio(&g->streams[i]);
        break;
      case GAVF_STREAM_VIDEO:
        gavf_stream_init_video(&g->streams[i]);
        break;
      case GAVF_STREAM_TEXT:
        gavf_stream_init_text(&g->streams[i]);
        break;
      }
    }
  
  }


typedef struct
  {
  int num_streams;
  struct
    {
    int stream_id;
    int64_t time;
    } * streams;
  } gavf_sync_header_t;



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
    
    }  
  return 1;
  }

int gavf_open_read(gavf_t * g, gavf_io_t * io)
  {
  int i;
  char sig[8];
  
  g->io = io;

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
    
    }
  
  return 1;
  }

const gavf_program_header_t * gavf_get_program_header(gavf_t * g)
  {
  return &g->ph;
  }

const gavf_packet_header_t * gavf_packet_read_header(gavf_t * gavf)
  {
  char c[2];

  while(1)
    {
    if(!gavf_io_read_data(gavf->io, (uint8_t*)&c, 1))
      return NULL;
    
    c[1] = '\0';

  
    if(!strcmp(c, GAVF_TAG_PACKET_HEADER))
      {
      /* Got new packet */
      if(!gavf_io_read_uint32v(gavf->io, &gavf->pkthdr.stream_id) ||
         !gavf_io_read_uint32v(gavf->io, &gavf->pkthdr.len))
        return 0;
      return &gavf->pkthdr;
      }
    }
  return NULL;
  }

void gavf_packet_skip(gavf_t * gavf)
  {
  gavf_io_skip(gavf->io, gavf->pkthdr.len);
  }

void gavf_packet_read_packet(gavf_t * gavf, gavl_packet_t * p)
  {
  
  }


/* Write support */

int gavf_open_write(gavf_t * g, gavf_io_t * io)
  {
  g->io = io;

  /* Initialize packet buffer */
  gavf_io_init_buf_write(&g->pkt_io, &g->pkt_buf);
  
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

static int write_packet(gavf_t * g, gavf_stream_t * s,
                        gavl_packet_t * p)
  {
  if((gavf_io_write_data(g->io, (const uint8_t*)GAVF_TAG_PACKET_HEADER, 1) < 1) ||
     (gavf_io_write_uint32v(g->io, s->h->id)))
    return 0;

  gavf_buffer_reset(&g->pkt_buf);

  if(!gavf_write_gavl_packet(&g->pkt_io, s, p) ||
     !gavf_io_write_buffer(g->io, &g->pkt_buf))
    return 0;
  
  return 1;
  }

int gavf_write_packet(gavf_t * g, int stream, gavl_packet_t * p)
  {
  gavf_stream_t * s;

  if(!g->streams)
    {
    init_streams(g);
    
    }

  s = &g->streams[stream];
  
  if(!g->header_written)
    {
    /* Write program header */
    
    g->header_written = 1;
    }
  else
    {
    /* Decide whether to write a sync header */
    }
  
  return write_packet(g, s, p);
  }

/* Close */

void gavf_close(gavf_t * g)
  {
  gavf_file_index_free(&g->fi);
  gavf_program_header_free(&g->ph);
  free(g);
  }
