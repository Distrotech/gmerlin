/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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
#include <stdio.h>
#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <png.h>

#include "pngwriter.h"

#define PADD(i, size) i = ((i + size - 1) / size) * size


/* PNG writer */

static void * create_png()
  {
  bg_pngwriter_t * ret;
  ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void destroy_png(void * priv)
  {
  bg_pngwriter_t * png = (bg_pngwriter_t*)priv;
  free(png);
  }



/* Configuration stuff */

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

static const bg_parameter_info_t * get_parameters_png(void * p)
  {
  return parameters;
  }

static char const * const png_extension = ".png";

static const char * get_extension_png(void * p)
  {
  return png_extension;
  }

const bg_image_writer_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "iw_png",
      .long_name =      TRS("PNG writer"),
      .description =    TRS("Writer for PNG images"),
      .type =           BG_PLUGIN_IMAGE_WRITER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         create_png,
      .destroy =        destroy_png,
      .get_parameters = get_parameters_png,
      .set_parameter =  bg_pngwriter_set_parameter
    },
    .extensions = "png",
    .get_extension = get_extension_png,
    .write_header =  bg_pngwriter_write_header,
    .write_image =   bg_pngwriter_write_image,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
