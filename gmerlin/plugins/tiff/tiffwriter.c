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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <tiffio.h>
#include <inttypes.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginfuncs.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "tiffwriter"

#include "tiffwriter.h"

struct bg_tiff_writer_s
  {
  TIFF *output;
  gavl_video_format_t format;
  uint16_t compression;
  int jpeg_quality;
  int zip_level;  /* Compression level, actually */

  int packet_pos;
  gavl_packet_t * p;
  };

bg_tiff_writer_t * bg_tiff_writer_create()
  {
  bg_tiff_writer_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "compression",
      .long_name =   TRS("Compression"),
      .type =        BG_PARAMETER_STRINGLIST,
      .multi_names = (char const *[]){ "none",
                                       "packbits",
                                       "deflate",
                                       "jpeg", NULL },
      
      .multi_labels =  (char const *[]){ TRS("None"),
                                         TRS("Packbits"),
                                         TRS("Deflate"),
                                         TRS("JPEG"),
                                         NULL },
      
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
      .name =        "zip_level",
      .long_name =   TRS("Deflate compression level"),
      .type =        BG_PARAMETER_SLIDER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 9 },
      .val_default = { .val_i = 9 }      
    },
    { /* End of parameters */ }
  };


const bg_parameter_info_t * bg_tiff_writer_get_parameters(void)
  {
  return parameters;
  }

void bg_tiff_writer_destroy(bg_tiff_writer_t * t)
  {
  free(t);
  }

void bg_tiff_writer_set_parameter(bg_tiff_writer_t * tiff, const char * name,
                                  const bg_parameter_value_t * val)
  {
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
  else if(!strcmp(name, "zip_level"))
    tiff->zip_level = val->val_i;
  
  }

static tsize_t read_function(thandle_t fd, tdata_t data, tsize_t length)
  {
  uint32_t bytes_read;

  bg_tiff_writer_t * tiff = (bg_tiff_writer_t *)fd;

  fprintf(stderr, "read_function %d [%d %d]\n", length,
          tiff->p->data_len, tiff->packet_pos);
  
  bytes_read = length;
  if(bytes_read > tiff->p->data_len - tiff->packet_pos)
    bytes_read = tiff->p->data_len - tiff->packet_pos;

  memcpy(data, tiff->p->data + tiff->packet_pos, bytes_read);
  tiff->packet_pos += bytes_read;
  return bytes_read;
  }

static toff_t seek_function(thandle_t fd, toff_t off, int whence)
  {
  bg_tiff_writer_t * tiff = (bg_tiff_writer_t *)fd;
  
  fprintf(stderr, "seek_function %d %d  [%d %d]\n", off, whence,
          tiff->p->data_len, tiff->packet_pos);
  
  if (whence == SEEK_SET)
    tiff->packet_pos = off;
  else if (whence == SEEK_CUR)
    tiff->packet_pos += off;
  else if (whence == SEEK_END)
    tiff->packet_pos = tiff->p->data_len + off;
  
  if(tiff->packet_pos > tiff->p->data_len)
    {
    /* Realloc */
    gavl_packet_alloc(tiff->p, tiff->packet_pos);
    memset(tiff->p->data + tiff->p->data_len, 0, tiff->packet_pos - tiff->p->data_len);
    tiff->p->data_len = tiff->packet_pos;
    }
  return tiff->packet_pos;
  }

static toff_t size_function(thandle_t fd)
  {
  bg_tiff_writer_t * tiff = (bg_tiff_writer_t *)fd;
  return tiff->p->data_len;
  }

static int close_function(thandle_t fd)
  {
  return 0;
  }

static tsize_t write_function(thandle_t fd, tdata_t data, tsize_t length)
  {
  bg_tiff_writer_t * tiff = (bg_tiff_writer_t *)fd;

  fprintf(stderr, "write_function %d [%d %d]\n", length,
          tiff->p->data_len, tiff->packet_pos);
  
  /* Realloc */
  if(tiff->packet_pos + length > tiff->p->data_len)
    gavl_packet_alloc(tiff->p, tiff->packet_pos + length);

  /* Copy + advance */
  memcpy(tiff->p->data + tiff->packet_pos, data, length);
  tiff->packet_pos += length;

  /* Adjust length */
  if(tiff->p->data_len < tiff->packet_pos)
    tiff->p->data_len = tiff->packet_pos;
  
  return length;
  }

