/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <a52_header.h>

#include <a52dec/a52.h>

#define FRAME_SAMPLES 1536

#define LOG_DOMAIN "audio_a52"

// #define DUMP_PACKET

typedef struct
  {
  a52_state_t * state;
  sample_t    * samples;

  gavl_audio_frame_t * frame;
  int need_format;
  } a52_priv;

#if 0
static void resync_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  priv = s->data.audio.decoder->priv;

  priv->packet = (bgav_packet_t*)0;
  priv->packet_ptr = (uint8_t*)0;
  priv->bytes_in_buffer = 0;
  do_resync(s);
  }
#endif

static int decode_frame_a52(bgav_stream_t * s)
  {
  int flags;
  int sample_rate;
  int bit_rate;
  int i, j;
  int frame_bytes;
  bgav_packet_t * p;
  
  sample_t level = 1.0;
  
  a52_priv * priv;
  priv = s->data.audio.decoder->priv;

  p = bgav_stream_get_packet_read(s);
  if(!p)
    return 0;

#ifdef DUMP_PACKET
  bgav_dprintf("a52 packet: ");
  bgav_packet_dump(p);
  bgav_hexdump((uint8_t*)p->data, 16, 16);
#endif
  
  if(priv->need_format)
    {
    bgav_a52_header_t h;
    
    if(!bgav_a52_header_read(&h, p->data))
      return 0;

    //    bgav_a52_header_dump(&h);

    s->codec_bitrate = h.bitrate;

    s->description =
      bgav_sprintf("AC3 %d kb/sec", h.bitrate/1000);
#ifdef LIBA52_DOUBLE
    s->data.audio.format.sample_format = GAVL_SAMPLE_DOUBLE;
#else
    s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
#endif
    s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_NONE; 
    bgav_a52_header_get_format(&h, &s->data.audio.format);
    priv->frame = gavl_audio_frame_create(&s->data.audio.format);
    }
  
  /* Now, decode this */
  
  frame_bytes = a52_syncinfo(p->data, &flags,
                             &sample_rate, &bit_rate);

  if(!frame_bytes)
    return 0;

  if(frame_bytes < p->data_size)
    return 0;


  
  a52_frame(priv->state, p->data, &flags, &level, 0.0);
  if(!s->opt->audio_dynrange)
    {
    a52_dynrng(priv->state, NULL, NULL);
    }
  
  for(i = 0; i < 6; i++)
    {
    a52_block (priv->state);

    //    if(!i)
    //      bgav_hexdump((uint8_t*)priv->samples, 16, 16);
    
    for(j = 0; j < s->data.audio.format.num_channels; j++)
      {
      
#ifdef LIBA52_DOUBLE
      memcpy(&priv->frame->channels.d[j][i*256],
             priv->samples + j*256,
             256 * sizeof(*priv->samples));
#else      
      memcpy(&priv->frame->channels.f[j][i*256],
             priv->samples + j*256,
             256 * sizeof(*priv->samples));
#endif
      }
    }
  
  priv->frame->valid_samples = FRAME_SAMPLES;
  gavl_audio_frame_copy_ptrs(&s->data.audio.format, s->data.audio.frame, priv->frame);

  bgav_stream_done_packet_read(s, p);

  return 1;
  }

static int init_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;
  
  //  a52_header_dump(&priv->header);
    
  /* Get format */


  priv->state = a52_init(0);
  priv->samples = a52_samples(priv->state);
  memset(priv->samples, 0, 256*6*sizeof(*priv->samples));
  
  priv->need_format = 1;
  decode_frame_a52(s);
  priv->need_format = 0;

  s->data.audio.preroll = 32 * FRAME_SAMPLES;
  
  return 1;
  }

static void close_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  priv = s->data.audio.decoder->priv;

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  if(priv->state)
    a52_free(priv->state);
  free(priv);
  }

static void resync_a52(bgav_stream_t * s)
  {
  a52_priv * priv;
  
  priv = s->data.audio.decoder->priv;
  if(!priv->state)
    return;

  a52_free(priv->state);
  priv->state = a52_init(0);
  priv->samples = a52_samples(priv->state);
  memset(priv->samples, 0, 256*6*sizeof(*priv->samples));

#if 0
  p = bgav_stream_peek_packet_read(s, 1);
  if(p->pts == s->start_time)
    first_packet = 1;
  else
    first_packet = 0;

  decode_frame_a52(s);

  // Skip packet before this one
  if(!first_packet)
    {
    s->data.audio.frame->valid_samples = 0; 
    p = bgav_stream_peek_packet_read(s, 1);
    s->out_time = p->pts;
    bgav_log(s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN, "Resync %"PRId64, s->out_time);
    }
#endif
  }

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x2000),
                             BGAV_MK_FOURCC('.', 'a', 'c', '3'),
                           /* Will be swapped to AC3 by the demuxer */
                             BGAV_MK_FOURCC('d', 'n', 'e', 't'), 
                             BGAV_MK_FOURCC('a', 'c', '-', '3'), // mp4, mov
                             0x00 },
    .name = "liba52 based decoder",

    .init   = init_a52,
    .decode_frame = decode_frame_a52,
    .close  = close_a52,
    .resync = resync_a52,
  };

void bgav_init_audio_decoders_a52()
  {
  bgav_audio_decoder_register(&decoder);
  }
