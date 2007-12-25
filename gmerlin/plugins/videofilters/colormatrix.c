#include <stdlib.h>
#include <stdio.h>

#include <config.h>

#include <gavl/gavl.h>
#include <colormatrix.h>
#include <translation.h>

#include <log.h>
#define LOG_DOMAIN "colormatrix"

typedef struct
  {
  float coeffs_f[4][5];
  int   coeffs_i[4][5];
  } matrix_t;

#if 0
static void dump_matrix(float coeffs_f[4][5])
  {
  int i;
  fprintf(stderr, "Matrix\n");
  for(i = 0; i < 4; i++)
    {
    fprintf(stderr, "%f %f %f %f %f\n",
            coeffs_f[i][0],
            coeffs_f[i][1],
            coeffs_f[i][2],
            coeffs_f[i][3],
            coeffs_f[i][4]);
    }
  }
#endif

struct bg_colormatrix_s
  {
  matrix_t rgba;
  matrix_t yuva;
  
  void (*func)(bg_colormatrix_t * m,
               const gavl_video_frame_t * in,
               const gavl_video_frame_t * out);
  gavl_video_format_t format;
  };

static void matrixmult(float coeffs1[4][5],
                       float coeffs2[4][5],
                       float result[4][5])
  {
  int i, j;

  for(i = 0; i < 4; i++)
    {
    for(j = 0; j < 5; j++)
      {
      result[i][j] = 
        coeffs1[i][0] * coeffs2[0][j] +
        coeffs1[i][1] * coeffs2[1][j] +
        coeffs1[i][2] * coeffs2[2][j] +
        coeffs1[i][3] * coeffs2[3][j];
      }
    result[i][4] += coeffs1[i][4];
    }
  }

static float rgba_2_yuva[4][5] =
  {
    /*       ry         gy         by   ay   oy */
    {  0.299000,  0.587000,  0.114000, 0.0, 0.0 },
    /*       ru         gu         bu   au   ou */
    { -0.168736, -0.331264,  0.500000, 0.0, 0.0 },
    /*       rv         gv         bv   av   ov */
    {  0.500000, -0.418688, -0.081312, 0.0, 0.0 },
    /*       ra         ga         ba   aa   oa */
    {       0.0,       0.0,       0.0, 1.0, 0.0 },
  };


static float yuva_2_rgba[4][5] =
  {
    /* yr         ur         vr   ar   or */
    { 1.0,  0.000000,  1.402000, 0.0, 0.0 },
    /* yg         ug         vg   ag   og */
    { 1.0, -0.344136, -0.714136, 0.0, 0.0},
    /* yb         ub         vb   ab   ob */
    { 1.0,  1.772000,  0.000000, 0.0, 0.0},
    /* ya         ua         va   aa   oa */
    { 0.0,  0.000000,  0.000000, 1.0, 0.0 },
  };

static void colormatrix_rgb2yuv(float coeffs_in[4][5],
                                float coeffs_out[4][5])
  {
  float coeffs_tmp[4][5];
  matrixmult(rgba_2_yuva, coeffs_in, coeffs_tmp);
  matrixmult(coeffs_tmp, yuva_2_rgba, coeffs_out);
  }

static void colormatrix_yuv2rgb(float coeffs_in[4][5],
                                float coeffs_out[4][5])
  {
  float coeffs_tmp[4][5];
  matrixmult(yuva_2_rgba,  coeffs_in, coeffs_tmp);
  matrixmult(coeffs_tmp, rgba_2_yuva, coeffs_out);
  }

static void colormatrix_set_4(float coeffs_in[4][5],
                              float coeffs_out[4][5])
  {
  int i, j;
  for(i = 0; i < 4; i++)
    {
    for(j = 0; j < 5; j++)
      coeffs_out[i][j] = coeffs_in[i][j];
    }
  }

static void colormatrix_set_3(float coeffs_in[3][4],
                              float coeffs_out[4][5])
  {
  int i, j;
  for(i = 0; i < 3; i++)
    {
    for(j = 0; j < 3; j++)
      coeffs_out[i][j] = coeffs_in[i][j];
    
    coeffs_out[i][3] = 0.0;
    coeffs_out[i][4] = coeffs_in[i][3];
    }
  coeffs_out[3][0] = 0.0;
  coeffs_out[3][1] = 0.0;
  coeffs_out[3][2] = 0.0;
  coeffs_out[3][3] = 1.0;
  }

bg_colormatrix_t * bg_colormatrix_create()
  {
  bg_colormatrix_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  return ret;
  }

void bg_colormatrix_destroy(bg_colormatrix_t * m)
  {
  free(m);
  }

/* */


