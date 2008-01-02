/*****************************************************************
 
  fv_invert_rgb.c
 
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

#define LOG_DOMAIN "fv_invert_rgb"

typedef struct
  {
  bg_colormatrix_t * mat;
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;

  float coeffs[4][5];
  int invert[4];
  
  } invert_priv_t;

static float coeffs_unity[4][5] =
  {
    { 1.0, 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 1.0, 0.0 },
  };

static float coeffs_invert[4][5] =
  {
    { -1.0,  0.0,  0.0,  0.0, 1.0 },
    {  0.0, -1.0,  0.0,  0.0, 1.0 },
    {  0.0,  0.0, -1.0,  0.0, 1.0 },
    {  0.0,  0.0,  0.0, -1.0, 1.0 },
  };

static void * create_invert()
  {
  invert_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->mat = bg_colormatrix_create();
  return ret;
  }

static void destroy_invert(void * priv)
  {
  invert_priv_t * vp;
  vp = (invert_priv_t *)priv;
  bg_colormatrix_destroy(vp->mat);
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name:        "r",
      long_name:   TRS("Invert red"),
      type:        BG_PARAMETER_CHECKBUTTON,
      flags: BG_PARAMETER_SYNC,
      val_default: { val_i: 1 },
    },
    {
      name:        "g",
      long_name:   TRS("Invert green"),
      type:        BG_PARAMETER_CHECKBUTTON,
      flags: BG_PARAMETER_SYNC,
      val_default: { val_i: 1 },
    },
    {
      name:        "b",
      long_name:   TRS("Invert blue"),
      type:        BG_PARAMETER_CHECKBUTTON,
      flags: BG_PARAMETER_SYNC,
      val_default: { val_i: 1 },
    },
    {
      name:        "a",
      long_name:   TRS("Invert alpha"),
      type:        BG_PARAMETER_CHECKBUTTON,
      flags: BG_PARAMETER_SYNC,
      val_default: { val_i: 1 },
    },
    { /* End of parameters */ },
  };


static bg_parameter_info_t * get_parameters_invert(void * priv)
  {
  return parameters;
  }

static void set_coeffs(invert_priv_t * vp)
  {
  int i, j;
  for(i = 0; i < 4; i++)
    {
    if(vp->invert[i])
      {
      for(j = 0; j < 5; j++)
        vp->coeffs[i][j] = coeffs_invert[i][j];
      }
    else
      {
      for(j = 0; j < 5; j++)
        vp->coeffs[i][j] = coeffs_unity[i][j];
      }
    }
  }

static void set_parameter_invert(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  invert_priv_t * vp;
  int changed = 0;
  vp = (invert_priv_t *)priv;
  if(!name)
    return;
  
  if(!strcmp(name, "r"))
    {
    if(vp->invert[0] != val->val_i)
      {
      vp->invert[0] = val->val_i;
      changed = 1;
      }
    }
  else if(!strcmp(name, "g"))
    {
    if(vp->invert[1] != val->val_i)
      {
      vp->invert[1] = val->val_i;
      changed = 1;
      }
    }
  else if(!strcmp(name, "b"))
    {
    if(vp->invert[2] != val->val_i)
      {
      vp->invert[2] = val->val_i;
      changed = 1;
      }
    }
  else if(!strcmp(name, "a"))
    {
    if(vp->invert[3] != val->val_i)
      {
      vp->invert[3] = val->val_i;
      changed = 1;
      }
    }
  if(changed)
    {
    set_coeffs(vp);
    bg_colormatrix_set_rgba(vp->mat, vp->coeffs);
    }
  }

static void connect_input_port_invert(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  invert_priv_t * vp;
  vp = (invert_priv_t *)priv;
  
  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  }

static void set_input_format_invert(void * priv, gavl_video_format_t * format, int port)
  {
  invert_priv_t * vp;
  vp = (invert_priv_t *)priv;

  if(!port)
    {
    bg_colormatrix_init(vp->mat, format, 0);
    gavl_video_format_copy(&vp->format, format);
    }
  }

static void get_output_format_invert(void * priv, gavl_video_format_t * format)
  {
  invert_priv_t * vp;
  vp = (invert_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_invert(void * priv, gavl_video_frame_t * frame, int stream)
  {
  invert_priv_t * vp;
  vp = (invert_priv_t *)priv;

#if 0  
  if(!vp->invert_h && !vp->invert_v)
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
      name:      "fv_invert",
      long_name: TRS("Invert RGBA"),
      description: TRS("Invert single color channels"),
      type:     BG_PLUGIN_FILTER_VIDEO,
      flags:    BG_PLUGIN_FILTER_1,
      create:   create_invert,
      destroy:   destroy_invert,
      get_parameters:   get_parameters_invert,
      set_parameter:    set_parameter_invert,
      priority:         1,
    },
    
    connect_input_port: connect_input_port_invert,
    
    set_input_format: set_input_format_invert,
    get_output_format: get_output_format_invert,

    read_video: read_video_invert,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
