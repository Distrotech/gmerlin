/*****************************************************************
 
  iw_png.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <plugin.h>
#include <utils.h>

#include <png.h>

#define PADD(i, size) i = ((i + size - 1) / size) * size

#define BITS_AUTO 0
#define BITS_8    8
#define BITS_16   16

/* PNG writer */

typedef struct
  {
  FILE * output;
  gavl_video_format_t format;

  png_structp png_ptr;
  png_infop   info_ptr;
  
  int compression_level;

  int bit_mode;
  int transform_flags;
  } png_t;


static void * create_png()
  {
  png_t * ret;
  ret = calloc(1, sizeof(*ret));

  return ret;
  }

static void destroy_png(void * priv)
  {
  png_t * png = (png_t*)priv;
  free(png);
  }

static int write_header_png(void * priv, const char * filename,
                     gavl_video_format_t * format)
  {
  int color_type;
  int bits = 8;
  png_t * png = (png_t*)priv;

  png->transform_flags = PNG_TRANSFORM_IDENTITY;
  
  png->output = fopen(filename, "wb");
  if(!png->output)
    return 0;

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
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
//  fprintf(stderr, "LITTLE ENDIAN\n");
  if(bits > 8)
    png->transform_flags |= PNG_TRANSFORM_SWAP_ENDIAN;
#endif
  
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
  
  png_set_compression_level(png->png_ptr, png->compression_level);
  png_set_IHDR(png->png_ptr, png->info_ptr,
               format->image_width,
               format->image_height,
               bits, color_type, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  
  gavl_video_format_copy(&(png->format), format);
  
  return 1;
  }

static int write_image_png(void * priv, gavl_video_frame_t * frame)
  {
  int i;
  unsigned char ** rows;
  png_t * png = (png_t*)priv;

  rows = malloc(png->format.image_height * sizeof(*rows));

  for(i = 0; i < png->format.image_height; i++)
    rows[i] = frame->planes[0] + i * frame->strides[0];

  png_set_rows(png->png_ptr, png->info_ptr, rows);
  png_write_png(png->png_ptr, png->info_ptr, png->transform_flags, NULL);
 
  png_destroy_write_struct(&png->png_ptr, &png->info_ptr);
  fclose(png->output);
  free(rows);
  return 1;
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "compression",
      long_name:   "Compression level",
      type:        BG_PARAMETER_SLIDER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 9 },
      val_default: { val_i: 9 },
    },
    {
      name:        "bit_mode",
      long_name:   "Bits per channel",
      type:        BG_PARAMETER_STRINGLIST,
      multi_names: (char*[]){ "Auto", "8", "16" },
      val_default: { val_str: "8" },
      help_string: "If you select auto, the depth will be chosen according to the input format"
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_png(void * p)
  {
  return parameters;
  }

static void set_parameter_png(void * p, char * name,
                               bg_parameter_value_t * val)
  {
  png_t * png;
  png = (png_t *)p;
  
  if(!name)
    return;

  if(!strcmp(name, "compression"))
    png->compression_level = val->val_i;
  if(!strcmp(name, "bit_mode"))
    {
    fprintf(stderr, "SET BIT MODE: %s\n", val->val_str);
    if(!strcmp(val->val_str, "Auto"))
      png->bit_mode = BITS_AUTO;
    else
      png->bit_mode = atoi(val->val_str);
    }

  }

static char * png_extension = ".png";

static const char * get_extension_png(void * p)
  {
  return png_extension;
  }

bg_image_writer_plugin_t the_plugin =
  {
    common:
    {
      name:           "iw_png",
      long_name:      "PNG writer",
      mimetypes:      (char*)0,
      extensions:     "png",
      type:           BG_PLUGIN_IMAGE_WRITER,
      flags:          BG_PLUGIN_FILE,
      priority:       5,
      create:         create_png,
      destroy:        destroy_png,
      get_parameters: get_parameters_png,
      set_parameter:  set_parameter_png
    },
    get_extension: get_extension_png,
    write_header:  write_header_png,
    write_image:   write_image_png,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
