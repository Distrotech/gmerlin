/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>
#include <log.h>

#define LOG_DOMAIN "fv_deinterlace"

#define DEINTERLACE_NONE     0
#define DEINTERLACE_GAVL     1
#define DEINTERLACE_SCALE_HW 2
#define DEINTERLACE_MJPEGTOOLS 3

#ifdef  HAVE_MJPEGTOOLS
#include <yuvdeinterlace.h>
#endif

typedef struct deinterlace_priv_s
  {
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  
  gavl_video_frame_t * frame;

  gavl_video_options_t * opt;

  gavl_video_deinterlacer_t * deint;
  
  gavl_video_frame_t * src_field_1;
  
  int method;
  int sub_method;
  int src_field;
  int force;
  
  int need_restart;
  int need_reinit;
  
  int (*deint_func)(struct deinterlace_priv_s * p,
                   gavl_video_frame_t * frame);

#ifdef HAVE_MJPEGTOOLS
  yuvdeinterlacer_t * yuvd;
#endif
  } deinterlace_priv_t;

static void * create_deinterlace()
  {
  deinterlace_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->deint = gavl_video_deinterlacer_create();
  ret->opt = gavl_video_deinterlacer_get_options(ret->deint);

#ifdef HAVE_MJPEGTOOLS
  ret->yuvd = yuvdeinterlacer_create();
#endif
  
  ret->src_field_1 = gavl_video_frame_create((gavl_video_format_t*)0);
  return ret;
  }

static void destroy_deinterlace(void * priv)
  {
  deinterlace_priv_t * vp;
  vp = (deinterlace_priv_t *)priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);

  gavl_video_deinterlacer_destroy(vp->deint);

  gavl_video_frame_null(vp->src_field_1);
  gavl_video_frame_destroy(vp->src_field_1);
#ifdef HAVE_MJPEGTOOLS
  yuvdeinterlacer_destroy(vp->yuvd);
#endif
  
  free(vp);
  }



