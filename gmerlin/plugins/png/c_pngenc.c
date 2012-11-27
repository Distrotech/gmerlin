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

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "pngwriter"

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gavl/metatags.h>

#include "pngwriter.h"

typedef struct
  {
  bg_pngwriter_t w;
  gavl_packet_sink_t * psink;
  gavl_video_sink_t * vsink;
  gavl_video_format_t fmt;
  int have_header;
  gavl_packet_t p;
  } stream_codec_t;

static void * create_codec()
  {
  stream_codec_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_codec(void * priv)
  {
  stream_codec_t * c = priv;
  free(c);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "compression",
      .long_name =   TRS("Compression level"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 9 },
      .val_default = { .val_i = 9 },
    },
    {
      .name =        "bit_mode",
      .long_name =   TRS("Bits per channel"),
      .type =        BG_PARAMETER_STRINGLIST,
      .multi_names = (char const *[]){ "Auto", "8", "16" },
      .val_default = { .val_str = "8" },
      .help_string = TRS("If you select auto, the depth will be chosen according to the input format")
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters(void * priv)
  {
  return parameters;
  }

static void set_parameter(void * priv, const char * name,
                          const bg_parameter_value_t * val)
  {
  stream_codec_t * c = priv;
  bg_pngwriter_set_parameter(&c->w, name, val);
  }

static gavl_sink_status_t put_frame(void * priv,
                                    gavl_video_frame_t * f)
  {
  stream_codec_t * c = priv;
  
  }

static gavl_video_sink_t * open_video(void * priv,
                                      gavl_compression_info_t * ci,
                                      gavl_video_format_t * fmt,
                                      gavl_metadata_t * m)
  {
  stream_codec_t * c = priv;
  
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
      .name =            "c_pngenc",       /* Unique short name */
      .long_name =       "PNG",
      .description =     "libpng based encoder",
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
