#include <stdlib.h>
#include <string.h>


#include <gavfprivate.h>

/* Formats */

#define MAX_EXT_SIZE_AF 16
#define MAX_EXT_SIZE_VF 32
#define MAX_EXT_SIZE_PK 16
#define MAX_EXT_SIZE_CI 16

int gavf_read_audio_format(gavf_io_t * io, gavl_audio_format_t * format)
  {
  int i;
  uint32_t num_extensions;
  
  gavl_extension_header_t eh;
  
  if(!gavf_io_read_uint32v(io, &format->samplerate) ||
     !gavf_io_read_uint32v(io, &format->num_channels))
    return 0;

  for(i = 0; i < format->num_channels; i++)
    {
    if(!gavf_io_read_uint32v(io, &format->channel_locations[i]))
      return 0;
    }

  if(!gavf_io_read_uint32v(io, &num_extensions))
    return 0;

  for(i = 0; i < num_extensions; i++)
    {
    if(!gavf_extension_header_read(io, &eh))
      return 0;
    switch(eh.key)
      {
      case GAVF_EXT_AF_SAMPLESPERFRAME:
        if(!gavf_io_read_uint32v(io, &format->samples_per_frame))
          return 0;
        break;
      case GAVF_EXT_AF_SAMPLEFORMAT:
        if(!gavf_io_read_uint32v(io, &format->sample_format))
          return 0;
        break;
      case GAVF_EXT_AF_INTERLEAVE:
        if(!gavf_io_read_uint32v(io, &format->interleave_mode))
          return 0;
        break;
      case GAVF_EXT_AF_CENTER_LEVEL:
        if(!gavf_io_read_float(io, &format->center_level))
          return 0;
        break;
      case GAVF_EXT_AF_REAR_LEVEL:
        if(!gavf_io_read_float(io, &format->rear_level))
          return 0;
        break;
      default:
        /* Skip */
        gavf_io_skip(io, eh.len);
        break;
      }
    }
  return 1;
  }

int gavf_write_audio_format(gavf_io_t * io, const gavl_audio_format_t * format)
  {
  int num_extensions;
  uint8_t data[MAX_EXT_SIZE_AF];
  gavf_buffer_t buf;
  gavf_io_t bufio;
  int i;
  
  /* Write common stuff */
  if(!gavf_io_write_uint32v(io, format->samplerate) ||
     !gavf_io_write_uint32v(io, format->num_channels))
    return 0;
  
  for(i = 0; i < format->num_channels; i++)
    {
    if(!gavf_io_write_uint32v(io, format->channel_locations[i]))
      return 0;
    }

  /* Count extensions */
  num_extensions = 0;

  if(format->samples_per_frame != 0)
    num_extensions++;

  if(format->interleave_mode != GAVL_INTERLEAVE_NONE)
    num_extensions++;

  if(format->sample_format != GAVL_SAMPLE_NONE)
    num_extensions++;

  if(format->center_level != 0.0)
    num_extensions++;

  if(format->rear_level != 0.0)
    num_extensions++;
  
  /* Write extensions */
  if(!gavf_io_write_uint32v(io, num_extensions))
    return 0;

  if(!num_extensions)
    return 1;

  gavf_buffer_init_static(&buf, data, MAX_EXT_SIZE_AF);
  gavf_io_init_buf_write(&bufio, &buf);
  
  if(format->samples_per_frame != 0)
    {
    buf.len = 0;
    if(!gavf_io_write_uint32v(&bufio, format->samples_per_frame) ||
       !gavf_extension_write(io, GAVF_EXT_AF_SAMPLESPERFRAME,
                            buf.len, buf.buf))
      return 0;
    }
  if(format->interleave_mode != GAVL_INTERLEAVE_NONE)
    {
    buf.len = 0;
    if(!gavf_io_write_uint32v(&bufio, format->interleave_mode) ||
       !gavf_extension_write(io, GAVF_EXT_AF_INTERLEAVE,
                            buf.len, buf.buf))
      return 0;
    }
  if(format->sample_format != GAVL_SAMPLE_NONE)
    {
    buf.len = 0;
    if(!gavf_io_write_uint32v(&bufio, format->sample_format) ||
       !gavf_extension_write(io, GAVF_EXT_AF_SAMPLEFORMAT,
                            buf.len, buf.buf))
      return 0;
    }
  if(format->center_level != 0.0)
    {
    buf.len = 0;
    if(!gavf_io_write_float(&bufio, format->center_level) ||
       !gavf_extension_write(io, GAVF_EXT_AF_CENTER_LEVEL,
                            buf.len, buf.buf))
      return 0;
    }
  if(format->rear_level != 0.0)
    {
    buf.len = 0;
    if(!gavf_io_write_float(&bufio, format->rear_level) ||
       !gavf_extension_write(io, GAVF_EXT_AF_REAR_LEVEL,
                            buf.len, buf.buf))
      return 0;
    }
  
  
  return 1;
  }

