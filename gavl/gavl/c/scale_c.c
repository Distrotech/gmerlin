#include <stdio.h>
#include <gavl/gavl.h>
#include <video.h>
#include <scale.h>

#define SCALE_FUNC_HEAD \
  for(i = 0; i < s->table[plane].num_coeffs_h; i++) \
    {

#define SCALE_FUNC_TAIL \
    }

/* Nearest neighbor */

static void scale_16_16_nearest_c(gavl_video_scaler_t * s,
                                  uint8_t * _src,
                                  int src_stride,
                                  uint8_t * _dst,
                                  int plane,
                                  int scanline)
  {
  uint16_t * src, *dst;
  int i;
  src = (uint16_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);
  dst = (uint16_t*)_dst;
  SCALE_FUNC_HEAD
    dst[i] = src[s->table[plane].coeffs_h[i].index[0]];
  SCALE_FUNC_TAIL
  }

static void scale_24_24_nearest_c(gavl_video_scaler_t * s,
                                  uint8_t * _src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  uint8_t * src, *src1;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);
  SCALE_FUNC_HEAD
    src1 = src + s->table[plane].coeffs_h[i].index[0]*3;
    dst[0] = src1[0];
    dst[1] = src1[1];
    dst[2] = src1[2];
    dst += 3;
  SCALE_FUNC_TAIL
  }

static void scale_24_32_nearest_c(gavl_video_scaler_t * s,
                                  uint8_t * _src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  uint8_t *src1, *src;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);
  SCALE_FUNC_HEAD
    src1 = src + s->table[plane].coeffs_h[i].index[0]*4;
    dst[0] = src1[0];
    dst[1] = src1[1];
    dst[2] = src1[2];
    dst += 4;
  SCALE_FUNC_TAIL

  }

static void scale_32_32_nearest_c(gavl_video_scaler_t * s,
                                  uint8_t * _src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  uint8_t * src, *src1;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);
  SCALE_FUNC_HEAD
    src1 = src + s->table[plane].coeffs_h[i].index[0]*4;
    dst[0] = src1[0];
    dst[1] = src1[1];
    dst[2] = src1[2];
    dst[3] = src1[3];
    dst += 4;
  SCALE_FUNC_TAIL
  }

static void scale_8_nearest_c(gavl_video_scaler_t * s,
                              uint8_t * _src,
                              int src_stride,
                              uint8_t * dst,
                              int plane,
                              int scanline)
  {
  int i;
  uint8_t * src;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);

  SCALE_FUNC_HEAD
    *dst = *(src + s->table[plane].coeffs_h[i].index[0]);
  SCALE_FUNC_TAIL
  }

static void scale_yuy2_nearest_c(gavl_video_scaler_t * s,
                                 uint8_t * src,
                                 int src_stride,
                                 uint8_t * dst,
                                 int plane,
                                 int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  
  SCALE_FUNC_TAIL

  }

static void scale_uyvy_nearest_c(gavl_video_scaler_t * s,
                                 uint8_t * src,
                                 int src_stride,
                                 uint8_t * dst,
                                 int plane,
                                 int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  
  SCALE_FUNC_TAIL

  }

/* Bilinear x */

static void scale_x_15_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
    
  SCALE_FUNC_TAIL
  }

static void scale_x_16_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_x_24_24_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_x_24_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_x_8_bilinear_c(gavl_video_scaler_t * s,
                               uint8_t * src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_x_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_x_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

/* Bilinear y */

static void scale_y_15_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
    
  SCALE_FUNC_TAIL
  }

static void scale_y_16_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_y_24_24_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_y_24_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_y_8_bilinear_c(gavl_video_scaler_t * s,
                               uint8_t * src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_y_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_y_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }


/* Bilinear x and y */

static void scale_xy_15_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
    
  SCALE_FUNC_TAIL
  }

static void scale_xy_16_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_24_24_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_24_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_8_bilinear_c(gavl_video_scaler_t * s,
                               uint8_t * src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  SCALE_FUNC_HEAD
  
  SCALE_FUNC_TAIL
  }

void gavl_init_scale_funcs_c(gavl_scale_funcs_t * tab,
                             gavl_scale_mode_t scale_mode,
                             int scale_x, int scale_y)
  {
  switch(scale_mode)
    {
    case GAVL_SCALE_AUTO:
      fprintf(stderr,
              "ERROR: gavl_init_scale_funcs_c called with GAVL_SCALE_AUTO\n");
      break;
    case GAVL_SCALE_NEAREST:

      tab->scale_15_16 = scale_16_16_nearest_c;
      tab->scale_16_16 = scale_16_16_nearest_c;
      tab->scale_24_24 = scale_24_24_nearest_c;
      tab->scale_24_32 = scale_24_32_nearest_c;
      tab->scale_32_32 = scale_32_32_nearest_c;
      
      tab->scale_8     = scale_8_nearest_c;
      tab->scale_yuy2  = scale_yuy2_nearest_c;
      tab->scale_uyvy  = scale_uyvy_nearest_c;
      
      break;
    case GAVL_SCALE_BILINEAR:
      if(scale_x && scale_y)
        {
        tab->scale_15_16 = scale_xy_15_16_bilinear_c;
        tab->scale_16_16 = scale_xy_16_16_bilinear_c;
        tab->scale_24_24 = scale_xy_24_24_bilinear_c;
        tab->scale_24_32 = scale_xy_24_32_bilinear_c;
        
        tab->scale_8     = scale_xy_8_bilinear_c;
        tab->scale_yuy2  = scale_xy_yuy2_bilinear_c;
        tab->scale_uyvy  = scale_xy_uyvy_bilinear_c;
        }
      else if(scale_x)
        {
        tab->scale_15_16 = scale_x_15_16_bilinear_c;
        tab->scale_16_16 = scale_x_16_16_bilinear_c;
        tab->scale_24_24 = scale_x_24_24_bilinear_c;
        tab->scale_24_32 = scale_x_24_32_bilinear_c;
        
        tab->scale_8     = scale_x_8_bilinear_c;
        tab->scale_yuy2  = scale_x_yuy2_bilinear_c;
        tab->scale_uyvy  = scale_x_uyvy_bilinear_c;
        }
      else if(scale_y)
        {
        tab->scale_15_16 = scale_y_15_16_bilinear_c;
        tab->scale_16_16 = scale_y_16_16_bilinear_c;
        tab->scale_24_24 = scale_y_24_24_bilinear_c;
        tab->scale_24_32 = scale_y_24_32_bilinear_c;
        
        tab->scale_8     = scale_y_8_bilinear_c;
        tab->scale_yuy2  = scale_y_yuy2_bilinear_c;
        tab->scale_uyvy  = scale_y_uyvy_bilinear_c;
        }
      break;
    }
  }

