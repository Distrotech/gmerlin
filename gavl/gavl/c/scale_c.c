#include <stdio.h>
#include <gavl/gavl.h>
#include <video.h>
#include <scale.h>

// Masks for BGR16 and RGB16 formats

#define RGB16_LOWER_MASK  0x001f
#define RGB16_MIDDLE_MASK 0x07e0
#define RGB16_UPPER_MASK  0xf800

// Extract unsigned char RGB values from 15 bit pixels

#define RGB16_TO_R(pixel) ((pixel & RGB16_UPPER_MASK)>>8)
#define RGB16_TO_G(pixel) ((pixel & RGB16_MIDDLE_MASK)>>3) 
#define RGB16_TO_B(pixel) ((pixel & RGB16_LOWER_MASK)<<3)

// Masks for BGR16 and RGB16 formats

#define RGB15_LOWER_MASK  0x001f
#define RGB15_MIDDLE_MASK 0x03e0
#define RGB15_UPPER_MASK  0x7C00

// Extract unsigned char RGB values from 16 bit pixels

#define RGB15_TO_R(pixel) ((pixel & RGB15_UPPER_MASK)>>7)
#define RGB15_TO_G(pixel) ((pixel & RGB15_MIDDLE_MASK)>>2) 
#define RGB15_TO_B(pixel) ((pixel & RGB15_LOWER_MASK)<<3)

/* Combine r, g and b values to a 16 bit rgb pixel (taken from avifile) */

#define PACK_RGB16(c,pixel) pixel=((((((c[0]<<5)&0xff00)|c[1])<<6)&0xfff00)|c[2])>>3;
#define PACK_RGB15(c,pixel) pixel=((((((c[0]<<5)&0xff00)|c[1])<<5)&0xfff00)|c[2])>>3;

#define UNPACK_RGB16(pixel,c) c[0]=RGB16_TO_R(pixel);c[1]=RGB16_TO_R(pixel);c[2]=RGB16_TO_R(pixel);
#define UNPACK_RGB15(pixel,c) c[0]=RGB15_TO_R(pixel);c[1]=RGB15_TO_R(pixel);c[2]=RGB15_TO_R(pixel);

#define SCALE_FUNC_HEAD(a) \
  imax = s->table[plane].num_coeffs_h/a; \
  for(i = 0; i < imax; i++)       \
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
  int i, imax;
  src = (uint16_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);
  dst = (uint16_t*)_dst;
  SCALE_FUNC_HEAD(1)
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
  int i, imax;
  uint8_t * src, *src1;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);
  SCALE_FUNC_HEAD(1)
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
  int i, imax;
  uint8_t *src1, *src;

  //  fprintf(stderr, "scale_24_32_nearest_c\n");
  
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);
  SCALE_FUNC_HEAD(1)
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
  int i, imax;
  uint8_t * src, *src1;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);
  SCALE_FUNC_HEAD(1)
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
  int i, imax;
  uint8_t * src;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);

  SCALE_FUNC_HEAD(1)
    *dst = *(src + s->table[plane].coeffs_h[i].index[0]);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_8_advance(gavl_video_scaler_t * s,
                            uint8_t * _src,
                            int src_stride,
                            uint8_t * dst,
                            int plane,
                            int scanline, int advance)
  {
  int i, imax;
  uint8_t * src;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index[0] * src_stride);

  SCALE_FUNC_HEAD(1)
    *dst = *(src + s->table[plane].coeffs_h[i].index[0] * advance);
    dst+=advance;
  SCALE_FUNC_TAIL
  }


static void scale_yuy2_nearest_c(gavl_video_scaler_t * s,
                                 uint8_t * src,
                                 int src_stride,
                                 uint8_t * dst,
                                 int plane,
                                 int scanline)
  {
  scale_8_advance(s,
                  src,
                  src_stride,
                  dst,
                  0,
                  scanline, 2);

  scale_8_advance(s,
                  src+1,
                  src_stride,
                  dst+1,
                  1,
                  scanline, 4);

  scale_8_advance(s,
                  src+3,
                  src_stride,
                  dst+3,
                  1,
                  scanline, 4);
  
  }

static void scale_uyvy_nearest_c(gavl_video_scaler_t * s,
                                 uint8_t * src,
                                 int src_stride,
                                 uint8_t * dst,
                                 int plane,
                                 int scanline)
  {
  scale_8_advance(s,
                  src+1,
                  src_stride,
                  dst+1,
                  0,
                  scanline, 2);

  scale_8_advance(s,
                  src,
                  src_stride,
                  dst,
                  1,
                  scanline, 4);

  scale_8_advance(s,
                  src+2,
                  src_stride,
                  dst+2,
                  1,
                  scanline, 4);

  }

/* Bilinear x */

#define BILINEAR_1D_1(src_c_1, src_c_2, factor_1, factor_2, dst_c) \
  tmp = src_c_1[0] * factor_1 + src_c_2[0] * factor_2;             \
  dst[0] = tmp >> 8;
  