int gavf_read_video_format(gavf_io_t * io, gavl_video_format_t * format)
  {
  int i;
  uint32_t num_extensions;
  
  gavl_extension_header_t eh;

  memset(format, 0, sizeof(*format));
  
  /* Read mandatory stuff */
  if(!gavf_io_read_uint32v(io, &format->image_width) ||
     !gavf_io_read_uint32v(io, &format->image_height) ||
     !gavf_io_read_int32v(io, &format->framerate_mode) ||
     !gavf_io_read_uint32v(io, &format->timescale))
    return 0;

  if(format->framerate_mode != GAVL_FRAMERATE_STILL)
    {
    if(!gavf_io_read_uint32v(io, &format->frame_duration))
      return 0;
    }

  /* Set defaults */
  format->pixel_width = 1;
  format->pixel_height = 1;
  
  /* Read extensions */
  if(!gavf_io_read_uint32v(io, &num_extensions))
    return 0;

  for(i = 0; i < num_extensions; i++)
    {
    if(!gavf_extension_header_read(io, &eh))
      return 0;
    switch(eh.key)
      {
      case GAVF_EXT_VF_PIXELFORMAT:
        if(!gavf_io_read_uint32v(io, &format->pixelformat))
          return 0;
        break;
      case GAVF_EXT_VF_PIXEL_ASPECT:
        if(!gavf_io_read_uint32v(io, &format->pixel_width) ||
           !gavf_io_read_uint32v(io, &format->pixel_height))
          return 0;
        break;
      case GAVF_EXT_VF_INTERLACE:
        if(!gavf_io_read_int32v(io, &format->interlace_mode))
          return 0;
        break;
      case GAVF_EXT_VF_FRAME_SIZE:
        if(!gavf_io_read_uint32v(io, &format->frame_width) ||
           !gavf_io_read_uint32v(io, &format->frame_height))
          return 0;
        break;
      default:
        /* Skip */
        gavf_io_skip(io, eh.len);
        break;
        
      }
    }

  if(!format->frame_width || !format->frame_height)
    gavl_video_format_set_frame_size(format, 0, 0);
  
  return 1;
  }

int gavf_write_video_format(gavf_io_t * io, const gavl_video_format_t * format)
  {
  int num_extensions;
  uint8_t data[MAX_EXT_SIZE_VF];
  gavf_buffer_t buf;
  gavf_io_t bufio;

  /* Write mandatory stuff */
  if(!gavf_io_write_uint32v(io, format->image_width) ||
     !gavf_io_write_uint32v(io, format->image_height) ||
     !gavf_io_write_int32v(io, format->framerate_mode) ||
     !gavf_io_write_uint32v(io, format->timescale))
    return 0;

  if(format->framerate_mode != GAVL_FRAMERATE_STILL)
    {
    if(!gavf_io_write_uint32v(io, format->frame_duration))
      return 0;
    }

  /* Count extensions */
  num_extensions = 0;

  if(format->pixelformat != GAVL_PIXELFORMAT_NONE)
    num_extensions++;
  
  if(format->pixel_width != format->pixel_height)
    num_extensions++;
    
  if(format->interlace_mode != GAVL_INTERLACE_NONE)
    num_extensions++;

  if((format->image_width != format->frame_width) ||
     (format->frame_width != format->frame_height))
    num_extensions++;

  /* Write extensions */

  if(!gavf_io_write_uint32v(io, num_extensions))
    return 0;

  if(!num_extensions)
    return 1;

  gavf_buffer_init_static(&buf, data, MAX_EXT_SIZE_VF);
  gavf_io_init_buf_write(&bufio, &buf);

  if(format->pixelformat != GAVL_PIXELFORMAT_NONE)
    {
    buf.len = 0;
    if(!gavf_io_write_uint32v(&bufio, format->pixelformat) ||
       !gavf_extension_write(io, GAVF_EXT_VF_PIXELFORMAT,
                            buf.len, buf.buf))
      return 0;
    }
  
  if(format->pixel_width != format->pixel_height)
    {
    buf.len = 0;
    if(!gavf_io_write_uint32v(&bufio, format->pixel_width) ||
       !gavf_io_write_uint32v(&bufio, format->pixel_height) ||
       !gavf_extension_write(io, GAVF_EXT_VF_PIXEL_ASPECT,
                            buf.len, buf.buf))
      return 0;
    }
  
  if(format->interlace_mode != GAVL_INTERLACE_NONE)
    {
    buf.len = 0;
    if(!gavf_io_write_uint32v(&bufio, format->interlace_mode) ||
       !gavf_extension_write(io, GAVF_EXT_VF_INTERLACE,
                             buf.len, buf.buf))
      return 0;
    }

  if((format->image_width != format->frame_width) ||
     (format->frame_width != format->frame_height))
    {
    buf.len = 0;
    if(!gavf_io_write_uint32v(&bufio, format->frame_width) ||
       !gavf_io_write_uint32v(&bufio, format->frame_height) ||
       !gavf_extension_write(io, GAVF_EXT_VF_FRAME_SIZE,
                             buf.len, buf.buf))
      return 0;
    }
  
  return 1;
  }

