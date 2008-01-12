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
#include <errno.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>

#include <log.h>
#define LOG_DOMAIN "iw_jpeg"


#include <jpeglib.h>

#define PADD(i, size) i = ((i + size - 1) / size) * size

/* JPEG writer */

typedef struct
  {
  //  char * filename;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
 
  // For writing planar YUV images
   
  uint8_t ** yuv_rows[3];
  uint8_t *  rows_0[16];
  uint8_t *  rows_1[16];
  uint8_t *  rows_2[16];
  
  FILE * output;

  gavl_pixelformat_t pixelformat;
  
  int quality;
  } jpeg_t;

static void * create_jpeg()
  {
  jpeg_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cinfo.err = jpeg_std_error(&(ret->jerr));
  jpeg_create_compress(&(ret->cinfo));

  ret->yuv_rows[0] = ret->rows_0;
  ret->yuv_rows[1] = ret->rows_1;
  ret->yuv_rows[2] = ret->rows_2;

  return ret;
  }

static void destroy_jpeg(void * priv)
  {
  jpeg_t * jpeg = (jpeg_t*)priv;
  jpeg_destroy_compress(&(jpeg->cinfo));
  free(jpeg);
  }

static
int write_header_jpeg(void * priv, const char * filename,
                      gavl_video_format_t * format)
  {
  jpeg_t * jpeg = (jpeg_t*)priv;
  
  jpeg->output = fopen(filename, "wb");
  if(!jpeg->output)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s: %s",
           filename, strerror(errno));
    return 0;
    }
  jpeg_stdio_dest(&(jpeg->cinfo), jpeg->output);

  jpeg->cinfo.image_width  = format->image_width;
  jpeg->cinfo.image_height = format->image_height;

  /* We always have 3 components */

  jpeg->cinfo.input_components = 3;

  /* Set defaults */

  jpeg_set_defaults(&(jpeg->cinfo));
  
  /* Adjust pixelformats */

  format->pixelformat = jpeg->pixelformat;

  jpeg_set_colorspace(&(jpeg->cinfo), JCS_YCbCr);
  jpeg->cinfo.raw_data_in = TRUE;

  format->chroma_placement = GAVL_CHROMA_PLACEMENT_DEFAULT;
  format->interlace_mode = GAVL_INTERLACE_NONE;
  
  switch(format->pixelformat)
    {
    case GAVL_YUVJ_420_P:
      jpeg->cinfo.comp_info[0].h_samp_factor = 2;
      jpeg->cinfo.comp_info[0].v_samp_factor = 2;

      jpeg->cinfo.comp_info[1].h_samp_factor = 1;
      jpeg->cinfo.comp_info[1].v_samp_factor = 1;

      jpeg->cinfo.comp_info[2].h_samp_factor = 1;
      jpeg->cinfo.comp_info[2].v_samp_factor = 1;

      PADD(format->frame_width, 16);
      PADD(format->frame_height, 16);
      break;
    case GAVL_YUVJ_422_P:
      jpeg->cinfo.comp_info[0].h_samp_factor = 2;
      jpeg->cinfo.comp_info[0].v_samp_factor = 1;
      
      jpeg->cinfo.comp_info[1].h_samp_factor = 1;
      jpeg->cinfo.comp_info[1].v_samp_factor = 1;

      jpeg->cinfo.comp_info[2].h_samp_factor = 1;
      jpeg->cinfo.comp_info[2].v_samp_factor = 1;

      PADD(format->frame_width, 16);
      PADD(format->frame_height, 8);
            
      break;
    case GAVL_YUVJ_444_P:
      jpeg->cinfo.comp_info[0].h_samp_factor = 1;
      jpeg->cinfo.comp_info[0].v_samp_factor = 1;
      
      jpeg->cinfo.comp_info[1].h_samp_factor = 1;
      jpeg->cinfo.comp_info[1].v_samp_factor = 1;

      jpeg->cinfo.comp_info[2].h_samp_factor = 1;
      jpeg->cinfo.comp_info[2].v_samp_factor = 1;

      PADD(format->frame_width,  8);
      PADD(format->frame_height, 8);
      
      break;
    default:
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Illegal pixelformat: %s",
             gavl_pixelformat_to_string(jpeg->pixelformat));      
      break;
    }

  /* Set compression parameters */

  jpeg_set_quality(&jpeg->cinfo, jpeg->quality, TRUE);
  jpeg_start_compress(&jpeg->cinfo, TRUE);
  
  return 1;
  }

