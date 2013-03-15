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

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#include <gavl/gavldsp.h>

#define LOG_DOMAIN "fv_decimate"

typedef struct decimate_priv_s decimate_priv_t;

struct decimate_priv_s
  {
  gavl_dsp_context_t * dsp_ctx;
  gavl_dsp_funcs_t   * dsp_funcs;

  gavl_video_frame_t * frame;
  gavl_video_frame_t * in_frame;

  gavl_video_format_t format;

  gavl_video_frame_t * b1;
  gavl_video_frame_t * b2;

  float threshold_block;
  float threshold_total;
  int do_log;
  int skip_max;

  int have_frame;

  int num_planes;
  int sub_h;
  int sub_v;
  float scale_factors[GAVL_MAX_PLANES];
  int width_mul;
  
  float (*diff_block)(struct decimate_priv_s*, 
                      int width, int height);

  int (*sad_func)(const uint8_t * src_1, const uint8_t * src_2, 
                  int stride_1, int stride_2, 
                  int w, int h);

  gavl_video_source_t * in_src;
  gavl_video_source_t * out_src;
  };

static float diff_block_i(decimate_priv_t * vp, 
                          int width, int height)
  {
  int i;
  double ret = 0.0, tmp;
  
  ret = vp->sad_func(vp->b1->planes[0], vp->b2->planes[0],
                     vp->b1->strides[0], vp->b2->strides[0],
                     width * vp->width_mul, height);

  ret *= vp->scale_factors[0];
  
  width  /= vp->sub_h;
  height /= vp->sub_v;
  
  for(i = 1; i < vp->num_planes; i++)
    {
    tmp = vp->sad_func(vp->b1->planes[i], vp->b2->planes[i],
                       vp->b1->strides[i], vp->b2->strides[i],
                       width, height);
    tmp *= vp->scale_factors[i];
    ret += tmp;
    }
  return ret;
  }

static float diff_block_f(decimate_priv_t * vp, 
                          int width, int height)
  {
  float ret = 0.0;
  
  ret = vp->dsp_funcs->sad_f(vp->b1->planes[0], vp->b2->planes[0],
                             vp->b1->strides[0], vp->b2->strides[0],
                             width * vp->width_mul, height);
  
  ret *= vp->scale_factors[0];

  /* No planar float formats yet */
  return ret;
  }

static void * create_decimate()
  {
  decimate_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->b1 = gavl_video_frame_create(NULL);
  ret->b2 = gavl_video_frame_create(NULL);
  ret->dsp_ctx = gavl_dsp_context_create();
  ret->dsp_funcs = gavl_dsp_context_get_funcs(ret->dsp_ctx);
  return ret;
  }

