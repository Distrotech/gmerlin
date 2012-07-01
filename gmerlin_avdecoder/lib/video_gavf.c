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
  gavl_video_frame_t * frame;
  } gavf_video_t;

static int decode_frame_gavf_video(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;
  
  gavl_packet_t gp;
  gavf_video_t * priv = s->data.video.decoder->priv;
  
  if(!(p = bgav_stream_get_packet_read(s)))
    return 0;

  gavl_packet_init(&gp);
  gp.data     = p->data;
  gp.data_len = p->data_size;
  gp.pts      = p->pts;
  gp.duration = p->duration;

  gavf_packet_to_video_frame(&gp, priv->frame, &s->data.video.format);

  if(frame)
    {
    bgav_set_video_frame_from_packet(p, frame);

    gavl_video_frame_copy(&s->data.video.format, frame, priv->frame);
    }

  bgav_stream_done_packet_read(s, p);
  
  return 1;
  }

static void close_gavf_video(bgav_stream_t * s)
  {
  gavf_video_t * priv = s->data.video.decoder->priv;
  
  if(priv->frame)
    {
    gavl_video_frame_null(priv->frame);
    gavl_video_frame_destroy(priv->frame);
    }
  free(priv);
  }

static int init_gavf_video(bgav_stream_t * s)
  {
  gavf_video_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;

  priv->frame = gavl_video_frame_create(NULL);
  return 1;
  }

static bgav_video_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('g','a','v','f'),
                             0x00 },
    .name = "GAVF video decoder",
    .init = init_gavf_video,
    .close = close_gavf_video,
    .decode = decode_frame_gavf_video
  };

void bgav_init_video_decoders_gavf()
  {
  bgav_video_decoder_register(&decoder);
  }
