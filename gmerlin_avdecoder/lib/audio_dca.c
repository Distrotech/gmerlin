/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <config.h>

#include <avdec_private.h>
#include <codecs.h>
// #include <dca_header.h>

#include <bgav_dca.h>

#define BLOCK_SAMPLES 256

typedef struct
  {
  dts_state_t * state;
  sample_t    * samples;
  
  bgav_packet_t * packet;
  gavl_audio_frame_t * frame;

  int blocks_left;
  int frame_length;

  int need_format;
  } dts_priv;

#define MAX_FRAME_SIZE 3840

/*
 *  Parse header and read data bytes for that frame
 *  (also does resync)
 */

#if 0
static int do_resync(bgav_stream_t * s, int * flags,
                     int * sample_rate, int * bit_rate, int * frame_length)
  {
  int dummy;
  dts_priv * priv;
  priv = (dts_priv*)(s->data.audio.decoder->priv);

  while(1)
    {
    if(!get_data(s, 14))
      return 0;
    if((*frame_length = dts_syncinfo(priv->state, priv->buffer,
                                     flags, sample_rate, bit_rate, &dummy)))
      {
      return 1;
      }
    done_data(s, 1);
    }
  return 0;
  }
#endif

static void resync_dts(bgav_stream_t * s)
  {
  dts_priv * priv = s->data.audio.decoder->priv;
  priv->blocks_left = 0;
  priv->packet = NULL;
  }

static int decode_frame_dts(bgav_stream_t * s)
  {
  int flags;
  int sample_rate;
  int bit_rate;
  int j;
  int frame_bytes;
#ifdef LIBDTS_DOUBLE
  int k;
#endif
  int dummy;
  sample_t level = 1.0;
  
  dts_priv * priv;
  priv = (dts_priv*)s->data.audio.decoder->priv;

  if(!priv->blocks_left)
    {
    if(priv->packet)
      bgav_demuxer_done_packet_read(s->demuxer, priv->packet);
    
    priv->packet = bgav_demuxer_get_packet_read(s->demuxer, s);

    if(!priv->packet)
      return 0;
    
    /* Now, decode this */

    frame_bytes = dts_syncinfo(priv->state, priv->packet->data, &flags,
                               &sample_rate, &bit_rate, &dummy);
    
    if(!frame_bytes)
      return 0;
    
    if(frame_bytes < priv->packet->data_size)
      return 0;

    if(priv->need_format)
      {
      s->data.audio.format.samplerate = sample_rate;
      s->data.audio.format.samples_per_frame = BLOCK_SAMPLES;
      s->codec_bitrate = bit_rate;
      s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  
      bgav_dca_flags_2_channel_setup(flags, &(s->data.audio.format));
  
      priv->frame = gavl_audio_frame_create(&(s->data.audio.format));
      s->description =
        bgav_sprintf("DTS %d kb/sec", bit_rate/1000);
      }
    
    dts_frame(priv->state, priv->packet->data, &flags, &level, 0.0);
    
    if(!s->opt->audio_dynrange)
      dts_dynrng(priv->state, NULL, NULL);
    priv->blocks_left = dts_blocks_num(priv->state);
    //    fprintf(stderr, "DTS samples: %d\n", priv->blocks_left * 256);
    }
  
  dts_block (priv->state);

  for(j = 0; j < s->data.audio.format.num_channels; j++)
    {
#ifdef LIBDTS_DOUBLE
    for(k = 0; k < 256; k++)
      priv->frame->channels.f[i][k][i*256] = priv->samples[j*256 + k];
    
#else
    memcpy(priv->frame->channels.f[j],
           &(priv->samples[j * 256]),
           256 * sizeof(float));
#endif
    }
  priv->blocks_left--;
  if(!priv->blocks_left)
    {
    bgav_demuxer_done_packet_read(s->demuxer, priv->packet);
    priv->packet = NULL;
    }
  
  for(j = 0; j < s->data.audio.format.num_channels; j++)
    s->data.audio.frame->channels.f[j] = priv->frame->channels.f[j];
  
  priv->frame->valid_samples = BLOCK_SAMPLES;
  s->data.audio.frame->valid_samples = BLOCK_SAMPLES;
  return 1;
  }


static int init_dts(bgav_stream_t * s)
  {
  dts_priv * priv;
  
  priv = calloc(1, sizeof(*priv));
  priv->state = dts_init(0);
  priv->samples = dts_samples(priv->state);
  
  s->data.audio.decoder->priv = priv;

  //  dts_header_dump(&(priv->header));
    
  /* Get format */

  priv->need_format = 1;
  if(!decode_frame_dts(s))
    return 0;
  priv->need_format = 0;
  
  return 1;
  }


static void close_dts(bgav_stream_t * s)
  {
  dts_priv * priv;
  priv = (dts_priv*)s->data.audio.decoder->priv;

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  if(priv->state)
    dts_free(priv->state);
  free(priv);
  }

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('d', 't', 's', ' '),
                           0x00 },
    .name = "libdca based decoder",

    .init =   init_dts,
    .decode_frame = decode_frame_dts,
    .close =  close_dts,
    .resync = resync_dts,
  };

void bgav_init_audio_decoders_dca()
  {
  bgav_audio_decoder_register(&decoder);
  }
