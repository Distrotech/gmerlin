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
#include <avdec_private.h>

#define FRAME_SAMPLES 1024

/* Decode functions */

static void decode_8(bgav_stream_t * s)
  {
  
  }

static void decode_s_16(bgav_stream_t * s)
  {
  
  }

static void decode_s_16_swap(bgav_stream_t * s)
  {
  
  }

static void decode_s_24(bgav_stream_t * s)
  {

  }

static void decode_s_24_swap(bgav_stream_t * s)
  {

  }

static void decode_s_32(bgav_stream_t * s)
  {

  }

static void decode_s_32_swap(bgav_stream_t * s)
  {

  }

typedef struct
  {
  void (*decode_func)(bgav_stream_t * s);
  gavl_audio_frame_t * frame;
  int last_frame_samples;

  bgav_packet_t * p;
  int             bytes_in_packet;
  uint8_t *       packet_ptr;
  } pcm_t;

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
        s->data.audio.format.sample_format = GAVL_SAMPLE_S8;
        }
      else if(s->data.audio.bits_per_sample <= 16)
        {
        s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_16_swap;
#else
        priv->decode_func = decode_s_16;
#endif
        }
      else if(s->data.audio.bits_per_sample <= 24)
        {
        s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        priv->decode_func = decode_s_24_swap;
#else
        priv->decode_func = decode_s_24;
#endif
        }
      else if(s->data.audio.bits_per_sample <= 32)
        {
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
    case BGAV_MK_FOURCC('r', 'a', 'w', ' '):
    case BGAV_MK_FOURCC('s', 'o', 'w', 't'):
      switch(s->data.audio.bits_per_sample)
        {
        case 8:
          if(s->fourcc == BGAV_MK_FOURCC('s', 'o', 'w', 't'))
            s->data.audio.format.sample_format = GAVL_SAMPLE_S8;
          else
            s->data.audio.format.sample_format = GAVL_SAMPLE_U8;
          priv->decode_func = decode_8;
          break;
        case 16:
          s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_16;
#else
          priv->decode_func = decode_s_16_swap;
#endif
          break;
        case 24:
          s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_24;
#else
          priv->decode_func = decode_s_24_swap;
#endif
          break;
        case 32:
          s->data.audio.format.sample_format = GAVL_SAMPLE_S32;
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
          priv->decode_func = decode_s_32;
#else
          priv->decode_func = decode_s_32_swap;
#endif
        }
      break;
    case BGAV_MK_FOURCC('l', 'p', 'c', 'm'):
      /* If we have LPCM Audio, we need to get the entire
         format from the header */
      break;
    default:
      fprintf(stderr, "Unkknown fourcc\n");
      return 0;
    }
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.samples_per_frame = FRAME_SAMPLES;
  gavl_set_channel_setup(&(s->data.audio.format));
  s->description = bgav_sprintf("PCM Audio Big Endian");
  
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
  bgav_packet_t * p;
  
  priv = (pcm_t*)(s->data.audio.decoder->priv);
  samples_decoded = 0;
  while(samples_decoded <  num_samples)
    {
    if(!priv->frame->valid_samples)
      {
      p = bgav_demuxer_get_packet_read(s->demuxer, s);
      
      /* EOF */
      
      if(!p)
        break;
      
      /* Decode stuff */

      priv->decode_func(s);
      priv->last_frame_samples = priv->frame->valid_samples;
      bgav_demuxer_done_packet_read(s->demuxer, p);
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
  if(!samples_decoded)
    return 0;
  return 1;
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
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('a', 'i', 'f', 'f'),
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
