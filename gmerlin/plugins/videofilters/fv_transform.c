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
#include <math.h>

#include <config.h>
#include <translation.h>
#include <plugin.h>
#include <utils.h>
#include <log.h>
#include <bggavl.h>

#define LOG_DOMAIN "fv_transform"

#define MODE_ROTATE 0

typedef struct
  {
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;
  
  gavl_video_frame_t * frame;
  gavl_image_transform_t * transform;
  gavl_video_options_t * opt;
  
  int changed;
  int quality;
  int scale_order;
  gavl_scale_mode_t scale_mode;

  int mode;
  
  float bg_color[4];
  
  /* Some transforms can be described by a matrix */
  double matrix[2][3];

  float rotate_angle;
  
  } transform_t;

static void * create_transform()
  {
  transform_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->transform = gavl_image_transform_create();
  ret->opt = gavl_image_transform_get_options(ret->transform);
  return ret;
  }

static void destroy_transform(void * priv)
  {
  transform_t * vp;
  vp = (transform_t *)priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);

  gavl_image_transform_destroy(vp->transform);
  
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "general",
      .long_name =   TRS("General"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name =        "mode",
      .long_name =   TRS("Transformation mode"),
      .opt =         "tm",
      .type =        BG_PARAMETER_STRINGLIST,
      .flags = BG_PARAMETER_SYNC,
      .multi_names = (const char*[]){ "rotate", (char*)0 },
      .multi_labels = (const char*[]){ TRS("Rotate"), (char*)0 },
      .val_default = { .val_str = "rotate" },
      .help_string = TRS("Choose Transformation method. Each method can be configured in it's section."),
      
    },
    {
    .name =        "scale_mode",
    .long_name =   TRS("Interpolation mode"),
    .opt =         "im",
    .type =        BG_PARAMETER_STRINGLIST,
    .flags = BG_PARAMETER_SYNC,
    .multi_names = BG_GAVL_SCALE_MODE_NAMES,
    .multi_labels = BG_GAVL_SCALE_MODE_LABELS,
    .val_default = { .val_str = "auto" },
    .help_string = TRS("Choose interpolation method. Auto means to choose based on the conversion quality. Nearest is fastest, Bicubic is slowest."),
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
    {
    .name =        "mode_rotate",
    .long_name =   TRS("Rotate"),
    .type =        BG_PARAMETER_SECTION,
    },
    {
      .name      =  "rotate_angle",
      .long_name =  "Angle",
      .type =        BG_PARAMETER_SLIDER_FLOAT,
      .flags =     BG_PARAMETER_SYNC,
      .val_default = { .val_f = 0.0 },
      .val_min = { .val_f = 0.0 },
      .val_max = { .val_f = 360.0 },
      .num_digits = 2,
    },
    { /* End of parameters */ },
  };
static const bg_parameter_info_t * get_parameters_transform(void * priv)
  {
  return parameters;
  }

