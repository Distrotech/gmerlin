#include <stdlib.h>
#include <stdio.h>

#include <config.h>
#include <gavl.h>

/* Taken from a52dec (thanks guys) */

#ifdef HAVE_MEMALIGN
/* some systems have memalign() but no declaration for it */
void * memalign (size_t align, size_t size);
#else
/* assume malloc alignment is sufficient */
#define memalign(align,size) malloc (size)
#endif

#define ALIGNMENT_BYTES 8

gavl_audio_frame_t *
gavl_audio_frame_create(const gavl_audio_format_t * format)
  {
  gavl_audio_frame_t * ret;
  int num_samples;
  int i;
  ret = calloc(1, sizeof(*ret));
  
  num_samples = ALIGNMENT_BYTES *
    ((format->samples_per_frame + ALIGNMENT_BYTES - 1) / ALIGNMENT_BYTES);
  
  switch(format->sample_format)
    {
    case GAVL_SAMPLE_U8:
      ret->samples.u_8 =
        memalign(ALIGNMENT_BYTES, num_samples * format->num_channels);

      ret->channels.u_8 = calloc(format->num_channels, sizeof(uint8_t*));
      for(i = 0; i < format->num_channels; i++)
        ret->channels.u_8[i] = &(ret->samples.u_8[i*num_samples]);

      break;
    case GAVL_SAMPLE_S8:
      ret->samples.s_8 =
        memalign(ALIGNMENT_BYTES, num_samples * format->num_channels);

      ret->channels.s_8 = calloc(format->num_channels, sizeof(int8_t*));
      for(i = 0; i < format->num_channels; i++)
        ret->channels.s_8[i] = &(ret->samples.s_8[i*num_samples]);

      break;
    case GAVL_SAMPLE_U16LE:
    case GAVL_SAMPLE_U16BE:
      ret->samples.u_16 =
        memalign(ALIGNMENT_BYTES, 2 * num_samples * format->num_channels);

      ret->channels.u_16 = calloc(format->num_channels, sizeof(uint16_t*));
      for(i = 0; i < format->num_channels; i++)
        ret->channels.u_16[i] = &(ret->samples.u_16[i*num_samples]);

      break;
    case GAVL_SAMPLE_S16LE:
    case GAVL_SAMPLE_S16BE:
      ret->samples.s_16 =
        memalign(ALIGNMENT_BYTES, 2 * num_samples * format->num_channels);
      ret->channels.s_16 = calloc(format->num_channels, sizeof(int16_t*));
      for(i = 0; i < format->num_channels; i++)
        ret->channels.s_16[i] = &(ret->samples.s_16[i*num_samples]);

      break;
    case GAVL_SAMPLE_FLOAT:
      ret->samples.f =
        memalign(ALIGNMENT_BYTES, sizeof(float) * num_samples * format->num_channels);

      ret->channels.f = calloc(format->num_channels, sizeof(float*));
      for(i = 0; i < format->num_channels; i++)
        ret->channels.f[i] = &(ret->samples.f[i*num_samples]);

      break;
    case GAVL_SAMPLE_NONE:
      {
      fprintf(stderr, "Sample format not specified for audio frame\n");
      return ret;
      }
    }
  return ret;
  }

void gavl_audio_frame_destroy(gavl_audio_frame_t * frame)
  {
  free(frame->samples.s_8);
  free(frame->channels.s_8);
  free(frame);
  }

void gavl_audio_frame_mute(gavl_audio_frame_t * frame,
                           const gavl_audio_format_t * format)
  {
  int i;
  int imax;
  imax = format->num_channels * format->samples_per_frame;
  
  switch(format->sample_format)
    {
    case GAVL_SAMPLE_NONE:
      break;
    case GAVL_SAMPLE_U8:
      for(i = 0; i < imax; i++)
        frame->samples.u_8[i] = 0x80;
      break;
    case GAVL_SAMPLE_S8:
      for(i = 0; i < imax; i++)
        frame->samples.u_8[i] = 0x00;
      break;
    case GAVL_SAMPLE_U16NE:
      for(i = 0; i < imax; i++)
        frame->samples.u_16[i] = 0x8000;
      break;
    case GAVL_SAMPLE_U16OE:
      for(i = 0; i < imax; i++)
        frame->samples.u_16[i] = 0x0080;
      break;
    case GAVL_SAMPLE_S16NE:
    case GAVL_SAMPLE_S16OE:
      for(i = 0; i < imax; i++)
        frame->samples.s_16[i] = 0x0000;
      break;
    case GAVL_SAMPLE_FLOAT:
      for(i = 0; i < imax; i++)
        frame->samples.f[i] = 0.0;
      break;
    }
  frame->valid_samples = format->samples_per_frame;
  }