static void destroy_decimate(void * priv)
  {
  decimate_priv_t * vp;
  vp = priv;
  if(vp->frame)
    gavl_video_frame_destroy(vp->frame);
  gavl_video_frame_null(vp->b1);
  gavl_video_frame_null(vp->b2);

  gavl_video_frame_destroy(vp->b1);
  gavl_video_frame_destroy(vp->b2);
  
  gavl_dsp_context_destroy(vp->dsp_ctx);
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "threshold_block",
      .long_name = TRS("Block threshold"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .val_min = { .val_f = 0.0 },
      .val_max = { .val_f = 1.0 },
      .num_digits = 2,
      .flags = BG_PARAMETER_SYNC,
      .help_string = TRS("Specifies how much a block may differ from the last non-skipped block. 0 means identical blocks, 1 means completely different blocks. Note that the meaning of \"completely different\" depends on the colorspace.")
    },
    {
      .name = "threshold_total",
      .long_name = TRS("Total threshold"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .val_min = { .val_f = 0.0 },
      .val_max = { .val_f = 1.0 },
      .num_digits = 2,
      .flags = BG_PARAMETER_SYNC,
      .help_string = TRS("Specifies how much a frame may differ from the last non-skipped frame. 0 means identical frames,  1 means completely different frames. Note that the meaning of \"completely different\" depends on the colorspace.")
    },
    {
      .name = "skip_max",
      .long_name = TRS("Maximum skipped frames"),
      .type = BG_PARAMETER_INT,
      .val_min =     { .val_i = 1 },
      .val_max =     { .val_i = 500 },
      .val_default = { .val_i = 10 },
      .flags = BG_PARAMETER_SYNC,
      .help_string = TRS("Maximum number of consecutive skipped frames")
    },
    {
      .name = "do_log",
      .long_name = TRS("Report results"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_SYNC,
      .help_string = TRS("Log reports about skipped frames"),
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_decimate(void * priv)
  {
  return parameters;
  }

static void set_parameter_decimate(void * priv, const char * name,
                                   const bg_parameter_value_t * val)
  {
  decimate_priv_t * vp;
  vp = priv;

  if(!name)
    return;

  if(!strcmp(name, "threshold_block"))
    vp->threshold_block = val->val_f;
  else if(!strcmp(name, "threshold_total"))
    vp->threshold_total = val->val_f;
  else if(!strcmp(name, "do_log"))
   vp->do_log = val->val_i;
  else if(!strcmp(name, "skip_max"))
   vp->skip_max = val->val_i;
  }

static void reset_decimate(void * priv)
  {
  decimate_priv_t * vp = priv;
  vp->have_frame = 0;
  }

static void 
set_format(decimate_priv_t * vp, const gavl_video_format_t * format)
  {
  gavl_video_format_copy(&vp->format, format);
  vp->format.framerate_mode = GAVL_FRAMERATE_VARIABLE;
  if(vp->frame)
    {
    gavl_video_frame_destroy(vp->frame);
    vp->frame = NULL;
    }
  gavl_pixelformat_chroma_sub(vp->format.pixelformat,
                              &vp->sub_h, &vp->sub_v);
  vp->num_planes = 
    gavl_pixelformat_num_planes(vp->format.pixelformat);

  vp->width_mul = 1;
  vp->diff_block = diff_block_i;

  switch(vp->format.pixelformat)
    {
    case GAVL_GRAY_8:
      vp->scale_factors[0] = 1.0/(1.0*255.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      break;
    case GAVL_GRAYA_16:
      vp->scale_factors[0] = 1.0/(2.0*255.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      vp->width_mul = 2;
      break;
    case GAVL_GRAY_16:
      vp->scale_factors[0] = 1.0/(1.0*65535.0);
      vp->sad_func = vp->dsp_funcs->sad_16;
      break;
    case GAVL_GRAYA_32:
      vp->scale_factors[0] = 1.0/(2.0*65535.0);
      vp->sad_func = vp->dsp_funcs->sad_16;
      vp->width_mul = 2;
      break;
    case GAVL_GRAY_FLOAT:
      vp->scale_factors[0] = 1.0;
      vp->diff_block = diff_block_f;
      break;
    case GAVL_GRAYA_FLOAT:
      vp->scale_factors[0] = 1.0/2.0;
      vp->diff_block = diff_block_f;
      vp->width_mul = 2;
      break;
    case GAVL_RGB_15:
    case GAVL_BGR_15:
      vp->scale_factors[0] = 1.0/(3.0*255.0);
      vp->sad_func = vp->dsp_funcs->sad_rgb15;
      break;
    case GAVL_RGB_16:
    case GAVL_BGR_16:
      vp->scale_factors[0] = 1.0/(3.0*255.0);
      vp->sad_func = vp->dsp_funcs->sad_rgb16;
      break;
    case GAVL_RGB_24:
    case GAVL_BGR_24:
      vp->scale_factors[0] = 1.0/(3.0*255.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      vp->width_mul = 3;
      break;
    case GAVL_RGB_32:
    case GAVL_BGR_32:
      vp->scale_factors[0] = 1.0/(3.0*255.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      vp->width_mul = 4;
      break;
    case GAVL_RGBA_32:
      vp->scale_factors[0] = 1.0/(4.0*255.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      vp->width_mul = 4;
      break;
    case GAVL_YUV_444_P:
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_422_P:
      vp->scale_factors[0] = 1.0/((235.0 - 16.0)*3.0);
      vp->scale_factors[1] = 
        (float)(vp->sub_h * vp->sub_v)/((240.0 - 16.0)*3.0);
      vp->scale_factors[2] = 
        (float)(vp->sub_h * vp->sub_v)/((240.0 - 16.0)*3.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      break;
    case GAVL_YUV_444_P_16:
    case GAVL_YUV_422_P_16:
      vp->scale_factors[0] = 1.0/((235.0 - 16.0)*256.0*3.0);
      vp->scale_factors[1] = 
        (float)(vp->sub_h * vp->sub_v)/((240.0 - 16.0)*256.0*3.0);
      vp->scale_factors[2] = 
        (float)(vp->sub_h * vp->sub_v)/((240.0 - 16.0)*256.0*3.0);
      vp->sad_func = vp->dsp_funcs->sad_16;
      break;
    case GAVL_RGB_48:
      vp->scale_factors[0] = 1.0/(3.0*65535.0);
      vp->sad_func = vp->dsp_funcs->sad_16;
      vp->width_mul = 3;
      break;
    case GAVL_RGBA_64:
      vp->scale_factors[0] = 1.0/(4.0*65535.0);
      vp->sad_func = vp->dsp_funcs->sad_16;
      vp->width_mul = 4;
      break;
    case GAVL_RGB_FLOAT:
      vp->scale_factors[0] = 1.0/3.0;
      vp->diff_block = diff_block_f;
      vp->width_mul = 3;
      break;
    case GAVL_RGBA_FLOAT:
      vp->scale_factors[0] = 1.0/4.0;
      vp->diff_block = diff_block_f;
      vp->width_mul = 4;
      break;
    case GAVL_YUVA_32:
      vp->scale_factors[0] = 
        1.0/(235.0 - 16.0 + 2.0 * (240.0 - 16.0) + 255.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      vp->width_mul = 4;
      break;
    case GAVL_YUVA_64:
      vp->scale_factors[0] = 
        1.0/((235.0 - 16.0)*256.0 + 2.0 * (240.0 - 16.0)*256.0 + 255.0*256.0);
      vp->sad_func = vp->dsp_funcs->sad_16;
      vp->width_mul = 4;
      break;
    case GAVL_YUV_FLOAT:
      vp->scale_factors[0] = 
        1.0/3.0;
      vp->diff_block = diff_block_f;
      vp->width_mul = 3;
      break;
    case GAVL_YUVA_FLOAT:
      vp->scale_factors[0] = 
        1.0/4.0;
      vp->diff_block = diff_block_f;
      vp->width_mul = 4;
      break;
    case GAVL_YUY2:
    case GAVL_UYVY:
      vp->scale_factors[0] = 
        1.0/(235.0 - 16.0 + 240.0 - 16.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      vp->width_mul = 2;
      break;
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      vp->scale_factors[0] = 1.0/(3.0 * 255.0);
      vp->scale_factors[1] = 
        (float)(vp->sub_h * vp->sub_v)/(3.0 * 255.0);
      vp->scale_factors[2] = 
        (float)(vp->sub_h * vp->sub_v)/(3.0 * 255.0);
      vp->sad_func = vp->dsp_funcs->sad_8;
      break;
    case GAVL_PIXELFORMAT_NONE:
      break;
    }
  vp->frame = gavl_video_frame_create(&vp->format);
  }

#define BLOCK_SIZE 16

static int do_skip(decimate_priv_t * vp,
                   gavl_video_frame_t * f1, gavl_video_frame_t * f2)
  {
  int i, j, imax, jmax;
  float diff_block;
  float diff_total = 0.0;

  int threshold_total = 
    vp->threshold_total * 
    vp->format.image_width * 
    vp->format.image_height;

  gavl_rectangle_i_t rect;
  imax = (vp->format.image_height + BLOCK_SIZE - 1)/BLOCK_SIZE;
  jmax = (vp->format.image_width  + BLOCK_SIZE - 1)/BLOCK_SIZE;

  for(i = 0; i < imax; i++)
    {
    for(j = 0; j < jmax; j++)
      {
      rect.x = j * BLOCK_SIZE;
      rect.y = i * BLOCK_SIZE;
      rect.w = BLOCK_SIZE;
      rect.h = BLOCK_SIZE;
      gavl_rectangle_i_crop_to_format(&rect, &vp->format);
      gavl_video_frame_get_subframe(vp->format.pixelformat,
                                    f1, vp->b1, &rect);
      gavl_video_frame_get_subframe(vp->format.pixelformat,
                                    f2, vp->b2, &rect);
      diff_block = vp->diff_block(vp, rect.w, rect.h);
      if(diff_block > vp->threshold_block * rect.w * rect.h)
        return 0;
      diff_total += diff_block;
      if(diff_total > threshold_total)
        return 0;
      }
    }
  return 1;
  }

static gavl_source_status_t
read_func(void * priv, gavl_video_frame_t ** frame)
  {
  gavl_source_status_t st;
  decimate_priv_t * vp;
  int skipped = 0;

  vp = priv;
  
  /* Read frame */
  if(!vp->have_frame)
    {
    vp->in_frame = NULL;
    if((st = gavl_video_source_read_frame(vp->in_src, &vp->in_frame)) !=
       GAVL_SOURCE_OK)
      return st;
    vp->have_frame = 1;
    }
  
  gavl_video_frame_copy(&vp->format, vp->frame, vp->in_frame);
  gavl_video_frame_copy_metadata(vp->frame, vp->in_frame);
  
  while(1)
    {
    vp->in_frame = NULL;
    if((st = gavl_video_source_read_frame(vp->in_src, &vp->in_frame)) !=
       GAVL_SOURCE_OK)
      {
      if(st == GAVL_SOURCE_EOF)
        {
        // Return last frame
        *frame = vp->frame;
        return GAVL_SOURCE_OK;
        }
      else
        return st;
      }
    if((skipped >= vp->skip_max) || !do_skip(vp, vp->in_frame, vp->frame))
      break;
    skipped++;

    vp->frame->duration += vp->in_frame->duration;
    }
  
  vp->frame->duration = vp->in_frame->timestamp - vp->frame->timestamp;
  
  *frame = vp->frame;
  
  return GAVL_SOURCE_OK;
  }

static gavl_video_source_t *
connect_decimate(void * priv,
                 gavl_video_source_t * src,
                 const gavl_video_options_t * opt)
  {
  decimate_priv_t * vp = priv;
  vp->have_frame = 0;
  vp->in_src = src;
  vp->have_frame = 0;
  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);
  
  set_format(vp, gavl_video_source_get_src_format(vp->in_src));
  
  gavl_video_source_set_dst(vp->in_src, 0, &vp->format);
  
  vp->format.framerate_mode = GAVL_FRAMERATE_VARIABLE;

  vp->out_src =
    gavl_video_source_create_source(read_func,
                                 vp, GAVL_SOURCE_SRC_ALLOC,
                                 vp->in_src);
  return vp->out_src;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_decimate",
      .long_name = TRS("Decimate"),
      .description = TRS("Skip almost identical frames"),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_decimate,
      .destroy =   destroy_decimate,
      .get_parameters =   get_parameters_decimate,
      .set_parameter =    set_parameter_decimate,
      .priority =         1,
    },
    .connect = connect_decimate,
    .reset = reset_decimate,
    
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
