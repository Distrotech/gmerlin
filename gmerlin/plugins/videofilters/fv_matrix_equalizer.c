/*****************************************************************
 
  fv_matrix_equalizer.c
 
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
#include <math.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>
#include <log.h>

#include "colormatrix.h"

#define LOG_DOMAIN "fv_matrix_equalizer"

typedef struct
  {
  bg_colormatrix_t * mat;
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;

  gavl_video_frame_t * frame;
  
  float coeffs[3][4];

  int brightness;
  int contrast;
  float hue;
  float saturation;
  
  } equalizer_priv_t;

static void * create_equalizer()
  {
  equalizer_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->mat = bg_colormatrix_create();
  return ret;
  }

static void destroy_equalizer(void * priv)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);
  bg_colormatrix_destroy(vp->mat);
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "brightness",
      long_name: TRS("Brightness"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f: -10.0 },
      val_max: { val_f:  10.0 },
      val_default: { val_f:  0.0 },
      num_digits: 1,
    },
    {
      name: "contrast",
      long_name: TRS("Contrast"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f: -10.0 },
      val_max: { val_f:  10.0 },
      val_default: { val_f:  0.0 },
      num_digits: 1,
    },
    {
      name: "saturation",
      long_name: TRS("Saturation"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f: -10.0 },
      val_max: { val_f:  10.0 },
      val_default: { val_f:  0.0 },
      num_digits: 1,
    },
    {
      name: "hue",
      long_name: TRS("Hue"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      val_min: { val_f: -180.0 },
      val_max: { val_f:  180.0 },
      val_default: { val_f:  0.0 },
      num_digits: 1,
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_equalizer(void * priv)
  {
  return parameters;
  }

static void set_coeffs(equalizer_priv_t * vp)
  {
  float s, c;
  float contrast, brightness;
  
  /* Brightness and contrast go to Y */

  contrast = (vp->contrast+100.0) / 100.0;
  brightness = (vp->brightness+100.0)/100.0 - 0.5 * (1.0 + contrast);
  
  vp->coeffs[0][0] = contrast;
  vp->coeffs[0][1] = 0.0;
  vp->coeffs[0][2] = 0.0;
  vp->coeffs[0][3] = brightness;
  
  /* Saturation and Hue go to U and V */

  s = sin(vp->hue) * vp->saturation;
  c = cos(vp->hue) * vp->saturation;

  vp->coeffs[1][0] = 0.0;
  vp->coeffs[1][1] = c;
  vp->coeffs[1][2] = -s;
  vp->coeffs[1][3] = 0.0;

  vp->coeffs[2][0] = 0.0;
  vp->coeffs[2][1] = s;
  vp->coeffs[2][2] = c;
  vp->coeffs[2][3] = 0.0;
  
  }


static void set_parameter_equalizer(void * priv, const char * name,
                                    const bg_parameter_value_t * val)
  {
  int changed = 0;
  equalizer_priv_t * vp;
  int test_i;
  float test_f;

  vp = (equalizer_priv_t *)priv;
  
  if(!name)
    return;
  
  if(!strcmp(name, "brightness"))
    {
    test_i = 10*val->val_f;
    if(vp->brightness != test_i)
      {
      vp->brightness = test_i;
      changed = 1;
      }
    }
  else if(!strcmp(name, "contrast"))
    {
    test_i = 10*val->val_f;
    if(vp->contrast != test_i)
      {
      vp->contrast = test_i;
      changed = 1;
      }
    }
  else if(!strcmp(name, "saturation"))
    {
    test_f = (val->val_f + 10.0)/10.0;
    
    if(vp->saturation != test_f)
      {
      vp->saturation = test_f;
      changed = 1;
      }
    }
  else if(!strcmp(name, "hue"))
    {
    test_f = val->val_f * M_PI / 180.0;
    if(vp->hue != test_f)
      {
      vp->hue = test_f;
      changed = 1;
      }
    }
  if(changed)
    {
    set_coeffs(vp);
    bg_colormatrix_set_yuv(vp->mat, vp->coeffs);
    }
  }


static void connect_input_port_equalizer(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;
  
  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  }

static void set_input_format_equalizer(void * priv, gavl_video_format_t * format, int port)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;

  if(!port)
    {
    bg_colormatrix_init(vp->mat, format, 0);
    gavl_video_format_copy(&vp->format, format);
    }
  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = (gavl_video_frame_t*)0;
    }
  }

static void get_output_format_equalizer(void * priv, gavl_video_format_t * format)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_equalizer(void * priv, gavl_video_frame_t * frame, int stream)
  {
  equalizer_priv_t * vp;
  vp = (equalizer_priv_t *)priv;

#if 0  
  if(!vp->equalizer_h && !vp->equalizer_v)
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
      name:      "fv_matrix_equalizer",
      long_name: TRS("Matrix equalizer"),
      description: TRS("Control hue, saturation, contrast and brightness using the colormatrix. This works for all pixelformats, which are not subsampled. For a fast Y'CbCr Equalizer, use the Simple yuv equalizer"),
      type:     BG_PLUGIN_FILTER_VIDEO,
      flags:    BG_PLUGIN_FILTER_1,
      create:   create_equalizer,
      destroy:   destroy_equalizer,
      get_parameters:   get_parameters_equalizer,
      set_parameter:    set_parameter_equalizer,
      priority:         1,
    },
    
    connect_input_port: connect_input_port_equalizer,
    
    set_input_format: set_input_format_equalizer,
    get_output_format: get_output_format_equalizer,

    read_video: read_video_equalizer,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
