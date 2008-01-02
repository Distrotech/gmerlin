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
#include <string.h>
#include <avdec_private.h>
#include <codecs.h>

typedef struct
  {
  int last_frame_samples;
  bgav_packet_t * p;
  } gavl_t;

static int init_gavl(bgav_stream_t * s)
  {
  gavl_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.audio.decoder->priv = priv;

  /* Need to get the first packet because the dv avi decoder
     won't know the format before */
#if 1
  priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);
  if(!priv->p || !priv->p->audio_frame)
    return 0;
  priv->last_frame_samples = priv->p->audio_frame->valid_samples;

#endif

  return 1;
  }

static int decode_gavl(bgav_stream_t * s,
                      gavl_audio_frame_t * frame,
                      int num_samples)
  {
  gavl_t * priv;
  int samples_decoded;
  int samples_copied;
  
  priv = (gavl_t*)(s->data.audio.decoder->priv);
  samples_decoded = 0;
  while(samples_decoded <  num_samples)
    {
    /* Get new frame */
    if(!priv->p)      
      {
      priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);
      
      /* EOF */
      
      if(!priv->p || !priv->p->audio_frame)
        {
        break;
        }
      priv->last_frame_samples = priv->p->audio_frame->valid_samples;
      }

    /* Decode */
    samples_copied =
      gavl_audio_frame_copy(&(s->data.audio.format),
                            frame,       /* dst */
                            priv->p->audio_frame, /* src */
                            samples_decoded, /* int dst_pos */
                            priv->last_frame_samples - priv->p->audio_frame->valid_samples, /* int src_pos */
                            num_samples - samples_decoded, /* int dst_size, */
                            priv->p->audio_frame->valid_samples /* int src_size*/ );
    priv->p->audio_frame->valid_samples -= samples_copied;
    samples_decoded += samples_copied;
    
    if(!priv->p->audio_frame->valid_samples)
      {
      bgav_demuxer_done_packet_read(s->demuxer, priv->p);
      priv->p = (bgav_packet_t*)0;
      }
    }
  if(frame)
    frame->valid_samples = samples_decoded;
  return samples_decoded;
  }

static void close_gavl(bgav_stream_t * s)
  {
  gavl_t * priv;
  priv = (gavl_t*)(s->data.audio.decoder->priv);
  free(priv);
  }

static void resync_gavl(bgav_stream_t * s)
  {
  gavl_t * priv;
  priv = (gavl_t*)(s->data.audio.decoder->priv);
  if(priv->p && priv->p->audio_frame)
    priv->p->audio_frame->valid_samples = 0;
  priv->p = (bgav_packet_t*)0;
  }

static bgav_audio_decoder_t decoder =
  {
    fourccs: (uint32_t[]){ BGAV_MK_FOURCC('g', 'a', 'v', 'l'),
                           0x00 },
    name: "gavl audio decoder",
    init: init_gavl,
    close: close_gavl,
    resync: resync_gavl,
    decode: decode_gavl
  };

void bgav_init_audio_decoders_gavl()
  {
  bgav_audio_decoder_register(&decoder);
  }
