#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gavl.h>
#include <audio.h>

struct gavl_audio_buffer_s
  {
  gavl_audio_format_t format;
  
  void (*copy_samples)(gavl_audio_buffer_t * buf,
                       gavl_audio_frame_t* in, gavl_audio_frame_t* out,
                       int num_samples);

  int in_samples_start;
  int bytes_per_sample;
  };

/* Copy routines */

static void copy_samples_all(gavl_audio_buffer_t * buf,
                             gavl_audio_frame_t* in,
                             gavl_audio_frame_t* out,
                             int num_samples)
  {
  int offset_factor = buf->bytes_per_sample*buf->format.num_channels;
    
  memcpy(&(out->samples.s_8[out->valid_samples*offset_factor]),
         &(in->samples.s_8[buf->in_samples_start*offset_factor]),
         num_samples * offset_factor);
  }

static void copy_samples_2(gavl_audio_buffer_t * buf,
                           gavl_audio_frame_t* in,
                           gavl_audio_frame_t* out,
                           int num_samples)
  {
  int i;
  int offset_factor = buf->bytes_per_sample*2;

  for(i = 0; i < buf->format.num_channels/2; i++)
    {
    memcpy(&(out->channels.s_8[i*2][out->valid_samples*offset_factor]),
           &(in->channels.s_8[i*2][buf->in_samples_start*offset_factor]),
           num_samples * offset_factor);
    }

  /* Last odd channel is not interleaved */

  if(buf->format.num_channels % 2)
    {
    memcpy(&(out->channels.s_8[buf->format.num_channels/2][out->valid_samples*offset_factor]),
           &(in->channels.s_8[buf->format.num_channels][buf->in_samples_start*offset_factor]),
           num_samples * buf->bytes_per_sample);
    }
  
  }

static void copy_samples_none(gavl_audio_buffer_t * buf,
                              gavl_audio_frame_t* in,
                              gavl_audio_frame_t* out,
                              int num_samples)
  {
  int i;
  
  for(i = 0; i < buf->format.num_channels; i++)
    {
    memcpy(&(out->channels.s_8[i][out->valid_samples*buf->bytes_per_sample]),
           &(in->channels.s_8[i][buf->in_samples_start*buf->bytes_per_sample]),
           num_samples * buf->bytes_per_sample);
    }

  
  }

gavl_audio_buffer_t * gavl_create_audio_buffer(gavl_audio_format_t * format)
  {
  gavl_audio_buffer_t * ret = calloc(1, sizeof(*ret));

  memcpy(&(ret->format), format, sizeof(gavl_audio_format_t));

  ret->bytes_per_sample = gavl_bytes_per_sample(format->sample_format);
  
  switch(format->interleave_mode)
    {
    case GAVL_INTERLEAVE_NONE: /* No interleaving, all channels separate */
      ret->copy_samples = copy_samples_none;
      break;
    case GAVL_INTERLEAVE_2:    /* Interleaved pairs of channels          */ 
      ret->copy_samples = copy_samples_2;
      break;
    case GAVL_INTERLEAVE_ALL:  /* Everything interleaved                 */
      ret->copy_samples = copy_samples_all;
      break;
    }
  return ret;
  }


void gavl_destroy_audio_buffer(gavl_audio_buffer_t * b)
  {
  free(b);
  }

int gavl_buffer_audio(gavl_audio_buffer_t * b,
                      gavl_audio_frame_t * in, gavl_audio_frame_t * out)
  {
  /* Check if output buffer gets full */

  int samples_available;
  int samples_needed;
  int samples_to_copy;
  
  samples_available = in->valid_samples - b->in_samples_start;
  samples_needed    = b->format.samples_per_frame - out->valid_samples;

  samples_to_copy =
    (samples_available >= samples_needed) ? samples_needed : samples_available;
  b->copy_samples(b, in, out, samples_to_copy);

  out->valid_samples += samples_to_copy;
    
  b->in_samples_start += samples_to_copy;

  /* Output frame gets full */
  
  if(b->in_samples_start >= in->valid_samples)
    {
    b->in_samples_start = 0;
    return 1;
    }
  else
    return 0;
  }