static void process_bgr_24(bg_colormatrix_t * m,
                           const gavl_video_frame_t * in,
                           const gavl_video_frame_t * out)
  {
  int tmp;
  int i, j;
  uint8_t * src;
  uint8_t * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = in->planes[0]  + i * in->strides[0];
    dst = out->planes[0] + i * out->strides[0];

    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        ((m->rgba.coeffs_i[0][0] * src[2] +
          m->rgba.coeffs_i[0][1] * src[1] + 
          m->rgba.coeffs_i[0][2] * src[0]) >> 8) +
        m->rgba.coeffs_i[0][4];
      dst[2] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[1][0] * src[2] +
          m->rgba.coeffs_i[1][1] * src[1] + 
          m->rgba.coeffs_i[1][2] * src[0]) >> 8) +
        m->rgba.coeffs_i[1][4];
      dst[1] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[2][0] * src[2] +
          m->rgba.coeffs_i[2][1] * src[1] + 
          m->rgba.coeffs_i[2][2] * src[0]) >> 8) +
        m->rgba.coeffs_i[2][4];
      dst[0] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      src += 3;
      dst += 3;
      }
    }
  }


static void process_rgb_32(bg_colormatrix_t * m,
                           const gavl_video_frame_t * in,
                           const gavl_video_frame_t * out)
  {
  int tmp;
  int i, j;
  uint8_t * src;
  uint8_t * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = in->planes[0]  + i * in->strides[0];
    dst = out->planes[0] + i * out->strides[0];

    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        ((m->rgba.coeffs_i[0][0] * src[0] +
          m->rgba.coeffs_i[0][1] * src[1] + 
          m->rgba.coeffs_i[0][2] * src[2]) >> 8) +
        m->rgba.coeffs_i[0][4];
      dst[0] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[1][0] * src[0] +
          m->rgba.coeffs_i[1][1] * src[1] + 
          m->rgba.coeffs_i[1][2] * src[2]) >> 8) +
        m->rgba.coeffs_i[1][4];
      dst[1] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[2][0] * src[0] +
          m->rgba.coeffs_i[2][1] * src[1] + 
          m->rgba.coeffs_i[2][2] * src[2]) >> 8) +
        m->rgba.coeffs_i[2][4];
      dst[2] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      src += 4;
      dst += 4;
      }
    }
  }

static void process_bgr_32(bg_colormatrix_t * m,
                           const gavl_video_frame_t * in,
                           const gavl_video_frame_t * out)
  {
  int tmp;
  int i, j;
  uint8_t * src;
  uint8_t * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = in->planes[0]  + i * in->strides[0];
    dst = out->planes[0] + i * out->strides[0];

    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        ((m->rgba.coeffs_i[0][0] * src[2] +
          m->rgba.coeffs_i[0][1] * src[1] + 
          m->rgba.coeffs_i[0][2] * src[0]) >> 8) +
        m->rgba.coeffs_i[0][4];
      dst[2] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[1][0] * src[2] +
          m->rgba.coeffs_i[1][1] * src[1] + 
          m->rgba.coeffs_i[1][2] * src[0]) >> 8) +
        m->rgba.coeffs_i[1][4];
      dst[1] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[2][0] * src[2] +
          m->rgba.coeffs_i[2][1] * src[1] + 
          m->rgba.coeffs_i[2][2] * src[0]) >> 8) +
        m->rgba.coeffs_i[2][4];
      dst[0] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      src += 4;
      dst += 4;
      }
    }
  }

/* RGB(A) 8 bit */

static void process_rgb_24(bg_colormatrix_t * m,
                           const gavl_video_frame_t * in,
                           const gavl_video_frame_t * out)
  {
  int tmp;
  int i, j;
  uint8_t * src;
  uint8_t * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = in->planes[0]  + i * in->strides[0];
    dst = out->planes[0] + i * out->strides[0];

    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        ((m->rgba.coeffs_i[0][0] * src[0] +
          m->rgba.coeffs_i[0][1] * src[1] + 
          m->rgba.coeffs_i[0][2] * src[2]) >> 8) +
        m->rgba.coeffs_i[0][4];
      dst[0] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[1][0] * src[0] +
          m->rgba.coeffs_i[1][1] * src[1] + 
          m->rgba.coeffs_i[1][2] * src[2]) >> 8) +
        m->rgba.coeffs_i[1][4];
      dst[1] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[2][0] * src[0] +
          m->rgba.coeffs_i[2][1] * src[1] + 
          m->rgba.coeffs_i[2][2] * src[2]) >> 8) +
        m->rgba.coeffs_i[2][4];
      dst[2] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      src += 3;
      dst += 3;
      }
    }
  }


