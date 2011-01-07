/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <gmerlin/pluginfuncs.h>

#include <gavl/gavldsp.h>

#include <gmerlin/serialize.h>
#include <gmerlin/fileformat.h>


typedef struct
  {
  gavl_video_format_t format;
  bg_f_io_t io;
  bg_iw_callbacks_t * cb;
  
  } gavl_t;

/* GAVL writer */

static void * create_gavl()
  {
  gavl_t * ret;
  ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void set_callbacks_gavl(void * data, bg_iw_callbacks_t * cb)
  {
  gavl_t * e = data;
  e->cb = cb;
  }

static void destroy_gavl(void* priv)
  {
  gavl_t * gavl = (gavl_t*)priv;

  free(gavl);
  }


static int write_header_gavl(void * priv, const char * filename,
                             gavl_video_format_t * format,
                             const bg_metadata_t * metadata)
  {
  bg_f_signature_t sig;
  char * real_filename;
  gavl_t * gavl = (gavl_t*)priv;
  
  real_filename = bg_filename_ensure_extension(filename, "gavi");

  if(!bg_iw_cb_create_output_file(gavl->cb, real_filename))
    {
    free(real_filename);
    return 0;
    }
  
  if(!bg_f_io_open_stdio_write(&gavl->io, real_filename))
    return 0;
  free(real_filename);
  
  sig.type = SIG_TYPE_IMAGE;
  if(!bg_f_signature_write(&gavl->io, &sig))
    return 0;

  if(!bg_f_video_format_write(&gavl->io, format))
    return 0;
  gavl_video_format_copy(&gavl->format, format);
  return 1;
  }

static int write_image_gavl(void * priv, gavl_video_frame_t * frame)
  {
  gavl_t * gavl = (gavl_t*)priv;
  if(!bg_f_video_frame_write(&gavl->io, &gavl->format, frame))
    return 0;
  bg_f_io_close(&gavl->io);
  return 1;
  }

const bg_image_writer_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "iw_gavl",
      .long_name =      TRS("gavl image writer"),
      .description =    TRS("Writer for GAVL images"),
      .type =           BG_PLUGIN_IMAGE_WRITER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         create_gavl,
      .destroy =        destroy_gavl,
    },
    .extensions = "gavi",
    .write_header =  write_header_gavl,
    .write_image =   write_image_gavl,
    .set_callbacks = set_callbacks_gavl,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
