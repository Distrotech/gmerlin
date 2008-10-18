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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <tiffio.h>
#include <inttypes.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "iw_tiff"

typedef struct
  {
  TIFF *output;
  uint32_t Width;
  uint32_t Height;
  uint16 SamplesPerPixel;
  uint16 Orientation;
  gavl_video_format_t format;
  uint16_t compression;
  int jpeg_quality;
  int zip_quality;  /* Compression level, actually */
  } tiff_t;

static void * create_tiff()
  {
  tiff_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_tiff(void* priv)
  {
  tiff_t * tiff = (tiff_t*)priv;
  free(tiff);
  }

static int write_header_tiff(void * priv, const char * filename,
                     gavl_video_format_t * format)
  {
  tiff_t * p = (tiff_t*)priv;
  uint16_t v[1];

  p->Width =  format->image_width;
  p->Height =  format->image_height;

  if(gavl_pixelformat_has_alpha(format->pixelformat))
    {
    format->pixelformat = GAVL_RGBA_32;
    p->SamplesPerPixel = 4;
    }
  else
    {
    format->pixelformat = GAVL_RGB_24;
    p->SamplesPerPixel = 3;
    }

  errno = 0;
  p->output = TIFFOpen(filename,"w");

  if(!p->output)
    {
    
    if(errno)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s: %s",
             filename, strerror(errno));
    else
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s",  filename);
    return 0;
    }
  
  TIFFSetField(p->output, TIFFTAG_IMAGEWIDTH, p->Width);
  TIFFSetField(p->output, TIFFTAG_IMAGELENGTH, p->Height);
  TIFFSetField(p->output, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(p->output, TIFFTAG_SAMPLESPERPIXEL, p->SamplesPerPixel);
  TIFFSetField(p->output, TIFFTAG_COMPRESSION, p->compression);
  if(p->compression == COMPRESSION_JPEG)
    TIFFSetField(p->output, TIFFTAG_JPEGQUALITY, p->jpeg_quality);
  if(p->compression == COMPRESSION_DEFLATE)
    TIFFSetField(p->output, TIFFTAG_ZIPQUALITY, p->zip_quality);
  TIFFSetField(p->output, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
  TIFFSetField(p->output, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(p->output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(p->output, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

  if(p->SamplesPerPixel == 4)
    {
    v[0] = EXTRASAMPLE_ASSOCALPHA;
    TIFFSetField(p->output, TIFFTAG_EXTRASAMPLES, 1, v);
    }
  return 1;
  }

static int write_image_tiff(void *priv, gavl_video_frame_t *frame)
  {
  int i;
  tiff_t *p = (tiff_t*)priv;
  uint8_t * frame_ptr_start;
  
  frame_ptr_start = frame->planes[0];

  for(i = 0; i < p->Height; i++)
    {
    TIFFWriteScanline(p->output, frame_ptr_start, i, 0);
    frame_ptr_start += frame->strides[0];
    }

  TIFFClose(p->output);
  return 1;
  }


/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "compression",
      .long_name =   TRS("Compression"),
      .type =        BG_PARAMETER_STRINGLIST,
      .multi_names = (char const *[]){ "none", "packbits", "deflate", "jpeg", (char*)0 },
      .multi_labels =  (char const *[]){ TRS("None"), TRS("Packbits"), TRS("Deflate"), TRS("JPEG"),
                                (char*)0 },

      .val_default = { .val_str = "none" },
    },
    {
      .name =        "jpeg_quality",
      .long_name =   TRS("JPEG quality"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 100 },
      .val_default = { .val_i = 75 }      
    },
    {
      .name =        "zip_quality",
      .long_name =   TRS("Deflate compression level"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 9 },
      .val_default = { .val_i = 6 }      
    },
    { /* End of parameters */ }
  };

static const bg_parameter_info_t * get_parameters_tiff(void * p)
  {
  return parameters;
  }

static void set_parameter_tiff(void * p, const char * name,
                               const bg_parameter_value_t * val)
  {
  tiff_t * tiff;
  tiff = (tiff_t *)p;

  if(!name)
    return;

  else if(!strcmp(name, "compression"))
    {
    if(!strcmp(val->val_str, "none"))
      tiff->compression = COMPRESSION_NONE;
    else if(!strcmp(val->val_str, "packbits"))
      tiff->compression = COMPRESSION_PACKBITS;
    else if(!strcmp(val->val_str, "deflate"))
      tiff->compression = COMPRESSION_DEFLATE;
    else if(!strcmp(val->val_str, "jpeg"))
      tiff->compression = COMPRESSION_JPEG;
    }
  else if(!strcmp(name, "jpeg_quality"))
    tiff->jpeg_quality = val->val_i;
  else if(!strcmp(name, "zip_quality"))
    tiff->zip_quality = val->val_i;
  
  }

static char const * const tiff_extension = ".tif";

static const char * get_extension_tiff(void * p)
  {
  return tiff_extension;
  }


const bg_image_writer_plugin_t the_plugin =
  {
    .common =
    {
      BG_LOCALE,
      .name =           "iw_tiff",
      .long_name =      TRS("TIFF writer"),
      .description =    TRS("Writer for TIFF images"),
      .type =           BG_PLUGIN_IMAGE_WRITER,
      .flags =          BG_PLUGIN_FILE,
      .priority =       5,
      .create =         create_tiff,
      .destroy =        destroy_tiff,
      .get_parameters = get_parameters_tiff,
      .set_parameter =  set_parameter_tiff
    },
    .get_extension = get_extension_tiff,
    .write_header =  write_header_tiff,
    .write_image =   write_image_tiff,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

