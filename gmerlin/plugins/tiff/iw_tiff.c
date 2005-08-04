
/*****************************************************************
 
  iw_tiff.c
 
  Copyright (c) 2003-2004 by Michael Gruenert - one78@web.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>
#include <inttypes.h>
#include <plugin.h>

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
  p->output = TIFFOpen(filename,"w");

  if(!p->output)
    return 0;

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

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "compression",
      long_name:   "Compression",
      type:        BG_PARAMETER_STRINGLIST,
      multi_names: (char*[]){ "none", "packbits", "deflate", "jpeg", (char*)0 },
      multi_labels:  (char*[]){ "None", "Packbits", "Deflate", "JPEG", (char*)0 },

      val_default: { val_str: "none" },
    },
    {
      name:        "jpeg_quality",
      long_name:   "JPEG Quality",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 100 },
      val_default: { val_i: 75 }      
    },
    {
      name:        "zip_quality",
      long_name:   "Deflate compression level",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 9 },
      val_default: { val_i: 6 }      
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_tiff(void * p)
  {
  return parameters;
  }

static void set_parameter_tiff(void * p, char * name,
                               bg_parameter_value_t * val)
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

static char * tiff_extension = ".tif";

static const char * get_extension_tiff(void * p)
  {
  return tiff_extension;
  }

bg_image_writer_plugin_t the_plugin =
  {
    common:
    {
      name:           "iw_tiff",
      long_name:      "TIFF writer",
      mimetypes:      (char*)0,
      extensions:     "tif",
      type:           BG_PLUGIN_IMAGE_WRITER,
      flags:          BG_PLUGIN_FILE,
      priority:       5,
      create:         create_tiff,
      destroy:        destroy_tiff,
      get_parameters: get_parameters_tiff,
      set_parameter:  set_parameter_tiff
    },
    get_extension: get_extension_tiff,
    write_header:  write_header_tiff,
    write_image:   write_image_tiff,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;

