#include <gavfprivate.h>

int gavf_stream_header_read(gavf_io_t * io, gavf_stream_header_t * h)
  {
  if(!gavf_io_read_uint32v(io, &h->type) ||
     !gavf_io_read_uint32v(io, &h->id))
    return 0;

  if(!gavf_read_metadata(io, &h->m))
    return 0;
  
  switch(h->type)
    {
    case GAVF_STREAM_AUDIO:
      if(!gavf_read_compression_info(io, &h->ci) ||
         !gavf_read_audio_format(io, &h->format.audio))
        return 0;
      break;
    case GAVF_STREAM_VIDEO:
      if(!gavf_read_compression_info(io, &h->ci) ||
         !gavf_read_video_format(io, &h->format.video))
        return 0;
      break;
    case GAVF_STREAM_TEXT:
      if(!gavf_io_read_uint32v(io, &h->format.text.timescale))
        return 0;
      break;
    }
  return 1;
  }

int gavf_stream_header_write(gavf_io_t * io, const gavf_stream_header_t * h)
  {
  if(!gavf_io_write_uint32v(io, h->type) ||
     !gavf_io_write_uint32v(io, h->id))
    return 0;

  if(!gavf_write_metadata(io, &h->m))
    return 0;
  
  switch(h->type)
    {
    case GAVF_STREAM_AUDIO:
      if(!gavf_write_compression_info(io, &h->ci) ||
         !gavf_write_audio_format(io, &h->format.audio))
        return 0;
      break;
    case GAVF_STREAM_VIDEO:
      if(!gavf_write_compression_info(io, &h->ci) ||
         !gavf_write_video_format(io, &h->format.video))
        return 0;
      break;
    case GAVF_STREAM_TEXT:
      if(!gavf_io_write_uint32v(io, h->format.text.timescale))
        return 0;
      break;
    }
  return 1;
  }

void gavf_stream_header_free(gavf_stream_header_t * h)
  {
  gavl_compression_info_free(&h->ci);
  gavl_metadata_free(&h->m);
  }
