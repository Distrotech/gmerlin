#include <string.h>
#include <stdlib.h>

#include <gavfprivate.h>

int gavf_program_header_read(gavf_io_t * io, gavf_program_header_t * ph)
  {
  gavf_buffer_t buf;
  gavf_io_t bufio;
  int i;
  int ret = 0;
  
  gavf_buffer_init(&buf);

  if(!gavf_io_read_buffer(io, &buf))
    goto fail;
  
  gavf_io_init_buf_read(&bufio, &buf);

  if(!gavf_io_read_uint32v(&bufio, &ph->num_streams))
    goto fail;

  ph->streams = calloc(ph->num_streams, sizeof(*ph->streams));

  for(i = 0; i < ph->num_streams; i++)
    {
    if(!gavf_stream_header_read(&bufio, &ph->streams[i]))
      goto fail;
    }

  /* Read metadata */
  if(!gavf_read_metadata(&bufio, &ph->m))
    goto fail;
  
  ret = 1;
  fail:
  
  gavf_buffer_free(&buf);
  
  return ret;
  }

int gavf_program_header_write(gavf_io_t * io,
                              const gavf_program_header_t * ph)
  {
  int i;
  gavf_buffer_t buf;
  gavf_io_t bufio;
  int ret = 0;

  if(gavf_io_write_data(io, (uint8_t*)GAVF_TAG_PROGRAM_HEADER, 8) < 8)
    return 0;
  
  gavf_buffer_init(&buf);
  gavf_io_init_buf_write(&bufio, &buf);

  if(!gavf_io_write_uint32v(&bufio, ph->num_streams))
    goto fail;
  
  for(i = 0; i < ph->num_streams; i++)
    {
    if(!gavf_stream_header_write(&bufio, &ph->streams[i]))
      goto fail;
    }

  /* Write metadata */
  if(!gavf_write_metadata(&bufio, &ph->m))
    goto fail;
  
  if(!gavf_io_write_buffer(io, &buf))
    goto fail;

  ret = 1;
  fail:
  
  gavf_buffer_free(&buf);
  
  return ret;
  }

static gavf_stream_header_t *
add_stream(gavf_program_header_t * ph, const gavl_metadata_t * m)
  {
  gavf_stream_header_t * ret;
  ph->num_streams++;
  
  ph->streams = realloc(ph->streams,
                        ph->num_streams * sizeof(*ph->streams));
  
  ret = &ph->streams[ph->num_streams-1];
  memset(ret, 0, sizeof(*ret));
  gavl_metadata_copy(&ret->m, m);

  ret->id = ph->num_streams;
  return ret;
  }

int gavf_program_header_add_audio_stream(gavf_program_header_t * ph,
                                         const gavl_compression_info_t * ci,
                                         const gavl_audio_format_t * format,
                                         const gavl_metadata_t * m)
  {
  gavf_stream_header_t * h = add_stream(ph, m);

  h->type = GAVF_STREAM_AUDIO;
  
  gavl_compression_info_copy(&h->ci, ci);
  gavl_audio_format_copy(&h->format.audio, format);
  return ph->num_streams-1;
  }

int gavf_program_header_add_video_stream(gavf_program_header_t * ph,
                                         const gavl_compression_info_t * ci,
                                         const gavl_video_format_t * format,
                                         const gavl_metadata_t * m)
  {
  gavf_stream_header_t * h = add_stream(ph, m);
  h->type = GAVF_STREAM_VIDEO;

  gavl_compression_info_copy(&h->ci, ci);
  gavl_video_format_copy(&h->format.video, format);
  if(h->ci.id == GAVL_CODEC_ID_NONE)
    gavl_video_format_set_frame_size(&h->format.video, 0, 0);

  return ph->num_streams-1;
  }

int gavf_program_header_add_text_stream(gavf_program_header_t * ph,
                                        uint32_t timescale,
                                        const gavl_metadata_t * m)
  {
  gavf_stream_header_t * h = add_stream(ph, m);
  h->type = GAVF_STREAM_TEXT;
  h->format.text.timescale = timescale;
  return ph->num_streams-1;
  }

void gavf_program_header_free(gavf_program_header_t * ph)
  {
  int i;
  for(i = 0; i < ph->num_streams; i++)
    gavf_stream_header_free(&ph->streams[i]);

  if(ph->streams)
    free(ph->streams);
  gavl_metadata_free(&ph->m);
  }

void gavf_program_header_dump(gavf_program_header_t * ph)
  {
  int i;
  
  fprintf(stderr, "Program header\n");
  fprintf(stderr, "  Metadata\n");
  gavl_metadata_dump(&ph->m, 4);

  for(i = 0; i < ph->num_streams; i++)
    {
    fprintf(stderr, "  Stream %d\n", i+1);
    gavf_stream_header_dump(&ph->streams[i]);
    }
  
  }
