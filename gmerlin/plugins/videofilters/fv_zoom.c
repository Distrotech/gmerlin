/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#include <gmerlin/bggavl.h>

#define LOG_DOMAIN "fv_zoomshift"

typedef struct
  {
  float scale_h;
  float scale_v;
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;
  
  gavl_video_frame_t * frame;
  gavl_video_scaler_t * scaler;
  gavl_video_options_t * opt;
  
  int changed;
  int quality;
  int scale_order;
  gavl_scale_mode_t scale_mode;
  
  float bg_color[4];
  } zs_priv_t;

static void * create_zoom()
  {
  zs_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->scaler = gavl_video_scaler_create();
  ret->opt = gavl_video_scaler_get_options(ret->scaler);
  return ret;
  }

static void destroy_zoom(void * priv)
  {
  zs_priv_t * vp;
  vp = priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);

  gavl_video_scaler_destroy(vp->scaler);
  
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "zoom_h",
      .long_name = TRS("Zoom horizontally"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .num_digits = 2,
      .val_min = { .val_f = 0.5 },
      .val_max = { .val_f = 2.0 },
      .val_default = { .val_f = 1.0 },
    },
    {
      .name = "zoom_v",
      .long_name = TRS("Zoom vertically"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .num_digits = 2,
      .val_min = { .val_f = 0.5 },
      .val_max = { .val_f = 2.0 },
      .val_default = { .val_f = 1.0 },

    },
    {
    .name =        "scale_mode",
    .long_name =   TRS("Scale mode"),
    .opt =         "sm",
    .type =        BG_PARAMETER_STRINGLIST,
    .flags = BG_PARAMETER_SYNC,
    .multi_names = BG_GAVL_SCALE_MODE_NAMES,
    .multi_labels = BG_GAVL_SCALE_MODE_LABELS,
    .val_default = { .val_str = "auto" },
    .help_string = TRS("Choose scaling method. Auto means to choose based on the conversion quality. Nearest is fastest, Sinc with Lanczos window is slowest."),
    },
    {
      .name =        "scale_order",
      .long_name =   TRS("Scale order"),
      .opt =       "so",
      .type =        BG_PARAMETER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = 4 },
      .val_max =     { .val_i = 1000 },
      .val_default = { .val_i = 4 },
      .help_string = TRS("Order for sinc scaling"),
    },

    {
      .name      =  "downscale_filter",
      .long_name =  "Antialiasing for downscaling",
      .type =        BG_PARAMETER_STRINGLIST,
      .flags =     BG_PARAMETER_SYNC,
      .multi_names = BG_GAVL_DOWNSCALE_FILTER_NAMES,
      .multi_labels = BG_GAVL_DOWNSCALE_FILTER_LABELS,
      .val_default = { .val_str = "auto" },
      .help_string = TRS("Specifies the antialiasing filter to be used when downscaling images."),
    },
    {
      .name      =  "downscale_blur",
      .long_name =  "Blur factor for downscaling",
      .type =        BG_PARAMETER_SLIDER_FLOAT,
      .flags =     BG_PARAMETER_SYNC,
      .val_default = { .val_f = 1.0 },
      .val_min     = { .val_f = 0.0 },
      .val_max     = { .val_f = 2.0 },
      .num_digits  = 2,
      .help_string = TRS("Specifies how much blurring should be applied when downscaling. Smaller values can speed up scaling, but might result in strong aliasing."),
      
    },
    {
      .name = "quality",
      .long_name = TRS("Quality"),
      .type = BG_PARAMETER_SLIDER_INT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_i = GAVL_QUALITY_FASTEST },
      .val_max =     { .val_i = GAVL_QUALITY_BEST },
      .val_default = { .val_i = GAVL_QUALITY_DEFAULT },
    },
    {
      .name = "bg_color",
      .long_name = TRS("Background color"),
      .type = BG_PARAMETER_COLOR_RGBA,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_color = { 0.0, 0.0, 0.0, 1.0 } },
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_zoom(void * priv)
  {
  return parameters;
  }

