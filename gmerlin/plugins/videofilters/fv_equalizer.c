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

/*
 *  Based on vf_eq.c (by Richard Felker) and
 *  vf_hue.c (by Michael Niedermayer) from MPlayer
 *
 *  Added support for packed YUV formats, "in place"
 *  conversion and processing of RGB formats via the
 *  colormatrix.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>


#define LOG_DOMAIN "fv_equalizer"

#include "colormatrix.h"

#define CLAMP_Y(val) (val > 1.0) ? 1.0 : ((val < 0.0) ? 0.0 : val)
#define CLAMP_UV(val) (val > 0.5) ? 0.5 : ((val < -0.5) ? -0.5 : val)

typedef struct equalizer_priv_s
  {
  int brightness;
  int contrast;
  float hue;
  float saturation;

  bg_colormatrix_t * mat;
  float coeffs[3][4];
  
  /* Offsets (in bytes) and advance (in components) */
  int offset[3];
  int advance[2];
  
  int chroma_width;
  int chroma_height;
  int use_matrix;
  
  gavl_video_format_t format;
  gavl_video_options_t * global_opt;

  void (*process_bc)(unsigned char *dest, int dstride,
                     int w, int h, int brightness, int contrast, int advance);
  
  void (*process_sh)(uint8_t *udst, uint8_t *vdst,
                     int dststride,
                     int w, int h, float hue, float sat, int advance);
  
  void (*process)(struct equalizer_priv_s *,
                  gavl_video_frame_t * frame);

  gavl_video_source_t * in_src;
  gavl_video_source_t * out_src;
  } equalizer_priv_t;

static void set_coeffs(equalizer_priv_t * vp)
  {
  float s, c;
  float contrast, brightness;
  
  /* Brightness and contrast go to Y */

  contrast = (vp->contrast+100.0) / 100.0;
  brightness = (vp->brightness+100.0)/100.0 - 0.5 * (1.0 + contrast);
  
  vp->coeffs[0][0] = contrast;
  vp->coeffs[0][1] = 0.0;
  vp->coeffs[0][2] = 0.0;
  vp->coeffs[0][3] = brightness;
  
  /* Saturation and Hue go to U and V */

  s = sin(vp->hue) * vp->saturation;
  c = cos(vp->hue) * vp->saturation;

  vp->coeffs[1][0] = 0.0;
  vp->coeffs[1][1] = c;
  vp->coeffs[1][2] = -s;
  vp->coeffs[1][3] = 0.0;

  vp->coeffs[2][0] = 0.0;
  vp->coeffs[2][1] = s;
  vp->coeffs[2][2] = c;
  vp->coeffs[2][3] = 0.0;
  
  }


static void process_sh_C(uint8_t *udst, uint8_t *vdst,
                         int dststride, 
                         int w, int h, float hue, float sat, int advance)
  {
  int i, index;
  const int s= rint(sin(hue) * (1<<16) * sat);
  const int c= rint(cos(hue) * (1<<16) * sat);
  
  while (h--)
    {
    index = 0;
    for (i = 0; i<w; i++)
      {
      const int u= udst[index] - 128;
      const int v= vdst[index] - 128;
      int32_t new_u= (c*u - s*v + (1<<15) + (128<<16))>>16;
      int32_t new_v= (s*u + c*v + (1<<15) + (128<<16))>>16;
      if(new_u & 0xFFFF00) new_u= (-new_u)>>31;
      if(new_v & 0xFFFF00) new_v= (-new_v)>>31;
      udst[index]= new_u;
      vdst[index]= new_v;
      index += advance;
      }
    udst += dststride;
    vdst += dststride;
    }
  }

static void process_sh_float_C(uint8_t *udst1, uint8_t *vdst1,
                               int dststride, 
                               int w, int h, float hue, float sat, int advance)
  {
  int i, index;
  const float s= sin(hue) * sat;
  const float c= cos(hue) * sat;

  float * udst;
  float * vdst;
  
  while (h--)
    {
    index = 0;
    udst = (float*)udst1;
    vdst = (float*)vdst1;
    
    for (i = 0; i<w; i++)
      {
      float new_u= (c*udst[index] - s*vdst[index]);
      float new_v= (s*udst[index] + c*vdst[index]);
      udst[index]= CLAMP_UV(new_u);
      vdst[index]= CLAMP_UV(new_v);
      index += advance;
      }
    udst1 += dststride;
    vdst1 += dststride;
    }
  }


