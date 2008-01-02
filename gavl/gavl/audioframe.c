/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <config.h>
#include <gavl.h>
#include <accel.h>
#include <bswap.h>

/* Taken from a52dec (thanks guys) */

#ifdef HAVE_MEMALIGN
/* some systems have memalign() but no declaration for it */
void * memalign (size_t align, size_t size);
#else
/* assume malloc alignment is sufficient */
#define memalign(align,size) malloc (size)
#endif

#define ALIGNMENT_BYTES 16

gavl_audio_frame_t *
gavl_audio_frame_create(const gavl_audio_format_t * format)
  {
  gavl_audio_frame_t * ret;
  int num_samples;
  int i;
  ret = calloc(1, sizeof(*ret));

  if(!format)
    return ret;
  
  num_samples = ALIGNMENT_BYTES *
    ((format->samples_per_frame + ALIGNMENT_BYTES - 1) / ALIGNMENT_BYTES);
  
  switch(format->sample_format)
    {
    case GAVL_SAMPLE_U8:
      ret->channel_stride = num_samples;
      ret->samples.u_8 =
        memalign(ALIGNMENT_BYTES, num_samples * format->num_channels);

      for(i = 0; i < format->num_channels; i++)
        ret->channels.u_8[i] = &(ret->samples.u_8[i*num_samples]);

      break;
    case GAVL_SAMPLE_S8:
      ret->channel_stride = num_samples;
      ret->samples.s_8 =
        memalign(ALIGNMENT_BYTES, num_samples * format->num_channels);

      for(i = 0; i < format->num_channels; i++)
        ret->channels.s_8[i] = &(ret->samples.s_8[i*num_samples]);

      break;
    case GAVL_SAMPLE_U16:
      ret->channel_stride = num_samples * 2;
      ret->samples.u_16 =
        memalign(ALIGNMENT_BYTES, 2 * num_samples * format->num_channels);
      for(i = 0; i < format->num_channels; i++)
        ret->channels.u_16[i] = &(ret->samples.u_16[i*num_samples]);

      break;
    case GAVL_SAMPLE_S16:
      ret->channel_stride = num_samples * 2;
      ret->samples.s_16 =
        memalign(ALIGNMENT_BYTES, 2 * num_samples * format->num_channels);
      for(i = 0; i < format->num_channels; i++)
        ret->channels.s_16[i] = &(ret->samples.s_16[i*num_samples]);

      break;

    case GAVL_SAMPLE_S32:
      ret->channel_stride = num_samples * 4;
      ret->samples.s_32 =
        memalign(ALIGNMENT_BYTES, 4 * num_samples * format->num_channels);
      for(i = 0; i < format->num_channels; i++)
        ret->channels.s_32[i] = &(ret->samples.s_32[i*num_samples]);

      break;

    case GAVL_SAMPLE_FLOAT:
      ret->channel_stride = num_samples * sizeof(float);
      ret->samples.f =
        memalign(ALIGNMENT_BYTES, sizeof(float) * num_samples * format->num_channels);

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
  if(frame->samples.s_8)
    free(frame->samples.s_8);
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
    case GAVL_SAMPLE_U16:
      for(i = 0; i < imax; i++)
        frame->samples.u_16[i] = 0x8000;
      break;
    case GAVL_SAMPLE_S16:
      for(i = 0; i < imax; i++)
        frame->samples.s_16[i] = 0x0000;
      break;
    case GAVL_SAMPLE_S32:
      for(i = 0; i < imax; i++)
        frame->samples.s_32[i] = 0x00000000;
      break;
    case GAVL_SAMPLE_FLOAT:
      for(i = 0; i < imax; i++)
        frame->samples.f[i] = 0.0;
      break;
    }
  frame->valid_samples = format->samples_per_frame;
  }

static void do_swap_16(uint8_t * data, int len)
  {
  int i;
  uint16_t * ptr = (uint16_t*)data;

  for(i = 0; i < len; i++)
    {
    *ptr = bswap_16(*ptr);
    ptr++;
    }
  
  }

static void do_swap_32(uint8_t * data, int len)
  {
  int i;
  uint32_t * ptr = (uint32_t*)data;
  for(i = 0; i < len; i++)
    {
    *ptr = bswap_32(*ptr);
    ptr++;
    }
  }

static void do_swap_64(uint8_t * data, int len)
  {
  int i;
  uint64_t * ptr = (uint64_t*)data;
  for(i = 0; i < len; i++)
    {
    *ptr = bswap_64(*ptr);
    ptr++;
    }
  }