static void set_parameter_zoom(void * priv, const char * name, const bg_parameter_value_t * val)
  {
  zs_priv_t * vp;
  gavl_scale_mode_t scale_mode;
  gavl_downscale_filter_t new_downscale_filter;

  vp = priv;

  if(!name)
    return;

  if(!strcmp(name, "zoom_h"))
    {
    if(vp->scale_h != val->val_f)
      {
      vp->scale_h = val->val_f;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "zoom_v"))
    {
    if(vp->scale_v != val->val_f)
      {
      vp->scale_v = val->val_f;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "quality"))
    {
    if(vp->quality != val->val_i)
      {
      vp->quality = val->val_i;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "scale_mode"))
    {
    scale_mode = bg_gavl_string_to_scale_mode(val->val_str);
    if(vp->scale_mode != scale_mode)
      {
      vp->scale_mode = scale_mode;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "scale_order"))
    {
    if(vp->scale_order != val->val_i)
      {
      vp->scale_order = val->val_i;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "downscale_filter"))
    {
    new_downscale_filter = bg_gavl_string_to_downscale_filter(val->val_str);
    if(new_downscale_filter != gavl_video_options_get_downscale_filter(vp->opt))
      {
      gavl_video_options_set_downscale_filter(vp->opt, new_downscale_filter);
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "downscale_blur"))
    {
    if(val->val_f != gavl_video_options_get_downscale_blur(vp->opt))
      {
      gavl_video_options_set_downscale_blur(vp->opt, val->val_f);
      vp->changed = 1;
      }
    }


  
  else if(!strcmp(name, "bg_color"))
    {
    memcpy(vp->bg_color, val->val_color, sizeof(vp->bg_color));
    }
  }

static void connect_input_port_zoom(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  zs_priv_t * vp;
  vp = priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_zoom(void * priv,
                                gavl_video_format_t * format, int port)
  {
  zs_priv_t * vp;
  vp = priv;

  if(!port)
    gavl_video_format_copy(&vp->format, format);

  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = NULL;
    }
  vp->changed = 1;
  }

static void get_output_format_zoom(void * priv,
                                 gavl_video_format_t * format)
  {
  zs_priv_t * vp;
  vp = priv;
  
  gavl_video_format_copy(format, &vp->format);
  }

static void init_scaler(zs_priv_t * vp)
  {
  int i_tmp;
  float f_tmp;
  gavl_rectangle_f_t src_rect;
  gavl_rectangle_i_t dst_rect;

  gavl_rectangle_f_set_all(&src_rect, &vp->format);
  gavl_rectangle_i_set_all(&dst_rect, &vp->format);

  if(vp->scale_h > 1.0)
    {
    f_tmp = vp->format.image_width * vp->scale_h;
    src_rect.w = (float)vp->format.image_width / vp->scale_h;
    src_rect.x = ((float)vp->format.image_width - src_rect.w)/2.0;
    }
  else if(vp->scale_h < 1.0)
    {
    i_tmp = (int)((float)dst_rect.w * vp->scale_h + 0.5);

    dst_rect.x = (dst_rect.w - i_tmp) / 2;
          
    dst_rect.w = i_tmp;
    }

  if(vp->scale_v > 1.0)
    {
    f_tmp = vp->format.image_height * vp->scale_v;
    src_rect.h = (float)vp->format.image_height / vp->scale_v;
    src_rect.y = ((float)vp->format.image_height - src_rect.h)/2.0;
    }
  else if(vp->scale_v < 1.0)
    {
    i_tmp = (int)((float)dst_rect.h * vp->scale_v + 0.5);
    dst_rect.y = (dst_rect.h - i_tmp) / 2;
    dst_rect.h = i_tmp;
    }
  
  gavl_video_options_set_rectangles(vp->opt, &src_rect, &dst_rect);
  
  gavl_video_options_set_quality(vp->opt, vp->quality);
  gavl_video_options_set_scale_mode(vp->opt, vp->scale_mode);
  gavl_video_options_set_scale_order(vp->opt, vp->scale_order);
  
  gavl_video_scaler_init(vp->scaler, &vp->format, &vp->format);
  vp->changed = 0;
  }

static int read_video_zoom(void * priv, gavl_video_frame_t * frame,
                         int stream)
  {
  zs_priv_t * vp;
  vp = priv;

  if((vp->scale_h == 1.0) && (vp->scale_v == 1.0))
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

  if(vp->changed)
    init_scaler(vp);
  
  gavl_video_frame_fill(frame, &vp->format, vp->bg_color);
  
  gavl_video_scaler_scale(vp->scaler, vp->frame, frame);

  gavl_video_frame_copy_metadata(frame, vp->frame);
  
  return 1;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_zoom",
      .long_name = TRS("Zoom"),
      .description = TRS("Zoom horizontally and/or vertically"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_zoom,
      .destroy =   destroy_zoom,
      .get_parameters =   get_parameters_zoom,
      .set_parameter =    set_parameter_zoom,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_zoom,
    
    .set_input_format = set_input_format_zoom,
    .get_output_format = get_output_format_zoom,

    .read_video = read_video_zoom,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