static void process_rgba_32(bg_colormatrix_t * m,
                            const gavl_video_frame_t * in,
                            const gavl_video_frame_t * out)
  {
  int tmp;
  int i, j;
  uint8_t * src;
  uint8_t * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = in->planes[0]  + i * in->strides[0];
    dst = out->planes[0] + i * out->strides[0];

    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        ((m->rgba.coeffs_i[0][0] * src[0] +
          m->rgba.coeffs_i[0][1] * src[1] + 
          m->rgba.coeffs_i[0][2] * src[2] + 
          m->rgba.coeffs_i[0][3] * src[3]) >> 8) +
        m->rgba.coeffs_i[0][4];
      dst[0] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[1][0] * src[0] +
          m->rgba.coeffs_i[1][1] * src[1] + 
          m->rgba.coeffs_i[1][2] * src[2] + 
          m->rgba.coeffs_i[1][3] * src[3]) >> 8) +
        m->rgba.coeffs_i[1][4];
      dst[1] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->rgba.coeffs_i[2][0] * src[0] +
          m->rgba.coeffs_i[2][1] * src[1] + 
          m->rgba.coeffs_i[2][2] * src[2] + 
          m->rgba.coeffs_i[2][3] * src[3]) >> 8) +
        m->rgba.coeffs_i[2][4];
      dst[2] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      tmp =
        ((m->rgba.coeffs_i[3][0] * src[0] +
          m->rgba.coeffs_i[3][1] * src[1] + 
          m->rgba.coeffs_i[3][2] * src[2] + 
          m->rgba.coeffs_i[3][3] * src[3]) >> 8) +
        m->rgba.coeffs_i[3][4];
      dst[3] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      src += 4;
      dst += 4;
      }
    }
  
  }

/* RGB(A) 16 bit */

static void process_rgb_48(bg_colormatrix_t * m,
                           const gavl_video_frame_t * in,
                           const gavl_video_frame_t * out)
  {
  int64_t tmp;
  int i, j;
  uint16_t * src;
  uint16_t * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = (uint16_t *)(in->planes[0]  + i * in->strides[0]);
    dst = (uint16_t *)(out->planes[0] + i * out->strides[0]);
    
    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        (((int64_t)m->rgba.coeffs_i[0][0] * (int64_t)src[0] +
          (int64_t)m->rgba.coeffs_i[0][1] * (int64_t)src[1] + 
          (int64_t)m->rgba.coeffs_i[0][2] * (int64_t)src[2]) >> 16) +
        (int64_t)m->rgba.coeffs_i[0][4];
      dst[0] = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      tmp =
        (((int64_t)m->rgba.coeffs_i[1][0] * (int64_t)src[0] +
          (int64_t)m->rgba.coeffs_i[1][1] * (int64_t)src[1] + 
          (int64_t)m->rgba.coeffs_i[1][2] * (int64_t)src[2]) >> 16) +
        (int64_t)m->rgba.coeffs_i[1][4];
      dst[1] = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      tmp =
        (((int64_t)m->rgba.coeffs_i[2][0] * (int64_t)src[0] +
          (int64_t)m->rgba.coeffs_i[2][1] * (int64_t)src[1] + 
          (int64_t)m->rgba.coeffs_i[2][2] * (int64_t)src[2]) >> 16) +
        (int64_t)m->rgba.coeffs_i[2][4];
      dst[2] = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      src += 3;
      dst += 3;
      }
    }
  }


static void process_rgba_64(bg_colormatrix_t * m,
                            const gavl_video_frame_t * in,
                            const gavl_video_frame_t * out)
  {
  int64_t tmp;
  int i, j;
  uint16_t * src;
  uint16_t * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = (uint16_t *)(in->planes[0]  + i * in->strides[0]);
    dst = (uint16_t *)(out->planes[0] + i * out->strides[0]);
    
    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        (((int64_t)m->rgba.coeffs_i[0][0] * (int64_t)src[0] +
          (int64_t)m->rgba.coeffs_i[0][1] * (int64_t)src[1] + 
          (int64_t)m->rgba.coeffs_i[0][2] * (int64_t)src[2] + 
          (int64_t)m->rgba.coeffs_i[0][3] * (int64_t)src[3]) >> 16) +
        (int64_t)m->rgba.coeffs_i[0][4];
      dst[0] = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      tmp =
        (((int64_t)m->rgba.coeffs_i[1][0] * (int64_t)src[0] +
          (int64_t)m->rgba.coeffs_i[1][1] * (int64_t)src[1] + 
          (int64_t)m->rgba.coeffs_i[1][2] * (int64_t)src[2] + 
          (int64_t)m->rgba.coeffs_i[1][3] * (int64_t)src[3]) >> 16) +
        (int64_t)m->rgba.coeffs_i[1][4];
      dst[1] = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      tmp =
        (((int64_t)m->rgba.coeffs_i[2][0] * (int64_t)src[0] +
          (int64_t)m->rgba.coeffs_i[2][1] * (int64_t)src[1] + 
          (int64_t)m->rgba.coeffs_i[2][2] * (int64_t)src[2] + 
          (int64_t)m->rgba.coeffs_i[2][3] * (int64_t)src[3]) >> 16) +
        (int64_t)m->rgba.coeffs_i[2][4];
      dst[2] = (uint16_t)((tmp & ~0xFFff)?((-tmp) >> 63) : tmp);

      tmp =
        (((int64_t)m->rgba.coeffs_i[3][0] * (int64_t)src[0] +
          (int64_t)m->rgba.coeffs_i[3][1] * (int64_t)src[1] + 
          (int64_t)m->rgba.coeffs_i[3][2] * (int64_t)src[2] + 
          (int64_t)m->rgba.coeffs_i[3][3] * (int64_t)src[3]) >> 16) +
        (int64_t)m->rgba.coeffs_i[3][4];
      dst[3] = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      src += 4;
      dst += 4;
      }
    }
  
  }


