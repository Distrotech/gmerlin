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

  } tga_t;

void * create_tga()
  {
  tga_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void destroy_tga(void * priv)
  {
  tga_t * tga = (tga_t*)priv;
  free(tga);
  }

int write_header_tga(void * priv, const char * filename_base,
                     gavl_video_format_t * format)
  {
  
  return 1;
  }

int write_image_tga(void * priv, gavl_video_frame_t * frame)
  {
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

  //  if(!strcmp(name, "compression"))
  //    tga->compression_level = val->val_i;
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
      create:         create_tga,
      destroy:        destroy_tga,
      get_parameters: get_parameters_tga,
      set_parameter:  set_parameter_tga
    },
    write_header: write_header_tga,
    write_image:  write_image_tga,
  };
