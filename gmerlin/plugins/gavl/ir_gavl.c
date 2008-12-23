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

#include <config.h>

#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>

#include <gavl/gavldsp.h>

#include <gmerlin/serialize.h>
#include <gmerlin/fileformat.h>

typedef struct
  {
  gavl_video_format_t format;
  bg_f_io_t io;
  int big_endian;
  gavl_dsp_context_t * ctx;
  } gavl_t;

static void * create_gavl()
  {
  gavl_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->ctx = gavl_dsp_context_create();
  return ret;
  }

static void destroy_gavl(void* priv)
  {
  gavl_t * gavl = (gavl_t*)priv;
  gavl_dsp_context_destroy(gavl->ctx);
  free(gavl);
  }

static int read_header_gavl(void * priv, const char * filename,
                            gavl_video_format_t * format)
  {
  bg_f_chunk_t ch;
  bg_f_signature_t sig;
  
  gavl_t * gavl = (gavl_t*)priv;

  if(!bg_f_io_open_stdio_read(&gavl->io, filename))
    return 0;

  /* Read signature */
  if(!bg_f_chunk_read(&gavl->io, &ch)) 
    return 0;

  if(ch.type != CHUNK_TYPE_SIGNATURE)
    return 0;
  
  if(!bg_f_signature_read(&gavl->io, &ch, &sig)) 
    return 0;

  if(sig.type != SIG_TYPE_IMAGE)
    return 0;
  
  /* Read format */
  if(!bg_f_chunk_read(&gavl->io, &ch)) 
    return 0;
  
  if(ch.type != CHUNK_TYPE_VIDEO_FORMAT)
    return 0;

  if(!bg_f_video_format_read(&gavl->io, &ch, format, &gavl->big_endian))
    return 0;
  
  gavl_video_format_copy(&(gavl->format), format);
  return 1;
  
  }

static int read_image_gavl(void * priv, gavl_video_frame_t * frame)
  {
  bg_f_chunk_t ch;
  gavl_t * gavl = (gavl_t*)priv;
  
  /* Read frame */
  if(!bg_f_chunk_read(&gavl->io, &ch)) 
    return 0;

  if(ch.type != CHUNK_TYPE_VIDEO_FRAME)
    return 0;
  
  if(!bg_f_video_frame_read(&gavl->io, &ch, &gavl->format, frame, gavl->big_endian, gavl->ctx))
    return 0;
  
  bg_f_io_close(&gavl->io);
  
  return 1;
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
