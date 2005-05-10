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

#define UNPACK_RGB16(pixel,c) c[0]=RGB16_TO_R(pixel);c[1]=RGB16_TO_G(pixel);c[2]=RGB16_TO_B(pixel);
#define UNPACK_RGB15(pixel,c) c[0]=RGB15_TO_R(pixel);c[1]=RGB15_TO_G(pixel);c[2]=RGB15_TO_B(pixel);

#define SCALE_FUNC_HEAD(a) \
  for(i = 0; i < s->table[plane].num_coeffs_h; i++)       \
    {

#define SCALE_FUNC_TAIL \
    }

/* Packed formats for 15/16 colors (idea from libvisual) */

typedef struct {
	uint16_t b:5, g:6, r:5;
} color_16;

typedef struct {
	uint16_t b:5, g:5, r:5;
} color_15;

/* Nearest neighbor */

static void scale_16_16_nearest_c(gavl_video_scaler_t * s,
                                  uint8_t * _src,
                                  int src_stride,
                                  uint8_t * _dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  uint16_t * src, *dst;
  src = (uint16_t*)(_src + s->table[plane].coeffs_v[scanline].index * src_stride);
  dst = (uint16_t*)_dst;
  SCALE_FUNC_HEAD(1)
    dst[i] = src[s->table[plane].coeffs_h[i].index];
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
  src = _src + (s->table[plane].coeffs_v[scanline].index * src_stride);
  SCALE_FUNC_HEAD(1)
    src1 = src + s->table[plane].coeffs_h[i].index*3;
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

  //  fprintf(stderr, "scale_24_32_nearest_c\n");
  
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index * src_stride);
  SCALE_FUNC_HEAD(1)
    src1 = src + s->table[plane].coeffs_h[i].index*4;
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
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index * src_stride);
  SCALE_FUNC_HEAD(1)
    src1 = src + s->table[plane].coeffs_h[i].index*4;
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
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index * src_stride);

  SCALE_FUNC_HEAD(1)
    *dst = *(src + s->table[plane].coeffs_h[i].index);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_8_nearest_advance(gavl_video_scaler_t * s,
                                    uint8_t * _src,
                                    int src_stride,
                                    uint8_t * dst,
                                    int plane,
                                    int scanline, int advance)
  {
  int i;
  uint8_t * src;
  src = (uint8_t*)(_src + s->table[plane].coeffs_v[scanline].index * src_stride);

  SCALE_FUNC_HEAD(1)
    *dst = *(src + s->table[plane].coeffs_h[i].index * advance);
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
  scale_8_nearest_advance(s,
                  src,
                  src_stride,
                  dst,
                  0,
                  scanline, 2);

  scale_8_nearest_advance(s,
                  src+1,
                  src_stride,
                  dst+1,
                  1,
                  scanline, 4);

  scale_8_nearest_advance(s,
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
  scale_8_nearest_advance(s,
                  src+1,
                  src_stride,
                  dst+1,
                  0,
                  scanline, 2);

  scale_8_nearest_advance(s,
                  src,
                  src_stride,
                  dst,
                  1,
                  scanline, 4);

  scale_8_nearest_advance(s,
                  src+2,
                  src_stride,
                  dst+2,
                  1,
                  scanline, 4);

  }

/* Bilinear x */

#define BILINEAR_1D_1(src_c_1, src_c_2, factor_1, factor_2, dst_c) \
  tmp = src_c_1[0] * factor_1 + src_c_2[0] * factor_2;             \
  dst_c[0] = tmp >> 8;
  

#define BILINEAR_1D_3(src_c_1, src_c_2, factor_1, factor_2, dst_c) \
  tmp = src_c_1[0] * factor_1 + src_c_2[0] * factor_2;             \
  dst_c[0] = tmp >> 8;                                               \
  tmp = src_c_1[1] * factor_1 + src_c_2[1] * factor_2;             \
  dst_c[1] = tmp >> 8;                                               \
  tmp = src_c_1[2] * factor_1 + src_c_2[2] * factor_2;             \
  dst_c[2] = tmp >> 8;
  

#define BILINEAR_1D_4(src_c_1, src_c_2, factor_1, factor_2, dst_c) \
  tmp = src_c_1[0] * factor_1 + src_c_2[0] * factor_2;             \
  dst_c[0] = tmp >> 8;                                               \
  tmp = src_c_1[1] * factor_1 + src_c_2[1] * factor_2;             \
  dst_c[1] = tmp >> 8;                                               \
  tmp = src_c_1[2] * factor_1 + src_c_2[2] * factor_2;             \
  dst_c[2] = tmp >> 8;                                               \
  tmp = src_c_1[3] * factor_1 + src_c_2[3] * factor_2;             \
  dst_c[3] = tmp >> 8;

#define BILINEAR_1D_PACKED(src_c_1, src_c_2, factor_1, factor_2, dst_c) \
  tmp = src_c_1->r * factor_1 + src_c_2->r * factor_2;             \
  dst_c->r = tmp >> 8;                                               \
  tmp = src_c_1->g * factor_1 + src_c_2->g * factor_2;             \
  dst_c->g = tmp >> 8;                                               \
  tmp = src_c_1->b * factor_1 + src_c_2->b * factor_2;             \
  dst_c->b = tmp >> 8;

/*
 *  dst = ((src_c_11 * factor_h1 + src_c_12 * factor_h1) * factor_v1 +
 *         (src_c_21 * factor_h1 + src_c_22 * factor_h1) * factor_v2) >> 16
 */

#define BILINEAR_2D_1(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c) \
  tmp = ((src_c_11[0] * factor_h1 + src_c_12[0] * factor_h2) * factor_v1 + \
         (src_c_21[0] * factor_h1 + src_c_22[0] * factor_h2) * factor_v2); \
  dst_c[0] = tmp >> 16;

#define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c) \
  tmp = ((src_c_11[0] * factor_h1 + src_c_12[0] * factor_h2) * factor_v1 + \
         (src_c_21[0] * factor_h1 + src_c_22[0] * factor_h2) * factor_v2); \
  dst_c[0] = tmp >> 16;                                                   \
  tmp = ((src_c_11[1] * factor_h1 + src_c_12[1] * factor_h2) * factor_v1 + \
         (src_c_21[1] * factor_h1 + src_c_22[1] * factor_h2) * factor_v2); \
  dst_c[1] = tmp >> 16;                                                   \
  tmp = ((src_c_11[2] * factor_h1 + src_c_12[2] * factor_h2) * factor_v1 + \
         (src_c_21[2] * factor_h1 + src_c_22[2] * factor_h2) * factor_v2); \
  dst_c[2] = tmp >> 16;

#define BILINEAR_2D_4(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c) \
  tmp = ((src_c_11[0] * factor_h1 + src_c_12[0] * factor_h2) * factor_v1 + \
         (src_c_21[0] * factor_h1 + src_c_22[0] * factor_h2) * factor_v2); \
  dst_c[0] = tmp >> 16;                                                   \
  tmp = ((src_c_11[1] * factor_h1 + src_c_12[1] * factor_h2) * factor_v1 + \
         (src_c_21[1] * factor_h1 + src_c_22[1] * factor_h2) * factor_v2); \
  dst_c[1] = tmp >> 16;                                                   \
  tmp = ((src_c_11[2] * factor_h1 + src_c_12[2] * factor_h2) * factor_v1 + \
         (src_c_21[2] * factor_h1 + src_c_22[2] * factor_h2) * factor_v2); \
  dst_c[2] = tmp >> 16;                                                   \
  tmp = ((src_c_11[3] * factor_h1 + src_c_12[3] * factor_h2) * factor_v1 + \
         (src_c_21[3] * factor_h1 + src_c_22[3] * factor_h2) * factor_v2); \
  dst_c[3] = tmp >> 16;

#define BILINEAR_2D_PACKED(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c) \
  tmp = ((src_c_11->r * factor_h1 + src_c_12->r * factor_h2) * factor_v1 + \
         (src_c_21->r * factor_h1 + src_c_22->r * factor_h2) * factor_v2); \
  dst_c->r = tmp >> 16;                                                   \
  tmp = ((src_c_11->g * factor_h1 + src_c_12->g * factor_h2) * factor_v1 + \
         (src_c_21->g * factor_h1 + src_c_22->g * factor_h2) * factor_v2); \
  dst_c->g = tmp >> 16;                                                   \
  tmp = ((src_c_11->b * factor_h1 + src_c_12->b * factor_h2) * factor_v1 + \
         (src_c_21->b * factor_h1 + src_c_22->b * factor_h2) * factor_v2); \
  dst_c->b = tmp >> 16;


static void scale_x_15_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * _src,
                                   int src_stride,
                                   uint8_t * _dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  int tmp;
  color_15 * src;
  color_15 * dst;
  color_15 * src_1;
  color_15 * src_2;
  src = (color_15*)(_src + scanline * src_stride);
  dst = (color_15*)_dst;
  
  SCALE_FUNC_HEAD(1)
    src_1 = src + s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 1;
    BILINEAR_1D_PACKED(src_1, src_2, s->table[plane].coeffs_h[i].factor[0],
                       s->table[plane].coeffs_h[i].factor[1], dst);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_x_16_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * _src,
                                   int src_stride,
                                   uint8_t * _dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  int tmp;
  color_16 * src;
  color_16 * dst;
  color_16 * src_1;
  color_16 * src_2;
  src = (color_16*)(_src + scanline * src_stride);
  dst = (color_16*)_dst;
  
  SCALE_FUNC_HEAD(1)
    src_1 = src + s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 1;
    BILINEAR_1D_PACKED(src_1, src_2, s->table[plane].coeffs_h[i].factor[0],
                       s->table[plane].coeffs_h[i].factor[1], dst);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_x_24_24_bilinear_c(gavl_video_scaler_t * s,
                                     uint8_t * _src,
                                     int src_stride,
                                     uint8_t * dst,
                                     int plane,
                                     int scanline)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = _src + scanline * src_stride;
  
  SCALE_FUNC_HEAD(1)
    src_1 = src + 3 * s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 3;
    BILINEAR_1D_3(src_1, src_2, s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst += 3;
  SCALE_FUNC_TAIL
  }

static void scale_x_24_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * _src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(1)
    src_1 = src + 4 * s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 4;
    BILINEAR_1D_3(src_1, src_2, s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst += 4;
  
  SCALE_FUNC_TAIL
  }

static void scale_x_32_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * _src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(1)
    src_1 = src + 4 * s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 4;
    BILINEAR_1D_4(src_1, src_2, s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst += 4;
  
  SCALE_FUNC_TAIL
  }

static void scale_x_8_bilinear_c(gavl_video_scaler_t * s,
                               uint8_t * _src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(1)
    src_1 = src + s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 1;
    BILINEAR_1D_1(src_1, src_2, s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_x_8_bilinear_advance(gavl_video_scaler_t * s,
                                       uint8_t * _src,
                                       int src_stride,
                                       uint8_t * dst,
                                       int plane,
                                       int scanline,
                                       int advance)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(1)
    src_1 = src + advance * s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + advance;
    BILINEAR_1D_1(src_1, src_2, s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst+=advance;
  SCALE_FUNC_TAIL
  }


static void scale_x_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  scale_x_8_bilinear_advance(s,
                             src,
                             src_stride,
                             dst,
                             0,
                             scanline, 2);
  
  scale_x_8_bilinear_advance(s,
                             src+1,
                             src_stride,
                             dst+1,
                             1,
                             scanline, 4);
  
  scale_x_8_bilinear_advance(s,
                             src+3,
                             src_stride,
                             dst+3,
                             1,
                             scanline, 4);
  }

static void scale_x_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  scale_x_8_bilinear_advance(s,
                             src+1,
                             src_stride,
                             dst+1,
                             0,
                             scanline, 2);

  scale_x_8_bilinear_advance(s,
                             src,
                             src_stride,
                             dst,
                             1,
                             scanline, 4);

  scale_x_8_bilinear_advance(s,
                             src+2,
                             src_stride,
                             dst+2,
                             1,
                             scanline, 4);

  }

/* Bilinear y */

static void scale_y_15_16_bilinear_c(gavl_video_scaler_t * s,
                                     uint8_t * src,
                                     int src_stride,
                                     uint8_t * _dst,
                                     int plane,
                                     int scanline)
  {
  int i;
  int tmp;
  color_15 * dst;
  color_15 * src_1;
  color_15 * src_2;

  src_1 = (color_15*)(src + s->table[plane].coeffs_v[scanline].index * src_stride);
  src_2 = (color_15*)((uint8_t*)src_1 + src_stride);

  dst = (color_15*)_dst;
  
  SCALE_FUNC_HEAD(1)
    BILINEAR_1D_PACKED(src_1, src_2, s->table[plane].coeffs_v[scanline].factor[0],
                       s->table[plane].coeffs_v[scanline].factor[1], dst);
    src_1++;
    src_2++;
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_y_16_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * _dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  int tmp;
  color_16 * dst;
  color_16 * src_1;
  color_16 * src_2;

  src_1 = (color_16*)(src + s->table[plane].coeffs_v[scanline].index * src_stride);
  src_2 = (color_16*)((uint8_t*)src_1 + src_stride);

  dst = (color_16*)_dst;
  
  SCALE_FUNC_HEAD(1)
    BILINEAR_1D_PACKED(src_1, src_2, s->table[plane].coeffs_v[scanline].factor[0],
                       s->table[plane].coeffs_v[scanline].factor[1], dst);
    src_1++;
    src_2++;
    dst++;
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
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_2 = src_1 + src_stride;
  
  SCALE_FUNC_HEAD(1)
    BILINEAR_1D_3(src_1, src_2, s->table[plane].coeffs_v[scanline].factor[0],
                  s->table[plane].coeffs_v[scanline].factor[1], dst);
    src_1 += 3;
    src_2 += 3;
    dst += 3;
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
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_2 = src_1 + src_stride;
  
  SCALE_FUNC_HEAD(1)
    BILINEAR_1D_3(src_1, src_2, s->table[plane].coeffs_v[scanline].factor[0],
                  s->table[plane].coeffs_v[scanline].factor[1], dst);
    src_1 += 4;
    src_2 += 4;
    dst += 4;
  SCALE_FUNC_TAIL
  }

static void scale_y_32_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_2 = src_1 + src_stride;
  
  SCALE_FUNC_HEAD(1)
    BILINEAR_1D_4(src_1, src_2, s->table[plane].coeffs_v[scanline].factor[0],
                  s->table[plane].coeffs_v[scanline].factor[1], dst);
    src_1 += 4;
    src_2 += 4;
    dst += 4;
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
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_2 = src_1 + src_stride;
  
  SCALE_FUNC_HEAD(1)
    BILINEAR_1D_1(src_1, src_2, s->table[plane].coeffs_v[scanline].factor[0],
                  s->table[plane].coeffs_v[scanline].factor[1], dst);
    src_1++;
    src_2++;
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_y_8_bilinear_advance(gavl_video_scaler_t * s,
                                       uint8_t * src,
                                       int src_stride,
                                       uint8_t * dst,
                                       int plane,
                                       int scanline, int advance)
  {
  int i;
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_2 = src_1 + src_stride;
  
  SCALE_FUNC_HEAD(1)
    BILINEAR_1D_1(src_1, src_2, s->table[plane].coeffs_v[scanline].factor[0],
                  s->table[plane].coeffs_v[scanline].factor[1], dst);
    src_1 += advance;
    src_2 += advance;
    dst+=advance;
  SCALE_FUNC_TAIL
  }


static void scale_y_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  scale_y_8_bilinear_advance(s,
                             src,
                             src_stride,
                             dst,
                             0,
                             scanline, 2);
  
  scale_y_8_bilinear_advance(s,
                             src+1,
                             src_stride,
                             dst+1,
                             1,
                             scanline, 4);
  
  scale_y_8_bilinear_advance(s,
                             src+3,
                             src_stride,
                             dst+3,
                             1,
                             scanline, 4);
  }

