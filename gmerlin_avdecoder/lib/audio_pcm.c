/*****************************************************************
 
  audio_pcm.c
 
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

#include <stdlib.h>
#include <string.h>
#include <avdec_private.h>
#include <codecs.h>

#include <bswap.h>

#define FRAME_SAMPLES 1024

typedef struct
  {
  void (*decode_func)(bgav_stream_t * s);
  gavl_audio_frame_t * frame;
  int last_frame_samples;

  bgav_packet_t * p;
  int             bytes_in_packet;
  uint8_t *       packet_ptr;
  } pcm_t;

/* Decode functions */

static void decode_8(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes;
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (s->data.audio.format.num_channels);
  
  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * s->data.audio.format.num_channels;

  memcpy(priv->frame->samples.u_8, priv->packet_ptr, num_bytes);

  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_s_16(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes;
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (2 * s->data.audio.format.num_channels);
  
  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 2 * s->data.audio.format.num_channels;

  memcpy(priv->frame->samples.s_16, priv->packet_ptr, num_bytes);

  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_s_16_swap(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  int16_t * src, *dst;
  
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (2 * s->data.audio.format.num_channels);
  
  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 2 * s->data.audio.format.num_channels;

  src = (int16_t*)priv->packet_ptr;
  dst = priv->frame->samples.s_16;

  i = num_samples * s->data.audio.format.num_channels;

  while(i--)
    {
    *dst = bswap_16(*src);
    src++;
    dst++;
    }
  
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  
  
  }

static void decode_s_24_le(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  uint32_t * dst;

  num_samples = priv->bytes_in_packet / (3 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 3 * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (uint32_t*)(priv->frame->samples.s_32);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst =
      ((uint32_t)(src[0]) << 8)  |
      ((uint32_t)(src[1]) << 16)  |
      ((uint32_t)(src[2]) << 24);
    src+=3;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  
  }

static void decode_s_24_be(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes, i;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  uint8_t * src;
  uint32_t * dst;

  num_samples = priv->bytes_in_packet / (3 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 3 * s->data.audio.format.num_channels;

  src = priv->packet_ptr;
  dst = (uint32_t*)(priv->frame->samples.s_32);

  i = num_samples * s->data.audio.format.num_channels;
  
  while(i--)
    {
    *dst =
      ((uint32_t)(src[2]) << 8)  |
      ((uint32_t)(src[1]) << 16)  |
      ((uint32_t)(src[0]) << 24);
    src+=3;
    dst++;
    }
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  }

static void decode_s_32(bgav_stream_t * s)
  {
  pcm_t * priv;
  int num_samples, num_bytes;
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (4 * s->data.audio.format.num_channels);

  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 4 * s->data.audio.format.num_channels;
  memcpy(priv->frame->samples.s_32, priv->packet_ptr, num_bytes);
  
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  //  fprintf(stderr, "Decode %d %d\n", num_bytes, num_samples);
  
  }

static void decode_s_32_swap(bgav_stream_t * s)
  {

  pcm_t * priv;
  int num_samples, num_bytes, i;
  int32_t * src, *dst;
  
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  num_samples = priv->bytes_in_packet / (4 * s->data.audio.format.num_channels);
  
  if(num_samples > FRAME_SAMPLES)
    num_samples = FRAME_SAMPLES;

  num_bytes   = num_samples * 4 * s->data.audio.format.num_channels;

  src = (int32_t*)priv->packet_ptr;
  dst = priv->frame->samples.s_32;

  i = num_samples * s->data.audio.format.num_channels;

  while(i--)
    {
    *dst = bswap_32(*src);
    src++;
    dst++;
    }
  
  priv->packet_ptr += num_bytes;
  priv->bytes_in_packet -= num_bytes;
  priv->frame->valid_samples = num_samples;
  
  }

static int init_pcm(bgav_stream_t * s)
  {
  pcm_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  switch(s->fourcc)
    {
    /* Big endian */
    case BGAV_MK_FOURCC('t', 'w', 'o', 's'):
    case BGAV_MK_FOURCC('a', 'i', 'f', 'f'):
      if(s->data.audio.bits_per_sample <= 8)
        {
        s->description = bgav_sprintf("%d bit PCM", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S8;
        priv->decode_func = decode_8;
        }
      else if(s->data.audio.bits_per_sample <= 16)
        {
        s->description = bgav_sprintf("%d bit PCM (big endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_16_swap;
#else
        priv->decode_func = decode_s_16;
#endif
        }
      else if(s->data.audio.bits_per_sample <= 24)
        {
        s->description = bgav_sprintf("%d bit PCM (big endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
        priv->decode_func = decode_s_24_be;
        }
      else if(s->data.audio.bits_per_sample <= 32)
        {
        s->description = bgav_sprintf("%d bit PCM (big endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_32_swap;
#else
        priv->decode_func = decode_s_32;
#endif
        }
      else
        {
        fprintf(stderr, "Audio bits %d not supported!!\n",
                s->data.audio.bits_per_sample);
        return 0;
        }
      break;
    case BGAV_WAVID_2_FOURCC(0x01):
      if(s->data.audio.bits_per_sample <= 8)
        {
        s->description = bgav_sprintf("%d bit PCM", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_U8;
        priv->decode_func = decode_8;
        }
      else if(s->data.audio.bits_per_sample <= 16)
        {
        s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_16;
#else
        priv->decode_func = decode_s_16_swap;
#endif
        }
      else if(s->data.audio.bits_per_sample <= 24)
        {
        s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
        priv->decode_func = decode_s_24_le;
        }
      else if(s->data.audio.bits_per_sample <= 32)
        {
        s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_32;
#else
        priv->decode_func = decode_s_32_swap;
#endif
        }
      break;
    case BGAV_MK_FOURCC('r', 'a', 'w', ' '):
    case BGAV_MK_FOURCC('s', 'o', 'w', 't'):
      switch(s->data.audio.bits_per_sample)
        {
        case 8:
          s->description = bgav_sprintf("%d bit PCM", s->data.audio.bits_per_sample);
          if(s->fourcc == BGAV_MK_FOURCC('s', 'o', 'w', 't'))
            s->data.audio.format.sample_format = GAVL_SAMPLE_S8;
          else
            s->data.audio.format.sample_format = GAVL_SAMPLE_U8;
          priv->decode_func = decode_8;
          break;
        case 16:
          s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
          s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_16;
#else
          priv->decode_func = decode_s_16_swap;
#endif
          break;
        case 24:
          s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
          s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
          priv->decode_func = decode_s_24_le;
          break;
        case 32:
          s->description = bgav_sprintf("%d bit PCM (little endian)", s->data.audio.bits_per_sample);
          s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_32;
#else
          priv->decode_func = decode_s_32_swap;
#endif
          break;
        default:
          fprintf(stderr, "Audio bits %d not supported!!\n",
                  s->data.audio.bits_per_sample);
          return 0;
        }
      break;
    case BGAV_MK_FOURCC('l', 'p', 'c', 'm'):
      switch(s->data.audio.bits_per_sample)
        {
        case 16:
          s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_16_swap;
#else
          priv->decode_func = decode_s_16;
#endif
          break;
        default:
          fprintf(stderr, "Error: %d bit lpcm not supported\n", s->data.audio.bits_per_sample);
          return 0;
        }
      
      break;
    default:
      fprintf(stderr, "Unkknown fourcc\n");
      return 0;
    }
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.samples_per_frame = FRAME_SAMPLES;
  gavl_set_channel_setup(&(s->data.audio.format));
  
  priv->frame = gavl_audio_frame_create(&s->data.audio.format);
  
  return 1;
  }

static int decode_pcm(bgav_stream_t * s,
                      gavl_audio_frame_t * frame,
                      int num_samples)
  {
  pcm_t * priv;
  int samples_decoded;
  int samples_copied;
  
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  samples_decoded = 0;
  while(samples_decoded <  num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      if(!priv->p)      
        {
        priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);

        /* EOF */
        
        if(!priv->p)
          {
          //          fprintf(stderr, "Reached EOF\n");
          break;
          }
        priv->bytes_in_packet = priv->p->data_size;
        priv->packet_ptr = priv->p->data;
        }
      
      
      /* Decode stuff */

      priv->decode_func(s);
      priv->last_frame_samples = priv->frame->valid_samples;
      
      if(!priv->bytes_in_packet)
        {
        bgav_demuxer_done_packet_read(s->demuxer, priv->p);
        priv->p = (bgav_packet_t*)0;
        }
      if(!priv->last_frame_samples)
        break;
      }
    samples_copied =
      gavl_audio_frame_copy(&(s->data.audio.format),
                            frame,       /* dst */
                            priv->frame, /* src */
                            samples_decoded, /* int dst_pos */
                            priv->last_frame_samples - priv->frame->valid_samples, /* int src_pos */
                            num_samples - samples_decoded, /* int dst_size, */
                            priv->frame->valid_samples /* int src_size*/ );
    priv->frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    }
  if(frame)
    frame->valid_samples = samples_decoded;
  return samples_decoded;
  }

static void close_pcm(bgav_stream_t * s)
  {
  pcm_t * priv;
  priv = (pcm_t*)(s->data.audio.decoder->priv);

  gavl_audio_frame_destroy(priv->frame);
  free(priv);
  }

static void resync_pcm(bgav_stream_t * s)
  {
  pcm_t * priv;
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  priv->frame->valid_samples = 0;
  }

static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x01),
                           BGAV_MK_FOURCC('a', 'i', 'f', 'f'),
                           BGAV_MK_FOURCC('t', 'w', 'o', 's'),
                           BGAV_MK_FOURCC('s', 'o', 'w', 't'),
                           BGAV_MK_FOURCC('r', 'a', 'w', ' '),
                           BGAV_MK_FOURCC('l', 'p', 'c', 'm'),
                           0x00 },
    name: "PCM audio decoder",
    init: init_pcm,
    close: close_pcm,
    resync: resync_pcm,
    decode: decode_pcm
  };

void bgav_init_audio_decoders_pcm()
  {
  bgav_audio_decoder_register(&decoder);
  }
