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

/* PNG decoder for quicktime and avi */

#include <stdlib.h>
#include <string.h>
#include <png.h>

#include <config.h>
#include <avdec_private.h>
#include <codecs.h>
#include <pngreader.h>

#define LOG_DOMAIN "video_png"

typedef struct
  {
  bgav_png_reader_t * png_reader;
  int have_header;
  int need_header;
  bgav_packet_t * p;
  } png_priv_t;

static int decode_png(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  char * error_msg = (char*)0;
  png_priv_t * priv;
  priv = (png_priv_t*)(s->data.video.decoder->priv);

  if(!priv->have_header)
    {
    priv->p = bgav_demuxer_get_packet_read(s->demuxer, s);
    if(!priv->p)
      {
      bgav_log(s->opt, BGAV_LOG_DEBUG, LOG_DOMAIN, "EOF");
      return 0;
      }
    }
  if(priv->need_header)
    {
    
    if(!bgav_png_reader_read_header(priv->png_reader,
                                    priv->p->data, priv->p->data_size,
                                    &(s->data.video.format), &error_msg))
      {
      if(error_msg)
        {
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "%s", error_msg);
        free(error_msg);
        }
      else
        bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Reading png header failed");
        
      return 0;
      }
    priv->have_header = 1;
    return 1;
    }
  /* We decode only if we have a frame */
  if(frame)
    {
    if(!priv->have_header &&
       !bgav_png_reader_read_header(priv->png_reader,
                                    priv->p->data, priv->p->data_size,
                                    &(s->data.video.format), (char**)0))
      return 0;
    if(!bgav_png_reader_read_image(priv->png_reader, frame))
      return 0;
    priv->have_header = 0;

    frame->timestamp = priv->p->pts;
    frame->duration = priv->p->duration;
    }
  bgav_demuxer_done_packet_read(s->demuxer, priv->p);
  priv->p = (bgav_packet_t*)0;
  return 1;
  }

static int init_png(bgav_stream_t * s)
  {
  png_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  s->flags |= STREAM_INTRA_ONLY;
  
  priv->png_reader = bgav_png_reader_create(s->data.video.depth);
  priv->need_header = 1;
  if(!decode_png(s, (gavl_video_frame_t*)0))
    {
    bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Decode png failed");
    return 0;
    }
  priv->need_header = 0;
  
  s->description = bgav_sprintf("PNG Video (%s)",
                                ((s->data.video.format.pixelformat ==
                                  GAVL_RGBA_32) ? "RGBA" : "RGB")); 
  

  return 1;
  }

static void close_png(bgav_stream_t * s)
  {
  png_priv_t * priv;
  priv = (png_priv_t*)(s->data.video.decoder->priv);
  if(priv->png_reader)
    bgav_png_reader_destroy(priv->png_reader);
  free(priv);
  }

static void resync_png(bgav_stream_t * s)
  {
  png_priv_t * priv;
  priv = (png_priv_t*)(s->data.video.decoder->priv);
  bgav_png_reader_reset(priv->png_reader);
  priv->have_header = 0;
  }

static bgav_video_decoder_t decoder =
  {
    .name =   "PNG video decoder",
    .fourccs =  (uint32_t[]){ BGAV_MK_FOURCC('p', 'n', 'g', ' '),
                            BGAV_MK_FOURCC('M', 'P', 'N', 'G'),
                            0x00  },
    .init =   init_png,
    .decode = decode_png,
    .resync = resync_png,
    .close =  close_png,
    .resync = NULL,
  };

void bgav_init_video_decoders_png()
  {
  bgav_video_decoder_register(&decoder);
  }