static void scale_y_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  scale_y_8_bilinear_advance(s,
                             src+1,
                             src_stride,
                             dst+1,
                             0,
                             scanline, 2);

  scale_y_8_bilinear_advance(s,
                             src,
                             src_stride,
                             dst,
                             1,
                             scanline, 4);

  scale_y_8_bilinear_advance(s,
                             src+2,
                             src_stride,
                             dst+2,
                             1,
                             scanline, 4);
  
  }

/* Bilinear x and y */

static void scale_xy_15_16_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * _dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  int tmp;
  color_15 * src_start_1;
  color_15 * src_start_2;
  color_15 * src_11;
  color_15 * src_12;
  color_15 * src_21;
  color_15 * src_22;
  color_15 * dst;
  int factor_v1;
  int factor_v2;
  
  src_start_1 = (color_15*)(src + s->table[plane].coeffs_v[scanline].index * src_stride);
  src_start_2 = (color_15*)((uint8_t*)src_start_1 + src_stride);
  dst = (color_15*)(_dst);

  factor_v1 = s->table[plane].coeffs_v[scanline].factor[0];
  factor_v2 = s->table[plane].coeffs_v[scanline].factor[1];
  
  SCALE_FUNC_HEAD(1)
    src_11 = src_start_1 + s->table[plane].coeffs_h[i].index;
    src_12 = src_11 + 1;

    src_21 = src_start_2 + s->table[plane].coeffs_h[i].index;
    src_22 = src_21 + 1;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_PACKED(src_11, src_12, src_21, src_22,
                       s->table[plane].coeffs_h[i].factor[0],
                       s->table[plane].coeffs_h[i].factor[1], dst);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_xy_16_16_bilinear_c(gavl_video_scaler_t * s,
                                      uint8_t * src,
                                      int src_stride,
                                      uint8_t * _dst,
                                      int plane,
                                      int scanline)
  {
  int i;
  int tmp;
  color_16 * src_start_1;
  color_16 * src_start_2;
  color_16 * src_11;
  color_16 * src_12;
  color_16 * src_21;
  color_16 * src_22;
  color_16 * dst;
  int factor_v1;
  int factor_v2;
  
  src_start_1 = (color_16*)(src + s->table[plane].coeffs_v[scanline].index * src_stride);
  src_start_2 = (color_16*)((uint8_t*)src_start_1 + src_stride);

  dst = (color_16*)(_dst);

  factor_v1 = s->table[plane].coeffs_v[scanline].factor[0];
  factor_v2 = s->table[plane].coeffs_v[scanline].factor[1];
  
  SCALE_FUNC_HEAD(1)
    src_11 = src_start_1 + s->table[plane].coeffs_h[i].index;
    src_12 = src_11 + 1;

    src_21 = src_start_2 + s->table[plane].coeffs_h[i].index;
    src_22 = src_21 + 1;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_PACKED(src_11, src_12, src_21, src_22,
                       s->table[plane].coeffs_h[i].factor[0],
                       s->table[plane].coeffs_h[i].factor[1], dst);
    dst++;
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
  int tmp;
  uint8_t * src_start_1;
  uint8_t * src_start_2;
  uint8_t * src_11;
  uint8_t * src_12;
  uint8_t * src_21;
  uint8_t * src_22;
  int factor_v1;
  int factor_v2;
  
  src_start_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = s->table[plane].coeffs_v[scanline].factor[0];
  factor_v2 = s->table[plane].coeffs_v[scanline].factor[1];
  
  SCALE_FUNC_HEAD(1)
    src_11 = src_start_1 + 3 * s->table[plane].coeffs_h[i].index;
    src_12 = src_11 + 3;

    src_21 = src_start_2 + 3 * s->table[plane].coeffs_h[i].index;
    src_22 = src_21 + 3;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_3(src_11, src_12, src_21, src_22,
                  s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst += 3;
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
  int tmp;
  uint8_t * src_start_1;
  uint8_t * src_start_2;
  uint8_t * src_11;
  uint8_t * src_12;
  uint8_t * src_21;
  uint8_t * src_22;
  int factor_v1;
  int factor_v2;
  
  src_start_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = s->table[plane].coeffs_v[scanline].factor[0];
  factor_v2 = s->table[plane].coeffs_v[scanline].factor[1];
  
  SCALE_FUNC_HEAD(1)
    src_11 = src_start_1 + 4 * s->table[plane].coeffs_h[i].index;
    src_12 = src_11 + 4;

    src_21 = src_start_2 + 4 * s->table[plane].coeffs_h[i].index;
    src_22 = src_21 + 4;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_3(src_11, src_12, src_21, src_22,
                  s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst += 4;
  SCALE_FUNC_TAIL
  }

static void scale_xy_32_32_bilinear_c(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  int tmp;
  uint8_t * src_start_1;
  uint8_t * src_start_2;
  uint8_t * src_11;
  uint8_t * src_12;
  uint8_t * src_21;
  uint8_t * src_22;
  int factor_v1;
  int factor_v2;
  
  src_start_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = s->table[plane].coeffs_v[scanline].factor[0];
  factor_v2 = s->table[plane].coeffs_v[scanline].factor[1];
  
  SCALE_FUNC_HEAD(1)
    src_11 = src_start_1 + 4 * s->table[plane].coeffs_h[i].index;
    src_12 = src_11 + 4;

    src_21 = src_start_2 + 4 * s->table[plane].coeffs_h[i].index;
    src_22 = src_21 + 4;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_4(src_11, src_12, src_21, src_22,
                  s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst += 4;
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
  int tmp;
  uint8_t * src_start_1;
  uint8_t * src_start_2;
  uint8_t * src_11;
  uint8_t * src_12;
  uint8_t * src_21;
  uint8_t * src_22;
  int factor_v1;
  int factor_v2;
  
  src_start_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = s->table[plane].coeffs_v[scanline].factor[0];
  factor_v2 = s->table[plane].coeffs_v[scanline].factor[1];
  
  SCALE_FUNC_HEAD(1)
    src_11 = src_start_1 + s->table[plane].coeffs_h[i].index;
    src_12 = src_11 + 1;

    src_21 = src_start_2 + s->table[plane].coeffs_h[i].index;
    src_22 = src_21 + 1;
    
    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_1(src_11, src_12, src_21, src_22,
                  s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst++;
  SCALE_FUNC_TAIL
  
  }

static void scale_xy_8_bilinear_advance(gavl_video_scaler_t * s,
                                        uint8_t * src,
                                        int src_stride,
                                        uint8_t * dst,
                                        int plane,
                                        int scanline,
                                        int advance)
  {
  int i;
  int tmp;
  uint8_t * src_start_1;
  uint8_t * src_start_2;
  uint8_t * src_11;
  uint8_t * src_12;
  uint8_t * src_21;
  uint8_t * src_22;
  int factor_v1;
  int factor_v2;
  //  fprintf(stderr, "Plane: %d\n", plane);

  src_start_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = s->table[plane].coeffs_v[scanline].factor[0];
  factor_v2 = s->table[plane].coeffs_v[scanline].factor[1];
  
  SCALE_FUNC_HEAD(1)
    src_11 = src_start_1 + advance * s->table[plane].coeffs_h[i].index;
    src_12 = src_11 + advance;

    src_21 = src_start_2 + advance * s->table[plane].coeffs_h[i].index;
    src_22 = src_21 + advance;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_1(src_11, src_12, src_21, src_22,
                  s->table[plane].coeffs_h[i].factor[0],
                  s->table[plane].coeffs_h[i].factor[1], dst);
    dst+=advance;
  SCALE_FUNC_TAIL
  
  }

static void scale_xy_yuy2_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  scale_xy_8_bilinear_advance(s,
                             src,
                             src_stride,
                             dst,
                             0,
                             scanline, 2);
  
  scale_xy_8_bilinear_advance(s,
                             src+1,
                             src_stride,
                             dst+1,
                             1,
                             scanline, 4);
  
  scale_xy_8_bilinear_advance(s,
                             src+3,
                             src_stride,
                             dst+3,
                             1,
                             scanline, 4);
  }

static void scale_xy_uyvy_bilinear_c(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  scale_xy_8_bilinear_advance(s,
                             src+1,
                             src_stride,
                             dst+1,
                             0,
                             scanline, 2);

  scale_xy_8_bilinear_advance(s,
                             src,
                             src_stride,
                             dst,
                             1,
                             scanline, 4);

  scale_xy_8_bilinear_advance(s,
                             src+2,
                             src_stride,
                             dst+2,
                             1,
                             scanline, 4);

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
        tab->scale_32_32 = scale_xy_32_32_bilinear_c;
        
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
        tab->scale_32_32 = scale_x_32_32_bilinear_c;
        
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
        tab->scale_32_32 = scale_y_32_32_bilinear_c;
        
        tab->scale_8     = scale_y_8_bilinear_c;
        tab->scale_yuy2  = scale_y_yuy2_bilinear_c;
        tab->scale_uyvy  = scale_y_uyvy_bilinear_c;
        }
      break;
    case GAVL_SCALE_NONE:
      break;
    }
  }

