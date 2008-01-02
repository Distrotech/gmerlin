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

#include "colormatrix.h"

#define LOG_DOMAIN "fv_invert_rgb"

typedef struct invert_priv_s
  {
  bg_colormatrix_t * mat;
  
  bg_read_video_func_t read_func;
  void * read_data;
  int read_stream;
  
  gavl_video_format_t format;

  float coeffs[4][5];
  int invert[4];

  void (*process)(struct invert_priv_s * p, gavl_video_frame_t * f);
  
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

static void process_rgb24(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  int mask[3];
  int anti_mask[3];
  uint8_t * src;
  int i, j;
  mask[0] = vp->invert[0] ? 0x00 : 0xff;
  mask[1] = vp->invert[1] ? 0x00 : 0xff;
  mask[2] = vp->invert[2] ? 0x00 : 0xff;

  anti_mask[0] = ~mask[0];
  anti_mask[1] = ~mask[1];
  anti_mask[2] = ~mask[2];
  
  for(i = 0; i < vp->format.image_height; i++)
    {
    src = frame->planes[0] + i * frame->strides[0];
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] & mask[0]) | ((0xff - src[0]) & anti_mask[0]);
      src[1] = (src[1] & mask[1]) | ((0xff - src[1]) & anti_mask[1]);
      src[2] = (src[2] & mask[2]) | ((0xff - src[2]) & anti_mask[2]);
      src+=3;
      }
    }
  }

static void process_rgb32(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  int mask[3];
  int anti_mask[3];
  uint8_t * src;
  int i, j;
  mask[0] = vp->invert[0] ? 0x00 : 0xff;
  mask[1] = vp->invert[1] ? 0x00 : 0xff;
  mask[2] = vp->invert[2] ? 0x00 : 0xff;
  anti_mask[0] = ~mask[0];
  anti_mask[1] = ~mask[1];
  anti_mask[2] = ~mask[2];

  for(i = 0; i < vp->format.image_height; i++)
    {
    src = frame->planes[0] + i * frame->strides[0];
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] & mask[0]) | ((0xff - src[0]) & anti_mask[0]);
      src[1] = (src[1] & mask[1]) | ((0xff - src[1]) & anti_mask[1]);
      src[2] = (src[2] & mask[2]) | ((0xff - src[2]) & anti_mask[2]);
      src+=4;
      }
    }
  }

static void process_bgr24(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  int mask[3];
  int anti_mask[3];
  uint8_t * src;
  int i, j;
  mask[2] = vp->invert[0] ? 0x00 : 0xff;
  mask[1] = vp->invert[1] ? 0x00 : 0xff;
  mask[0] = vp->invert[2] ? 0x00 : 0xff;
  anti_mask[0] = ~mask[0];
  anti_mask[1] = ~mask[1];
  anti_mask[2] = ~mask[2];

  for(i = 0; i < vp->format.image_height; i++)
    {
    src = frame->planes[0] + i * frame->strides[0];
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] & mask[0]) | ((0xff - src[0]) & anti_mask[0]);
      src[1] = (src[1] & mask[1]) | ((0xff - src[1]) & anti_mask[1]);
      src[2] = (src[2] & mask[2]) | ((0xff - src[2]) & anti_mask[2]);
      src+=3;
      }
    }
  }

static void process_bgr32(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  int mask[3];
  int anti_mask[3];
  uint8_t * src;
  int i, j;
  mask[2] = vp->invert[0] ? 0x00 : 0xff;
  mask[1] = vp->invert[1] ? 0x00 : 0xff;
  mask[0] = vp->invert[2] ? 0x00 : 0xff;
  anti_mask[0] = ~mask[0];
  anti_mask[1] = ~mask[1];
  anti_mask[2] = ~mask[2];

  for(i = 0; i < vp->format.image_height; i++)
    {
    src = frame->planes[0] + i * frame->strides[0];
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] & mask[0]) | ((0xff - src[0]) & anti_mask[0]);
      src[1] = (src[1] & mask[1]) | ((0xff - src[1]) & anti_mask[1]);
      src[2] = (src[2] & mask[2]) | ((0xff - src[2]) & anti_mask[2]);
      src+=4;
      }
    }
  }

static void process_rgba32(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  int mask[4];
  int anti_mask[4];
  uint8_t * src;
  int i, j;
  mask[0] = vp->invert[0] ? 0x00 : 0xff;
  mask[1] = vp->invert[1] ? 0x00 : 0xff;
  mask[2] = vp->invert[2] ? 0x00 : 0xff;
  mask[3] = vp->invert[3] ? 0x00 : 0xff;
  anti_mask[0] = ~mask[0];
  anti_mask[1] = ~mask[1];
  anti_mask[2] = ~mask[2];
  anti_mask[3] = ~mask[3];

  for(i = 0; i < vp->format.image_height; i++)
    {
    src = frame->planes[0] + i * frame->strides[0];
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] & mask[0]) | ((0xff - src[0]) & anti_mask[0]);
      src[1] = (src[1] & mask[1]) | ((0xff - src[1]) & anti_mask[1]);
      src[2] = (src[2] & mask[2]) | ((0xff - src[2]) & anti_mask[2]);
      src[3] = (src[3] & mask[3]) | ((0xff - src[3]) & anti_mask[3]);
      src+=4;
      }
    }
  }