static void process_bc_C(unsigned char *dest, int dstride,
                         int w, int h, int brightness, int contrast, int advance)
  {
  int i, index;
  int pel;
  
  contrast = ((contrast+100)*256*256)/100;
  brightness = (((brightness+100)*511*219)/200-128*219 - (contrast*219)/512) / 255;
  
  while (h--)
    {
    index = 0;
    for (i = w; i; i--)
      {
      pel = ((dest[index] * contrast)>>16) + brightness;
      if(pel&768) pel = (-pel)>>31;
      dest[index] = pel;
      index += advance;
      }
    dest += dstride;
    }
  }

static void process_bc_float_C(uint8_t *ydst1, int dststride, 
                               int w, int h, int brightness1, int contrast1, int advance)
  {
  int i, index;
  float * ydst;
  float contrast, brightness;
  float new_y;
  contrast = (contrast1+100.0) / 100.0;
  brightness = (brightness1+100.0)/100.0 - 0.5 * (1.0 + contrast);
  
  while (h--)
    {
    index = 0;
    ydst = (float*)ydst1;
    
    for (i = 0; i<w; i++)
      {
      new_y = contrast * ydst[index] + brightness;
      ydst[index] = CLAMP_Y(new_y);
      index += advance;
      }
    ydst1 += dststride;
    }
  }


static void process_bcj_C(unsigned char *dest, int dstride,
                          int w, int h, int brightness, int contrast, int advance)
  {
  int i, index;
  int pel;
  
  contrast = ((contrast+100)*256*256)/100;
  brightness = ((brightness+100)*511)/200-128 - contrast/512;
  
  while (h--)
    {
    index = 0;
    for (i = w; i; i--)
      {
      pel = ((dest[index] * contrast)>>16) + brightness;
      if(pel&768) pel = (-pel)>>31;
      dest[index] = pel;
      index += advance;
      }
    dest += dstride;
    }
  }

/* 16 bit YUV */

static void process_sh16_C(uint8_t *udst1, uint8_t *vdst1,
                           int dststride, 
                           int w, int h, float hue, float sat, int advance)
  {
  int i, index;
  const int64_t s= rint(sin(hue) * (1<<16) * sat);
  const int64_t c= rint(cos(hue) * (1<<16) * sat);
  uint16_t * udst;
  uint16_t * vdst;
  
  while (h--)
    {
    udst = (uint16_t *)udst1;
    vdst = (uint16_t *)vdst1;
    
    index = 0;
    for (i = 0; i<w; i++)
      {
      const int64_t u= udst[index] - (128<<8);
      const int64_t v= vdst[index] - (128<<8);
      int64_t new_u= (c*u - s*v + (1<<(15+8)) + (128LL<<(16+8)))>>16;
      int64_t new_v= (s*u + c*v + (1<<(15+8)) + (128LL<<(16+8)))>>16;
      if(new_u & 0xFFFF0000LL) new_u= (-new_u)>>63;
      if(new_v & 0xFFFF0000LL) new_v= (-new_v)>>63;
      udst[index]= new_u;
      vdst[index]= new_v;
      index += advance;
      }
    udst1 += dststride;
    vdst1 += dststride;
    }
  }

static void process_bc16_C(unsigned char *dest1, int dstride,
                           int w, int h, int brightness1, int contrast1, int advance)
  {
  int i, index;
  int64_t pel;
  uint16_t * dest;
  int64_t brightness, contrast;
  
  contrast = ((contrast1+100)*256*256)/100;
  brightness = ((brightness1+100)*511*219)/200-128*219 - (contrast*219)/512;
  brightness = (brightness * 0x0101)/255;
  
  while (h--)
    {
    dest = (uint16_t *)dest1;
    
    index = 0;
    for (i = w; i; i--)
      {
      pel = (((int64_t)dest[index] * contrast)>>16) + brightness;
      if(pel & 0xFFFF0000LL) pel= (-pel)>>63;
      dest[index] = pel;
      index += advance;
      }
    dest1 += dstride;
    }
  }

static void process_bcj16_C(unsigned char *dest1, int dstride,
                            int w, int h, int brightness1, int contrast1, int advance)
  {
  int i, index;
  int64_t pel;
  uint16_t * dest;
  int64_t brightness, contrast;
  
  contrast = ((contrast1+100)*256*256)/100;
  brightness = ((brightness1+100)*511*256)/200-128*256 - (contrast*256)/512;
  
  while (h--)
    {
    dest = (uint16_t *)dest1;
    index = 0;
    for (i = w; i; i--)
      {
      pel = (((int64_t)dest[index] * contrast)>>16) + brightness;
      if(pel & 0xFFFF0000LL) pel= (-pel)>>63;
      dest[index] = pel;
      index += advance;
      }
    dest1 += dstride;
    }
  }


