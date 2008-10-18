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
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#include "colormatrix.h"

#define LOG_DOMAIN "fv_technicolor"

typedef struct
  {
  bg_colormatrix_t * mat;
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;
  
  float coeffs[3][4];
  int style;
  float strength;
  float gain[3];
  } technicolor_priv_t;

#define STYLE_BW    0
#define STYLE_TECH1 1
#define STYLE_TECH2 2

static const float coeffs_bw[3][4] =
  {
    { 0.299000,  0.587000,  0.114000, 0.0 },
    { 0.299000,  0.587000,  0.114000, 0.0 },
    { 0.299000,  0.587000,  0.114000, 0.0 },
  };

static const float coeffs_tech1[3][4] =
  {
    { 1.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.5, 0.5, 0.0 },
    { 0.0, 0.5, 0.5, 0.0 },
  };

static const float coeffs_tech2[3][4] =
  {
    {  1.75  -0.50, -0.25, 0.0 },
    { -0.25,  1.50, -0.25, 0.0 },
    { -0.25, -0.50,  1.75, 0.0 },
  };

static const float coeffs_unity[3][4] =
  {
    { 1.0, 0.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0, 0.0 },
  };

static void interpolate(const float coeffs[3][4], float result[3][4], float strength, float * gain)
  {
  int i, j;
  for(i = 0; i < 3; i++)
    {
    for(j = 0; j < 4; j++)
      {
      result[i][j] = gain[i] * (strength * coeffs[i][j] + (1.0 - strength) * coeffs_unity[i][j]);
      }
    }
  }

static void * create_technicolor()
  {
  technicolor_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->mat = bg_colormatrix_create();
  return ret;
  }

static void destroy_technicolor(void * priv)
  {
  technicolor_priv_t * vp;
  vp = (technicolor_priv_t *)priv;
  bg_colormatrix_destroy(vp->mat);
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name =        "style",
      .long_name =   TRS("Style"),
      .type =        BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .val_default = { .val_str = "tech1" },
      .multi_names =
      (char const *[])
      {
        "bw",
        "tech1",
        "tech2",
        (char*)0
      },
      .multi_labels =
      (char const *[])
      {
        "B/W",
        "Technicolor 2-Stripe",
        "Technicolor 3-Stripe",
        (char*)0
      },
    },
    {
      .name = "strength",
      .long_name = TRS("Strength"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 1.0 },
      .val_default = { .val_f = 1.0 },
      .num_digits = 3,
    },
    {
      .name = "r_gain",
      .long_name = TRS("Red gain"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 2.0 },
      .val_default = { .val_f = 1.0 },
      .num_digits = 3,
    },
    {
      .name = "g_gain",
      .long_name = TRS("Green gain"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 2.0 },
      .val_default = { .val_f = 1.0 },
      .num_digits = 3,
    },
    {
      .name = "b_gain",
      .long_name = TRS("Blue gain"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min =     { .val_f = 0.0 },
      .val_max =     { .val_f = 2.0 },
      .val_default = { .val_f = 1.0 },
      .num_digits = 3,
    },
    { /* End of parameters */ },
  };


static const bg_parameter_info_t * get_parameters_technicolor(void * priv)
  {
  return parameters;
  }

static void set_coeffs(technicolor_priv_t * vp)
  {
  switch(vp->style)
    {
    case STYLE_BW:
      interpolate(coeffs_bw, vp->coeffs, vp->strength, vp->gain);
      break;
    case STYLE_TECH1:
      interpolate(coeffs_tech1, vp->coeffs, vp->strength, vp->gain);
      break;
    case STYLE_TECH2:
      interpolate(coeffs_tech2, vp->coeffs, vp->strength, vp->gain);
      break;
    }
  }

static void set_parameter_technicolor(void * priv, const char * name,
                               const bg_parameter_value_t * val)
  {
  technicolor_priv_t * vp;
  int changed = 0;
  vp = (technicolor_priv_t *)priv;
  if(!name)
    return;
  
  if(!strcmp(name, "style"))
    {
    if(!strcmp(val->val_str, "bw"))
      {
      if(vp->style != STYLE_BW)
        {
        vp->style = STYLE_BW;
        changed = 1;
        }
      }
    if(!strcmp(val->val_str, "tech1"))
      {
      if(vp->style != STYLE_TECH1)
        {
        vp->style = STYLE_TECH1;
        changed = 1;
        }
      }
    if(!strcmp(val->val_str, "tech2"))
      {
      if(vp->style != STYLE_TECH2)
        {
        vp->style = STYLE_TECH2;
        changed = 1;
        }
      }
    }
  else if(!strcmp(name, "strength"))
    {
    if(vp->strength != val->val_f)
      {
      vp->strength = val->val_f;
      changed = 1;
      }
    }
  else if(!strcmp(name, "r_gain"))
    {
    if(vp->gain[0] != val->val_f)
      {
      vp->gain[0] = val->val_f;
      changed = 1;
      }
    }
  else if(!strcmp(name, "g_gain"))
    {
    if(vp->gain[1] != val->val_f)
      {
      vp->gain[1] = val->val_f;
      changed = 1;
      }
    }
  else if(!strcmp(name, "b_gain"))
    {
    if(vp->gain[2] != val->val_f)
      {
      vp->gain[2] = val->val_f;
      changed = 1;
      }
    }
  if(changed)
    {
    set_coeffs(vp);
    bg_colormatrix_set_rgb(vp->mat, vp->coeffs);
    }
  }

static void connect_input_port_technicolor(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  technicolor_priv_t * vp;
  vp = (technicolor_priv_t *)priv;
  
  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  }

static void set_input_format_technicolor(void * priv, gavl_video_format_t * format, int port)
  {
  technicolor_priv_t * vp;
  vp = (technicolor_priv_t *)priv;

  if(!port)
    {
    bg_colormatrix_init(vp->mat, format, 0);
    gavl_video_format_copy(&vp->format, format);
    }
  }

static void get_output_format_technicolor(void * priv, gavl_video_format_t * format)
  {
  technicolor_priv_t * vp;
  vp = (technicolor_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }

static int read_video_technicolor(void * priv, gavl_video_frame_t * frame, int stream)
  {
  technicolor_priv_t * vp;
  vp = (technicolor_priv_t *)priv;

#if 0  
  if(!vp->technicolor_h && !vp->technicolor_v)
    {
    return vp->read_func(vp->read_data, frame, vp->read_stream);
    }
#endif
  if(!vp->read_func(vp->read_data, frame, vp->read_stream))
    return 0;
  
  bg_colormatrix_process(vp->mat, frame);
  return 1;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_oldcolor",
      .long_name = TRS("Old color"),
      .description = TRS("Simulate old color- and B/W movies"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_technicolor,
      .destroy =   destroy_technicolor,
      .get_parameters =   get_parameters_technicolor,
      .set_parameter =    set_parameter_technicolor,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_technicolor,
    
    .set_input_format = set_input_format_technicolor,
    .get_output_format = get_output_format_technicolor,

    .read_video = read_video_technicolor,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
