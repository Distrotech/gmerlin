/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <tiffio.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "tiffenc"

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gavl/metatags.h>
#include <gavl/gavf.h>



#include "tiffwriter.h"

typedef struct
  {
  bg_tiff_writer_t * tiff;
  
  gavl_packet_sink_t * psink;
  gavl_video_sink_t * vsink;
  gavl_video_format_t fmt;
  int have_header;
  gavl_packet_t p;
  } stream_codec_t;

static void * create_codec()
  {
  stream_codec_t * ret = calloc(1, sizeof(*ret));
  ret->tiff = bg_tiff_writer_create();
  return ret;
  }

static void destroy_codec(void * priv)
  {
  stream_codec_t * c = priv;
  bg_tiff_writer_destroy(c->tiff);
  free(c);
  }

static const bg_parameter_info_t * get_parameters(void * priv)
  {
  return bg_tiff_writer_get_parameters();
  }

static void set_parameter(void * priv, const char * name,
                          const bg_parameter_value_t * val)
  {
  stream_codec_t * c = priv;
  bg_tiff_writer_set_parameter(c->tiff, name, val);
  }

static gavl_sink_status_t put_frame(void * priv,
                                    gavl_video_frame_t * f)
  {
  stream_codec_t * c = priv;
  
  if(!c->have_header)
    {
    gavl_packet_reset(&c->p);
    if(!bg_tiff_writer_write_header(c->tiff, NULL,
                                    &c->p,
                                    &c->fmt,
                                    NULL))
      return GAVL_SINK_ERROR;
    }
  bg_tiff_writer_write_image(c->tiff, f);
  gavf_video_frame_to_packet_metadata(f, &c->p);
  c->have_header = 0;
  c->p.flags |= GAVL_PACKET_KEYFRAME;
  return gavl_packet_sink_put_packet(c->psink, &c->p);
  }

static gavl_video_sink_t * open_video(void * priv,
                                      gavl_compression_info_t * ci,
                                      gavl_video_format_t * fmt,
                                      gavl_metadata_t * m)
  {
  stream_codec_t * c = priv;
  gavl_packet_reset(&c->p);
  if(!bg_tiff_writer_write_header(c->tiff, NULL,
                                  &c->p,
                                  fmt,
                                  NULL))
    return NULL;
  
  gavl_metadata_set(m, GAVL_META_SOFTWARE, TIFFGetVersion());
  gavl_video_format_copy(&c->fmt, fmt);
  c->have_header = 1;
  c->vsink = gavl_video_sink_create(NULL, put_frame, c, &c->fmt);

  ci->id = GAVL_CODEC_ID_TIFF;

  return c->vsink;
  }

static void set_packet_sink(void * priv, gavl_packet_sink_t * s)
  {
  stream_codec_t * c = priv;
  c->psink = s;
  }

const bg_codec_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =            "c_tiffenc",       /* Unique short name */
      .long_name =       "TIFF",
      .description =     "libtiff based encoder",
      .type =            BG_PLUGIN_CODEC,
      .flags =           BG_PLUGIN_VIDEO_COMPRESSOR,
      .priority =        5,
      .create =          create_codec,
      .destroy =         destroy_codec,
      .get_parameters =    get_parameters,
      .set_parameter =     set_parameter,
    },
    .open_encode_video = open_video,
    .set_packet_sink = set_packet_sink,
  };
/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
