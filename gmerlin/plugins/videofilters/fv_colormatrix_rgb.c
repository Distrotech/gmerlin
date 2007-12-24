/*****************************************************************
 
  fv_colormatrix_rgb.c
 
  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>
#include <log.h>

#include "colormatrix.h"

#define LOG_DOMAIN "fv_colormatrix_rgb"

typedef struct
  {
  bg_colormatrix_t * mat;
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;

  gavl_video_frame_t * frame;
  
  float coeffs[4][5];
  } colormatrix_priv_t;

static void * create_colormatrix()
  {
  colormatrix_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->mat = bg_colormatrix_create();
  return ret;
  }

static void destroy_colormatrix(void * priv)
  {
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "Red",
      long_name: TRS("Red"),
      type: BG_PARAMETER_SECTION,
      flags: BG_PARAMETER_SYNC,
    },
    {
      name: "r_to_r",
      long_name: TRS("Red -> Red"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   1.0 },
      num_digits:  6,
    },
    {
      name: "g_to_r",
      long_name: TRS("Green -> Red"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "b_to_r",
      long_name: TRS("Blue -> Red"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "a_to_r",
      long_name: TRS("Alpha -> Red"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "off_r",
      long_name: TRS("Red offset"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "Green",
      long_name: TRS("Green"),
      type: BG_PARAMETER_SECTION,
      flags: BG_PARAMETER_SYNC,
    },
    {
      name: "r_to_g",
      long_name: TRS("Red -> Green"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "g_to_g",
      long_name: TRS("Green -> Green"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   1.0 },
      num_digits:  6,
    },
    {
      name: "b_to_g",
      long_name: TRS("Blue -> Green"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "a_to_g",
      long_name: TRS("Alpha -> Green"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "off_g",
      long_name: TRS("Green offset"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "Blue",
      long_name: TRS("Blue"),
      type: BG_PARAMETER_SECTION,
      flags: BG_PARAMETER_SYNC,
    },
    {
      name: "r_to_b",
      long_name: TRS("Red -> Blue"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "g_to_b",
      long_name: TRS("Green -> Blue"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "b_to_b",
      long_name: TRS("Blue -> Blue"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   1.0 },
      num_digits:  6,
    },
    {
      name: "a_to_b",
      long_name: TRS("Alpha -> Blue"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "off_b",
      long_name: TRS("Blue offset"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "Alpha",
      long_name: TRS("Alpha"),
      type: BG_PARAMETER_SECTION,
      flags: BG_PARAMETER_SYNC,
    },
    {
      name: "r_to_a",
      long_name: TRS("Red -> Alpha"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "g_to_a",
      long_name: TRS("Green -> Alpha"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "b_to_a",
      long_name: TRS("Blue -> Alpha"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    {
      name: "a_to_a",
      long_name: TRS("Alpha -> Alpha"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   1.0 },
      num_digits:  6,
    },
    {
      name: "off_a",
      long_name: TRS("Alpha offset"),
      type: BG_PARAMETER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits:  6,
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_colormatrix(void * priv)
  {
  return parameters;
  }

#define MATRIX_PARAM(n, r, c) \
  else if(!strcmp(name, n)) \
    { \
    if(vp->coeffs[r][c] != val->val_f) \
      { \
      vp->coeffs[r][c] = val->val_f; \
      changed = 1; \
      } \
    }

static void set_parameter_colormatrix(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  int changed = 0;
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;

  if(!name)
    return;
  MATRIX_PARAM("r_to_r", 0, 0)
  MATRIX_PARAM("g_to_r", 0, 1)
  MATRIX_PARAM("b_to_r", 0, 2)
  MATRIX_PARAM("a_to_r", 0, 3)
  MATRIX_PARAM( "off_r", 0, 4)

  MATRIX_PARAM("r_to_g", 1, 0)
  MATRIX_PARAM("g_to_g", 1, 1)
  MATRIX_PARAM("b_to_g", 1, 2)
  MATRIX_PARAM("a_to_g", 1, 3)
  MATRIX_PARAM( "off_g", 1, 4)

  MATRIX_PARAM("r_to_b", 2, 0)
  MATRIX_PARAM("g_to_b", 2, 1)
  MATRIX_PARAM("b_to_b", 2, 2)
  MATRIX_PARAM("a_to_b", 2, 3)
  MATRIX_PARAM( "off_b", 2, 4)

  MATRIX_PARAM("r_to_a", 3, 0)
  MATRIX_PARAM("g_to_a", 3, 1)
  MATRIX_PARAM("b_to_a", 3, 2)
  MATRIX_PARAM("a_to_a", 3, 3)
  MATRIX_PARAM( "off_a", 3, 4)
  
  if(changed)
    bg_colormatrix_set_rgba(vp->mat, vp->coeffs);
  }

static void connect_input_port_colormatrix(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;
  
  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  }

static void set_input_format_colormatrix(void * priv, gavl_video_format_t * format, int port)
  {
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;

  if(!port)
    {
    bg_colormatrix_init(vp->mat, format);
    gavl_video_format_copy(&vp->format, format);
    }
  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = (gavl_video_frame_t*)0;
    }
  }

static void get_output_format_colormatrix(void * priv, gavl_video_format_t * format)
  {
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_colormatrix(void * priv, gavl_video_frame_t * frame, int stream)
  {
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;

#if 0  
  if(!vp->colormatrix_h && !vp->colormatrix_v)
    {
    return vp->read_func(vp->read_data, frame, vp->read_stream);
    }
#endif
  if(!vp->frame)
    {
    vp->frame = gavl_video_frame_create(&vp->format);
    gavl_video_frame_clear(vp->frame, &vp->format);
    }
  if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
    return 0;
  
  bg_colormatrix_process(vp->mat, vp->frame, frame);
  frame->timestamp = vp->frame->timestamp;
  frame->duration = vp->frame->duration;
  return 1;
  }

bg_fv_plugin_t the_plugin = 
  {
    common:
    {
      BG_LOCALE,
      name:      "fv_colormatrix",
      long_name: TRS("RGB Colormatrix"),
      description: TRS("Colormatrix video images horizontally and/or vertically"),
      type:     BG_PLUGIN_FILTER_VIDEO,
      flags:    BG_PLUGIN_FILTER_1,
      create:   create_colormatrix,
      destroy:   destroy_colormatrix,
      get_parameters:   get_parameters_colormatrix,
      set_parameter:    set_parameter_colormatrix,
      priority:         1,
    },
    
    connect_input_port: connect_input_port_colormatrix,
    
    set_input_format: set_input_format_colormatrix,
    get_output_format: get_output_format_colormatrix,

    read_video: read_video_colormatrix,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