static void process_fast(equalizer_priv_t * vp,
                         gavl_video_frame_t * frame)
  {
  /* Do the conversion */

  if((vp->contrast != 0.0) || (vp->brightness != 0.0))
    {
    vp->process_bc(frame->planes[0] + vp->offset[0], frame->strides[0],
                   vp->format.image_width,
                   vp->format.image_height,
                   vp->brightness, vp->contrast, vp->advance[0]);
    }

  if(vp->process_sh)
    {
    if((vp->hue != 0.0) || (vp->saturation != 1.0))
      {
      if(gavl_pixelformat_is_planar(vp->format.pixelformat))
        {
        vp->process_sh(frame->planes[1] + vp->offset[1],
                       frame->planes[2] + vp->offset[2],
                       frame->strides[1],
                       vp->chroma_width,
                       vp->chroma_height,
                       vp->hue, vp->saturation, vp->advance[1]);
        }
      else
        {
        vp->process_sh(frame->planes[0] + vp->offset[1],
                       frame->planes[0] + vp->offset[2],
                       frame->strides[0],
                       vp->chroma_width,
                       vp->chroma_height,
                       vp->hue, vp->saturation, vp->advance[1]);
        }
      }
    }
  }

static void process_matrix(equalizer_priv_t * vp,
                           gavl_video_frame_t * frame)
  {
  if((vp->contrast != 0.0) || (vp->brightness != 0.0) ||
     (vp->hue != 0.0) || (vp->saturation != 1.0))
    bg_colormatrix_process(vp->mat, frame);
  }

static void * create_equalizer()
  {
  equalizer_priv_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->mat = bg_colormatrix_create();
  ret->global_opt = gavl_video_options_create();
  return ret;
  }

static void destroy_equalizer(void * priv)
  {
  equalizer_priv_t * vp;
  vp = priv;
  bg_colormatrix_destroy(vp->mat);
  gavl_video_options_destroy(vp->global_opt);
  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);
  free(vp);
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .gettext_domain = PACKAGE,
      .gettext_directory = LOCALE_DIR,
      .name = "brightness",
      .long_name = TRS("Brightness"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f = -10.0 },
      .val_max = { .val_f =  10.0 },
      .val_default = { .val_f =  0.0 },
      .num_digits = 1,
    },
    {
      .name = "contrast",
      .long_name = TRS("Contrast"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f = -10.0 },
      .val_max = { .val_f =  10.0 },
      .val_default = { .val_f =  0.0 },
      .num_digits = 1,
    },
    {
      .name = "saturation",
      .long_name = TRS("Saturation"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f = -10.0 },
      .val_max = { .val_f =  10.0 },
      .val_default = { .val_f =  0.0 },
      .num_digits = 1,
    },
    {
      .name = "hue",
      .long_name = TRS("Hue"),
      .type = BG_PARAMETER_SLIDER_FLOAT,
      .flags = BG_PARAMETER_SYNC,
      .val_min = { .val_f = -180.0 },
      .val_max = { .val_f =  180.0 },
      .val_default = { .val_f =  0.0 },
      .num_digits = 1,
    },
    { /* End of parameters */ },
  };

static const bg_parameter_info_t * get_parameters_equalizer(void * priv)
  {
  return parameters;
  }

static void set_parameter_equalizer(void * priv, const char * name,
                                    const bg_parameter_value_t * val)
  {
  int changed = 0;
  equalizer_priv_t * vp;
  int test_i;
  float test_f;

  vp = priv;
  
  if(!name)
    return;
  
  if(!strcmp(name, "brightness"))
    {
    test_i = 10*val->val_f;
    if(vp->brightness != test_i)
      {
      vp->brightness = test_i;
      changed = 1;
      }
    }
  else if(!strcmp(name, "contrast"))
    {
    test_i = 10*val->val_f;
    if(vp->contrast != test_i)
      {
      vp->contrast = test_i;
      changed = 1;
      }
    }
  else if(!strcmp(name, "saturation"))
    {
    test_f = (val->val_f + 10.0)/10.0;
    
    if(vp->saturation != test_f)
      {
      vp->saturation = test_f;
      changed = 1;
      }
    }
  else if(!strcmp(name, "hue"))
    {
    test_f = val->val_f * M_PI / 180.0;
    if(vp->hue != test_f)
      {
      vp->hue = test_f;
      changed = 1;
      }
    }
  if(changed && vp->use_matrix)
    {
    set_coeffs(vp);
    bg_colormatrix_set_yuv(vp->mat, vp->coeffs);
    }
  }

