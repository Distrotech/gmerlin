/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
#include <math.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>


#define LOG_DOMAIN "fv_blur"

#define MODE_GAUSS      0
#define MODE_TRIANGULAR 1
#define MODE_BOX        2

typedef struct
  {
  int mode;
  float radius_h;
  float radius_v;
  int blur_chroma;
  int correct_nonsquare;
  int changed;
  gavl_video_options_t * opt;
  gavl_video_scaler_t * scaler;
  gavl_video_options_t * global_opt;

  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  gavl_video_format_t format;
  gavl_video_frame_t * frame;
  } blur_priv_t;

static void * create_blur()
  {
  blur_priv_t * ret;
  int flags;
  ret = calloc(1, sizeof(*ret));
  ret->scaler = gavl_video_scaler_create();
  ret->opt = gavl_video_scaler_get_options(ret->scaler);
  flags = gavl_video_options_get_conversion_flags(ret->opt);
  flags |= GAVL_CONVOLVE_NORMALIZE;
  gavl_video_options_set_conversion_flags(ret->opt,
                                          flags);

  ret->global_opt = gavl_video_options_create();
  return ret;
  }

static gavl_video_options_t * get_options_blur(void * priv)
  {
  blur_priv_t * vp = priv;
  return vp->global_opt;
  }


static void transfer_global_options(gavl_video_options_t * opt,
                                    gavl_video_options_t * global_opt)
  {
  void * client_data;
  gavl_video_stop_func stop_func;
  gavl_video_run_func  run_func;
  
  gavl_video_options_set_quality(opt, gavl_video_options_get_quality(global_opt));
  gavl_video_options_set_num_threads(opt, gavl_video_options_get_num_threads(global_opt));
                                     
  run_func = gavl_video_options_get_run_func(global_opt, &client_data);
  gavl_video_options_set_run_func(opt, run_func, client_data);

  stop_func = gavl_video_options_get_stop_func(global_opt, &client_data);
  gavl_video_options_set_stop_func(opt, stop_func, client_data);
  
  }


static void destroy_blur(void * priv)
  {
  blur_priv_t * vp;
  vp = (blur_priv_t *)priv;
  if(vp->frame) gavl_video_frame_destroy(vp->frame);
  if(vp->scaler) gavl_video_scaler_destroy(vp->scaler);
  gavl_video_options_destroy(vp->global_opt);
  free(vp);
  }

static float get_coeff_rectangular(float radius)
  {
  if(radius <= 1.0)
    return radius;
  return 1.0;
  }

static float get_coeff_triangular(float radius)
  {
  if(radius <= 1.0)
    return 1.0 - (1.0-radius)*(1.0-radius);
  return 1.0;
  }

static float get_coeff_gauss(float radius)
  {
  return erf(radius);
  }


static float * get_coeffs(float radius, int * r_i, int mode)
  {
  float * ret;
  float coeff, last_coeff;
  int i;
  float (*get_coeff)(float);
  if(radius == 0.0)
    return NULL;
  switch(mode)
    {
    case MODE_GAUSS:
      get_coeff = get_coeff_gauss;
      *r_i = 2*(int)(radius + 0.4999);
      break;
    case MODE_TRIANGULAR:
      get_coeff = get_coeff_triangular;
      *r_i = (int)(radius + 0.4999);
      break;
    case MODE_BOX:
      get_coeff = get_coeff_rectangular;
      *r_i = (int)(radius + 0.4999);
      break;
    default:
      return NULL;
    }
  if(*r_i < 1)
    return (float*)(0);
  /* Allocate and set return values */
  ret = malloc(((*r_i * 2) + 1)*sizeof(*ret));

//  ret[*r_i] =   
  last_coeff = 0.0;
  coeff = get_coeff(0.5 / radius);
  ret[*r_i] = 2.0 * coeff;

  for(i = 1; i <= *r_i; i++)
    {
    last_coeff = coeff;
    coeff = get_coeff((i+0.5) / radius);
    ret[*r_i+i] = coeff - last_coeff;
    ret[*r_i-i] = ret[*r_i+i];
    }
  return ret;
  }

