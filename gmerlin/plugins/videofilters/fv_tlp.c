/*****************************************************************
 
  fv_tlp.c
 
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

#define LOG_DOMAIN "fv_tlp"

#include <gavl/gavldsp.h>

typedef struct tlp_priv_s
  {
  float factor;
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;
  
  gavl_video_frame_t * frame_1;
  gavl_video_frame_t * frame_2;
  
  gavl_dsp_context_t * ctx;
  int init;
  } tlp_priv_t;

static void * create_tlp()
  {
  tlp_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->ctx = gavl_dsp_context_create();
  return ret;
  }

static void destroy_tlp(void * priv)
  {
  tlp_priv_t * vp;
  vp = (tlp_priv_t *)priv;
  if(vp->frame_1)
    gavl_video_frame_destroy(vp->frame_1);
  if(vp->frame_2)
    gavl_video_frame_destroy(vp->frame_2);
  if(vp->ctx)
    gavl_dsp_context_destroy(vp->ctx);
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "factor",
      long_name: TRS("Factor"),
      type: BG_PARAMETER_SLIDER_FLOAT,
      flags: BG_PARAMETER_SYNC,
      num_digits: 2,
      val_min:     { val_f: 0.0 },
      val_max:     { val_f: 1.0 },
      val_default: { val_f: 0.5 },
      help_string: TRS("Factor for the new image. Lower values mean a larger delay time."),
    },
    {
      name: "quality",
      long_name: TRS("Quality"),
      type: BG_PARAMETER_SLIDER_INT,
      flags: BG_PARAMETER_SYNC,
      val_min:     { val_i: GAVL_QUALITY_FASTEST },
      val_max:     { val_i: GAVL_QUALITY_BEST },
      val_default: { val_i: GAVL_QUALITY_DEFAULT },
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_tlp(void * priv)
  {
  return parameters;
  }

static void set_parameter_tlp(void * priv, char * name, bg_parameter_value_t * val)
  {
  tlp_priv_t * vp;
  vp = (tlp_priv_t *)priv;

  if(!name)
    return;
  if(!strcmp(name, "factor"))
    vp->factor = val->val_f;
  else if(!strcmp(name, "quality"))
    gavl_dsp_context_set_quality(vp->ctx, val->val_i);
  }

static void connect_input_port_tlp(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  tlp_priv_t * vp;
  vp = (tlp_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_tlp(void * priv, gavl_video_format_t * format, int port)
  {
  tlp_priv_t * vp;
  vp = (tlp_priv_t *)priv;

  gavl_video_format_copy(&vp->format, format);

  if(vp->frame_1)
    {
    gavl_video_frame_destroy(vp->frame_1);
    vp->frame_1 = (gavl_video_frame_t*)0;
    }
  if(vp->frame_2)
    {
    gavl_video_frame_destroy(vp->frame_2);
    vp->frame_2 = (gavl_video_frame_t*)0;
    }
  vp->init = 1;
  }

static void get_output_format_tlp(void * priv, gavl_video_format_t * format)
  {
  tlp_priv_t * vp;
  vp = (tlp_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static void reset_tlp(void * priv)
  {
  tlp_priv_t * vp;
  vp = (tlp_priv_t *)priv;
  vp->init = 1;
  }

static int read_video_tlp(void * priv, gavl_video_frame_t * frame, int stream)
  {
  tlp_priv_t * vp;
  vp = (tlp_priv_t *)priv;
  
  if(!vp->frame_1)
    vp->frame_1 = gavl_video_frame_create(&vp->format);
  if(!vp->frame_2)
    vp->frame_2 = gavl_video_frame_create(&vp->format);
  
  if(vp->init)
    {
    if(!vp->read_func(vp->read_data, vp->frame_1, vp->read_stream))
      return 0;
    gavl_video_frame_copy(&vp->format, frame, vp->frame_1);
    vp->init = 0;
    }
  else
    {
    if(!vp->read_func(vp->read_data, vp->frame_2, vp->read_stream))
      return 0;
    
    gavl_dsp_interpolate_video_frame(vp->ctx, &vp->format, vp->frame_1, vp->frame_2,
                                     frame, vp->factor);
    
    gavl_video_frame_copy(&vp->format, vp->frame_1, frame);
    }
  frame->timestamp     = vp->frame_2->timestamp;
  frame->duration = vp->frame_2->duration;
  return 1;
  }

bg_fv_plugin_t the_plugin = 
  {
    common:
    {
      BG_LOCALE,
      name:      "fv_tlp",
      long_name: TRS("Temporal lowpass"),
      description: TRS("Simple temporal lowpass"),
      type:     BG_PLUGIN_FILTER_VIDEO,
      flags:    BG_PLUGIN_FILTER_1,
      create:   create_tlp,
      destroy:   destroy_tlp,
      get_parameters:   get_parameters_tlp,
      set_parameter:    set_parameter_tlp,
      priority:         1,
    },
    
    connect_input_port: connect_input_port_tlp,
    
    set_input_format: set_input_format_tlp,
    get_output_format: get_output_format_tlp,
    
    read_video: read_video_tlp,
    reset: reset_tlp,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