/* Float */

static void process_rgb_float(bg_colormatrix_t * m,
                              const gavl_video_frame_t * in,
                              const gavl_video_frame_t * out)
  {
  int64_t tmp;
  int i, j;
  float * src;
  float * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = (float *)(in->planes[0]  + i * in->strides[0]);
    dst = (float *)(out->planes[0] + i * out->strides[0]);
    
    for(j = 0; j < m->format.image_width; j++)
      {
      dst[0] =
        m->rgba.coeffs_f[0][0] * src[0] +
        m->rgba.coeffs_f[0][1] * src[1] + 
        m->rgba.coeffs_f[0][2] * src[2] +
        m->rgba.coeffs_f[0][4];
      dst[1] =
        m->rgba.coeffs_f[1][0] * src[0] +
        m->rgba.coeffs_f[1][1] * src[1] + 
        m->rgba.coeffs_f[1][2] * src[2] +
        m->rgba.coeffs_f[1][4];
      dst[2] =
        m->rgba.coeffs_f[2][0] * src[0] +
        m->rgba.coeffs_f[2][1] * src[1] + 
        m->rgba.coeffs_f[2][2] * src[2] +
        m->rgba.coeffs_f[2][4];
      
      src += 3;
      dst += 3;
      }
    }
  }


static void process_rgba_float(bg_colormatrix_t * m,
                               const gavl_video_frame_t * in,
                               const gavl_video_frame_t * out)
  {
  int64_t tmp;
  int i, j;
  float * src;
  float * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = (float *)(in->planes[0]  + i * in->strides[0]);
    dst = (float *)(out->planes[0] + i * out->strides[0]);
    
    for(j = 0; j < m->format.image_width; j++)
      {
      dst[0] =
        m->rgba.coeffs_f[0][0] * src[0] +
        m->rgba.coeffs_f[0][1] * src[1] + 
        m->rgba.coeffs_f[0][2] * src[2] +
        m->rgba.coeffs_f[0][3] * src[3] +
        m->rgba.coeffs_f[0][4];
      dst[1] =
        m->rgba.coeffs_f[1][0] * src[0] +
        m->rgba.coeffs_f[1][1] * src[1] + 
        m->rgba.coeffs_f[1][2] * src[2] +
        m->rgba.coeffs_f[1][3] * src[3] +
        m->rgba.coeffs_f[1][4];
      dst[2] =
        m->rgba.coeffs_f[2][0] * src[0] +
        m->rgba.coeffs_f[2][1] * src[1] + 
        m->rgba.coeffs_f[2][2] * src[2] +
        m->rgba.coeffs_f[2][3] * src[3] +
        m->rgba.coeffs_f[2][4];
      dst[3] =
        m->rgba.coeffs_f[3][0] * src[0] +
        m->rgba.coeffs_f[3][1] * src[1] + 
        m->rgba.coeffs_f[3][2] * src[2] +
        m->rgba.coeffs_f[3][3] * src[3] +
        m->rgba.coeffs_f[3][4];
      
      src += 4;
      dst += 4;
      }
    }
  }




static void process_yuva_32(bg_colormatrix_t * m,
                            const gavl_video_frame_t * in,
                            const gavl_video_frame_t * out)
  {
  int tmp;
  int i, j;
  uint8_t * src;
  uint8_t * dst;
  for(i = 0; i < m->format.image_height; i++)
    {
    src = in->planes[0]  + i * in->strides[0];
    dst = out->planes[0] + i * out->strides[0];

    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        ((m->yuva.coeffs_i[0][0] * (src[0] - 0x10) +
          m->yuva.coeffs_i[0][1] * (src[1] - 0x80) + 
          m->yuva.coeffs_i[0][2] * (src[2] - 0x80) + 
          m->yuva.coeffs_i[0][3] * src[3]) >> 8) +
        m->yuva.coeffs_i[0][4];
      tmp += 0x10;
      dst[0] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->yuva.coeffs_i[1][0] * (src[0] - 0x10) +
          m->yuva.coeffs_i[1][1] * (src[1] - 0x80) + 
          m->yuva.coeffs_i[1][2] * (src[2] - 0x80) + 
          m->yuva.coeffs_i[1][3] * src[3]) >> 8) +
        m->yuva.coeffs_i[1][4];
      tmp += 0x80;
      dst[1] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->yuva.coeffs_i[2][0] * (src[0] - 0x10) +
          m->yuva.coeffs_i[2][1] * (src[1] - 0x80) + 
          m->yuva.coeffs_i[2][2] * (src[2] - 0x80) + 
          m->yuva.coeffs_i[2][3] * src[3]) >> 8) +
        m->yuva.coeffs_i[2][4];
      tmp += 0x80;
      dst[2] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      tmp =
        ((m->yuva.coeffs_i[3][0] * (src[0] - 0x10) +
          m->yuva.coeffs_i[3][1] * (src[1] - 0x80) + 
          m->yuva.coeffs_i[3][2] * (src[2] - 0x80) + 
          m->yuva.coeffs_i[3][3] * src[3]) >> 8) +
        m->yuva.coeffs_i[3][4];
      dst[3] = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      src += 4;
      dst += 4;
      }
    }
  
  }


