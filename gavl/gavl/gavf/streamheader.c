#include <gavfprivate.h>

void gavf_stream_header_init_audio(gavf_stream_header_t * h)
  {
  h->timescale = h->format.audio.samplerate;

  /* Figure out the samples per frame */
  if(gavl_compression_constant_frame_samples(h->ci.id))
    h->packet_duration = h->format.audio.samples_per_frame;
  }

void gavf_stream_header_init_video(gavf_stream_header_t * h)
  {
  h->timescale = h->format.video.timescale;

  if(h->ci.flags & GAVL_COMPRESSION_HAS_B_FRAMES)
    h->has_pts = 1;
  
  if(h->format.video.framerate_mode == GAVL_FRAMERATE_CONSTANT)
    h->packet_duration = h->format.video.frame_duration;
  }

void gavf_stream_header_init_text(gavf_stream_header_t * h)
  {
  h->timescale = h->format.text.timescale;
  h->has_pts = 1;
  }

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
      gavf_stream_header_init_audio(h);
      break;
    case GAVF_STREAM_VIDEO:
      if(!gavf_read_compression_info(io, &h->ci) ||
         !gavf_read_video_format(io, &h->format.video))
        return 0;
      gavf_stream_header_init_video(h);
      
      break;
    case GAVF_STREAM_TEXT:
      if(!gavf_io_read_uint32v(io, &h->format.text.timescale))
        return 0;
      gavf_stream_header_init_text(h);
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
