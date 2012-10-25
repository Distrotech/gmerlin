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
  } png_priv_t;

static gavl_source_status_t
decode_png(bgav_stream_t * s, gavl_video_frame_t * frame)
  {
  char * error_msg = NULL;
  png_priv_t * priv;
  bgav_packet_t * p = NULL;
  gavl_source_status_t st;
  
  priv = s->data.video.decoder->priv;

  if((st = bgav_stream_get_packet_read(s, &p)) != GAVL_SOURCE_OK)
    return st;
  
  if(!bgav_png_reader_read_header(priv->png_reader,
                                  p->data, p->data_size,
                                  &s->data.video.format, &error_msg))
    {
    if(error_msg)
      {
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "%s", error_msg);
      free(error_msg);
      }
    else
      bgav_log(s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Reading png header failed");
    
    return GAVL_SOURCE_EOF;
    }
  
  /* We decode only if we have a frame */
  if(frame)
    {
    if(!priv->have_header &&
       !bgav_png_reader_read_header(priv->png_reader,
                                    p->data, p->data_size,
                                    &s->data.video.format, NULL))
      return GAVL_SOURCE_EOF;
    if(!bgav_png_reader_read_image(priv->png_reader, frame))
      return GAVL_SOURCE_EOF;
    priv->have_header = 0;

    bgav_set_video_frame_from_packet(p, frame);
    }
  bgav_stream_done_packet_read(s, p);
  return GAVL_SOURCE_OK;
  }

static int init_png(bgav_stream_t * s)
  {
  png_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  s->data.video.decoder->priv = priv;
  
  priv->png_reader = bgav_png_reader_create(s->data.video.depth);
  gavl_metadata_set(&s->m, GAVL_META_FORMAT, "PNG");
  return 1;
  }

static void close_png(bgav_stream_t * s)
  {
  png_priv_t * priv;
  priv = s->data.video.decoder->priv;
  if(priv->png_reader)
    bgav_png_reader_destroy(priv->png_reader);
  free(priv);
  }

static void resync_png(bgav_stream_t * s)
  {
  png_priv_t * priv;
  priv = s->data.video.decoder->priv;
  bgav_png_reader_reset(priv->png_reader);
  priv->have_header = 0;
  }

static bgav_video_decoder_t decoder =
  {
    .name =   "PNG video decoder",
    .fourccs = bgav_png_fourccs,
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