static void process_rgb48(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  int mask[3];
  int anti_mask[3];
  uint16_t * src;
  int i, j;
  mask[0] = vp->invert[0] ? 0x0000 : 0xffff;
  mask[1] = vp->invert[1] ? 0x0000 : 0xffff;
  mask[2] = vp->invert[2] ? 0x0000 : 0xffff;
  anti_mask[0] = ~mask[0];
  anti_mask[1] = ~mask[1];
  anti_mask[2] = ~mask[2];
  
  for(i = 0; i < vp->format.image_height; i++)
    {
    src = (uint16_t *)(frame->planes[0] + i * frame->strides[0]);
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] & mask[0]) | ((0xffff - src[0]) & anti_mask[0]);
      src[1] = (src[1] & mask[1]) | ((0xffff - src[1]) & anti_mask[1]);
      src[2] = (src[2] & mask[2]) | ((0xffff - src[2]) & anti_mask[2]);
      src+=3;
      }
    }
  }


static void process_rgba64(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  int mask[4];
  int anti_mask[4];
  uint16_t * src;
  int i, j;
  mask[0] = vp->invert[0] ? 0x0000 : 0xffff;
  mask[1] = vp->invert[1] ? 0x0000 : 0xffff;
  mask[2] = vp->invert[2] ? 0x0000 : 0xffff;
  mask[3] = vp->invert[3] ? 0x0000 : 0xffff;
  anti_mask[0] = ~mask[0];
  anti_mask[1] = ~mask[1];
  anti_mask[2] = ~mask[2];
  anti_mask[3] = ~mask[3];

  for(i = 0; i < vp->format.image_height; i++)
    {
    src = (uint16_t *)(frame->planes[0] + i * frame->strides[0]);
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] & mask[0]) | ((0xffff - src[0]) & anti_mask[0]);
      src[1] = (src[1] & mask[1]) | ((0xffff - src[1]) & anti_mask[1]);
      src[2] = (src[2] & mask[2]) | ((0xffff - src[2]) & anti_mask[2]);
      src[3] = (src[3] & mask[3]) | ((0xffff - src[3]) & anti_mask[3]);
      src+=4;
      }
    }
  }

static void process_rgb_float(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  float mask[3];
  float anti_mask[3];
  float * src;
  int i, j;
  mask[0] = vp->invert[0] ? 0.0 : 1.0;
  mask[1] = vp->invert[1] ? 0.0 : 1.0;
  mask[2] = vp->invert[2] ? 0.0 : 1.0;
  anti_mask[0] = 1.0 - mask[0];
  anti_mask[1] = 1.0 - mask[1];
  anti_mask[2] = 1.0 - mask[2];
  
  for(i = 0; i < vp->format.image_height; i++)
    {
    src = (float *)(frame->planes[0] + i * frame->strides[0]);
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] * mask[0]) + ((1.0 - src[0]) * anti_mask[0]);
      src[1] = (src[1] * mask[1]) + ((1.0 - src[1]) * anti_mask[1]);
      src[2] = (src[2] * mask[2]) + ((1.0 - src[2]) * anti_mask[2]);
      src+=3;
      }
    }
  }


static void process_rgba_float(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  float mask[4];
  float anti_mask[4];
  float * src;
  int i, j;
  mask[0] = vp->invert[0] ? 0.0 : 1.0;
  mask[1] = vp->invert[1] ? 0.0 : 1.0;
  mask[2] = vp->invert[2] ? 0.0 : 1.0;
  mask[3] = vp->invert[3] ? 0.0 : 1.0;
  anti_mask[0] = 1.0 - mask[0];
  anti_mask[1] = 1.0 - mask[1];
  anti_mask[2] = 1.0 - mask[2];
  anti_mask[3] = 1.0 - mask[3];
  
  for(i = 0; i < vp->format.image_height; i++)
    {
    src = (float *)(frame->planes[0] + i * frame->strides[0]);
    /* The following should be faster than the 9 multiplications and 3
       additions per pixel */
    for(j = 0; j < vp->format.image_width; j++)
      {
      src[0] = (src[0] * mask[0]) + ((1.0 - src[0]) * anti_mask[0]);
      src[1] = (src[1] * mask[1]) + ((1.0 - src[1]) * anti_mask[1]);
      src[2] = (src[2] * mask[2]) + ((1.0 - src[2]) * anti_mask[2]);
      src[3] = (src[3] * mask[3]) + ((1.0 - src[3]) * anti_mask[3]);
      src+=4;
      }
    }
  }


static void process_matrix(invert_priv_t * vp, gavl_video_frame_t * frame)
  {
  bg_colormatrix_process(vp->mat, frame);
  }

static void set_input_format_invert(void * priv, gavl_video_format_t * format, int port)
  {
  invert_priv_t * vp;
  vp = (invert_priv_t *)priv;

  if(!port)
    {
    switch(format->pixelformat)
      {
      case GAVL_RGB_24:
        vp->process = process_rgb24;
        break;
      case GAVL_RGB_32:
        vp->process = process_rgb32;
        break;
      case GAVL_BGR_24:
        vp->process = process_bgr24;
        break;
      case GAVL_BGR_32:
        vp->process = process_bgr32;
        break;
      case GAVL_RGBA_32:
        vp->process = process_rgba32;
        break;
      case GAVL_RGB_48:
        vp->process = process_rgb48;
        break;
      case GAVL_RGBA_64:
        vp->process = process_rgba64;
        break;
      case GAVL_RGB_FLOAT:
        vp->process = process_rgb_float;
        break;
      case GAVL_RGBA_FLOAT:
        vp->process = process_rgba_float;
        break;
      default:
        vp->process = process_matrix;
        bg_colormatrix_init(vp->mat, format, 0);
        break;
      }
    
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
  
  if(!vp->read_func(vp->read_data, frame, vp->read_stream))
    return 0;
  
  if(vp->invert[0] || vp->invert[1] || vp->invert[2] || vp->invert[3])
    vp->process(vp, frame);
  
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