static void set_format(void * priv, const gavl_video_format_t * format)
  {
  int sub_h, sub_v;
  equalizer_priv_t * vp;
  vp = priv;
  
  vp->use_matrix = 0;

  gavl_video_format_copy(&vp->format, format);
  
  switch(vp->format.pixelformat)
    {
    case GAVL_GRAY_8:
      vp->advance[0] = 1;
      vp->offset[0]  = 0;
      vp->process_bc = process_bcj_C;
      vp->process_sh = NULL;
      break;
    case GAVL_GRAYA_16:
      vp->advance[0] = 2;
      vp->offset[0]  = 0;
      vp->process_bc = process_bcj_C;
      vp->process_sh = NULL;
      break;
    case GAVL_GRAY_16:
      vp->advance[0] = 1;
      vp->offset[0]  = 0;
      vp->process_bc = process_bcj16_C;
      vp->process_sh = NULL;
      break;
    case GAVL_GRAYA_32:
      vp->advance[0] = 2;
      vp->offset[0]  = 0;
      vp->process_bc = process_bcj16_C;
      vp->process_sh = NULL;
      break;
    case GAVL_GRAY_FLOAT:
      vp->advance[0] = 1;
      vp->offset[0]  = 0;
      vp->process_bc = process_bc_float_C;
      vp->process_sh = NULL;
      break;
    case GAVL_GRAYA_FLOAT:
      vp->advance[0] = 2;
      vp->offset[0]  = 0;
      vp->process_bc = process_bc_float_C;
      vp->process_sh = NULL;
      break;
    case GAVL_RGB_15:
    case GAVL_BGR_15:
    case GAVL_RGB_16:
    case GAVL_BGR_16:
    case GAVL_RGB_24:
    case GAVL_BGR_24:
    case GAVL_RGB_32:
    case GAVL_BGR_32:
    case GAVL_RGB_48:
    case GAVL_RGB_FLOAT:
    case GAVL_RGBA_32:
    case GAVL_RGBA_64:
    case GAVL_RGBA_FLOAT:
      vp->use_matrix = 1;
      break;
    case GAVL_YUV_444_P:
      vp->advance[0] = 1;
      vp->advance[1] = 1;
      vp->offset[0]  = 0;
      vp->offset[1]  = 0;
      vp->offset[2]  = 0;
      vp->process_bc = process_bc_C;
      vp->process_sh = process_sh_C;
      break;
    case GAVL_YUVA_32:
      vp->advance[0] = 4;
      vp->advance[1] = 4;
      vp->offset[0]  = 0;
      vp->offset[1]  = 1;
      vp->offset[2]  = 2;
      vp->process_bc = process_bc_C;
      vp->process_sh = process_sh_C;
      break;
    case GAVL_YUVA_64:
      vp->advance[0] = 4;
      vp->advance[1] = 4;
      vp->offset[0]  = 0;
      vp->offset[1]  = 2;
      vp->offset[2]  = 4;
      vp->process_bc = process_bc16_C;
      vp->process_sh = process_sh16_C;
      break;
    case GAVL_YUV_FLOAT:
      vp->advance[0] = 3;
      vp->advance[1] = 3;
      vp->offset[0]  = 0;
      vp->offset[1]  = sizeof(float);
      vp->offset[2]  = 2*sizeof(float);
      vp->process_bc = process_bc_float_C;
      vp->process_sh = process_sh_float_C;
      break;
    case GAVL_YUVA_FLOAT:
      vp->advance[0] = 4;
      vp->advance[1] = 4;
      vp->offset[0]  = 0;
      vp->offset[1]  = sizeof(float);
      vp->offset[2]  = 2*sizeof(float);
      vp->process_bc = process_bc_float_C;
      vp->process_sh = process_sh_float_C;
      break;
    case GAVL_YUY2:
      vp->advance[0] = 2;
      vp->advance[1] = 4;
      vp->offset[0]  = 0;
      vp->offset[1]  = 1;
      vp->offset[2]  = 3;
      vp->process_bc = process_bc_C;
      vp->process_sh = process_sh_C;
      break;
    case GAVL_UYVY:
      vp->advance[0] = 2;
      vp->advance[1] = 4;
      vp->offset[0]  = 1;
      vp->offset[1]  = 0;
      vp->offset[2]  = 2;
      vp->process_bc = process_bc_C;
      vp->process_sh = process_sh_C;
      break;
    case GAVL_YUV_444_P_16:
    case GAVL_YUV_422_P_16:
      vp->advance[0] = 1;
      vp->advance[1] = 1;
      vp->offset[0]  = 0;
      vp->offset[1]  = 0;
      vp->offset[2]  = 0;
      vp->process_bc = process_bc16_C;
      vp->process_sh = process_sh16_C;
      break;
    case GAVL_YUV_420_P:
    case GAVL_YUV_410_P:
    case GAVL_YUV_411_P:
    case GAVL_YUV_422_P:
      vp->advance[0] = 1;
      vp->advance[1] = 1;
      vp->offset[0]  = 0;
      vp->offset[1]  = 0;
      vp->offset[2]  = 0;
      vp->process_bc = process_bc_C;
      vp->process_sh = process_sh_C;
      break;
    case GAVL_YUVJ_420_P:
    case GAVL_YUVJ_422_P:
    case GAVL_YUVJ_444_P:
      vp->process_bc = process_bcj_C;
      vp->process_sh = process_sh_C;
      vp->advance[0] = 1;
      vp->advance[1] = 1;
      vp->offset[0]  = 0;
      vp->offset[1]  = 0;
      vp->offset[2]  = 0;
      break;
    case GAVL_PIXELFORMAT_NONE:
      break;
    }

  if(vp->use_matrix)
    {
    set_coeffs(vp);
    bg_colormatrix_init(vp->mat, &vp->format, 0, vp->global_opt);
    bg_colormatrix_set_yuv(vp->mat, vp->coeffs);
    vp->process = process_matrix;
    }
  else
    {
    gavl_pixelformat_chroma_sub(format->pixelformat, &sub_h, &sub_v);
    
    vp->chroma_width  = format->image_width / sub_h;
    vp->chroma_height = format->image_height / sub_v;
    vp->process = process_fast;
    }
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Pixelformat: %s",
         TRD(gavl_pixelformat_to_string(format->pixelformat), NULL));
  }

