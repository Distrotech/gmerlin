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


#define LOG_DOMAIN "fv_colorbalance"

#include "colormatrix.h"

typedef struct colorbalance_priv_s
  {
  float gain[3];
  
  bg_colormatrix_t * mat;
  float coeffs[3][4];
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;

  int use_matrix;
  int normalize;
  
  gavl_video_format_t format;

  void (*process)(struct colorbalance_priv_s *, gavl_video_frame_t*);
  
  int (*read_video)(struct colorbalance_priv_s *,
                    gavl_video_frame_t * frame);
  
  } colorbalance_priv_t;

static void normalize_gain(colorbalance_priv_t * vp, float * gain_ret)
  {
  float luminance;
  if(vp->normalize)
    {
    luminance =
      0.299000 * vp->gain[0] +
      0.587000 * vp->gain[1] +
      0.114000 * vp->gain[2];
    gain_ret[0] = vp->gain[0] / luminance;
    gain_ret[1] = vp->gain[1] / luminance;
    gain_ret[2] = vp->gain[2] / luminance;
    }
  else
    {
    gain_ret[0] = vp->gain[0];
    gain_ret[1] = vp->gain[1];
    gain_ret[2] = vp->gain[2];
    }
  }
     
static void set_coeffs(colorbalance_priv_t * vp)
  {
  float gain_norm[3];
  normalize_gain(vp, gain_norm);
  
  vp->coeffs[0][0] = gain_norm[0];
  vp->coeffs[0][1] = 0.0;
  vp->coeffs[0][2] = 0.0;
  vp->coeffs[0][3] = 0.0;
  
  vp->coeffs[1][0] = 0.0;
  vp->coeffs[1][1] = gain_norm[1];
  vp->coeffs[1][2] = 0.0;
  vp->coeffs[1][3] = 0.0;

  vp->coeffs[2][0] = 0.0;
  vp->coeffs[2][1] = 0.0;
  vp->coeffs[2][2] = gain_norm[2];
  vp->coeffs[2][3] = 0.0;
  }

static int read_video_fast(colorbalance_priv_t * vp,
                           gavl_video_frame_t * frame)
  {
  if(!vp->read_func(vp->read_data, frame, vp->read_stream))
    return 0;

  /* Do the conversion */

  if((vp->gain[0] != 1.0) || (vp->gain[1] != 1.0) || (vp->gain[2] != 1.0))
    {
    vp->process(vp, frame);
    }
  
  return 1;
  }

static int read_video_matrix(colorbalance_priv_t * vp,
                             gavl_video_frame_t * frame)
  {
  if(!vp->read_func(vp->read_data, frame, vp->read_stream))
    return 0;
  
  if((vp->gain[0] != 1.0) || (vp->gain[1] != 1.0) || (vp->gain[2] != 1.0))
    bg_colormatrix_process(vp->mat, frame);
  return 1;
  }

static void * create_colorbalance()
  {
  colorbalance_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->mat = bg_colormatrix_create();
  return ret;
  }