static void process_444j(bg_colormatrix_t * m,
                         const gavl_video_frame_t * in,
                         const gavl_video_frame_t * out)
  {
  int tmp;
  int i, j;
  uint8_t * src_y, * src_u, *src_v;
  uint8_t * dst_y, * dst_u, *dst_v;
  
  for(i = 0; i < m->format.image_height; i++)
    {
    src_y = in->planes[0]  + i * in->strides[0];
    src_u = in->planes[1]  + i * in->strides[1];
    src_v = in->planes[2]  + i * in->strides[2];

    dst_y = out->planes[0] + i * out->strides[0];
    dst_u = out->planes[1] + i * out->strides[1];
    dst_v = out->planes[2] + i * out->strides[2];

    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        ((m->yuva.coeffs_i[0][0] * *src_y +
          m->yuva.coeffs_i[0][1] * (*src_u-0x80) + 
          m->yuva.coeffs_i[0][2] * (*src_v-0x80)) >> 8) +
        m->yuva.coeffs_i[0][4];
      *dst_y = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      tmp =
        ((m->yuva.coeffs_i[1][0] * *src_y +
          m->yuva.coeffs_i[1][1] * (*src_u-0x80) + 
          m->yuva.coeffs_i[1][2] * (*src_v-0x80)) >> 8) +
        m->yuva.coeffs_i[1][4];
      tmp += 0x80;
      *dst_u = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->yuva.coeffs_i[2][0] * *src_y +
          m->yuva.coeffs_i[2][1] * (*src_u-0x80) + 
          m->yuva.coeffs_i[2][2] * (*src_v-0x80)) >> 8) +
        m->yuva.coeffs_i[2][4];
      tmp += 0x80;
      *dst_v = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      src_y++;
      src_u++;
      src_v++;

      dst_y++;
      dst_u++;
      dst_v++;
      }
    }
  }

static void process_444(bg_colormatrix_t * m,
                        const gavl_video_frame_t * in,
                        const gavl_video_frame_t * out)
  {
  int tmp;
  int i, j;
  uint8_t * src_y, * src_u, *src_v;
  uint8_t * dst_y, * dst_u, *dst_v;
  
  for(i = 0; i < m->format.image_height; i++)
    {
    src_y = in->planes[0]  + i * in->strides[0];
    src_u = in->planes[1]  + i * in->strides[1];
    src_v = in->planes[2]  + i * in->strides[2];

    dst_y = out->planes[0] + i * out->strides[0];
    dst_u = out->planes[1] + i * out->strides[1];
    dst_v = out->planes[2] + i * out->strides[2];

    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        ((m->yuva.coeffs_i[0][0] * (*src_y-0x10) +
          m->yuva.coeffs_i[0][1] * (*src_u-0x80) + 
          m->yuva.coeffs_i[0][2] * (*src_v-0x80)) >> 8) +
        m->yuva.coeffs_i[0][4];
      tmp += 0x10;
      *dst_y = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);

      tmp =
        ((m->yuva.coeffs_i[1][0] * (*src_y-0x10) +
          m->yuva.coeffs_i[1][1] * (*src_u-0x80) + 
          m->yuva.coeffs_i[1][2] * (*src_v-0x80)) >> 8) +
        m->yuva.coeffs_i[1][4];
      tmp += 0x80;
      *dst_u = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      tmp =
        ((m->yuva.coeffs_i[2][0] * (*src_y-0x10) +
          m->yuva.coeffs_i[2][1] * (*src_u-0x80) + 
          m->yuva.coeffs_i[2][2] * (*src_v-0x80)) >> 8) +
        m->yuva.coeffs_i[2][4];
      tmp += 0x80;
      *dst_v = (uint8_t)((tmp & ~0xFF)?((-tmp) >> 31) : tmp);
      
      src_y++;
      src_u++;
      src_v++;

      dst_y++;
      dst_u++;
      dst_v++;
      }
    }
  }

