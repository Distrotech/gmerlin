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
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>

#include <png.h>

#include "pngwriter.h"

/* PNG writer */

typedef struct
  {
  bg_pngwriter_t writer; /* Must come first */
  bg_iw_callbacks_t * cb;
  
  int dont_force_extension;
  } png_t;

static void set_callbacks_png(void * data, bg_iw_callbacks_t * cb)
  {
  png_t * e = data;
  e->cb = cb;
  }

static void * create_png()
  {
  png_t * ret;
  ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void destroy_png(void * priv)
  {
  png_t * png = priv;
  free(png);
  }

static int write_header_png(void * priv, const char * filename,
                            gavl_video_format_t * format, const bg_metadata_t * metadata)
  {
  int ret;
  char * real_filename;
  png_t * png = priv;
  
  if(png->writer.dont_force_extension)
    real_filename = bg_strdup(NULL, filename);
  else
    real_filename = bg_filename_ensure_extension(filename, "png");
  
  if(!bg_iw_cb_create_output_file(png->cb, real_filename))
    {
    free(real_filename);
    return 0;
    }
  ret = bg_pngwriter_write_header(&png->writer, real_filename,
                                  format, metadata);
  
  free(real_filename);
  return ret;
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
    {
      /* Needed for the gmerlin video thumbnailer */
      .name =        "dont_force_extension",
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags =        BG_PARAMETER_HIDE_DIALOG,
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_png(void * p)
  {
  return parameters;
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
    .set_callbacks = set_callbacks_png,
    .write_header =  write_header_png,
    .write_image =   bg_pngwriter_write_image,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