static void init_scaler(blur_priv_t * vp)
  {
  float * coeffs_h = NULL;
  float * coeffs_v = NULL;
  int num_coeffs_h = 0;
  int num_coeffs_v = 0;
  float radius_h;
  float radius_v;
  float pixel_aspect;
  int flags;
  
  radius_h = vp->radius_h;
  radius_v = vp->radius_v;

  flags = gavl_video_options_get_conversion_flags(vp->opt);
  if(vp->blur_chroma)
    flags |= GAVL_CONVOLVE_CHROMA;
  else
    flags &= ~GAVL_CONVOLVE_CHROMA;
  gavl_video_options_set_conversion_flags(vp->opt,
                                          flags);

  if(vp->correct_nonsquare)
    {
    pixel_aspect = 
      (float)(vp->format.pixel_width) / 
      (float)(vp->format.pixel_height);
    pixel_aspect = sqrt(pixel_aspect);
    radius_h /= pixel_aspect;
    radius_v *= pixel_aspect;
    }
  coeffs_h = get_coeffs(radius_h, &num_coeffs_h, vp->mode);
  coeffs_v = get_coeffs(radius_v, &num_coeffs_v, vp->mode);

  transfer_global_options(vp->opt, vp->global_opt);
  
  gavl_video_scaler_init_convolve(vp->scaler,
                                  &vp->format,
                                  num_coeffs_h, coeffs_h,
                                  num_coeffs_v, coeffs_v);
  if(coeffs_h) free(coeffs_h);
  if(coeffs_v) free(coeffs_v);

  vp->changed = 0;
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "mode",
      .long_name = TRS("Mode"),
      .type = BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "gauss" },
      .multi_names = (char const *[]){ "gauss", "triangular", "box", 
                              NULL },
      .multi_labels = (char const *[]){ TRS("Gauss"), 
                               TRS("Triangular"), 
                               TRS("Rectangular"),
                              NULL },
    },
    {
      .name = "radius_h",
      .long_name = TRS("Horizontal radius"),
      .type = BG_PARAMETER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f =  0.5 },
      .val_max = { .val_f = 50.0 },
      .val_default = { .val_f =  0.5 },
      .num_digits = 1,
    },
    {
      .name = "radius_v",
      .long_name = TRS("Vertical radius"),
      .type = BG_PARAMETER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f =  0.5 },
      .val_max = { .val_f = 50.0 },
      .val_default = { .val_f =  0.5 },
      .num_digits = 1,
    },
    {
      .name = "blur_chroma",
      .long_name = "Blur chroma planes",
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
      .flags = BG_PARAMETER_SYNC,
    },
    {
      .name = "correct_nonsquare",
      .long_name = "Correct radii for nonsquare pixels",
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
      .flags = BG_PARAMETER_SYNC,
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_blur(void * priv)
  {
  return parameters;
  }

static void set_parameter_blur(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  blur_priv_t * vp;
  vp = (blur_priv_t *)priv;

  if(!name)
    return;

  if(!strcmp(name, "radius_h"))
    {
    if(vp->radius_h != val->val_f)
      {
      vp->radius_h = val->val_f;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "radius_v"))
    {
    if(vp->radius_v != val->val_f)
      {
      vp->radius_v = val->val_f;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "correct_nonsquare"))
    {
    if(vp->correct_nonsquare != val->val_i)
      {
      vp->correct_nonsquare= val->val_i;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "blur_chroma"))
    {
    if(vp->blur_chroma != val->val_i)
      {
      vp->blur_chroma = val->val_i;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "mode"))
    {
    if(!strcmp(val->val_str, "gauss"))
      vp->mode = MODE_GAUSS;
    else if(!strcmp(val->val_str, "triangular"))
      vp->mode = MODE_TRIANGULAR;
    else if(!strcmp(val->val_str, "box"))
      vp->mode = MODE_BOX;
    vp->changed = 1;
    }
  }

static void connect_input_port_blur(void * priv,
                                         bg_read_video_func_t func,
                                         void * data, int stream, int port)
  {
  blur_priv_t * vp;
  vp = (blur_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_blur(void * priv, gavl_video_format_t * format, int port)
  {
  blur_priv_t * vp;
  vp = (blur_priv_t *)priv;

  if(!port)
    {
    vp->changed = 1;
    gavl_video_format_copy(&vp->format, format);
    }
  }

static void get_output_format_blur(void * priv, gavl_video_format_t * format)
  {
  blur_priv_t * vp;
  vp = (blur_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_blur(void * priv, gavl_video_frame_t * frame, int stream)
  {
  blur_priv_t * vp;
  vp = (blur_priv_t *)priv;

  if((vp->radius_h != 0.0) || (vp->radius_v != 0.0))
    {
    if(vp->changed)
      {
      init_scaler(vp);
      if(vp->frame)
        {
        gavl_video_frame_destroy(vp->frame);
        vp->frame = NULL;
        }
      }
    if(!vp->frame)
      vp->frame = gavl_video_frame_create(&vp->format);
    
    if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
      return 0;
    gavl_video_scaler_scale(vp->scaler,
                            vp->frame, frame);

    gavl_video_frame_copy_metadata(frame, vp->frame);
    return 1;
    }
  else 
    return vp->read_func(vp->read_data, frame, vp->read_stream);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_blur",
      .long_name = TRS("Blur"),
      .description = TRS("Blur filter based on gavl. Supports triangular, box and gauss blur."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_blur,
      .destroy =   destroy_blur,
      .get_parameters =   get_parameters_blur,
      .set_parameter =    set_parameter_blur,
      .priority =         1,
    },
    
    .get_options = get_options_blur,
    .connect_input_port = connect_input_port_blur,
    
    .set_input_format = set_input_format_blur,
    .get_output_format = get_output_format_blur,

    .read_video = read_video_blur,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
