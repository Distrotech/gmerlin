/*****************************************************************
 
  ir_tga.c
 
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

#include <log.h>
#define LOG_DOMAIN "ir_tga"

#include "targa.h"

typedef struct
  {
  tga_image tga;
  gavl_video_format_t format;
  gavl_video_frame_t * frame;
  int bytes_per_pixel;

  char * filename;
  
  } tga_t;

static void * create_tga()
  {
  tga_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->frame = gavl_video_frame_create(NULL);
  return ret;
  }

static void destroy_tga(void* priv)
  {
  tga_t * tga = (tga_t*)priv;

  if(tga->frame)
    {
    gavl_video_frame_null(tga->frame);
    gavl_video_frame_destroy(tga->frame);
    }
  free(tga);
  }

static gavl_pixelformat_t get_pixelformat(int depth, int * bytes_per_pixel)
  {
  switch(depth)
    {
    case 16:
      *bytes_per_pixel = 2;
      return GAVL_BGR_15;
      break;
    case 24:
      *bytes_per_pixel = 3;
      return GAVL_BGR_24;
      break;
    case 32:
      *bytes_per_pixel = 4;
      return GAVL_RGBA_32;
      break;
    default:
      return GAVL_PIXELFORMAT_NONE;
    }
  }

static int read_header_tga(void * priv, const char * filename,
                    gavl_video_format_t * format)
  {
  tga_t * tga = (tga_t*)priv;
  
  if(tga_read(&(tga->tga), filename) != TGA_NOERR)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Read tga failed");
    return 0;
    }
  /* Get header */

  format->frame_width = tga->tga.width;
  format->frame_height = tga->tga.height;

  format->image_width = tga->tga.width;
  format->image_height = tga->tga.height;
  
  format->pixel_width = 1;
  format->pixel_height = 1;
  switch(tga->tga.image_type)
    {
    case TGA_IMAGE_TYPE_COLORMAP:
    case TGA_IMAGE_TYPE_COLORMAP_RLE:
      format->pixelformat = get_pixelformat(tga->tga.color_map_depth,
                                          &(tga->bytes_per_pixel));
      break;
    default:
      format->pixelformat = get_pixelformat(tga->tga.pixel_depth,
                                          &(tga->bytes_per_pixel));
      break;
    }

  if(format->pixelformat == GAVL_PIXELFORMAT_NONE)
    goto fail;

  gavl_video_format_copy(&(tga->format), format);
  
  return 1;
  fail:
  return 0;

  }

static int read_image_tga(void * priv, gavl_video_frame_t * frame)
  {
  int ret;
  tga_t * tga = (tga_t*)priv;

  ret = 0;
  
  switch(tga->tga.image_type)
    {
    case TGA_IMAGE_TYPE_COLORMAP:
    case TGA_IMAGE_TYPE_COLORMAP_RLE:
      if(tga_color_unmap(&tga->tga) != TGA_NOERR)
        goto fail;
      break;
    default:
      break;
    }

  if(tga->format.pixelformat == GAVL_RGBA_32)
    tga_swap_red_blue(&(tga->tga));
  
  tga->frame->strides[0] = tga->bytes_per_pixel * tga->format.image_width;
  tga->frame->planes[0] = tga->tga.image_data;
  
  /* Figure out the copy function */

  if(tga->tga.image_descriptor & TGA_R_TO_L_BIT)
    {
    if(tga->tga.image_descriptor & TGA_T_TO_B_BIT)
      {
      gavl_video_frame_copy_flip_x(&(tga->format), frame, tga->frame);
      }
    else
      {
      gavl_video_frame_copy_flip_xy(&(tga->format), frame, tga->frame);
      }
    }
  else
    {
    if(tga->tga.image_descriptor & TGA_T_TO_B_BIT)
      {
      gavl_video_frame_copy(&(tga->format), frame, tga->frame);
      }
    else
      {
      gavl_video_frame_copy_flip_y(&(tga->format), frame, tga->frame);
      }
    }

  ret = 1;
  fail:

  /* Free anything */

  tga_free_buffers(&tga->tga);
  memset(&tga->tga, 0, sizeof(tga->tga));
  return ret;
  }

bg_image_reader_plugin_t the_plugin =
  {
    common:
    {
      name:          "ir_tga",
      long_name:     "TGA loader",
      mimetypes:     (char*)0,
      extensions:    "tga",
      type:          BG_PLUGIN_IMAGE_READER,
      flags:         BG_PLUGIN_FILE,
      priority:      BG_PLUGIN_PRIORITY_MAX,
      create:        create_tga,
      destroy:       destroy_tga,
    },
    read_header: read_header_tga,
    read_image:  read_image_tga,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
