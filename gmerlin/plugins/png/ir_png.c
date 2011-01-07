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

#include <config.h>
#include <gmerlin/bg_version.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

#include <png.h>

typedef struct
  {
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_info;

  gavl_video_format_t format;

  FILE * file;
  
  bg_metadata_t metadata;
  } png_t;

static void * create_png()
  {
  png_t * ret;
  ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void destroy_png(void* priv)
  {
  png_t * png = priv;

  free(png);
  }

static int read_header_png(void * priv, const char * filename,
                     gavl_video_format_t * format)
  {
  int bit_depth;
  int color_type;
  int has_alpha = 0;
  int is_gray = 0;
  
  png_text * text;
  int num_text;
  
  png_byte signature[8];
  png_t * png = priv;

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
#ifndef WORDS_BIGENDIAN
        png_set_swap(png->png_ptr);
#endif
        bits = 16;
        }
      if(bit_depth < 8)
#if BG_MAKE_BUILD(PNG_LIBPNG_VER_MAJOR, PNG_LIBPNG_VER_MINOR, PNG_LIBPNG_VER_RELEASE) < BG_MAKE_BUILD(1,2,9)
        png_set_gray_1_2_4_to_8(png->png_ptr);
#else
      png_set_expand_gray_1_2_4_to_8(png->png_ptr);
#endif

        png_set_gray_1_2_4_to_8(png->png_ptr);
      if (png_get_valid(png->png_ptr, png->info_ptr, PNG_INFO_tRNS))
        {
        png_set_tRNS_to_alpha(png->png_ptr);
        has_alpha = 1;
        }
      // png_set_gray_to_rgb(png->png_ptr);
      is_gray = 1;
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA: /*  (bit depths 8, 16) */
      if(bit_depth == 16)
        {
#ifndef WORDS_BIGENDIAN
        png_set_swap(png->png_ptr);
#endif
        bits = 16;
        }
      has_alpha = 1;
      // png_set_gray_to_rgb(png->png_ptr);
      is_gray = 1;
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
#ifndef WORDS_BIGENDIAN
        png_set_swap(png->png_ptr);
#endif
        bits = 16;
        }
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:  /*  (bit_depths 8, 16) */
      if(bit_depth == 16)
        {
#ifndef WORDS_BIGENDIAN
        png_set_swap(png->png_ptr);
#endif
        bits = 16;
        }
      has_alpha = 1;
      break;
    }

  if(is_gray)
    {
    if(bits == 8)
      {
      if(has_alpha)
        format->pixelformat = GAVL_GRAYA_16;
      else
        format->pixelformat = GAVL_GRAY_8;
      }
    else if(bits == 16)
      {
      if(has_alpha)
        format->pixelformat = GAVL_GRAYA_32;
      else
        format->pixelformat = GAVL_GRAY_16;
      }
    }
  else
    {
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
    }

  if(png_get_text(png->png_ptr, png->info_ptr, &text, &num_text))
    {
    int i;
    for(i = 0; i < num_text; i++)
      {
      if(text[i].compression == PNG_TEXT_COMPRESSION_NONE)
        {
        if(!strcasecmp(text[i].key, "Author"))
          png->metadata.author = bg_strdup(png->metadata.author, text[i].text);
        else if(!strcasecmp(text[i].key, "Title"))
          png->metadata.title = bg_strdup(png->metadata.title, text[i].text);
        else if(!strcasecmp(text[i].key, "Copyright"))
          png->metadata.copyright = bg_strdup(png->metadata.copyright, text[i].text);
        else
          bg_metadata_append_ext(&png->metadata, text[i].key, text[i].text);
        // fprintf(stderr, "png text: key: %s text: %s\n", text[i].key, text[i].text);
        }
      }
    }
  
  gavl_video_format_copy(&png->format, format);
  return 1;
  fail:
  return 0;

  }

static int get_compression_info_png(void * priv, gavl_compression_info_t * ci)
  {
  ci->id = GAVL_CODEC_ID_PNG;
  return 1;
  }


static const bg_metadata_t * get_metadata_png(void * priv)
  {
  png_t * png = priv;
  return(&png->metadata);
  }

static int read_image_png(void * priv, gavl_video_frame_t * frame)
  {
  int i;
  unsigned char ** rows = NULL;
  png_t * png = priv;


  if(frame)
    {
    setjmp(png_jmpbuf(png->png_ptr));
    rows = malloc(png->format.frame_height * sizeof(*rows));
    for(i = 0; i < png->format.frame_height; i++)
      rows[i] = frame->planes[0] + i * frame->strides[0];
    
    png_read_image(png->png_ptr, rows);
    png_read_end(png->png_ptr, png->end_info);
    }
  
  png_destroy_read_struct(&png->png_ptr, &png->info_ptr,
                          &png->end_info);
  fclose(png->file);

  if(rows)
    free(rows);
  bg_metadata_free(&png->metadata);
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
      .type =          BG_PLUGIN_IMAGE_READER,
      .flags =         BG_PLUGIN_FILE,
      .priority =      BG_PLUGIN_PRIORITY_MAX,
      .create =        create_png,
      .destroy =       destroy_png,
    },
    .extensions =    "png",
    .read_header = read_header_png,
    .get_metadata = get_metadata_png,
    .get_compression_info = get_compression_info_png,
    .read_image =  read_image_png,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
