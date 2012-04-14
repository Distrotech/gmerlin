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
#include <errno.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "pngwriter"

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gavl/metatags.h>

#include <png.h>


#include "pngwriter.h"

int bg_pngwriter_write_header(void * priv, const char * filename,
                              gavl_video_format_t * format,
                              const gavl_metadata_t * metadata)
  {
  int color_type;
  int bits = 8;
  int j;
  bg_pngwriter_t * png = priv;

  png->transform_flags = PNG_TRANSFORM_IDENTITY;
  
  png->output = fopen(filename, "wb");
  if(!png->output)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s: %s",
           filename, strerror(errno));
    return 0;
    }
  png->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                         NULL, NULL);

  png->info_ptr = png_create_info_struct(png->png_ptr);
  setjmp(png_jmpbuf(png->png_ptr));
  png_init_io(png->png_ptr, png->output);

  switch(png->bit_mode)
    {
    case BITS_AUTO:
      /* Decide according to the input format */
      if(gavl_pixelformat_is_planar(format->pixelformat))
        {
        if(gavl_pixelformat_bytes_per_component(format->pixelformat) > 1)
          bits = 16;
        }
      else if(gavl_pixelformat_bytes_per_pixel(format->pixelformat) > 4)
        bits = 16;
      break;
    case BITS_8:
      bits = 8;
      break;
    case BITS_16:
      bits = 16;
      break;
      
    }
#ifndef WORDS_BIGENDIAN
  if(bits > 8)
    png->transform_flags |= PNG_TRANSFORM_SWAP_ENDIAN;
#endif

  if(gavl_pixelformat_is_gray(format->pixelformat))
    {
    if(gavl_pixelformat_has_alpha(format->pixelformat))
      {
      color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
      if(bits == 8)
        format->pixelformat = GAVL_GRAYA_16;
      else
        format->pixelformat = GAVL_GRAYA_32;
      }
    else
      {
      color_type = PNG_COLOR_TYPE_GRAY;
      if(bits == 8)
        format->pixelformat = GAVL_GRAY_8;
      else
        format->pixelformat = GAVL_GRAY_16;
      }
    }
  else
    {
    if(gavl_pixelformat_has_alpha(format->pixelformat))
      {
      color_type = PNG_COLOR_TYPE_RGBA;
      if(bits == 8)
        format->pixelformat = GAVL_RGBA_32;
      else
        format->pixelformat = GAVL_RGBA_64;
      }
    else
      {
      color_type = PNG_COLOR_TYPE_RGB;
      if(bits == 8)
        format->pixelformat = GAVL_RGB_24;
      else
        format->pixelformat = GAVL_RGB_48;
      }
    }
  
  png_set_compression_level(png->png_ptr, png->compression_level);
  png_set_IHDR(png->png_ptr, png->info_ptr,
               format->image_width,
               format->image_height,
               bits, color_type, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  
  gavl_video_format_copy(&png->format, format);

  /* Set metadata */
  
  if(!metadata)
    return 1;

  png->num_text = metadata->num_tags;
  png->text = calloc(png->num_text, sizeof(*png->text));

  for(j = 0; j < metadata->num_tags; j++)
    {
    png->text[j].compression = PNG_TEXT_COMPRESSION_NONE;

    if(!strcmp(metadata->tags[j].key, GAVL_META_AUTHOR))
      png->text[j].key         = bg_strdup(png->text[j].key, "Author");
    else if(!strcmp(metadata->tags[j].key, GAVL_META_TITLE))
      png->text[j].key         = bg_strdup(png->text[j].key, "Title");
    else if(!strcmp(metadata->tags[j].key, GAVL_META_COPYRIGHT))
      png->text[j].key         = bg_strdup(png->text[j].key, "Copyright");
    else
      png->text[j].key         = bg_strdup(png->text[j].key, metadata->tags[j].key);
    
    png->text[j].text        = bg_strdup(png->text[j].text, metadata->tags[j].val);
    }
  
  png_set_text(png->png_ptr, png->info_ptr, png->text, png->num_text);
  return 1;
  }

int bg_pngwriter_write_image(void * priv, gavl_video_frame_t * frame)
  {
  int i;
  unsigned char ** rows;
  bg_pngwriter_t * png = priv;

  rows = malloc(png->format.image_height * sizeof(*rows));

  for(i = 0; i < png->format.image_height; i++)
    rows[i] = frame->planes[0] + i * frame->strides[0];

  png_set_rows(png->png_ptr, png->info_ptr, rows);
  png_write_png(png->png_ptr, png->info_ptr, png->transform_flags, NULL);
 
  png_destroy_write_struct(&png->png_ptr, &png->info_ptr);
  fclose(png->output);
  free(rows);

  if(png->num_text)
    {
    for(i = 0; i < png->num_text; i++)
      {
      free(png->text[i].key);
      free(png->text[i].text);
      }
    free(png->text);
    png->num_text = 0;
    png->text = NULL;
    }
  
  return 1;
  }

void bg_pngwriter_set_parameter(void * p, const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_pngwriter_t * png;
  png = p;
  
  if(!name)
    return;

  if(!strcmp(name, "compression"))
    png->compression_level = val->val_i;
  if(!strcmp(name, "dont_force_extension"))
    png->dont_force_extension = val->val_i;
  if(!strcmp(name, "bit_mode"))
    {
    if(!strcmp(val->val_str, "Auto"))
      png->bit_mode = BITS_AUTO;
    else
      png->bit_mode = atoi(val->val_str);
    }

  }