static void process_444_16(bg_colormatrix_t * m,
                           const gavl_video_frame_t * in,
                           const gavl_video_frame_t * out)
  {
  int64_t tmp;
  int i, j;
  uint16_t * src_y, * src_u, *src_v;
  uint16_t * dst_y, * dst_u, *dst_v;
  
  for(i = 0; i < m->format.image_height; i++)
    {
    src_y = (uint16_t*)(in->planes[0]  + i * in->strides[0]);
    src_u = (uint16_t*)(in->planes[1]  + i * in->strides[1]);
    src_v = (uint16_t*)(in->planes[2]  + i * in->strides[2]);

    dst_y = (uint16_t*)(out->planes[0] + i * out->strides[0]);
    dst_u = (uint16_t*)(out->planes[1] + i * out->strides[1]);
    dst_v = (uint16_t*)(out->planes[2] + i * out->strides[2]);
    
    for(j = 0; j < m->format.image_width; j++)
      {
      tmp =
        (((int64_t)m->yuva.coeffs_i[0][0] * (*src_y-0x1000) +
          m->yuva.coeffs_i[0][1] * (*src_u-0x8000) + 
          m->yuva.coeffs_i[0][2] * (*src_v-0x8000)) >> 16) +
        m->yuva.coeffs_i[0][4];
      tmp += 0x1000;
      
      *dst_y = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      tmp =
        (((int64_t)m->yuva.coeffs_i[1][0] * (*src_y-0x1000) +
          m->yuva.coeffs_i[1][1] * (*src_u-0x8000) + 
          m->yuva.coeffs_i[1][2] * (*src_v-0x8000)) >> 16) +
        m->yuva.coeffs_i[1][4];
      tmp += 0x8000;

      *dst_u = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      tmp =
        (((int64_t)m->yuva.coeffs_i[2][0] * (*src_y-0x1000) +
          m->yuva.coeffs_i[2][1] * (*src_u-0x8000) + 
          m->yuva.coeffs_i[2][2] * (*src_v-0x8000)) >> 16) +
        m->yuva.coeffs_i[2][4];
      tmp += 0x8000;
      *dst_v = (uint16_t)((tmp & ~0xFFFF)?((-tmp) >> 63) : tmp);
      
      src_y++;
      src_u++;
      src_v++;

      dst_y++;
      dst_u++;
      dst_v++;
      }
    }
  }



static void matrix_f_to_8(matrix_t * mat)
  {
  int i, j;
  for(i = 0; i < 4; i++)
    {
    for(j = 0; j < 5; j++)
      mat->coeffs_i[i][j] = (int)(mat->coeffs_f[i][j] * 256.0 + 0.5);
    }
  }

static void matrix_f_to_16(matrix_t * mat)
  {
  int i, j;
  for(i = 0; i < 4; i++)
    {
    for(j = 0; j < 5; j++)
      mat->coeffs_i[i][j] = (int)(mat->coeffs_f[i][j] * 65536.0 + 0.5);
    }
  }

#define SCALE_Y    219.0
#define SCALE_UV   224.0
#define SCALE_A    255.0
#define SCALE_OFF  255.0

static void matrix_f_to_8_yuv(matrix_t * mat)
  {
  int i, j;

  /* y -> y */
  mat->coeffs_i[0][0] = (int)(mat->coeffs_f[0][0] * 256.0 + 0.5);
  /* u -> y */
  mat->coeffs_i[0][1] = (int)(mat->coeffs_f[0][1] * 256.0 * SCALE_Y / SCALE_UV + 0.5);
  /* v -> y */
  mat->coeffs_i[0][2] = (int)(mat->coeffs_f[0][2] * 256.0 * SCALE_Y / SCALE_UV + 0.5);
  /* a -> y */
  mat->coeffs_i[0][3] = (int)(mat->coeffs_f[0][3] * 256.0 * SCALE_Y / SCALE_A + 0.5);
  /* y off */
  mat->coeffs_i[0][4] = (int)(mat->coeffs_f[0][4] * 256.0 * SCALE_Y / SCALE_OFF + 0.5);
  
  /* y -> u */
  mat->coeffs_i[1][0] = (int)(mat->coeffs_f[1][0] * 256.0 * SCALE_UV / SCALE_Y + 0.5);
  /* u -> u */
  mat->coeffs_i[1][1] = (int)(mat->coeffs_f[1][1] * 256.0 + 0.5);
  /* v -> u */
  mat->coeffs_i[1][2] = (int)(mat->coeffs_f[1][2] * 256.0 + 0.5);
  /* a -> u */
  mat->coeffs_i[1][3] = (int)(mat->coeffs_f[1][3] * 256.0 * SCALE_UV / SCALE_A + 0.5);
  /* u off */
  mat->coeffs_i[1][4] = (int)(mat->coeffs_f[1][4] * 256.0 * SCALE_UV / SCALE_OFF + 0.5);

  /* y -> v */
  mat->coeffs_i[2][0] = (int)(mat->coeffs_f[2][0] * 256.0 * SCALE_UV / SCALE_Y + 0.5);
  /* u -> v */
  mat->coeffs_i[2][1] = (int)(mat->coeffs_f[2][1] * 256.0 + 0.5);
  /* v -> v */
  mat->coeffs_i[2][2] = (int)(mat->coeffs_f[2][2] * 256.0 + 0.5);
  /* a -> v */
  mat->coeffs_i[2][3] = (int)(mat->coeffs_f[2][3] * 256.0 * SCALE_UV / SCALE_A + 0.5);
  /* v off */
  mat->coeffs_i[2][4] = (int)(mat->coeffs_f[2][4] * 256.0 * SCALE_UV / SCALE_OFF + 0.5);

  /* y -> a */
  mat->coeffs_i[3][0] = (int)(mat->coeffs_f[3][0] * 256.0 * SCALE_A / SCALE_Y + 0.5);
  /* u -> a */
  mat->coeffs_i[3][1] = (int)(mat->coeffs_f[3][1] * 256.0 * SCALE_A / SCALE_UV + 0.5);
  /* v -> a */
  mat->coeffs_i[3][2] = (int)(mat->coeffs_f[3][2] * 256.0 * SCALE_A / SCALE_UV + 0.5);
  /* a -> a */
  mat->coeffs_i[3][3] = (int)(mat->coeffs_f[3][3] * 256.0 + 0.5);
  /* a off */
  mat->coeffs_i[3][4] = (int)(mat->coeffs_f[3][4] * 256.0 * SCALE_A / SCALE_OFF + 0.5);
  }

