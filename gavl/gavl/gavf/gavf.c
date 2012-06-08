#include <stdlib.h>
#include <string.h>

#include <gavfprivate.h>

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

typedef struct
  {
  int num_streams;
  struct
    {
    int stream_id;
    int64_t time;
    } * streams;
  } gavf_sync_header_t;

struct gavf_s
  {
  gavf_io_t * io;
  gavf_program_header_t ph;
  gavf_sync_header_t    sh;
  gavf_file_index_t     fi;
  gavf_packet_header_t  pkthdr;
  
  int header_written;
  
  gavf_options_t opt;
  
  gavf_io_t pkt_io;
  gavf_buffer_t pkt_buf;
  };


gavf_t * gavf_create()
  {
  gavf_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

/* Read support */

int gavf_open_read(gavf_t * g, gavf_io_t * io)
  {
  g->io = io;

  /* Initialize packet buffer */
  gavf_io_init_buf_read(&g->pkt_io, &g->pkt_buf);
  
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

static int write_packet(gavf_t * g, gavf_stream_header_t * sh,
                        gavl_packet_t * p)
  {
  if((gavf_io_write_data(g->io, (const uint8_t*)GAVF_TAG_PACKET_HEADER, 1) < 1) ||
     (gavf_io_write_uint32v(g->io, sh->id)))
    return 0;

  gavf_buffer_reset(&g->pkt_buf);

  if(!gavf_write_gavl_packet(&g->pkt_io, sh, p) ||
     !gavf_io_write_buffer(g->io, &g->pkt_buf))
    return 0;
  
  return 1;
  }

int gavf_write_packet(gavf_t * g, int stream, gavl_packet_t * p)
  {
  gavf_stream_header_t * sh = &g->ph.streams[stream];

  if(!g->header_written)
    {
    /* Write program header */
    
    g->header_written = 1;
    }
  else
    {
    /* Decide whether to write a sync header */
    }
  
  return write_packet(g, sh, p);
  }

/* Close */

void gavf_close(gavf_t * g)
  {
  gavf_file_index_free(&g->fi);
  gavf_program_header_free(&g->ph);
  free(g);
  }
