/*****************************************************************
 
  audio_aiff.c
 
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

/* Simple linear pcm decoder for aiff files */

/*
 *  The difference between this and other PCM decoders is, that
 *  we support any bit depth from 1 to 32, which can occur in AIFF
 *  files.
 * 
 *  This decoder assumes, that the bits_per_sample member of the
 *  audio format is already set by the demuxer.
 */

/*
 *  Samples from 1 to 16 bits are output as integer numbers
 *  (8 or 16 bits), anything above 16 bits is output as floating point
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avdec_private.h>

#define PTR_2_INT_32(d) \
(((int32_t)d[0] << 24) | \
 ((uint8_t)d[1] << 16) | \
 ((uint8_t)d[2] << 8) | \
 ((uint8_t)d[3]))

#define PTR_2_INT_24(d) \
(((int32_t)d[0] << 24) | \
 ((uint8_t)d[1] << 16) | \
 ((uint8_t)d[2] << 8))

static void decode_8(uint8_t * data, int data_len,
                     gavl_audio_frame_t * f, int num_channels)
  {
  memcpy(f->samples.s_8, data, data_len);
  f->valid_samples = data_len / (num_channels);
  }

static void decode_16(uint8_t * data, int data_len,
                      gavl_audio_frame_t * f, int num_channels)
  {
  memcpy(f->samples.s_16, data, data_len);
  f->valid_samples = data_len / (num_channels * 2);
  }

static void decode_24(uint8_t * data, int data_len,
                      gavl_audio_frame_t * f, int num_channels)
  {
  int32_t tmp;
  float * dst;
  int i;
  int num_samples;
  num_samples = data_len / 3;
  //  fprintf(stderr, "decode_24: %d %d\n", data_len, num_samples);
  dst = f->samples.f;
  for(i = 0; i < num_samples; i++)
    {
    tmp = PTR_2_INT_24(data);
    *dst = ((float)(tmp))/2147483648.0;
    dst++;
    data += 3;
    }
  f->valid_samples = num_samples / num_channels;
  }

static void decode_32(uint8_t * data, int data_len,
                      gavl_audio_frame_t * f, int num_channels)
  {
  int32_t tmp;
  float * dst;
  int i;
  int num_samples;

  num_samples = data_len / 4;
  //  fprintf(stderr, "decode_32: %d %d\n", data_len, num_samples);
  dst = f->samples.f;
  for(i = 0; i < num_samples; i++)
    {
    tmp = PTR_2_INT_32(data);
    *dst = (float)(tmp)/2147483648.0;
    dst++;
    data += 4;
    }
  f->valid_samples = num_samples / num_channels;
  }

typedef struct
  {
  void (*decode_func)(uint8_t * data, int data_len,
                      gavl_audio_frame_t * f, int num_channels);
  
  gavl_audio_frame_t * frame;
  int last_frame_samples;
  } aiff_audio_t;

static int init_aiff(bgav_stream_t * s)
  {
  aiff_audio_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;
  if(s->data.audio.bits_per_sample <= 8)
    {
    s->data.audio.format.sample_format = GAVL_SAMPLE_S8;
    priv->decode_func = decode_8;
    }
  else if(s->data.audio.bits_per_sample <= 16)
    {
    s->data.audio.format.sample_format = GAVL_SAMPLE_S16;
    priv->decode_func = decode_16;
    }
  else if(s->data.audio.bits_per_sample <= 24)
    {
    s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
    priv->decode_func = decode_24;
    }
  else if(s->data.audio.bits_per_sample <= 32)
    {
    s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
    priv->decode_func = decode_32;
    }
  else
    {
    fprintf(stderr, "%d bit aiff not supported\n",
            s->data.audio.bits_per_sample);
    return 0;
    }
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  s->data.audio.format.samples_per_frame = 1024;
  gavl_set_channel_setup(&(s->data.audio.format));
  s->description = bgav_sprintf("PCM Audio Big Endian");

  priv->frame = gavl_audio_frame_create(&s->data.audio.format);
  
  return 1;
  }

static int decode_aiff(bgav_stream_t * s,
                       gavl_audio_frame_t * frame,
                       int num_samples)
  {
  aiff_audio_t * priv;
  int samples_decoded;
  int samples_copied;
  bgav_packet_t * p;
  
  priv = (aiff_audio_t*)(s->data.audio.decoder->priv);
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

      priv->decode_func(p->data, p->data_size, priv->frame,
                        s->data.audio.format.num_channels);
      
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

static void close_aiff(bgav_stream_t * s)
  {
  aiff_audio_t * priv;
  priv = (aiff_audio_t*)(s->data.audio.decoder->priv);

  gavl_audio_frame_destroy(priv->frame);
  free(priv);
  }

static void resync_aiff(bgav_stream_t * s)
  {
  aiff_audio_t * priv;
  priv = (aiff_audio_t*)(s->data.audio.decoder->priv);
  priv->frame->valid_samples = 0;
  }

static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('a', 'i', 'f', 'f'),
                           0x00 },
    name: "AIFF audio decoder",
    init: init_aiff,
    close: close_aiff,
    resync: resync_aiff,
    decode: decode_aiff
  };

void bgav_init_audio_decoders_aiff()
  {
  bgav_audio_decoder_register(&decoder);
  }
