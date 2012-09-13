/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <opus.h>
#include <opus_multistream.h>

#include <stdlib.h>

/* Internal includes */


#include <config.h>

#include <avdec_private.h>
#include <codecs.h>

#define LOG_DOMAIN "audio_opus"

#define MAX_FRAME_SIZE (960*6)

typedef struct
  {
  OpusMSDecoder *dec;
  gavl_audio_frame_t * frame;
  } opus_t;

static int decode_frame_opus(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  opus_t * priv;

  priv = s->data.audio.decoder->priv;
  
  p = bgav_stream_get_packet_read(s);
  if(!p)
    return 0;
  
  bgav_stream_done_packet_read(s, p);
  return 0;
  }

static int init_opus(bgav_stream_t * s)
  {
  opus_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  s->data.audio.format.samples_per_frame = MAX_FRAME_SIZE;
  
  return 1;
  }

static void close_opus(bgav_stream_t * s)
  {
  opus_t * priv;
  priv = s->data.audio.decoder->priv;

  
  }

static void resync_opus(bgav_stream_t * s)
  {
  opus_t * priv;
  priv = s->data.audio.decoder->priv;
  opus_multistream_decoder_ctl(priv->dec, OPUS_RESET_STATE);
  }

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('O', 'P', 'U', 'S'),
                             0x00 },
    .name = "libopus based decoder",

    .init   = init_opus,
    .decode_frame = decode_frame_opus,
    .close  = close_opus,
    .resync = resync_opus,
  };

void bgav_init_audio_decoders_opus()
  {
  bgav_audio_decoder_register(&decoder);
  }