static void set_parameter_transform(void * priv, const char * name, const bg_parameter_value_t * val)
  {
  transform_t * vp;
  gavl_scale_mode_t scale_mode;
  
  vp = (transform_t *)priv;

  if(!name)
    return;

  else if(!strcmp(name, "scale_mode"))
    {
    scale_mode = bg_gavl_string_to_scale_mode(val->val_str);
    if(vp->scale_mode != scale_mode)
      {
      vp->scale_mode = scale_mode;
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
  else if(!strcmp(name, "bg_color"))
    {
    memcpy(vp->bg_color, val->val_color, sizeof(vp->bg_color));
    }
  else if(!strcmp(name, "mode"))
    {
    int new_mode = 0;
    if(!strcmp(val->val_str, "rotate"))
      new_mode = MODE_ROTATE;

    if(new_mode != vp->mode)
      {
      vp->mode = new_mode;
      vp->changed = 1;
      }
    }
  else if(!strcmp(name, "rotate_angle"))
    {
    if(vp->rotate_angle != val->val_f)
      {
      vp->rotate_angle = val->val_f;
      vp->changed = 1;
      }
    }
  }

static void connect_input_port_transform(void * priv,
                                    bg_read_video_func_t func,
                                    void * data, int stream, int port)
  {
  transform_t * vp;
  vp = (transform_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

static void set_input_format_transform(void * priv,
                                gavl_video_format_t * format, int port)
  {
  transform_t * vp;
  vp = (transform_t *)priv;

  if(!port)
    gavl_video_format_copy(&vp->format, format);

  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = (gavl_video_frame_t*)0;
    }
  vp->changed = 1;
  }

static void get_output_format_transform(void * priv,
                                 gavl_video_format_t * format)
  {
  transform_t * vp;
  vp = (transform_t *)priv;
  
  gavl_video_format_copy(format, &vp->format);
  }

static void transform_func_matrix(void * priv,
                                  double dst_x, double dst_y,
                                  double * src_x, double * src_y)
  {
  transform_t * vp;
  vp = (transform_t *)priv;
  *src_x = dst_x * vp->matrix[0][0] + dst_y * vp->matrix[0][1] + vp->matrix[0][2];
  *src_y = dst_x * vp->matrix[1][0] + dst_y * vp->matrix[1][1] + vp->matrix[1][2];
  }

static void matrixmult(double src1[2][3], double src2[2][3], double dst[2][3])
  {
  int i, j;

  for(i = 0; i < 2; i++)
    {
    for(j = 0; j < 3; j++)
      {
      dst[i][j] = 
        src1[i][0] * src2[0][j] +
        src1[i][1] * src2[1][j];
      }
    dst[i][2] += src1[i][2];
    }

  }

static void init_matrix_rotate(transform_t * vp)
  {
  double mat1[2][3];
  double mat2[2][3];
  double sin_angle, cos_angle;

  sin_angle = sin(vp->rotate_angle / 180.0 * M_PI);
  cos_angle = cos(vp->rotate_angle / 180.0 * M_PI);
    
  mat1[0][0] = 1.0;
  mat1[1][0] = 0.0;
  mat1[0][1] = 0.0;
  mat1[1][1] = 1.0;
  mat1[0][2] = -0.5 * vp->format.image_width;
  mat1[1][2] = -0.5 * vp->format.image_height;

  mat2[0][0] = cos_angle;
  mat2[1][0] = sin_angle;
  mat2[0][1] = -sin_angle;
  mat2[1][1] = cos_angle;
  mat2[0][2] = 0.0;
  mat2[1][2] = 0.0;

  matrixmult(mat2, mat1, vp->matrix);
  vp->matrix[0][2] += 0.5 * vp->format.image_width;
  vp->matrix[1][2] += 0.5 * vp->format.image_height;
  
  }

static void init_transform(transform_t * vp)
  {
  gavl_image_transform_func func;
  switch(vp->mode)
    {
    case MODE_ROTATE:
      init_matrix_rotate(vp);
      func = transform_func_matrix;
      break;
    }

  gavl_video_options_set_scale_mode(vp->opt, vp->scale_mode);
  gavl_video_options_set_quality(vp->opt, vp->quality);
  
  
  gavl_image_transform_init(vp->transform, &vp->format,
                            func, vp);
  
  vp->changed = 0;
  }

static int read_video_transform(void * priv, gavl_video_frame_t * frame,
                         int stream)
  {
  transform_t * vp;
  vp = (transform_t *)priv;
  
  if(!vp->frame)
    {
    vp->frame = gavl_video_frame_create(&vp->format);
    gavl_video_frame_clear(vp->frame, &vp->format);
    }
  if(!vp->read_func(vp->read_data, vp->frame, vp->read_stream))
    return 0;

  if(vp->changed)
    init_transform(vp);
  
  gavl_video_frame_fill(frame, &vp->format, vp->bg_color);
  
  gavl_image_transform_transform(vp->transform, vp->frame, frame);

  gavl_video_frame_copy_metadata(frame, vp->frame);
  
  return 1;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_transform",
      .long_name = TRS("Transform"),
      .description = TRS("Transform the image with different methods"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_transform,
      .destroy =   destroy_transform,
      .get_parameters =   get_parameters_transform,
      .set_parameter =    set_parameter_transform,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_transform,
    
    .set_input_format = set_input_format_transform,
    .get_output_format = get_output_format_transform,

    .read_video = read_video_transform,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