static
int write_image_jpeg(void * priv, gavl_video_frame_t * frame)
  {
  int i;
  int num_lines;
  jpeg_t * jpeg = (jpeg_t*)priv;

  switch(jpeg->pixelformat)
    {
    case GAVL_YUVJ_420_P:
      while(jpeg->cinfo.next_scanline < jpeg->cinfo.image_height)
        {
        for(i = 0; i < 16; i++)
          {
          jpeg->rows_0[i] = frame->planes[0] + frame->strides[0] *
            (jpeg->cinfo.next_scanline + i);
          }
        for(i = 0; i < 8; i++)
          {
          jpeg->rows_1[i] = frame->planes[1] + frame->strides[1] *
            (jpeg->cinfo.next_scanline/2 + i);
          jpeg->rows_2[i] = frame->planes[2] + frame->strides[2] *
            (jpeg->cinfo.next_scanline/2 + i);
          }
        num_lines = jpeg->cinfo.image_height - jpeg->cinfo.next_scanline;
        if(num_lines > 16)
          num_lines = 16;
        jpeg_write_raw_data(&(jpeg->cinfo), jpeg->yuv_rows, 16);
        }
      break;
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      while(jpeg->cinfo.next_scanline < jpeg->cinfo.image_height)
        {
        for(i = 0; i < 8; i++)
          {
          jpeg->rows_0[i] = frame->planes[0] + frame->strides[0] *
            (jpeg->cinfo.next_scanline + i);
          jpeg->rows_1[i] = frame->planes[1] + frame->strides[1] *
            (jpeg->cinfo.next_scanline + i);
          jpeg->rows_2[i] = frame->planes[2] + frame->strides[2] *
            (jpeg->cinfo.next_scanline + i);
          }
        num_lines = jpeg->cinfo.image_height - jpeg->cinfo.next_scanline;
        if(num_lines > 8)
          num_lines = 8;
        jpeg_write_raw_data(&(jpeg->cinfo), jpeg->yuv_rows, 8);
        }
      break;
    default:
      return 0;
    }
  jpeg_finish_compress(&(jpeg->cinfo));
  fclose(jpeg->output);
  return 1;
  }

/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "quality",
      .long_name =   TRS("Quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 100 },
      .val_default = { .val_i = 95 },
    },
    {
      .name =               "chroma_sampling",
      .long_name =          TRS("Chroma sampling"),
      .type =               BG_PARAMETER_STRINGLIST,
      .val_default =        { .val_str = "4:2:0" },
      .multi_names = (char const *[]) { "4:2:0",
                           "4:2:2",
                           "4:4:4",
                           (char*)0 },
      
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_jpeg(void * p)
  {
  return parameters;
  }


static void set_parameter_jpeg(void * p, const char * name,
                               const bg_parameter_value_t * val)
  {
  jpeg_t * jpeg;
  jpeg = (jpeg_t *)p;
  
  if(!name)
    return;

  if(!strcmp(name, "quality"))
    jpeg->quality = val->val_i;
  else if(!strcmp(name, "chroma_sampling"))
    {
    if(!strcmp(val->val_str, "4:2:0"))
      {
      jpeg->pixelformat = GAVL_YUVJ_420_P;
      }
    else if(!strcmp(val->val_str, "4:2:2"))
      {
      jpeg->pixelformat = GAVL_YUVJ_422_P;
      }
    else if(!strcmp(val->val_str, "4:4:4"))
      {
      jpeg->pixelformat = GAVL_YUVJ_444_P;
      }
    }
  }

static char const * const jpeg_extension = ".jpg";

static const char * get_extension_jpeg(void * p)
  {
  return jpeg_extension;
  }

const bg_image_writer_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "iw_jpeg",
      .long_name =      TRS("JPEG writer"),
      .description =    TRS("Writer for JPEG images"),
      .mimetypes =      (char*)0,
      .extensions =     "jpeg jpg",
      .type =           BG_PLUGIN_IMAGE_WRITER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       BG_PLUGIN_PRIORITY_MAX,
      .create =         create_jpeg,
      .destroy =        destroy_jpeg,
      .get_parameters = get_parameters_jpeg,
      .set_parameter =  set_parameter_jpeg
    },
    .get_extension = get_extension_jpeg,
    .write_header = write_header_jpeg,
    .write_image =  write_image_jpeg,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
