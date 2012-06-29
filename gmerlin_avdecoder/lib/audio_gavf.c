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

#include <avdec_private.h>
#include <codecs.h>

#include <gavl/gavf.h>

#include <stdlib.h>

typedef struct
  {
  gavl_audio_frame_t * frame;
  bgav_packet_t * p;
  } gavf_audio_t;

static int decode_frame_gavf_audio(bgav_stream_t * s)
  {
  gavl_packet_t p;
  gavf_audio_t * priv = s->data.audio.decoder->priv;

  if(priv->p)
    {
    bgav_stream_done_packet_read(s, priv->p);
    priv->p = NULL;
    }
  
  if(!(priv->p = bgav_stream_get_packet_read(s)))
    return 0;

  gavl_packet_init(&p);
  p.data     = priv->p->data;
  p.data_len = priv->p->data_size;
  p.pts      = priv->p->pts;
  p.duration = priv->p->duration;

  gavf_packet_to_audio_frame(&p, priv->frame, &s->data.audio.format);
  
  gavl_audio_frame_copy_ptrs(&s->data.audio.format, s->data.audio.frame, priv->frame);
  
  return 1;
  }

static void close_gavf_audio(bgav_stream_t * s)
  {
  gavf_audio_t * priv = s->data.audio.decoder->priv;
  
  if(priv->frame)
    {
    gavl_audio_frame_null(priv->frame);
    gavl_audio_frame_destroy(priv->frame);
    }
  free(priv);
  }

static void resync_gavf_audio(bgav_stream_t * s)
  {
  gavf_audio_t * priv = s->data.audio.decoder->priv;

  if(priv->p)
    {
    bgav_stream_done_packet_read(s, priv->p);
    priv->p = NULL;
    }
  }

static int init_gavf_audio(bgav_stream_t * s)
  {
  gavf_audio_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  priv->frame = gavl_audio_frame_create(NULL);
  return 1;
  }

static bgav_audio_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('g','a','v','f'),
                             0x00 },
    .name = "GAVF audio decoder",
    .init = init_gavf_audio,
    .close = close_gavf_audio,
    .resync = resync_gavf_audio,
    .decode_frame = decode_frame_gavf_audio
  };

void bgav_init_audio_decoders_gavf()
  {
  bgav_audio_decoder_register(&decoder);
  }
