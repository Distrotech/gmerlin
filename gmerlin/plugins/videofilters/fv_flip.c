/*****************************************************************
 
  fv_flip.c
 
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

#define LOG_DOMAIN "fv_flip"

typedef struct
  {
  int flip_h;
  int flip_v;
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;

  gavl_video_frame_t * frame;
  } flip_priv_t;

static void * create_flip()
  {
  flip_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

static void destroy_flip(void * priv)
  {
  flip_priv_t * vp;
  vp = (flip_priv_t *)priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);
  free(vp);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "flip_h",
      long_name: TRS("Flip horizontally"),
      type: BG_PARAMETER_CHECKBUTTON,
      flags: BG_PARAMETER_SYNC,
    },
    {
      gettext_domain: PACKAGE,
      gettext_directory: LOCALE_DIR,
      name: "flip_v",
      long_name: TRS("Flip vertically"),
      type: BG_PARAMETER_CHECKBUTTON,
      flags: BG_PARAMETER_SYNC,
    },
    { /* End of parameters */ },
  };

static bg_parameter_info_t * get_parameters_flip(void * priv)
  {
  return parameters;
  }

static void set_parameter_flip(void * priv, char * name, bg_parameter_value_t * val)
  {
  flip_priv_t * vp;
  vp = (flip_priv_t *)priv;

  if(!name)
    return;

  if(!strcmp(name, "flip_h"))
    vp->flip_h = val->val_i;
  else if(!strcmp(name, "flip_v"))
    vp->flip_v = val->val_i;
  }

static void connect_input_port_flip(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  flip_priv_t * vp;
  vp = (flip_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_flip(void * priv, gavl_video_format_t * format, int port)
  {
  flip_priv_t * vp;
  vp = (flip_priv_t *)priv;

  if(!port)
    gavl_video_format_copy(&vp->format, format);

  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = (gavl_video_frame_t*)0;
    }
  }

static void get_output_format_flip(void * priv, gavl_video_format_t * format)
  {
  flip_priv_t * vp;
  vp = (flip_priv_t *)priv;
  
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_flip(void * priv, gavl_video_frame_t * frame, int stream)
  {
  flip_priv_t * vp;
  vp = (flip_priv_t *)priv;

  if(!vp->flip_h && !vp->flip_v)
    {
    return vp->read_func(vp->read_data, frame, vp->read_stream);
    }

  if(!vp->frame)
    {
    vp->frame = gavl_video_frame_create(&vp->format);
    gavl_video_frame_clear(vp->frame, &vp->format);
    }
  if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
    return 0;
  
  if(vp->flip_h)
    {
    if(vp->flip_v)
      gavl_video_frame_copy_flip_xy(&vp->format, frame, vp->frame);
    else
      gavl_video_frame_copy_flip_x(&vp->format, frame, vp->frame);
    }
  else /* Flip y */
    gavl_video_frame_copy_flip_y(&vp->format, frame, vp->frame);
  
  frame->time_scaled = vp->frame->time_scaled;
  frame->duration_scaled = vp->frame->duration_scaled;
  return 1;
  }

bg_fv_plugin_t the_plugin = 
  {
    common:
    {
      BG_LOCALE,
      name:      "fv_flip",
      long_name: TRS("Flip image"),
      description: TRS("Flip video images horizovtally and/or vertically"),
      type:     BG_PLUGIN_FILTER_VIDEO,
      flags:    BG_PLUGIN_FILTER_1,
      create:   create_flip,
      destroy:   destroy_flip,
      get_parameters:   get_parameters_flip,
      set_parameter:    set_parameter_flip,
      priority:         1,
    },
    
    connect_input_port: connect_input_port_flip,
    
    set_input_format: set_input_format_flip,
    get_output_format: get_output_format_flip,

    read_video: read_video_flip,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
