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
#include <opus_header.h>

#define LOG_DOMAIN "audio_opus"

#define MAX_FRAME_SIZE (960*6)

typedef struct
  {
  OpusMSDecoder *dec;
  gavl_audio_frame_t * frame;
  bgav_opus_header_t h;
  } opus_t;

static int decode_frame_opus(bgav_stream_t * s)
  {
  bgav_packet_t * p;
  opus_t * priv;
  int result;
  int ret = 0;
  
  priv = s->data.audio.decoder->priv;
  
  p = bgav_stream_get_packet_read(s);
  if(!p)
    return 0;

  result =
    opus_multistream_decode_float(priv->dec,
                                  p->data,
                                  p->data_size,
                                  priv->frame->samples.f,
                                  MAX_FRAME_SIZE,
                                  0); 

  if(result <= 0)
    goto fail;

  priv->frame->valid_samples = result;
  
  gavl_audio_frame_copy_ptrs(&s->data.audio.format,
                             s->data.audio.frame, priv->frame);
  

  ret = 1;

  fail:
  
  bgav_stream_done_packet_read(s, p);
  return ret;
  }

static int init_opus(bgav_stream_t * s)
  {
  opus_t * priv;
  bgav_input_context_t * input_mem;
  int ret = 0;
  int err;
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  /* Parse extradata */
  input_mem = bgav_input_open_memory(s->ext_data,
                                     s->ext_size,
                                     s->opt);
  
  if(!bgav_opus_header_read(input_mem, &priv->h))
    goto fail;

  err = 0;
  
  priv->dec =
    opus_multistream_decoder_create(s->data.audio.format.samplerate,
                                    s->data.audio.format.num_channels,
                                    priv->h.chtab.stream_count,
                                    priv->h.chtab.coupled_count,
                                    priv->h.chtab.map,
                                    &err);

  if(!priv->dec)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
             "opus_multistream_decoder_create failed: %s",
             opus_strerror(err));
    goto fail;
    }
  
  s->data.audio.format.sample_format = GAVL_SAMPLE_FLOAT;
  s->data.audio.format.samples_per_frame = MAX_FRAME_SIZE;
  s->data.audio.format.interleave_mode = GAVL_INTERLEAVE_ALL;
  
  if(s->data.audio.format.channel_locations[0] == GAVL_CHID_NONE)
    bgav_opus_set_channel_setup(&priv->h,
                                &s->data.audio.format);

  /* Samples per frame is just the maximum */
  s->src_flags |= GAVL_SOURCE_SRC_FRAMESIZE_MAX;
    
  priv->frame = gavl_audio_frame_create(&s->data.audio.format);

  /* Apply Gain */

  err = opus_multistream_decoder_ctl(priv->dec, OPUS_SET_GAIN(priv->h.output_gain));
  if(err != OPUS_OK)
    {
    bgav_log(s->opt, BGAV_LOG_WARNING, LOG_DOMAIN, "OPUS_SET_GAIN failed: %s",
             opus_strerror(err));
    }
  
  ret = 1;
  fail:

  bgav_input_close(input_mem);
  bgav_input_destroy(input_mem);

  return ret;
  }

static void close_opus(bgav_stream_t * s)
  {
  opus_t * priv;
  priv = s->data.audio.decoder->priv;

  if(priv->dec)
    opus_multistream_decoder_destroy(priv->dec);

  if(priv->frame)
    gavl_audio_frame_destroy(priv->frame);

  free(priv);
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