static gavl_source_status_t
read_func(void * priv, gavl_video_frame_t ** frame)
  {
  equalizer_priv_t * vp;
  gavl_source_status_t st;

  vp = priv;

  if((st = gavl_video_source_read_frame(vp->in_src, frame)) != GAVL_SOURCE_OK)
    return st;
  
  vp->process(vp, *frame);
  return GAVL_SOURCE_OK;
  }

static gavl_video_source_t * connect_equalizer(void * priv, gavl_video_source_t * src,
                                               const gavl_video_options_t * opt)
  {
  equalizer_priv_t * vp;
  vp = priv;
  if(vp->out_src)
    gavl_video_source_destroy(vp->out_src);

  vp->in_src = src;
  set_format(vp, gavl_video_source_get_src_format(vp->in_src));
  
  if(opt)
    gavl_video_options_copy(vp->global_opt, opt);
  
  gavl_video_source_set_dst(vp->in_src, 0, &vp->format);

  vp->out_src =
    gavl_video_source_create_source(read_func,
                                    vp, 0,
                                    vp->in_src);
  return vp->out_src;
  }

const bg_fv_plugin_t the_plugin = 
  {
    .common =
    {
      BG_LOCALE,
      .name =      "fv_equalizer",
      .long_name = TRS("Equalizer"),
      .description = TRS("Control hue, saturation, contrast and brightness. For Y'CbCr, it uses routines based on the vf_eq and vf_hue filters from the MPlayer project. For RGB formats, it uses the colormatrix."),
      .type =     BG_PLUGIN_FILTER_VIDEO,
      .flags =    BG_PLUGIN_FILTER_1,
      .create =   create_equalizer,
      .destroy =   destroy_equalizer,
      .get_parameters =   get_parameters_equalizer,
      .set_parameter =    set_parameter_equalizer,
      .priority =         1,
    },
    .connect = connect_equalizer,
  };

/* Include this into all plugin modules exactly once
   to let the plugin loader obtain the API version */
BG_GET_PLUGIN_API_VERSION;
