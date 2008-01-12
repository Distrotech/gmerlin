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
#include <translation.h>
#include <plugin.h>

#include <png.h>

typedef struct
  {
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_info;

  gavl_video_format_t format;

  FILE * file;
  } png_t;

static void * create_png()
  {
  png_t * ret;
  ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void destroy_png(void* priv)
  {
  png_t * png = (png_t*)priv;

  free(png);
  }

static int read_header_png(void * priv, const char * filename,
                     gavl_video_format_t * format)
  {
  int bit_depth;
  int color_type;
  int has_alpha = 0;

  png_byte signature[8];
  png_t * png = (png_t*)priv;

  int bits = 8;
  
  png->file = fopen(filename, "rb");

  if(!png->file)
    goto fail;

  if(fread(signature, 1, 8, png->file) < 8)
    goto fail;

  if(png_sig_cmp(signature, 0, 8))
    goto fail;

  png->png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)0,
     NULL, NULL);

  setjmp(png_jmpbuf(png->png_ptr));

  png->info_ptr = png_create_info_struct(png->png_ptr);

  if(!png->info_ptr)
    goto fail;
                                                                                
  png->end_info = png_create_info_struct(png->png_ptr);
  if(!png->end_info)
    return 0;

  png_init_io(png->png_ptr, png->file);
  png_set_sig_bytes(png->png_ptr, 8);
                                                                                
  png_read_info(png->png_ptr, png->info_ptr);

  format->frame_width  = png_get_image_width(png->png_ptr, png->info_ptr);
  format->frame_height = png_get_image_height(png->png_ptr, png->info_ptr);

  format->image_width  = format->frame_width;
  format->image_height = format->frame_height;
  format->pixel_width = 1;
  format->pixel_height = 1;

  bit_depth  = png_get_bit_depth(png->png_ptr,  png->info_ptr);
  color_type = png_get_color_type(png->png_ptr, png->info_ptr);

  switch(color_type)
    {
    case PNG_COLOR_TYPE_GRAY:       /*  (bit depths 1, 2, 4, 8, 16) */
      if(bit_depth == 16)
        {
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        png_set_swap(png->png_ptr);
#endif
        bits = 16;
        }
      if(bit_depth < 8)
        png_set_gray_1_2_4_to_8(png->png_ptr);
      if (png_get_valid(png->png_ptr, png->info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png->png_ptr);
        has_alpha = 1;
        }
      png_set_gray_to_rgb(png->png_ptr);
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA: /*  (bit depths 8, 16) */
      if(bit_depth == 16)
        {
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        png_set_swap(png->png_ptr);
#endif
        bits = 16;
        }
      has_alpha = 1;
      png_set_gray_to_rgb(png->png_ptr);
      break;
    case PNG_COLOR_TYPE_PALETTE:    /*  (bit depths 1, 2, 4, 8) */
      png_set_palette_to_rgb(png->png_ptr);
      if (png_get_valid(png->png_ptr, png->info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png->png_ptr);
        has_alpha = 1;
        }
      break;
    case PNG_COLOR_TYPE_RGB:        /*  (bit_depths 8, 16) */
      if(png_get_valid(png->png_ptr, png->info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png->png_ptr);
        has_alpha = 1;
        }
      if(bit_depth == 16)
        {
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        png_set_swap(png->png_ptr);
#endif
        bits = 16;
        }
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:  /*  (bit_depths 8, 16) */
      if(bit_depth == 16)
        {
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
        png_set_swap(png->png_ptr);
#endif
        bits = 16;
        }
      has_alpha = 1;
      break;
    }

  if(bits == 8)
    {
    if(has_alpha)
      format->pixelformat = GAVL_RGBA_32;
    else
      format->pixelformat = GAVL_RGB_24;
    }
  else if(bits == 16)
    {
    if(has_alpha)
      format->pixelformat = GAVL_RGBA_64;
    else
      format->pixelformat = GAVL_RGB_48;
    }

  
  gavl_video_format_copy(&(png->format), format);
  return 1;
  fail:
  return 0;

  }

static int read_image_png(void * priv, gavl_video_frame_t * frame)
  {
  int i;
  unsigned char ** rows;
  png_t * png = (png_t*)priv;

  setjmp(png_jmpbuf(png->png_ptr));
  
  rows = malloc(png->format.frame_height * sizeof(*rows));
  for(i = 0; i < png->format.frame_height; i++)
    rows[i] = frame->planes[0] + i * frame->strides[0];

  png_read_image(png->png_ptr, rows);
  png_read_end(png->png_ptr, png->end_info);

  png_destroy_read_struct(&(png->png_ptr), &(png->info_ptr),
                          &(png->end_info));
  fclose(png->file);
  free(rows);
  
  return 1;
  }

const bg_image_reader_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =          "ir_png",
      .long_name =     TRS("PNG reader"),
      .description =   TRS("Reader for PNG images"),
      .mimetypes =     (char*)0,
      .extensions =    "png",
      .type =          BG_PLUGIN_IMAGE_READER,
      .flags =         BG_PLUGIN_FILE,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        create_png,
      .destroy =       destroy_png,
    },
    .read_header = read_header_png,
    .read_image =  read_image_png,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