static void matrix_f_to_16_yuv(matrix_t * mat)
  {
  int i, j;

  /* y -> y */
  mat->coeffs_i[0][0] = (int)(mat->coeffs_f[0][0] * 65536.0 + 0.5);
  /* u -> y */
  mat->coeffs_i[0][1] = (int)(mat->coeffs_f[0][1] * 65536.0 * SCALE_Y / SCALE_UV + 0.5);
  /* v -> y */
  mat->coeffs_i[0][2] = (int)(mat->coeffs_f[0][2] * 65536.0 * SCALE_Y / SCALE_UV + 0.5);
  /* a -> y */
  mat->coeffs_i[0][3] = (int)(mat->coeffs_f[0][3] * 65536.0 * SCALE_Y / SCALE_A + 0.5);
  /* y off */
  mat->coeffs_i[0][4] = (int)(mat->coeffs_f[0][4] * 65536.0 * SCALE_Y / SCALE_OFF + 0.5);
  
  /* y -> u */
  mat->coeffs_i[1][0] = (int)(mat->coeffs_f[1][0] * 65536.0 * SCALE_UV / SCALE_Y + 0.5);
  /* u -> u */
  mat->coeffs_i[1][1] = (int)(mat->coeffs_f[1][1] * 65536.0 + 0.5);
  /* v -> u */
  mat->coeffs_i[1][2] = (int)(mat->coeffs_f[1][2] * 65536.0 + 0.5);
  /* a -> u */
  mat->coeffs_i[1][3] = (int)(mat->coeffs_f[1][3] * 65536.0 * SCALE_UV / SCALE_A + 0.5);
  /* u off */
  mat->coeffs_i[1][4] = (int)(mat->coeffs_f[1][4] * 65536.0 * SCALE_UV / SCALE_OFF + 0.5);

  /* y -> v */
  mat->coeffs_i[2][0] = (int)(mat->coeffs_f[2][0] * 65536.0 * SCALE_UV / SCALE_Y + 0.5);
  /* u -> v */
  mat->coeffs_i[2][1] = (int)(mat->coeffs_f[2][1] * 65536.0 + 0.5);
  /* v -> v */
  mat->coeffs_i[2][2] = (int)(mat->coeffs_f[2][2] * 65536.0 + 0.5);
  /* a -> v */
  mat->coeffs_i[2][3] = (int)(mat->coeffs_f[2][3] * 65536.0 * SCALE_UV / SCALE_A + 0.5);
  /* v off */
  mat->coeffs_i[2][4] = (int)(mat->coeffs_f[2][4] * 65536.0 * SCALE_UV / SCALE_OFF + 0.5);

  /* y -> a */
  mat->coeffs_i[3][0] = (int)(mat->coeffs_f[3][0] * 65536.0 * SCALE_A / SCALE_Y + 0.5);
  /* u -> a */
  mat->coeffs_i[3][1] = (int)(mat->coeffs_f[3][1] * 65536.0 * SCALE_A / SCALE_UV + 0.5);
  /* v -> a */
  mat->coeffs_i[3][2] = (int)(mat->coeffs_f[3][2] * 65536.0 * SCALE_A / SCALE_UV + 0.5);
  /* a -> a */
  mat->coeffs_i[3][3] = (int)(mat->coeffs_f[3][3] * 65536.0 + 0.5);
  /* a off */
  mat->coeffs_i[3][4] = (int)(mat->coeffs_f[3][4] * 65536.0 * SCALE_A / SCALE_OFF + 0.5);
  }


