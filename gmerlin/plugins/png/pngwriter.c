/*****************************************************************
 
  pngwriter.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <errno.h>

#include <config.h>
#include <translation.h>

#include <log.h>
#define LOG_DOMAIN "pngwriter"

#include <plugin.h>
#include <utils.h>

#include <png.h>


#include "pngwriter.h"

int bg_pngwriter_write_header(void * priv, const char * filename,
                              gavl_video_format_t * format)
  {
  int color_type;
  int bits = 8;
  bg_pngwriter_t * png = (bg_pngwriter_t*)priv;

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
#ifdef GAVL_PROCESSOR_LITTLE_ENDIAN
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

int bg_pngwriter_write_image(void * priv, gavl_video_frame_t * frame)
  {
  int i;
  unsigned char ** rows;
  bg_pngwriter_t * png = (bg_pngwriter_t*)priv;

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

void bg_pngwriter_set_parameter(void * p, const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_pngwriter_t * png;
  png = (bg_pngwriter_t *)p;
  
  if(!name)
    return;

  if(!strcmp(name, "compression"))
    png->compression_level = val->val_i;
  if(!strcmp(name, "bit_mode"))
    {
    if(!strcmp(val->val_str, "Auto"))
      png->bit_mode = BITS_AUTO;
    else
      png->bit_mode = atoi(val->val_str);
    }

  }