/* Compression info */

int gavf_read_compression_info(gavf_io_t * io,
                               gavl_compression_info_t * ci)
  {
  int i;
  uint32_t num_extensions;
  
  gavl_extension_header_t eh;

  /* Read mandatory stuff */
  if(!gavf_io_read_uint32v(io, &ci->flags) ||
     !gavf_io_read_uint32v(io, &ci->id))
    return 0;

  /* Read extensions */
  if(!gavf_io_read_uint32v(io, &num_extensions))
    return 0;
  
  for(i = 0; i < num_extensions; i++)
    {
    if(!gavf_extension_header_read(io, &eh))
      return 0;
    switch(eh.key)
      {
      case GAVF_EXT_CI_GLOBAL_HEADER:
        ci->global_header_len = eh.len;
        ci->global_header = malloc(ci->global_header_len);
        if(gavf_io_read_data(io, ci->global_header,
                             ci->global_header_len) < ci->global_header_len)
          return 0;
        break;
      case GAVF_EXT_CI_BITRATE:
        if(!gavf_io_read_int32v(io, &ci->bitrate))
          return 0;
        break;
      }
    }
  
  
  return 1;
  }

int gavf_write_compression_info(gavf_io_t * io,
                                const gavl_compression_info_t * ci)
  {
  uint32_t num_extensions;
  uint8_t data[MAX_EXT_SIZE_CI];
  gavf_buffer_t buf;
  gavf_io_t bufio;

  
  /* Read mandatory stuff */
  if(!gavf_io_write_uint32v(io, ci->flags) ||
     !gavf_io_write_uint32v(io, ci->id))
    return 0;

  /* Count extensions */
  num_extensions = 0;

  if(ci->global_header_len)
    num_extensions++;

  if(ci->bitrate)
    num_extensions++;
  
  /* Write extensions */
  if(!gavf_io_write_uint32v(io, num_extensions))
    return 0;

  if(!num_extensions)
    return 1;

  gavf_buffer_init_static(&buf, data, MAX_EXT_SIZE_AF);
  gavf_io_init_buf_write(&bufio, &buf);
  
  if(ci->global_header_len)
    {
    if(!gavf_extension_write(io, GAVF_EXT_CI_GLOBAL_HEADER,
                             ci->global_header_len,
                             ci->global_header))
      return 0;
    }

  if(ci->bitrate)
    {
    buf.len = 0;
    if(!gavf_io_write_int32v(&bufio, ci->bitrate) ||
       !gavf_extension_write(io, GAVF_EXT_CI_BITRATE,
                             buf.len, buf.buf))
      return 0;
    }
  
  return 1;
  }

/* Metadata */

int gavf_read_metadata(gavf_io_t * io, gavl_metadata_t * m)
  {
  int i;
  uint32_t num;
  if(!gavf_io_read_uint32v(io, &num))
    return 0;

  m->num_tags = num;
  m->tags_alloc = num;
  
  m->tags = calloc(m->num_tags, sizeof(*m->tags));
  
  for(i = 0; i < m->num_tags; i++)
    {
    if(!gavf_io_read_string(io, &m->tags[i].key) ||
       !gavf_io_read_string(io, &m->tags[i].val))
      return 0;
    }
  return 1;
  }

int gavf_write_metadata(gavf_io_t * io, const gavl_metadata_t * m)
  {
  int i;
  if(!gavf_io_write_uint32v(io, m->num_tags))
    return 0;

  for(i = 0; i < m->num_tags; i++)
    {
    if(!gavf_io_write_string(io, m->tags[i].key) ||
       !gavf_io_write_string(io, m->tags[i].val))
      return 0;
    }
  return 1;
  }

/* Packet */