static void init_internal(bg_colormatrix_t*m)
  {
  switch(m->format.pixelformat)
    {
    case GAVL_RGB_24:
      m->func = process_rgb_24;
      matrix_f_to_8(&m->rgba);
      break;
    case GAVL_RGB_48:
      m->func = process_rgb_48;
      matrix_f_to_16(&m->rgba);
      break;
    case GAVL_RGB_FLOAT:
      m->func = process_rgb_float;
      break;
    case GAVL_RGB_32:
      m->func = process_rgb_32;
      matrix_f_to_8(&m->rgba);
      break;
    case GAVL_BGR_24:
      m->func = process_bgr_24;
      matrix_f_to_8(&m->rgba);
      break;
    case GAVL_BGR_32:
      m->func = process_bgr_32;
      matrix_f_to_8(&m->rgba);
      break;
    case GAVL_RGBA_32:
      m->func = process_rgba_32;
      matrix_f_to_8(&m->rgba);
      break;
    case GAVL_RGBA_64:
      m->func = process_rgba_64;
      matrix_f_to_16(&m->rgba);
      break;
    case GAVL_RGBA_FLOAT:
      m->func = process_rgba_float;
      break;
    case GAVL_YUVJ_444_P:
      m->func = process_444j;
      matrix_f_to_8(&m->yuva);
      break;
    case GAVL_YUV_444_P:
      m->func = process_444;
      matrix_f_to_8_yuv(&m->yuva);
      break;
    case GAVL_YUVA_32:
      m->func = process_yuva_32;
      matrix_f_to_8_yuv(&m->yuva);
      break;
    case GAVL_YUV_444_P_16:
      m->func = process_444_16;
      matrix_f_to_16_yuv(&m->yuva);
      break;
    default:
      break;
    }
  }


void bg_colormatrix_set_rgba(bg_colormatrix_t * m, float coeffs[4][5])
  {
  colormatrix_set_4(coeffs, m->rgba.coeffs_f);
  colormatrix_rgb2yuv(m->rgba.coeffs_f, m->yuva.coeffs_f);
  init_internal(m);
  }

void bg_colormatrix_set_yuva(bg_colormatrix_t * m, float coeffs[4][5])
  {
  colormatrix_set_4(coeffs, m->yuva.coeffs_f);
  colormatrix_rgb2yuv(m->yuva.coeffs_f, m->rgba.coeffs_f);
  init_internal(m);
  }

void bg_colormatrix_set_rgb(bg_colormatrix_t * m, float coeffs[3][4])
  {
  colormatrix_set_3(coeffs, m->rgba.coeffs_f);
  colormatrix_rgb2yuv(m->rgba.coeffs_f, m->yuva.coeffs_f);
  init_internal(m);
  //  dump_matrix(m->rgba.coeffs_f);
  //  dump_matrix(m->yuva.coeffs_f);
  }

void bg_colormatrix_set_yuv(bg_colormatrix_t * m, float coeffs[3][4])
  {
  colormatrix_set_3(coeffs, m->yuva.coeffs_f);
  colormatrix_yuv2rgb(m->yuva.coeffs_f, m->rgba.coeffs_f);
  init_internal(m);
  }

static gavl_pixelformat_t pixelformats[] =
  {
    GAVL_RGB_24,
    GAVL_RGB_48,
    GAVL_RGB_32,
    GAVL_BGR_24,
    GAVL_BGR_32,
    GAVL_RGBA_32,
    GAVL_RGBA_64,
    GAVL_YUVJ_444_P,
    GAVL_YUV_444_P,
    GAVL_YUV_444_P_16,
    GAVL_YUVA_32,
    GAVL_PIXELFORMAT_NONE,
  };

static gavl_pixelformat_t pixelformats_alpha[] =
  {
    GAVL_RGBA_32,
    GAVL_RGBA_64,
    GAVL_YUVA_32,
    GAVL_PIXELFORMAT_NONE,
  };


void bg_colormatrix_init(bg_colormatrix_t * m,
                         gavl_video_format_t * format, int flags)
  {
  if(flags & BG_COLORMATRIX_FORCE_ALPHA)
    format->pixelformat = gavl_pixelformat_get_best(format->pixelformat,
                                                    pixelformats_alpha,
                                                    (int*)0);
  else
    format->pixelformat = gavl_pixelformat_get_best(format->pixelformat,
                                                    pixelformats,
                                                    (int*)0);
  
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Pixelformat: %s",
         TRD(gavl_pixelformat_to_string(format->pixelformat), NULL));
  
  gavl_video_format_copy(&m->format, format);
  init_internal(m);
  
  }

void bg_colormatrix_process(bg_colormatrix_t * m,
                            const gavl_video_frame_t * in_frame,
                            gavl_video_frame_t * out_frame)
  {
  m->func(m, in_frame, out_frame);
  }