void gavl_audio_frame_swap_endian(gavl_audio_frame_t * frame,
                                  const gavl_audio_format_t * format)
  {
  int bytes_per_sample, len, i;
  void (*do_swap)(uint8_t * data, int len);
  
  bytes_per_sample = gavl_bytes_per_sample(format->sample_format);
  
  switch(bytes_per_sample)
    {
    case 1:
      return;
      break;
    case 2:
      do_swap = do_swap_16;
      break;
    case 4:
      do_swap = do_swap_32;
      break;
    case 8:
      do_swap = do_swap_64;
      break;
    default:
      return;
    }
  switch(format->interleave_mode)
    {
    
    case GAVL_INTERLEAVE_NONE:
      len = frame->valid_samples;
      for(i = 0; i < format->num_channels; i++)
        do_swap(frame->channels.u_8[i], len);
      break;
    case GAVL_INTERLEAVE_2:
      len = frame->valid_samples * 2;
      for(i = 0; i < format->num_channels/2; i++)
        do_swap(frame->channels.u_8[2*i], len);
      
      if(format->num_channels % 2)
        {
        len = frame->valid_samples;
        do_swap(frame->channels.u_8[format->num_channels-1], len);
        }
    
      break;
    case GAVL_INTERLEAVE_ALL:
      len = frame->valid_samples * format->num_channels;
      do_swap(frame->samples.u_8, len);
      break;
    }
  }

void gavl_audio_frame_mute_channel(gavl_audio_frame_t * frame,
                                   const gavl_audio_format_t * format,
                                   int channel)
  {
  int i;
  int imax;
  int offset = 0;
  int advance = 0;

  switch(format->interleave_mode)
    {
    case GAVL_INTERLEAVE_ALL:
      offset = channel;
      advance = format->num_channels;
      break;
    case GAVL_INTERLEAVE_NONE:
      offset = channel * format->samples_per_frame;
      advance = 1;
      break;
    case GAVL_INTERLEAVE_2:
      if(channel & 1)
        offset = (channel-1) * format->samples_per_frame + 1;
      else
        offset = channel * format->samples_per_frame;

      if((channel == format->num_channels - 1) &&
         (format->num_channels & 1))
        advance = 1;
      else
        advance = 2;
      
      break;
    }
  
  imax = format->samples_per_frame;
  
  switch(format->sample_format)
    {
    case GAVL_SAMPLE_NONE:
      break;
    case GAVL_SAMPLE_U8:
      for(i = 0; i < imax; i++)
        frame->samples.u_8[offset + advance * i] = 0x80;
      break;
    case GAVL_SAMPLE_S8:
      for(i = 0; i < imax; i++)
        frame->samples.u_8[offset + advance * i] = 0x00;
      break;
    case GAVL_SAMPLE_U16:
      for(i = 0; i < imax; i++)
        frame->samples.u_16[offset + advance * i] = 0x8000;
      break;
    case GAVL_SAMPLE_S16:
      for(i = 0; i < imax; i++)
        frame->samples.s_16[offset + advance * i] = 0x0000;
      break;
    case GAVL_SAMPLE_S32:
      for(i = 0; i < imax; i++)
        frame->samples.s_32[offset + advance * i] = 0x00000000;
      break;
    case GAVL_SAMPLE_FLOAT:
      for(i = 0; i < imax; i++)
        frame->samples.f[offset + advance * i] = 0.0;
      break;
    }
  }


int gavl_audio_frame_copy(const gavl_audio_format_t * format,
                          gavl_audio_frame_t * dst,
                          const gavl_audio_frame_t * src,
                          int out_pos,
                          int in_pos,
                          int out_size,
                          int in_size)
  {
  int i;
  int bytes_per_sample;
  int samples_to_copy;
  gavl_init_memcpy();
  samples_to_copy = (in_size < out_size) ? in_size : out_size;

  if(!dst)
    return samples_to_copy;

  bytes_per_sample = gavl_bytes_per_sample(format->sample_format);
  
  switch(format->interleave_mode)
    {
    case GAVL_INTERLEAVE_NONE:
      for(i = 0; i < format->num_channels; i++)
        {
        gavl_memcpy(&(dst->channels.s_8[i][out_pos * bytes_per_sample]),
                    &(src->channels.s_8[i][in_pos * bytes_per_sample]),
                    samples_to_copy * bytes_per_sample);
        }
      break;
    case GAVL_INTERLEAVE_2:
      for(i = 0; i < format->num_channels/2; i++)
        {
        gavl_memcpy(&(dst->channels.s_8[i*2][2 * out_pos * bytes_per_sample]),
                    &(src->channels.s_8[i*2][2 * in_pos * bytes_per_sample]),
                    2*samples_to_copy * bytes_per_sample);
        }
      /* Last channel is not interleaved */
      if(format->num_channels & 1)
        {
        gavl_memcpy(&(dst->channels.s_8[format->num_channels-1][2 * out_pos * bytes_per_sample]),
                    &(src->channels.s_8[format->num_channels-1][2 * in_pos * bytes_per_sample]),
                    2*samples_to_copy * bytes_per_sample);
        }
      break;
    case GAVL_INTERLEAVE_ALL:
      gavl_memcpy(&(dst->samples.s_8[format->num_channels * out_pos * bytes_per_sample]),
                  &(src->samples.s_8[format->num_channels * in_pos * bytes_per_sample]),
                  format->num_channels * samples_to_copy * bytes_per_sample);
      break;
    }
  return samples_to_copy;
  }

void gavl_audio_frame_null(gavl_audio_frame_t * f)
  {
  memset(f, 0, sizeof(*f));
  }
