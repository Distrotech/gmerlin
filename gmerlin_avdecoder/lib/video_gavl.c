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

#include <avdec_private.h>
#include <codecs.h>

static int init_gavl(bgav_stream_t * s)
  {
  s->flags |= STREAM_INTRA_ONLY;
  return 1;
  }

static int decode_gavl(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  bgav_packet_t * p;

  p = bgav_stream_get_packet_read(s);
  if(!p || !(p->video_frame))
    return 0;
  
  if(frame)
    {
    gavl_video_frame_copy(&s->data.video.format, frame, p->video_frame);
    gavl_video_frame_copy_metadata(frame, p->video_frame);
    }
  bgav_stream_done_packet_read(s, p);
  return 1;
  }

static void close_gavl(bgav_stream_t * s)
  {
  
  }

static bgav_video_decoder_t decoder =
  {
    .fourccs = (uint32_t[]){ BGAV_MK_FOURCC('g', 'a', 'v', 'l'),
                           0x00 },
    .name = "gavl video decoder",
    .init = init_gavl,
    .close = close_gavl,
    .decode = decode_gavl
  };

void bgav_init_video_decoders_gavl()
  {
  bgav_video_decoder_register(&decoder);
  }