static const bg_parameter_info_t scale_parameters[] =
  {
    {
      .name =        "scale_mode",
      .long_name =   TRS("Scale mode"),
      .opt =       "sm",
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =     BG_PARAMETER_SYNC,
      .multi_names = (char const *[]){ "auto",\
                              "nearest",         \
                              "bilinear", \
                              "quadratic", \
                              "cubic_bspline", \
                              "cubic_mitchell", \
                              "cubic_catmull", \
                              "sinc_lanczos", \
                              (char*)0 },
      .multi_labels = (char const *[]){ TRS("Auto"), \
                               TRS("Nearest"),            \
                               TRS("Bilinear"), \
                               TRS("Quadratic"), \
                               TRS("Cubic B-Spline"), \
                               TRS("Cubic Mitchell-Netravali"), \
                               TRS("Cubic Catmull-Rom"), \
                               TRS("Sinc with Lanczos window"), \
                               (char*)0 },
      .val_default = { .val_str = "auto" },
      .help_string = TRS("Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest."),
    },
    {
      .name =        "scale_order",
      .long_name =   TRS("Scale order"),
      .opt =         "so",
      .type =        BG_PARAMETER_INT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 4 },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 4 },
      .help_string = TRS("Order for sinc scaling"),
    },
    { /* */ },
  };

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "method",
      .long_name = TRS("Method"),
      .type = BG_PARAMETER_MULTI_MENU,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "none" },
      .multi_names =  (char const *[]){ "none",
                               "copy",
                               "scale_hw",
                               "scale_sw",
                               "blend",
#ifdef HAVE_MJPEGTOOLS
                               "yuvdeinterlace_1",
                               "yuvdeinterlace_2",
                               "yuvdeinterlace_aa",
#endif                               
                               (char*)0 },
      
      .multi_labels = (char const *[]){ TRS("None"),
                               TRS("Scanline doubler"),
                               TRS("Scaler (hardware)"),
                               TRS("Scaler (software)"),
                               TRS("Blend"),
#ifdef HAVE_MJPEGTOOLS
                               TRS("yuvdeinterlace (1 field)"),
                               TRS("yuvdeinterlace (2 fields)"),
                               TRS("yuvdeinterlace (Antialiasing only)"),
#endif
                               (char*)0 },
      .multi_descriptions = (char const *[]){ TRS("Do nothing"),
                                     TRS("Simply double all scanlines. Very fast but \
low image quality"), 
                                     TRS("Drop one field and change the pixel aspect ratio such that a subsequent hardware scaler will scale the image to the original height"),
                                     TRS("Drop one field and scale the image to the original height"),
#ifdef HAVE_MJPEGTOOLS
                                     TRS("Motion compensating deinterlacer from the mjpegtools project"),
                                     TRS("Motion compensating deinterlacer from the mjpegtools project"),
                                     TRS("Motion compensating deinterlacer from the mjpegtools project"),
#endif                               
                                     (char*)0 },
      .multi_parameters = (bg_parameter_info_t const *[]) { (bg_parameter_info_t*)0, // None
                                                   (bg_parameter_info_t*)0, // Copy
                                                   (bg_parameter_info_t*)0, // Scale (HW)
                                                   scale_parameters, // Scale (SW)
                                                   (bg_parameter_info_t*)0, // Blend
#ifdef HAVE_MJPEGTOOLS
                                                   (bg_parameter_info_t*)0, // yuvdeinterlace
                                                   (bg_parameter_info_t*)0, // yuvdeinterlace
                                                   (bg_parameter_info_t*)0, // yuvdeinterlace
#endif
                                                   (bg_parameter_info_t*)0 } 
                                                   
    },
    {
      .name =        "quality",
      .long_name =   TRS("Quality"),
      .opt =         "q",
      .type =        BG_PARAMETER_SLIDER_INT,
      .flags =     BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST    },
      .val_default = { .val_i = GAVL_QUALITY_DEFAULT },
      .help_string = TRS("Lower quality means more speed. Values above 3 enable slow high quality calculations.")
    },
    {
      .name = "force",
      .long_name = TRS("Force deinterlacing"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
      .help_string = "Always perform deinterlacing even if the source format pretends to be progressive",
    },
    {
      .name = "drop_mode",
      .long_name = TRS("Drop mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "top" },
      .help_string = TRS("For modes, which take only one source field, specify which field to drop"),
      .multi_names = (char const *[]){ "top", "bottom", (char*)0 },
      .multi_labels = (char const *[]){ TRS("Drop top field"),
                               TRS("Drop bottom field"),
                               (char*)0 },
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_deinterlace(void * priv)
  {
  return parameters;
  }


static gavl_scale_mode_t string_to_scale_mode(const char * str)
  {
  if(!strcmp(str, "auto"))
    return GAVL_SCALE_AUTO;
  else if(!strcmp(str, "nearest"))
    return GAVL_SCALE_NEAREST;
  else if(!strcmp(str, "bilinear"))
    return GAVL_SCALE_BILINEAR;
  else if(!strcmp(str, "quadratic"))
    return GAVL_SCALE_QUADRATIC;
  else if(!strcmp(str, "cubic_bspline"))
    return GAVL_SCALE_CUBIC_BSPLINE;
  else if(!strcmp(str, "cubic_mitchell"))
    return GAVL_SCALE_CUBIC_MITCHELL;
  else if(!strcmp(str, "cubic_catmull"))
    return GAVL_SCALE_CUBIC_CATMULL;
  else if(!strcmp(str, "sinc_lanczos"))
    return GAVL_SCALE_SINC_LANCZOS;
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Unknown scale mode %s", str);
    return GAVL_SCALE_AUTO;
    }
  }

static void set_parameter_deinterlace(void * priv, const char * name,
                                      const bg_parameter_value_t * val)
  {
  int new_method = DEINTERLACE_NONE;
  gavl_deinterlace_mode_t new_sub_method = 0;
  gavl_deinterlace_drop_mode_t new_drop_mode;
  gavl_scale_mode_t new_scale_mode;
  
  deinterlace_priv_t * vp;
  int new_src_field = 0;
  vp = (deinterlace_priv_t *)priv;
  
  
  if(!name)
    return;

  if(!strcmp(name, "method"))
    {
    if(!strcmp(val->val_str, "none"))
      {
      new_method = DEINTERLACE_NONE;
      }
    else if(!strcmp(val->val_str, "copy"))
      {
      new_method = DEINTERLACE_GAVL;
      new_sub_method = GAVL_DEINTERLACE_COPY;
      }
    else if(!strcmp(val->val_str, "scale_hw"))
      {
      new_method = DEINTERLACE_SCALE_HW;
      }
    else if(!strcmp(val->val_str, "scale_sw"))
      {
      new_method = DEINTERLACE_GAVL;
      new_sub_method = GAVL_DEINTERLACE_SCALE;
      }
    else if(!strcmp(val->val_str, "blend"))
      {
      new_method = DEINTERLACE_GAVL;
      new_sub_method = GAVL_DEINTERLACE_BLEND;
      }
#ifdef HAVE_MJPEGTOOLS
    else if(!strcmp(val->val_str, "yuvdeinterlace_1"))
      {
      new_method = DEINTERLACE_MJPEGTOOLS;
      new_sub_method = YUVD_MODE_DEINT_1;
      }
    else if(!strcmp(val->val_str, "yuvdeinterlace_2"))
      {
      new_method = DEINTERLACE_MJPEGTOOLS;
      new_sub_method = YUVD_MODE_DEINT_2;
      }
    else if(!strcmp(val->val_str, "yuvdeinterlace_aa"))
      {
      new_method = DEINTERLACE_MJPEGTOOLS;
      new_sub_method = YUVD_MODE_ANTIALIAS;
      }
#endif
    if((new_method != vp->method) || 
       (new_sub_method != vp->sub_method))
      {
      vp->need_restart = 1;
      vp->method = new_method;
      vp->sub_method = new_sub_method;

      if(new_method == DEINTERLACE_GAVL)
        {
        gavl_video_options_set_deinterlace_mode(vp->opt,
                                                new_sub_method);
        }
#ifdef HAVE_MJPEGTOOLS
      else if(new_method == DEINTERLACE_MJPEGTOOLS)
        yuvdeinterlacer_set_mode(vp->yuvd, new_sub_method);
#endif
      }
    }
  else if(!strcmp(name, "force"))
    {
    if(vp->force != val->val_i)
      {
      vp->force = val->val_i;
      vp->need_restart = 1;
      }
    }
  else if(!strcmp(name, "drop_mode"))
    {
    if(!strcmp(val->val_str, "top"))
      {
      new_drop_mode = GAVL_DEINTERLACE_DROP_TOP;
      new_src_field = 1;
      }
    else
      {
      new_drop_mode = GAVL_DEINTERLACE_DROP_BOTTOM;
      new_src_field = 0;
      }
    if(new_drop_mode != gavl_video_options_get_deinterlace_drop_mode(vp->opt))
      {
      gavl_video_options_set_deinterlace_drop_mode(vp->opt, new_drop_mode);
      vp->need_restart = 1;
      }
    vp->src_field = new_src_field;
    }
  else if(!strcmp(name, "scale_mode"))
    {
    new_scale_mode = string_to_scale_mode(val->val_str);
    if(new_scale_mode != gavl_video_options_get_scale_mode(vp->opt))
      {
      gavl_video_options_set_scale_mode(vp->opt, new_scale_mode);
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "scale_order"))
    {
    if(gavl_video_options_get_scale_order(vp->opt) != val->val_i)
      {
      gavl_video_options_set_scale_order(vp->opt, val->val_i);
      vp->need_reinit = 1;
      }
    }
  else if(!strcmp(name, "quality"))
    {
    if(gavl_video_options_get_quality(vp->opt) != val->val_i)
      {
      gavl_video_options_set_quality(vp->opt, val->val_i);
      vp->need_reinit = 1;
      }
    }
  }


/*
 * Deinterlace functions
 */

static int deinterlace_none(struct deinterlace_priv_s * vp,
                            gavl_video_frame_t * frame)
  {
  return vp->read_func(vp->read_data, frame, vp->read_stream);
  }

static int deinterlace_gavl(struct deinterlace_priv_s * vp,
                            gavl_video_frame_t * frame)
  {
  if(!vp->frame)
    vp->frame = gavl_video_frame_create(&vp->in_format);
  
  if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
    return 0;
  
  gavl_video_deinterlacer_deinterlace(vp->deint, vp->frame, frame);

  frame->timestamp = vp->frame->timestamp;
  frame->duration = vp->frame->duration;
  frame->timecode = vp->frame->timecode;
  
  return 1;
  }

static int deinterlace_scale_hw(struct deinterlace_priv_s * vp,
                                gavl_video_frame_t * frame)
  {
  if(!vp->frame)
    vp->frame = gavl_video_frame_create(&vp->in_format);

  
  if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
    return 0;
  
  gavl_video_frame_get_field(vp->in_format.pixelformat,
                             vp->frame, vp->src_field_1, vp->src_field);
  
  gavl_video_frame_copy(&vp->out_format, frame, vp->src_field_1);
  
  frame->timestamp = vp->frame->timestamp;
  frame->duration = vp->frame->duration;
  frame->timecode = vp->frame->timecode;
  
  return 1;
  }

#ifdef HAVE_MJPEGTOOLS
static int deinterlace_mjpegtools(struct deinterlace_priv_s * vp,
                                   gavl_video_frame_t * frame)
  {
  return yuvdeinterlacer_read(vp->yuvd, frame, 0);
  }
#endif

static void connect_input_port_deinterlace(void * priv,
                                           bg_read_video_func_t func,
                                           void * data, int stream,
                                           int port)
  {
  deinterlace_priv_t * vp;
  vp = (deinterlace_priv_t *)priv;
  
  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
#ifdef HAVE_MJPEGTOOLS
    yuvdeinterlacer_connect_input(vp->yuvd, vp->read_func, vp->read_data, vp->read_stream);
#endif

    }
  }

static void
set_input_format_deinterlace(void * priv,
                             gavl_video_format_t * format,
                             int port)
  {
  deinterlace_priv_t * vp;
  vp = (deinterlace_priv_t *)priv;
  
  if(!port)
    {
    if(vp->frame)
      {
      gavl_video_frame_destroy(vp->frame);
      vp->frame = (gavl_video_frame_t*)0;
      }
    gavl_video_format_copy(&vp->in_format, format);
    gavl_video_format_copy(&vp->out_format, format);
    vp->out_format.interlace_mode = GAVL_INTERLACE_NONE;
    
    vp->need_reinit = 1;
    
    switch(vp->method)
      {
      case DEINTERLACE_NONE:
        vp->deint_func = deinterlace_none;
        break;
      case DEINTERLACE_GAVL:
        vp->deint_func = deinterlace_gavl;
        break;
      case DEINTERLACE_SCALE_HW:
        vp->deint_func = deinterlace_scale_hw;
        
        vp->out_format.image_height /= 2;
        vp->out_format.frame_height /= 2;
        vp->out_format.pixel_height *= 2;
        break;
#ifdef HAVE_MJPEGTOOLS
      case DEINTERLACE_MJPEGTOOLS:
        vp->deint_func = deinterlace_mjpegtools;
        yuvdeinterlacer_init(vp->yuvd, &vp->in_format);
        gavl_video_format_copy(format, &vp->in_format);
        yuvdeinterlacer_get_output_format(vp->yuvd, &vp->out_format);

        
        
        break;
#endif
      }
    
    vp->need_restart = 0;
    }
  
  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = (gavl_video_frame_t*)0;
    }
  }

