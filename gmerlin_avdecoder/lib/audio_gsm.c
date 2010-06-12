/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <avdec_private.h>
#include <codecs.h>

#include "GSM610/gsm.h"

/* Audio decoder for the internal libgsm */

/* From ffmpeg */
// gsm.h miss some essential constants
#define GSM_BLOCK_SIZE 33
#define GSM_FRAME_SAMPLES 160

// #define GSM_BLOCK_SIZE_MS 65
// #define GSM_FRAME_SIZE_MS 320

#define LOG_DOMAIN "gsm"

typedef struct
  {
  gsm gsm_state;
    
  bgav_packet_t * packet;
  uint8_t       * packet_ptr;
  gavl_audio_frame_t * frame;
  int ms;
  } gsm_priv;

static int init_gsm(bgav_stream_t * s)
  {
  int tmp;
  gsm_priv * priv;

  /* Allocate stuff */
  priv = calloc(1, sizeof(*priv));
  priv->gsm_state = gsm_create();
  s->data.audio.decoder->priv = priv;

  if(s->data.audio.format.num_channels > 1)
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
            "Multichannel GSM not supported");
    return 0;
    }

  if((s->fourcc == BGAV_WAVID_2_FOURCC(0x31)) ||
     (s->fourcc == BGAV_WAVID_2_FOURCC(0x32)))
    {
    priv->ms = 1;
    tmp = 1;
    gsm_option(priv->gsm_state, GSM_OPT_WAV49, &tmp);
    }

  /* Set format */
  s->data.audio.format.interleave_mode   = GAVL_INTERLEAVE_NONE;
  s->data.audio.format.sample_format     = GAVL_SAMPLE_S16;
  
  s->data.audio.format.samples_per_frame = priv->ms ? 2*GSM_FRAME_SAMPLES : GSM_FRAME_SAMPLES;
  gavl_set_channel_setup(&s->data.audio.format);
  
  priv->frame = gavl_audio_frame_create(&s->data.audio.format);

  if(priv->ms)
    s->description = bgav_sprintf("MSGM");
  else
    s->description = bgav_sprintf("GSM 6.10");
  return 1;
  }

static void close_gsm(bgav_stream_t * s)
  {
  gsm_priv * priv;
  priv = (gsm_priv*)s->data.audio.decoder->priv;

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);
  gsm_destroy(priv->gsm_state);
  free(priv);
  }

static int decode_frame_gsm(bgav_stream_t * s)
  {
  gsm_priv * priv;

  priv = (gsm_priv*)s->data.audio.decoder->priv;
  
  if(!priv->packet)
    {
    priv->packet = bgav_stream_get_packet_read(s);
    if(!priv->packet)
      return 0;
    priv->packet_ptr = priv->packet->data;
    }
  else if(priv->packet_ptr - priv->packet->data + // Data already decoded
          GSM_BLOCK_SIZE + priv->ms * (GSM_BLOCK_SIZE-1) // Next packet
          > priv->packet->data_size)
    {
    bgav_stream_done_packet_read(s, priv->packet);
    priv->packet = bgav_stream_get_packet_read(s);
    if(!priv->packet)
      return 0;
    priv->packet_ptr = priv->packet->data;
    }
  gsm_decode(priv->gsm_state, priv->packet_ptr, priv->frame->samples.s_16);
  priv->frame->valid_samples = GSM_FRAME_SAMPLES;
  
  if(priv->ms)
    {
    priv->packet_ptr += GSM_BLOCK_SIZE;
    //    priv->packet_ptr += block_size-1;
    gsm_decode(priv->gsm_state, priv->packet_ptr,
               priv->frame->samples.s_16+GSM_FRAME_SAMPLES);
    priv->frame->valid_samples += GSM_FRAME_SAMPLES;
    priv->packet_ptr += GSM_BLOCK_SIZE-1;
    }
  else
    priv->packet_ptr += GSM_BLOCK_SIZE;

  gavl_audio_frame_copy_ptrs(&s->data.audio.format, s->data.audio.frame, priv->frame);
  
  return 1;
  }

static void resync_gsm(bgav_stream_t * s)
  {
  gsm_priv * priv;
  priv = (gsm_priv*)s->data.audio.decoder->priv;
  
  priv->frame->valid_samples = 0;
  priv->packet = (bgav_packet_t*)0;
  priv->packet_ptr = (uint8_t*)0;
  
  }

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_WAVID_2_FOURCC(0x31),
               BGAV_WAVID_2_FOURCC(0x32),
               BGAV_MK_FOURCC('a', 'g', 's', 'm'),
               BGAV_MK_FOURCC('G', 'S', 'M', ' '),
               0x00 },
    .name = "libgsm based decoder",

    .init =   init_gsm,
    .decode_frame = decode_frame_gsm,
    .resync = resync_gsm,
    .close =  close_gsm,
  };

void bgav_init_audio_decoders_gsm()
  {
  bgav_audio_decoder_register(&decoder);
  }