#define BILINEAR_1D_3(src_c_1, src_c_2, factor_1, factor_2, dst_c) \
  tmp = src_c_1[0] * factor_1 + src_c_2[0] * factor_2;             \
  dst[0] = tmp >> 8;                                               \
  tmp = src_c_1[1] * factor_1 + src_c_2[1] * factor_2;             \
  dst[1] = tmp >> 8;                                               \
  tmp = src_c_1[2] * factor_1 + src_c_2[2] * factor_2;             \
  dst[2] = tmp >> 8;
  

#define BILINEAR_1D_4(src_c_1, src_c_2, factor_1, factor_2, dst_c) \
  tmp = src_c_1[0] * factor_1 + src_c_2[0] * factor_2;             \
  dst[0] = tmp >> 8;                                               \
  tmp = src_c_1[1] * factor_1 + src_c_2[1] * factor_2;             \
  dst[1] = tmp >> 8;                                               \
  tmp = src_c_1[2] * factor_1 + src_c_2[2] * factor_2;             \
  dst[2] = tmp >> 8;                                               \
  tmp = src_c_1[3] * factor_1 + src_c_2[3] * factor_2;             \
  dst[3] = tmp >> 8;

/*
 *  dst = ((src_c_11 * factor_h1 + src_c_12 * factor_h1) * factor_v1 +
 *         (src_c_21 * factor_h1 + src_c_22 * factor_h1) * factor_v2) >> 16
 */

#define BILINEAR_2D_1(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, factor_v1, factor_v2, dst_c) \
  tmp = ((src_c_11[0] * factor_h1 + src_c_12[0] * factor_h1) * factor_v1 + \
         (src_c_21[0] * factor_h1 + src_c_22[0] * factor_h1) * factor_v2); \
  dst[0] = tmp >> 16;

#define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, factor_v1, factor_v2, dst_c) \
  tmp = ((src_c_11[0] * factor_h1 + src_c_12[0] * factor_h1) * factor_v1 + \
         (src_c_21[0] * factor_h1 + src_c_22[0] * factor_h1) * factor_v2); \
  dst[0] = tmp >> 16;                                                   \
  tmp = ((src_c_11[1] * factor_h1 + src_c_12[1] * factor_h1) * factor_v1 + \
         (src_c_21[1] * factor_h1 + src_c_22[1] * factor_h1) * factor_v2); \
  dst[1] = tmp >> 16;                                                   \
  tmp = ((src_c_11[2] * factor_h1 + src_c_12[2] * factor_h1) * factor_v1 + \
         (src_c_21[2] * factor_h1 + src_c_22[2] * factor_h1) * factor_v2); \
  dst[2] = tmp >> 16;


#define BILINEAR_2D_4(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, factor_v1, factor_v2, dst_c) \
  tmp = ((src_c_11[0] * factor_h1 + src_c_12[0] * factor_h1) * factor_v1 + \
         (src_c_21[0] * factor_h1 + src_c_22[0] * factor_h1) * factor_v2); \
  dst[0] = tmp >> 16;                                                   \
  tmp = ((src_c_11[1] * factor_h1 + src_c_12[1] * factor_h1) * factor_v1 + \
         (src_c_21[1] * factor_h1 + src_c_22[1] * factor_h1) * factor_v2); \
  dst[1] = tmp >> 16;                                                   \
  tmp = ((src_c_11[2] * factor_h1 + src_c_12[2] * factor_h1) * factor_v1 + \
         (src_c_21[2] * factor_h1 + src_c_22[2] * factor_h1) * factor_v2); \
  dst[2] = tmp >> 16;                                                   \
  tmp = ((src_c_11[3] * factor_h1 + src_c_12[3] * factor_h1) * factor_v1 + \
         (src_c_21[3] * factor_h1 + src_c_22[3] * factor_h1) * factor_v2); \
  dst[3] = tmp >> 16;

static void scale_x_15_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
    
  SCALE_FUNC_TAIL
  }

static void scale_x_16_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_x_24_24_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_x_24_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_x_8_bilinear_c(gavl_video_scaler_t * s,
                               uint8_t * src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_x_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(2)
  
  SCALE_FUNC_TAIL
  }

static void scale_x_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(2)
  
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
  int i, imax;
  SCALE_FUNC_HEAD(1)
    
  SCALE_FUNC_TAIL
  }

static void scale_y_16_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_y_24_24_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_y_24_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_y_8_bilinear_c(gavl_video_scaler_t * s,
                               uint8_t * src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_y_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_y_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
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
  int i, imax;
  SCALE_FUNC_HEAD(1)
    
  SCALE_FUNC_TAIL
  }

static void scale_xy_16_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_24_24_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_24_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_8_bilinear_c(gavl_video_scaler_t * s,
                               uint8_t * src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

static void scale_xy_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i, imax;
  SCALE_FUNC_HEAD(1)
  
  SCALE_FUNC_TAIL
  }

void gavl_init_scale_funcs_c(gavl_scale_funcs_t * tab,
                             gavl_scale_mode_t scale_mode,
                             int scale_x, int scale_y)
  {
  switch(scale_mode)
    {
    case GAVL_SCALE_AUTO:
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

