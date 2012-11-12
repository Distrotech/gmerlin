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

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gavl/gavf.h>


typedef struct
  {
  gavl_video_format_t format;
  gavf_io_t * io;
  gavl_metadata_t m;
  } gavl_t;

static void * create_gavl()
  {
  gavl_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_gavl(void* priv)
  {
  gavl_t * gavl = priv;
  free(gavl);
  }

static int read_header_gavl(void * priv, const char * filename,
                            gavl_video_format_t * format)
  {
  FILE * f;
  gavl_t * gavl = priv;

  f = fopen(filename, "r");
  if(!f)
    return 0;
  gavl->io = gavf_io_create_file(f, 0, 1, 1);

  if(!gavl_image_read_header(gavl->io,
                             &gavl->m,
                             format))
    return 0;
  
  gavl_video_format_copy(&gavl->format, format);
  return 1;
  
  }

static int read_image_gavl(void * priv, gavl_video_frame_t * frame)
  {
  int result = 1;
  gavl_t * gavl = priv;

  if(frame)
    result = gavl_image_read_image(gavl->io,
                                   &gavl->format,
                                   frame);
  gavf_io_destroy(gavl->io);
  return result;
  }

const bg_image_reader_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "ir_gavl",
      .long_name =     TRS("GAVL image reader"),
      .description =   TRS("Reader for GAVL images"),
      .type =          BG_PLUGIN_IMAGE_READER,
      .flags =         BG_PLUGIN_FILE,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        create_gavl,
      .destroy =       destroy_gavl,
    },
    .extensions =    "gavi",
    .read_header = read_header_gavl,
    .read_image =  read_image_gavl,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
