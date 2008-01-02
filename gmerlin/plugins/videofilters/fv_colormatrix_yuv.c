/*****************************************************************
 
  fv_colormatrix_yuv.c
 
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

  float coeffs[4][5];
  int force_alpha;
  int need_restart;

  } colormatrix_priv_t;

static int need_restart_colormatrix(void * priv)
  {
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;
  return vp->need_restart;
  }

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
  bg_colormatrix_destroy(vp->mat);
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "Luminance",
      long_name: TRS("Luminance"),
      type: BG_PARAMETER_SECTION,
      flags: BG_PARAMETER_SYNC,
    },
    {
      name: "y_to_y",
      long_name: TRS("Luminance -> Luminance"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   1.0 },
      num_digits:  3,
    },
    {
      name: "u_to_y",
      long_name: TRS("Cb -> Luminance"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "v_to_y",
      long_name: TRS("Cr -> Luminance"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "a_to_y",
      long_name: TRS("Alpha -> Luminance"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "off_y",
      long_name: TRS("Luminance offset"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "Cb",
      long_name: TRS("Cb"),
      type: BG_PARAMETER_SECTION,
      flags: BG_PARAMETER_SYNC,
    },
    {
      name: "r_to_u",
      long_name: TRS("Luminance -> Cb"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "u_to_u",
      long_name: TRS("Cb -> Cb"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   1.0 },
      num_digits:  3,
    },
    {
      name: "b_to_u",
      long_name: TRS("Cr -> Cb"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "a_to_u",
      long_name: TRS("Alpha -> Cb"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "off_u",
      long_name: TRS("Cb offset"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "Cr",
      long_name: TRS("Cr"),
      type: BG_PARAMETER_SECTION,
      flags: BG_PARAMETER_SYNC,
    },
    {
      name: "y_to_v",
      long_name: TRS("Luminance -> Cr"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "u_to_v",
      long_name: TRS("Cb -> Cr"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "v_to_v",
      long_name: TRS("Cr -> Cr"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   1.0 },
      num_digits:  3,
    },
    {
      name: "a_to_v",
      long_name: TRS("Alpha -> Cr"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "off_v",
      long_name: TRS("Cr offset"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
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
      name: "y_to_a",
      long_name: TRS("Luminance -> Alpha"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "u_to_a",
      long_name: TRS("Cb -> Alpha"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "v_to_a",
      long_name: TRS("Cr -> Alpha"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "a_to_a",
      long_name: TRS("Alpha -> Alpha"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   1.0 },
      num_digits:  3,
    },
    {
      name: "off_a",
      long_name: TRS("Alpha offset"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_f: -2.0 },
      val_max:     { val_f:  2.0 },
      val_default: { val_f:   0.0 },
      num_digits:  3,
    },
    {
      name: "misc",
      long_name: TRS("Misc"),
      type: BG_PARAMETER_SECTION,
    },
    {
      name: "force_alpha",
      long_name: TRS("Force alpha"),
      type: BG_PARAMETER_CHECKBUTTON,
      flags: BG_PARAMETER_SYNC,
      help_string: TRS("Create video with alpha channel even if the input format has no alpha channel. Use this to generate the alpha channel from other channels using the colormatrix."),
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
  MATRIX_PARAM("y_to_y", 0, 0)
  MATRIX_PARAM("u_to_y", 0, 1)
  MATRIX_PARAM("v_to_y", 0, 2)
  MATRIX_PARAM("a_to_y", 0, 3)
  MATRIX_PARAM( "off_y", 0, 4)

  MATRIX_PARAM("y_to_u", 1, 0)
  MATRIX_PARAM("u_to_u", 1, 1)
  MATRIX_PARAM("v_to_u", 1, 2)
  MATRIX_PARAM("a_to_u", 1, 3)
  MATRIX_PARAM( "off_u", 1, 4)

  MATRIX_PARAM("y_to_v", 2, 0)
  MATRIX_PARAM("u_to_v", 2, 1)
  MATRIX_PARAM("v_to_v", 2, 2)
  MATRIX_PARAM("a_to_v", 2, 3)
  MATRIX_PARAM( "off_v", 2, 4)

  MATRIX_PARAM("y_to_a", 3, 0)
  MATRIX_PARAM("u_to_a", 3, 1)
  MATRIX_PARAM("v_to_a", 3, 2)
  MATRIX_PARAM("a_to_a", 3, 3)
  MATRIX_PARAM( "off_a", 3, 4)

  else if(!strcmp(name, "force_alpha"))
    {
    if(vp->force_alpha != val->val_i)
      {
      vp->force_alpha = val->val_i;
      vp->need_restart = 1;
      }
    }

  if(changed)
    bg_colormatrix_set_yuva(vp->mat, vp->coeffs);
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

static void
set_input_format_colormatrix(void * priv,
                             gavl_video_format_t * format, int port)
  {
  colormatrix_priv_t * vp;
  int flags = 0;
  vp = (colormatrix_priv_t *)priv;
  if(vp->force_alpha)
    flags |= BG_COLORMATRIX_FORCE_ALPHA;

  if(!port)
    {
    bg_colormatrix_init(vp->mat, format, flags);
    gavl_video_format_copy(&vp->format, format);
    }
  vp->need_restart = 0;
  }

static void get_output_format_colormatrix(void * priv,
                                          gavl_video_format_t * format)
  {
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_colormatrix(void * priv,
                                  gavl_video_frame_t * frame, int stream)
  {
  colormatrix_priv_t * vp;
  vp = (colormatrix_priv_t *)priv;

#if 0  
  if(!vp->colormatrix_h && !vp->colormatrix_v)
    {
    return vp->read_func(vp->read_data, frame, vp->read_stream);
    }
#endif
  if(!vp->read_func(vp->read_data, frame, vp->read_stream))
    return 0;
  
  bg_colormatrix_process(vp->mat, frame);
  return 1;
  }

bg_fv_plugin_t the_plugin = 
  {
    common:
    {
      BG_LOCALE,
      name:      "fv_colormatrix_yuv",
      long_name: TRS("Y'CbCr(A) Colormatrix"),
      description: TRS("Generic colormatrix (Y'CbCrA). You pass the coefficients in Y'CbCr(A) coordinates, but the processing will work in RGB(A) as well."),
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
    need_restart: need_restart_colormatrix,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
