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

static const struct
  {
  int type;
  const char * name;
  }
type_names[] =
  {
    { GAVF_STREAM_AUDIO, "Audio" },
    { GAVF_STREAM_VIDEO, "Video" },
    { GAVF_STREAM_TEXT,  "Text" },
    { /* End */ },
  };

static const char * get_type_name(int type)
  {
  int i = 0;
  while(type_names[i].name)
    {
    if(type_names[i].type == type)
      return type_names[i].name;
    i++;
    }
  return NULL;
  }

void gavf_stream_header_dump(gavf_stream_header_t * h)
  {
  fprintf(stderr, "    Type: %d (%s)\n", h->type, get_type_name(h->type));
  fprintf(stderr, "    ID:   %d\n", h->id);

  switch(h->type)
    {
    case GAVF_STREAM_AUDIO:
      gavl_compression_info_dump(&h->ci);
      gavl_audio_format_dump(&h->format.audio);
      break;
    case GAVF_STREAM_VIDEO:
      gavl_compression_info_dump(&h->ci);
      gavl_video_format_dump(&h->format.video);
      break;
    case GAVF_STREAM_TEXT:
      fprintf(stderr, "    Timescale: %d\n", h->format.text.timescale);
      break;
    }
  
  }