static int map_file_proc(thandle_t a, tdata_t* b, toff_t* c)
  {
  return 0;
  }

static void unmap_file_proc(thandle_t a, tdata_t b, toff_t c)
  {
  }



int bg_tiff_writer_write_header(bg_tiff_writer_t * tiff, const char * filename,
                         gavl_packet_t * p,
                         gavl_video_format_t * format,
                         const gavl_metadata_t * metadata)
  {
  uint16_t v[1];
  int photometric;
  int has_alpha;
  uint16_t samples_per_pixel;

  
  /* Open I/O */
  if(filename)
    {
    tiff->output = TIFFOpen(filename,"w");
    
    if(!tiff->output)
      {
      if(errno)
        bg_log(BG_LOG_ERROR, LOG_DOMAIN,
               "Cannot open %s: %s",
               filename, strerror(errno));
      else
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open %s",  filename);
      return 0;
      }
    }
  else /* Packet */
    {
    tiff->p = p;
    gavl_packet_reset(tiff->p);
    tiff->output = TIFFClientOpen("gmerlin", "w", (thandle_t)tiff,
                                  read_function,write_function ,
                                  seek_function, close_function,
                                  size_function, map_file_proc,
                                  unmap_file_proc);
    }
  
  /* Adjust format */
  
  if(gavl_pixelformat_has_alpha(format->pixelformat))
    {
    if(gavl_pixelformat_is_gray(format->pixelformat))
      {
      format->pixelformat = GAVL_GRAYA_16;
      samples_per_pixel = 2;
      photometric = PHOTOMETRIC_MINISBLACK;
      }
    else
      {
      format->pixelformat = GAVL_RGBA_32;
      samples_per_pixel = 4;
      photometric = PHOTOMETRIC_RGB;
      }
    has_alpha = 1;
    }
  else
    {
    if(gavl_pixelformat_is_gray(format->pixelformat))
      {
      format->pixelformat = GAVL_GRAY_8;
      samples_per_pixel = 1;
      photometric = PHOTOMETRIC_MINISBLACK;
      }
    else
      {
      format->pixelformat = GAVL_RGB_24;
      samples_per_pixel = 3;
      photometric = PHOTOMETRIC_RGB;
      }
    has_alpha = 0;
    }
  
  TIFFSetField(tiff->output, TIFFTAG_IMAGEWIDTH, format->image_width);
  TIFFSetField(tiff->output, TIFFTAG_IMAGELENGTH, format->image_height);
  TIFFSetField(tiff->output, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tiff->output, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel);
  TIFFSetField(tiff->output, TIFFTAG_COMPRESSION, tiff->compression);
  if(tiff->compression == COMPRESSION_JPEG)
    TIFFSetField(tiff->output, TIFFTAG_JPEGQUALITY, tiff->jpeg_quality);
  if(tiff->compression == COMPRESSION_DEFLATE)
    TIFFSetField(tiff->output, TIFFTAG_ZIPQUALITY, tiff->zip_level);
  TIFFSetField(tiff->output, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
  TIFFSetField(tiff->output, TIFFTAG_PHOTOMETRIC, photometric);
  TIFFSetField(tiff->output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tiff->output, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  
  if(has_alpha)
    {
    v[0] = EXTRASAMPLE_ASSOCALPHA;
    TIFFSetField(tiff->output, TIFFTAG_EXTRASAMPLES, 1, v);
    }

  gavl_video_format_copy(&tiff->format, format);
  return 1;
  }

int bg_tiff_writer_write_image(bg_tiff_writer_t * tiff, gavl_video_frame_t * frame)
  {
  int i;
  uint8_t * frame_ptr_start;
  
  frame_ptr_start = frame->planes[0];

  for(i = 0; i < tiff->format.image_height; i++)
    {
    TIFFWriteScanline(tiff->output, frame_ptr_start, i, 0);
    frame_ptr_start += frame->strides[0];
    }

  TIFFClose(tiff->output);
  tiff->output = NULL;
  tiff->packet_pos = 0;
  return 1;
  }