int gavf_read_gavl_packet(gavf_io_t * io,
                          gavf_stream_t * s,
                          gavl_packet_t * p)
  {
  int i;
  uint32_t num_extensions;
  gavl_extension_header_t eh;

  gavl_packet_reset(p);
  
  /* Flags */
  if(!gavf_io_read_uint32v(io, (uint32_t*)&p->flags))
    return 0;
  
  /* PTS */
  if(s->has_pts)
    {
    if(!gavf_io_read_int64v(io, &p->pts))
      return 0;
    p->pts += s->last_sync_pts;
    }
  else
    p->pts = s->next_pts;
  
  /* Duration */
  if(!s->packet_duration)
    {
    if(!gavf_io_read_int64v(io, &p->duration))
      return 0;
    }
  else
    p->duration = s->packet_duration;

  /* Field2 */
  if(s->h->ci.flags & GAVL_COMPRESSION_HAS_FIELD_PICTURES)
    {
    if(!gavf_io_read_uint32v(io, &p->field2_offset))
      return 0;
    }
  
  /* Extensions */
  if(!gavf_io_read_uint32v(io, &num_extensions))
    return 0;

  for(i = 0; i < num_extensions; i++)
    {
    if(!gavf_extension_header_read(io, &eh))
      return 0;
    switch(eh.key)
      {
      case GAVF_EXT_PK_DURATION:
        if(!gavf_io_read_int64v(io, &p->duration))
          return 0;
        break;
      case GAVF_EXT_PK_HEADER_SIZE:
        if(!gavf_io_read_uint32v(io, &p->header_size))
          return 0;
        break;
      default:
        /* Skip */
        gavf_io_skip(io, eh.len);
        break;
      }
    }
  
  /* Set pts */
  s->next_pts += p->duration;
  
  /* Payload */
  if(!gavf_io_read_uint32v(io, (uint32_t*)&p->data_len))
    return 0;
  gavl_packet_alloc(p, p->data_len);
  if(gavf_io_read_data(io, p->data, p->data_len) < p->data_len)
    return 0;
  
  return 1;
  }

int gavf_write_gavl_packet(gavf_io_t * io,
                           gavf_stream_t * s,
                           const gavl_packet_t * p)
  {
  uint32_t num_extensions;

  uint8_t data[MAX_EXT_SIZE_PK];
  gavf_buffer_t buf;
  gavf_io_t bufio;

  uint32_t data_len;
  uint32_t sequence_end_pos;
  uint32_t field2_offset;
  uint32_t header_size;
  
  const uint8_t * data_ptr;

  data_len         = p->data_len;
  sequence_end_pos = p->sequence_end_pos;
  field2_offset    = p->field2_offset;
  header_size      = p->header_size;
  data_ptr         = p->data;
  
  /* Check if we can remove redundant headers */
  if(header_size)
    {
    if((header_size == s->h->ci.global_header_len) &&
       !memcmp(p->data, s->h->ci.global_header, header_size))
      {
      data_len -= header_size;
      
      if(field2_offset)
        field2_offset -= header_size;

      if(sequence_end_pos)
        sequence_end_pos -= header_size;
      
      data_ptr += header_size;
      
      header_size = 0;
      }
    }
  
  /* Flags */
  if(!gavf_io_write_uint32v(io, p->flags))
    return 0;
    
  /* PTS */
  if(s->has_pts)
    {
    if(!gavf_io_write_int64v(io, p->pts - s->last_sync_pts))
      return 0;
    }
  
  /* Duration */
  if(!s->packet_duration)
    {
    if(!gavf_io_write_int64v(io, p->duration))
      return 0;
    }

  /* Field2 */
  if(s->h->ci.flags & GAVL_COMPRESSION_HAS_FIELD_PICTURES)
    {
    if(!gavf_io_write_uint32v(io, p->field2_offset))
      return 0;
    }
    
  /* Count Extensions */

  num_extensions = 0;
  
  if(s->packet_duration && (p->duration < s->packet_duration))
    num_extensions++;

  if(header_size)
    num_extensions++;
  
  /* Write Extensions */
  if(!gavf_io_write_uint32v(io, num_extensions))
    return 0;

  if(num_extensions)
    {
    gavf_buffer_init_static(&buf, data, MAX_EXT_SIZE_AF);
    gavf_io_init_buf_write(&bufio, &buf);
    
    if(s->packet_duration && (p->duration < s->packet_duration))
      {
      buf.len = 0;
      if(!gavf_io_write_int64v(&bufio, p->duration) ||
         !gavf_extension_write(io, GAVF_EXT_PK_DURATION,
                               buf.len, buf.buf))
        return 0;
      }

    if(header_size)
      {
      buf.len = 0;
      if(!gavf_io_write_uint32v(&bufio, header_size) ||
         !gavf_extension_write(io, GAVF_EXT_PK_HEADER_SIZE,
                               buf.len, buf.buf))
        return 0;
      }
    
    }
  
  /* Payload */
  if(!gavf_io_write_uint32v(io, data_len) ||
     !gavf_io_write_data(io, data_ptr, data_len))
    return 0;
  return 1;
  }
