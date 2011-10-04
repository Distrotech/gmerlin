/*****************************************************************
 * gavl - a general purpose audio/video processing library
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
#include <png.h>
#include <stdio.h>
#include <gavl.h>
#include <gavl_version.h>
#include <pngutil.h>
#include <stdlib.h>

void write_png(char * filename, gavl_video_format_t * format, gavl_video_frame_t * frame)
  {
  int i;
  unsigned char ** rows;
  gavl_video_options_t * opt;
  int color_type;
  FILE * output;

  png_structp png_ptr;
  png_infop   info_ptr;
  
  gavl_video_converter_t * cnv;
    
  gavl_video_format_t format_1;
  gavl_video_frame_t * frame_1 = NULL;

  
  if((format->pixelformat != GAVL_RGB_24) && (format->pixelformat != GAVL_RGBA_32))
    {
    cnv = gavl_video_converter_create();
    
    gavl_video_format_copy(&format_1, format);

    if(gavl_pixelformat_has_alpha(format->pixelformat))
      {
      format_1.pixelformat = GAVL_RGBA_32;
      color_type = PNG_COLOR_TYPE_RGBA;
      }
    else
      {
      format_1.pixelformat = GAVL_RGB_24;
      color_type = PNG_COLOR_TYPE_RGB;
      }
    frame_1 = gavl_video_frame_create(&format_1);

    opt = gavl_video_converter_get_options(cnv);
    gavl_video_options_set_alpha_mode(opt, GAVL_ALPHA_BLEND_COLOR);    
    gavl_video_converter_init(cnv, format, &format_1);
    
    gavl_video_convert(cnv, frame, frame_1);
    gavl_video_converter_destroy(cnv);
    }
  else if(format->pixelformat == GAVL_RGB_24)
    {
    color_type = PNG_COLOR_TYPE_RGB;
    }
  else
    {
    color_type = PNG_COLOR_TYPE_RGBA;
    }
  
  output = fopen(filename, "wb");
  if(!output)
    return;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                         NULL, NULL);

  info_ptr = png_create_info_struct(png_ptr);
  setjmp(png_jmpbuf(png_ptr));
  png_init_io(png_ptr, output);
  
  png_set_IHDR(png_ptr, info_ptr,
               format->image_width,
               format->image_height,
               8, color_type, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  rows = malloc(format->image_height * sizeof(*rows));

  if(frame_1)
    {
    for(i = 0; i < format->image_height; i++)
      rows[i] = frame_1->planes[0] + i * frame_1->strides[0];
    }
  else
    {
    for(i = 0; i < format->image_height; i++)
      rows[i] = frame->planes[0] + i * frame->strides[0];
    }
  
  png_set_rows(png_ptr, info_ptr, rows);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(output);
  free(rows);
  if(frame_1)
    gavl_video_frame_destroy(frame_1);
  }

gavl_video_frame_t * read_png(const char * filename,
                              gavl_video_format_t * format,
                              gavl_pixelformat_t pixelformat)
  {
  int i;
  unsigned char ** rows;
  
  gavl_video_converter_t * cnv;
  gavl_video_options_t * opt;
  gavl_video_format_t format_1;
  gavl_video_frame_t * frame, * frame_1;
    
  int bit_depth;
  int color_type;
  int has_alpha = 0;

  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_info;

  FILE * file;
  
  file = fopen(filename, "rb");

  if(!file)
    {
    fprintf(stderr, "Cannot open file %s\n", filename);
    return NULL;
    }
  
  png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, NULL,
     NULL, NULL);
  
  setjmp(png_jmpbuf(png_ptr));
  info_ptr = png_create_info_struct(png_ptr);


  end_info = png_create_info_struct(png_ptr);

  png_init_io(png_ptr, file);

  png_read_info(png_ptr, info_ptr);

  format->frame_width  = png_get_image_width(png_ptr, info_ptr);
  format->frame_height = png_get_image_height(png_ptr, info_ptr);

  format->image_width  = format->frame_width;
  format->image_height = format->frame_height;
  format->pixel_width = 1;
  format->pixel_height = 1;

  bit_depth  = png_get_bit_depth(png_ptr,  info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  switch(color_type)
    {
    case PNG_COLOR_TYPE_GRAY:       /*  (bit depths 1, 2, 4, 8, 16) */
      if(bit_depth < 8)
#if GAVL_MAKE_BUILD(PNG_LIBPNG_VER_MAJOR, PNG_LIBPNG_VER_MINOR, PNG_LIBPNG_VER_RELEASE) < GAVL_MAKE_BUILD(1,2,9)
        png_set_gray_1_2_4_to_8(png_ptr);
#else
      png_set_expand_gray_1_2_4_to_8(png_ptr);
#endif
      if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png_ptr);
        has_alpha = 1;
        }
      png_set_gray_to_rgb(png_ptr);
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA: /*  (bit depths 8, 16) */
      if(bit_depth == 16)
        png_set_strip_16(png_ptr);
      png_set_gray_to_rgb(png_ptr);
      break;
    case PNG_COLOR_TYPE_PALETTE:    /*  (bit depths 1, 2, 4, 8) */
      png_set_palette_to_rgb(png_ptr);
      if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png_ptr);
        has_alpha = 1;
        }
      break;
    case PNG_COLOR_TYPE_RGB:        /*  (bit_depths 8, 16) */
      if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png_ptr);
        has_alpha = 1;
        }
      if(bit_depth == 16)
        png_set_strip_16(png_ptr);
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:  /*  (bit_depths 8, 16) */
      if(bit_depth == 16)
        png_set_strip_16(png_ptr);
      has_alpha = 1;
      break;
    }
  if(has_alpha)
    format->pixelformat = GAVL_RGBA_32;
  else
    format->pixelformat = GAVL_RGB_24;

  frame = gavl_video_frame_create(format);
  rows = malloc(format->frame_height * sizeof(*rows));
  for(i = 0; i < format->frame_height; i++)
    rows[i] = frame->planes[0] + i * frame->strides[0];

  png_read_image(png_ptr, rows);
  png_read_end(png_ptr, end_info);

  png_destroy_read_struct(&png_ptr, &info_ptr,
                          &end_info);
  fclose(file);
  free(rows);
  
  /* Check wether to set up the converter */

  if(format->pixelformat != pixelformat)
    {
    cnv = gavl_video_converter_create();
    opt = gavl_video_converter_get_options(cnv);
    gavl_video_options_set_alpha_mode(opt, GAVL_ALPHA_BLEND_COLOR);    

    gavl_video_format_copy(&format_1, format);
    format_1.pixelformat = pixelformat;
    frame_1 = gavl_video_frame_create(&format_1);
    
    gavl_video_converter_init(cnv, format, &format_1);
    
    gavl_video_convert(cnv, frame, frame_1);
    gavl_video_converter_destroy(cnv);
    format->pixelformat = pixelformat;
    }
  else
    frame_1 = NULL;

  if(frame_1)
    {
    gavl_video_frame_destroy(frame);
    return frame_1;
    }
  else
    return frame;
  }
