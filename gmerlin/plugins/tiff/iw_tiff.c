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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <tiffio.h>
#include <inttypes.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "iw_tiff"

#include "tiffwriter.h"

typedef struct
  {
  bg_tiff_writer_t * tiff;
  bg_iw_callbacks_t * cb;
  } tiff_t;

static void set_callbacks_tiff(void * data, bg_iw_callbacks_t * cb)
  {
  tiff_t * e = data;
  e->cb = cb;
  }

static void * create_tiff()
  {
  tiff_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->tiff = bg_tiff_writer_create();
  return ret;
  }

static void destroy_tiff(void* priv)
  {
  tiff_t * tiff = priv;
  bg_tiff_writer_destroy(tiff->tiff);
  free(tiff);
  }

static int write_header_tiff(void * priv, const char * filename,
                             gavl_video_format_t * format,
                             const gavl_metadata_t * m)
  {
  int ret;
  char * real_filename;
  tiff_t * tiff = priv;
  
  real_filename = bg_filename_ensure_extension(filename, "tif");
  
  if(!bg_iw_cb_create_output_file(tiff->cb, real_filename))
    {
    free(real_filename);
    return 0;
    }

  ret = bg_tiff_writer_write_header(tiff->tiff, real_filename,
                                    NULL,
                                    format, m);
  free(real_filename);
  return ret;
  }

static int write_image_tiff(void *priv, gavl_video_frame_t *frame)
  {
  tiff_t * tiff = priv;
  return bg_tiff_writer_write_image(tiff->tiff, frame);
  }

static const bg_parameter_info_t * get_parameters_tiff(void * p)
  {
  return bg_tiff_writer_get_parameters();
  }

static void set_parameter_tiff(void * p, const char * name,
                               const bg_parameter_value_t * val)
  {
  tiff_t * tiff = p;
  bg_tiff_writer_set_parameter(tiff->tiff, name, val);
  }

const bg_image_writer_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "iw_tiff",
      .long_name =      TRS("TIFF writer"),
      .description =    TRS("Writer for TIFF images"),
      .type =           BG_PLUGIN_IMAGE_WRITER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         create_tiff,
      .destroy =        destroy_tiff,
      .get_parameters = get_parameters_tiff,
      .set_parameter =  set_parameter_tiff
    },
    .extensions = "tif",
    .set_callbacks = set_callbacks_tiff,
    .write_header =  write_header_tiff,
    .write_image =   write_image_tiff,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