static int need_restart_deinterlace(void * priv)
  {
  deinterlace_priv_t * vp;
  vp = (deinterlace_priv_t *)priv;
  return vp->need_restart;
  }

static void get_output_format_deinterlace(void * priv, gavl_video_format_t * format)
  {
  deinterlace_priv_t * vp;
  vp = (deinterlace_priv_t *)priv;
  
  gavl_video_format_copy(format, &vp->out_format);
  }

static int read_video_deinterlace(void * priv, gavl_video_frame_t * frame, int stream)
  {
  deinterlace_priv_t * vp;
  vp = (deinterlace_priv_t *)priv;

  if(vp->need_reinit)
    {
    switch(vp->method)
      {
      case DEINTERLACE_GAVL:
        gavl_video_deinterlacer_init(vp->deint, &vp->in_format);
        break;
      default:
        break;
      }
    vp->need_reinit = 0;
    }
  return vp->deint_func(vp, frame);
  }

const const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_deinterlace",
      .long_name = TRS("Deinterlacer"),
      .description = TRS("Deinterlace with various algorithms"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_deinterlace,
      .destroy =   destroy_deinterlace,
      .get_parameters =   get_parameters_deinterlace,
      .set_parameter =    set_parameter_deinterlace,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_deinterlace,
    
    .set_input_format = set_input_format_deinterlace,
    .get_output_format = get_output_format_deinterlace,
    
    .read_video = read_video_deinterlace,
    .need_restart = need_restart_deinterlace,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
