/*****************************************************************
 
  iw_tga.c
 
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

#include <targa.h>

#define PADD(i, size) i = ((i + size - 1) / size) * size

/* TGA writer */

typedef struct
  {
  gavl_video_format_t format;
  int rle;
  char * filename;
  } tga_t;

static void * create_tga()
  {
  tga_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_tga(void * priv)
  {
  tga_t * tga = (tga_t*)priv;
  free(tga);
  }

static int write_header_tga(void * priv, const char * filename,
                            gavl_video_format_t * format)
  {
  tga_t * tga = (tga_t*)priv;

  format->colorspace = GAVL_BGR_24;

  gavl_video_format_copy(&(tga->format), format);
  tga->filename = bg_strdup(tga->filename, filename);
  return 1;
  }

static int write_image_tga(void * priv, gavl_video_frame_t * frame)
  {
  tga_t * tga = (tga_t*)priv;
  
  if(tga->rle)
    {
    if(tga_write_bgr(tga->filename, frame->planes[0],
                     tga->format.image_width,
                     tga->format.image_height, 24,
                     frame->strides[0]) != TGA_NOERR)
      return 0;
    }
  else
    {
    if(tga_write_bgr_rle(tga->filename, frame->planes[0],
                         tga->format.image_width,
                         tga->format.image_height, 24,
                         frame->strides[0]) != TGA_NOERR)
      return 0;
    }
  free(tga->filename);
  tga->filename = (char*)0;
  return 1;
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "rle",
      long_name:   "Do RLE compression",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    { /* End of parameters */ }
  };

static bg_parameter_info_t * get_parameters_tga(void * p)
  {
  return parameters;
  }

static void set_parameter_tga(void * p, char * name,
                               bg_parameter_value_t * val)
  {
  tga_t * tga;
  tga = (tga_t *)p;
  
  if(!name)
    return;

  if(!strcmp(name, "rle"))
    tga->rle = val->val_i;
  }

static char * tga_extension = ".tga";

static const char * get_extension_tga(void * p)
  {
  return tga_extension;
  }

bg_image_writer_plugin_t the_plugin =
  {
    common:
    {
      name:           "iw_tga",
      long_name:      "TGA writer",
      mimetypes:      (char*)0,
      extensions:     "tga",
      type:           BG_PLUGIN_IMAGE_WRITER,
      flags:          BG_PLUGIN_FILE,
      priority:       5,
      create:         create_tga,
      destroy:        destroy_tga,
      get_parameters: get_parameters_tga,
      set_parameter:  set_parameter_tga
    },
    write_header: write_header_tga,
    get_extension: get_extension_tga,
    write_image:  write_image_tga,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
API_VERSION;
