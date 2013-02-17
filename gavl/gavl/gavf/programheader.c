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
  
  if(io->cb && !io->cb(io->cb_priv, GAVF_IO_CB_PROGRAM_HEADER, ph))
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
  
  if(!gavf_io_write_buffer(io, &buf) ||
     !gavf_io_flush(io))
    goto fail;
  
  if(io->cb && !io->cb(io->cb_priv, GAVF_IO_CB_PROGRAM_HEADER, ph))
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

  /* Initialize footer */
  ret->foot.duration_min = GAVL_TIME_UNDEFINED;
  ret->foot.duration_max = GAVL_TIME_UNDEFINED;
  ret->foot.pts_start    = GAVL_TIME_UNDEFINED;
  ret->foot.pts_end      = GAVL_TIME_UNDEFINED;
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

int gavf_program_header_add_overlay_stream(gavf_program_header_t * ph,
                                           const gavl_compression_info_t * ci,
                                           const gavl_video_format_t * format,
                                           const gavl_metadata_t * m)
  {
  gavf_stream_header_t * h = add_stream(ph, m);
  h->type = GAVF_STREAM_OVERLAY;
  
  gavl_compression_info_copy(&h->ci, ci);
  gavl_video_format_copy(&h->format.video, format);

  /* Correct the video format */
  h->format.video.framerate_mode = GAVL_FRAMERATE_VARIABLE;
  
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
  int i, num;
  const gavf_stream_header_t * h;
  
  fprintf(stderr, "Program header\n");
  fprintf(stderr, "  Metadata\n");
  gavl_metadata_dump(&ph->m, 4);

  num = gavf_program_header_get_num_streams(ph, GAVF_STREAM_AUDIO);
  for(i = 0; i < num; i++)
    {
    h = gavf_program_header_get_stream(ph, i, GAVF_STREAM_AUDIO);
    fprintf(stderr, "  Audio stream %d\n", i+1);
    gavf_stream_header_dump(h);
    }

  num = gavf_program_header_get_num_streams(ph, GAVF_STREAM_VIDEO);
  for(i = 0; i < num; i++)
    {
    h = gavf_program_header_get_stream(ph, i, GAVF_STREAM_VIDEO);
    fprintf(stderr, "  Video stream %d\n", i+1);
    gavf_stream_header_dump(h);
    }

  num = gavf_program_header_get_num_streams(ph, GAVF_STREAM_TEXT);
  for(i = 0; i < num; i++)
    {
    h = gavf_program_header_get_stream(ph, i, GAVF_STREAM_TEXT);
    fprintf(stderr, "  Text stream %d\n", i+1);
    gavf_stream_header_dump(h);
    }

  num = gavf_program_header_get_num_streams(ph, GAVF_STREAM_OVERLAY);
  for(i = 0; i < num; i++)
    {
    h = gavf_program_header_get_stream(ph, i, GAVF_STREAM_OVERLAY);
    fprintf(stderr, "  Overlay stream %d\n", i+1);
    gavf_stream_header_dump(h);
    }
  
  }

int
gavf_program_header_get_num_streams(const gavf_program_header_t * ph,
                                    int type)
  {
  int i;
  int ret = 0;

  for(i = 0; i < ph->num_streams; i++)
    {
    if(ph->streams[i].type == type)
      ret++;
    }
  return ret;
  }

const gavf_stream_header_t *
gavf_program_header_get_stream(const gavf_program_header_t * ph,
                               int index, int type)
  {
  int i;
  int idx = 0;
  
  for(i = 0; i < ph->num_streams; i++)
    {
    if(ph->streams[i].type == type)
      {
      if(idx == index)
        return &ph->streams[i];
      idx++;
      }
    }
  return NULL;
  }