static void destroy_colorbalance(void * priv)
  {
  colorbalance_priv_t * vp;
  vp = (colorbalance_priv_t *)priv;
  bg_colormatrix_destroy(vp->mat);
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "r",
      .long_name = TRS("Red gain"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f =   0.0 },
      .val_max = { .val_f =   2.0 },
      .val_default = { .val_f =  1.0 },
      .num_digits = 2,
    },
    {
      .name = "g",
      .long_name = TRS("Green gain"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f =   0.0 },
      .val_max = { .val_f =   2.0 },
      .val_default = { .val_f =  1.0 },
      .num_digits = 2,
    },
    {
      .name = "b",
      .long_name = TRS("Blue gain"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f =   0.0 },
      .val_max = { .val_f =   2.0 },
      .val_default = { .val_f =  1.0 },
      .num_digits = 2,
    },
    {
      .name = "n",
      .long_name = TRS("Normalize"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
      .help_string = TRS("Normalize for constant luminance."),
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_colorbalance(void * priv)
  {
  return parameters;
  }

static void set_parameter_colorbalance(void * priv, const char * name,
                                    const bg_parameter_value_t * val)
  {
  int changed = 0;
  colorbalance_priv_t * vp;
  
  vp = (colorbalance_priv_t *)priv;
  
  if(!name)
    return;
  
  if(!strcmp(name, "r"))
    {
    if(vp->gain[0] != val->val_f)
      {
      vp->gain[0] = val->val_f;
      changed = 1;
      }
    }
  else if(!strcmp(name, "g"))
    {
    if(vp->gain[1] != val->val_f)
      {
      vp->gain[1] = val->val_f;
      changed = 1;
      }
    }
  else if(!strcmp(name, "b"))
    {
    if(vp->gain[2] != val->val_f)
      {
      vp->gain[2] = val->val_f;
      changed = 1;
      }
    }
  else if(!strcmp(name, "n"))
    {
    if(vp->normalize != val->val_i)
      {
      vp->normalize = val->val_i;
      changed = 1;
      }
    }
  
  if(changed && vp->use_matrix)
    {
    set_coeffs(vp);
    bg_colormatrix_set_rgb(vp->mat, vp->coeffs);
    }
  }

static void connect_input_port_colorbalance(void * priv,
                                         bg_read_video_func_t func,
                                         void * data, int stream, int port)
  {
  colorbalance_priv_t * vp;
  vp = (colorbalance_priv_t *)priv;

  if(!port)
    {
    vp->read_func = func;
    vp->read_data = data;
    vp->read_stream = stream;
    }
  
  }

/* Processing functions */

#define CONVERT_GAIN \
  normalize_gain(vp, gain_norm);\
  for(i = 0; i < 3; i++) \
    gain_i[i] = (int)(gain_norm[i] * 65536.0 + 0.5)

#define CONVERT_8(p, f) \
   tmp = (p * f) >> 16; \
   if(tmp & 0xFFFF00) tmp= (-tmp)>>31; \
   p=tmp;

#define CONVERT_16(p, f) \
   tmp = (p * f) >> 16; \
   if(tmp & 0xFFFF0000) tmp= (-tmp)>>63; \
   p=tmp;

static void process_rgb24(colorbalance_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;
  float gain_norm[3];
  int gain_i[3];
  uint8_t * dest;
  uint8_t * dest1;
  int tmp;
  int w = vp->format.image_width;
  int h = vp->format.image_height;

  dest1 = frame->planes[0];
  
  CONVERT_GAIN;

  while (h--)
    {
    dest = dest1;
    for (i = 0; i<w; i++)
      {
      CONVERT_8(dest[0], gain_i[0]);
      CONVERT_8(dest[1], gain_i[1]);
      CONVERT_8(dest[2], gain_i[2]);
      dest += 3;
      }
    dest1 += frame->strides[0];
    }
  }

static void process_rgb32(colorbalance_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;
  float gain_norm[3];
  int gain_i[3];
  uint8_t * dest;
  uint8_t * dest1;
  int tmp;
  int w = vp->format.image_width;
  int h = vp->format.image_height;

  dest1 = frame->planes[0];
  
  CONVERT_GAIN;

  while (h--)
    {
    dest = dest1;
    for (i = 0; i<w; i++)
      {
      CONVERT_8(dest[0], gain_i[0]);
      CONVERT_8(dest[1], gain_i[1]);
      CONVERT_8(dest[2], gain_i[2]);
      dest += 4;
      }
    dest1 += frame->strides[0];
    }
  }

static void process_bgr24(colorbalance_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;
  float gain_norm[3];
  int gain_i[3];
  uint8_t * dest;
  uint8_t * dest1;
  int tmp;
  int w = vp->format.image_width;
  int h = vp->format.image_height;

  dest1 = frame->planes[0];
  
  CONVERT_GAIN;

  while (h--)
    {
    dest = dest1;
    for (i = 0; i<w; i++)
      {
      CONVERT_8(dest[0], gain_i[2]);
      CONVERT_8(dest[1], gain_i[1]);
      CONVERT_8(dest[2], gain_i[0]);
      dest += 3;
      }
    dest1 += frame->strides[0];
    }
  }

static void process_bgr32(colorbalance_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;
  float gain_norm[3];
  int gain_i[3];
  uint8_t * dest;
  uint8_t * dest1;
  int tmp;
  int w = vp->format.image_width;
  int h = vp->format.image_height;

  dest1 = frame->planes[0];
  
  CONVERT_GAIN;

  while (h--)
    {
    dest = dest1;
    for (i = 0; i<w; i++)
      {
      CONVERT_8(dest[0], gain_i[2]);
      CONVERT_8(dest[1], gain_i[1]);
      CONVERT_8(dest[2], gain_i[0]);
      dest += 4;
      }
    dest1 += frame->strides[0];
    }
  }

static void process_rgb48(colorbalance_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;
  float gain_norm[3];
  int64_t gain_i[3];
  uint16_t * dest;
  uint8_t * dest1;
  int64_t tmp;
  int w = vp->format.image_width;
  int h = vp->format.image_height;

  dest1 = frame->planes[0];
  
  CONVERT_GAIN;

  while (h--)
    {
    dest = (uint16_t *)dest1;
    for (i = 0; i<w; i++)
      {
      CONVERT_16(dest[0], gain_i[0]);
      CONVERT_16(dest[1], gain_i[1]);
      CONVERT_16(dest[2], gain_i[2]);
      dest += 3;
      }
    dest1 += frame->strides[0];
    }
  }

static void process_rgb64(colorbalance_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;
  float gain_norm[3];
  int64_t gain_i[3];
  uint16_t * dest;
  uint8_t * dest1;
  int64_t tmp;
  int w = vp->format.image_width;
  int h = vp->format.image_height;

  dest1 = frame->planes[0];
  
  CONVERT_GAIN;

  while (h--)
    {
    dest = (uint16_t *)dest1;
    for (i = 0; i<w; i++)
      {
      CONVERT_16(dest[0], gain_i[0]);
      CONVERT_16(dest[1], gain_i[1]);
      CONVERT_16(dest[2], gain_i[2]);
      dest += 4;
      }
    dest1 += frame->strides[0];
    }
  }

static void process_rgb_float(colorbalance_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;
  float gain_norm[3];
  float * dest;
  uint8_t * dest1;
  int w = vp->format.image_width;
  int h = vp->format.image_height;

  normalize_gain(vp, gain_norm);
  
  dest1 = frame->planes[0];
  
  while (h--)
    {
    dest = (float *)dest1;
    for (i = 0; i<w; i++)
      {
      dest[0] *= gain_norm[0];
      dest[1] *= gain_norm[1];
      dest[2] *= gain_norm[2];
      dest += 3;
      }
    dest1 += frame->strides[0];
    }
  }

static void process_rgba_float(colorbalance_priv_t * vp, gavl_video_frame_t * frame)
  {
  int i;
  float gain_norm[3];
  float * dest;
  uint8_t * dest1;
  int w = vp->format.image_width;
  int h = vp->format.image_height;

  normalize_gain(vp, gain_norm);
  
  dest1 = frame->planes[0];
  
  while (h--)
    {
    dest = (float *)dest1;
    for (i = 0; i<w; i++)
      {
      dest[0] *= gain_norm[0];
      dest[1] *= gain_norm[1];
      dest[2] *= gain_norm[2];
      dest += 4;
      }
    dest1 += frame->strides[0];
    }
  }

static void set_input_format_colorbalance(void * priv, gavl_video_format_t * format, int port)
  {
  colorbalance_priv_t * vp;
  vp = (colorbalance_priv_t *)priv;

  vp->use_matrix = 0;

  if(port)
    return;
  
  switch(format->pixelformat)
    {
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
      vp->process = process_rgb24;
      break;
    case GAVL_BGR_24:
      vp->process = process_bgr24;
      break;
    case GAVL_BGR_32:
      vp->process = process_bgr32;
      break;
    case GAVL_RGB_32:
    case GAVL_RGBA_32:
      vp->process = process_rgb32;
      break;
    case GAVL_RGB_48:
      vp->process = process_rgb48;
      break;
    case GAVL_RGBA_64:
      vp->process = process_rgb64;
      break;
    case GAVL_RGB_FLOAT:
      vp->process = process_rgb_float;
      break;
    case GAVL_RGBA_FLOAT:
      vp->process = process_rgba_float;
      break;
    case GAVL_YUV_444_P:
    case GAVL_YUVA_32:
    case GAVL_YUY2:
    case GAVL_UYVY:
    case GAVL_YUV_444_P_16:
    case GAVL_YUV_422_P_16:
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_422_P:
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      vp->use_matrix = 1;
      break;
    case GAVL_PIXELFORMAT_NONE:
      break;
    }

  if(vp->use_matrix)
    {
    set_coeffs(vp);
    
    bg_colormatrix_init(vp->mat, format, 0);
    bg_colormatrix_set_rgb(vp->mat, vp->coeffs);
    vp->read_video = read_video_matrix;
    }
  else
    {
    vp->read_video = read_video_fast;
    }


  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Pixelformat: %s",
         TRD(gavl_pixelformat_to_string(format->pixelformat), NULL));
  
  gavl_video_format_copy(&vp->format, format);
  
  }

static void get_output_format_colorbalance(void * priv, gavl_video_format_t * format)
  {
  colorbalance_priv_t * vp;
  vp = (colorbalance_priv_t *)priv;
  gavl_video_format_copy(format, &vp->format);
  }


static int
read_video_colorbalance(void * priv, gavl_video_frame_t * frame, int stream)
  {
  colorbalance_priv_t * vp;
  vp = (colorbalance_priv_t *)priv;
  return vp->read_video(vp, frame);
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_colorbalance",
      .long_name = TRS("Colorbalance"),
      .description = TRS("Apply gain for red, green and blue. RGB formats are processed directly, Y'CbCr formats are processed by the colormatrix."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_colorbalance,
      .destroy =   destroy_colorbalance,
      .get_parameters =   get_parameters_colorbalance,
      .set_parameter =    set_parameter_colorbalance,
      .priority =         1,
    },
    
    .connect_input_port = connect_input_port_colorbalance,
    
    .set_input_format = set_input_format_colorbalance,
    .get_output_format = get_output_format_colorbalance,

    .read_video = read_video_colorbalance,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
