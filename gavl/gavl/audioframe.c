/*****************************************************************
 
  audioframe.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/


#include <string.h>
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

  if(!format)
    return ret;
  
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
    case GAVL_SAMPLE_U16:
      ret->samples.u_16 =
        memalign(ALIGNMENT_BYTES, 2 * num_samples * format->num_channels);

      ret->channels.u_16 = calloc(format->num_channels, sizeof(uint16_t*));
      for(i = 0; i < format->num_channels; i++)
        ret->channels.u_16[i] = &(ret->samples.u_16[i*num_samples]);

      break;
    case GAVL_SAMPLE_S16:
      ret->samples.s_16 =
        memalign(ALIGNMENT_BYTES, 2 * num_samples * format->num_channels);
      ret->channels.s_16 = calloc(format->num_channels, sizeof(int16_t*));
      for(i = 0; i < format->num_channels; i++)
        ret->channels.s_16[i] = &(ret->samples.s_16[i*num_samples]);

      break;

    case GAVL_SAMPLE_S32:
      ret->samples.s_32 =
        memalign(ALIGNMENT_BYTES, 4 * num_samples * format->num_channels);
      ret->channels.s_32 = calloc(format->num_channels, sizeof(int32_t*));
      for(i = 0; i < format->num_channels; i++)
        ret->channels.s_32[i] = &(ret->samples.s_32[i*num_samples]);

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
  if(frame->samples.s_8)
    free(frame->samples.s_8);
  if(frame->channels.s_8)
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

int gavl_audio_frame_copy(gavl_audio_format_t * format,
                          gavl_audio_frame_t * dst,
                          gavl_audio_frame_t * src,
                          int out_pos,
                          int in_pos,
                          int out_size,
                          int in_size)
      {
  int i;
  int bytes_per_sample;
  int samples_to_copy;

  samples_to_copy = (in_size < out_size) ? in_size : out_size;

  if(!dst)
    return samples_to_copy;

  bytes_per_sample = gavl_bytes_per_sample(format->sample_format);
  
  switch(format->interleave_mode)
    {
    case GAVL_INTERLEAVE_NONE:
      for(i = 0; i < format->num_channels; i++)
        {
        memcpy(&(dst->channels.s_8[i][out_pos * bytes_per_sample]),
               &(src->channels.s_8[i][in_pos * bytes_per_sample]),
               samples_to_copy * bytes_per_sample);
        }
      break;
    case GAVL_INTERLEAVE_2:
      for(i = 0; i < format->num_channels/2; i++)
        {
        memcpy(&(dst->channels.s_8[i*2][2 * out_pos * bytes_per_sample]),
               &(src->channels.s_8[i*2][2 * in_pos * bytes_per_sample]),
               2*samples_to_copy * bytes_per_sample);
        }
      /* Last channel is not interleaved */
      if(format->num_channels & 1)
        {
        memcpy(&(dst->channels.s_8[format->num_channels-1][2 * out_pos * bytes_per_sample]),
               &(src->channels.s_8[format->num_channels-1][2 * in_pos * bytes_per_sample]),
               2*samples_to_copy * bytes_per_sample);
        }
      break;
    case GAVL_INTERLEAVE_ALL:
      memcpy(&(dst->samples.s_8[format->num_channels * out_pos * bytes_per_sample]),
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
