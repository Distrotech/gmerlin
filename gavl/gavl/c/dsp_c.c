#include <stdlib.h>

#include <gavl/gavl.h>
#include <gavl/gavldsp.h>
#include <dsp.h>

#include "colorspace_tables.h"
#include "colorspace_macros.h"

/* Sum of absolute differences */
static int sad_rgb15_c(uint8_t * src_1, uint8_t * src_2, 
                       int stride_1, int stride_2, 
                       int w, int h)
  {
  int ret = 0, i, j;
  uint16_t * s1, *s2;
  for(i = 0; i < h; i++)
    {
    s1 = (uint16_t *)src_1;
    s2 = (uint16_t *)src_2;

    for(j = 0; j < w; j++)
      {
      ret += 
        abs(RGB15_TO_R_8(*s1)-RGB15_TO_R_8(*s2)) +
        abs(RGB15_TO_G_8(*s1)-RGB15_TO_G_8(*s2)) +
        abs(RGB15_TO_B_8(*s1)-RGB15_TO_B_8(*s2));
      s1++;
      s2++;
      }
    src_1 += stride_1;
    src_2 += stride_2;
    }
  return ret;
  }

static int sad_rgb16_c(uint8_t * src_1, uint8_t * src_2, 
                   int stride_1, int stride_2, 
                   int w, int h)
  {
  int ret = 0, i, j;
  uint16_t * s1, *s2;
  for(i = 0; i < h; i++)
    {
    s1 = (uint16_t *)src_1;
    s2 = (uint16_t *)src_2;

    for(j = 0; j < w; j++)
      {
      ret += 
        abs(RGB16_TO_R_8(*s1)-RGB16_TO_R_8(*s2)) +
        abs(RGB16_TO_G_8(*s1)-RGB16_TO_G_8(*s2)) +
        abs(RGB16_TO_B_8(*s1)-RGB16_TO_B_8(*s2));
      s1++;
      s2++;
      }
    src_1 += stride_1;
    src_2 += stride_2;
    }
  return ret;
  }

static int sad_8_c(uint8_t * src_1, uint8_t * src_2, 
                   int stride_1, int stride_2, 
                   int w, int h)
  {
  int ret = 0, i, j;
  uint8_t * s1, *s2;
  for(i = 0; i < h; i++)
    {
    s1 = src_1;
    s2 = src_2;

    for(j = 0; j < w; j++)
      {
      ret += abs((*s1)-(*s2));
      s1++;
      s2++;
      }
    src_1 += stride_1;
    src_2 += stride_2;
    }
  return ret;
  }

static int sad_16_c(uint8_t * src_1, uint8_t * src_2, 
                    int stride_1, int stride_2, 
                    int w, int h)
  {
  int ret = 0, i, j;
  uint16_t * s1, *s2;
  for(i = 0; i < h; i++)
    {
    s1 = (uint16_t*)src_1;
    s2 = (uint16_t*)src_2;

    for(j = 0; j < w; j++)
      {
      ret += abs((*s1)-(*s2));
      s1++;
      s2++;
      }
    src_1 += stride_1;
    src_2 += stride_2;
    }
  return ret;
  }

static float sad_f_c(uint8_t * src_1, uint8_t * src_2, 
                     int stride_1, int stride_2, 
                     int w, int h)
  {
  float ret = 0.0;
  int i, j;
  float * s1, *s2;
  for(i = 0; i < h; i++)
    {
    s1 = (float*)src_1;
    s2 = (float*)src_2;

    for(j = 0; j < w; j++)
      {
      ret += abs((*s1)-(*s2));
      s1++;
      s2++;
      }
    src_1 += stride_1;
    src_2 += stride_2;
    }
  return ret;
  }

/* Averaging */
static void average_rgb15_c(uint8_t * src_1, uint8_t * src_2, 
                        uint8_t * dst, int num)
  {
  int i;
  uint16_t * s1, *s2, *d;

  s1 = (uint16_t*)src_1;
  s2 = (uint16_t*)src_2;
  d = (uint16_t*)dst;

  for(i = 0; i < num; i++)
    {
    *d = 
      (((*s1 & RGB15_LOWER_MASK) + 
        (*s2 & RGB15_LOWER_MASK)) >> 1) & RGB15_LOWER_MASK;
    *d |= 
      (((*s1 & RGB15_MIDDLE_MASK) + 
        (*s2 & RGB15_MIDDLE_MASK)) >> 1) & RGB15_MIDDLE_MASK;
    *d |= 
      (((*s1 & RGB15_UPPER_MASK) + 
        (*s2 & RGB15_UPPER_MASK)) >> 1) & RGB15_UPPER_MASK;
    s1++;
    s2++;
    d++;
    }
  
  }

static void average_rgb16_c(uint8_t * src_1, uint8_t * src_2, 
                        uint8_t * dst, int num)
  {
  int i;
  uint16_t * s1, *s2, *d;

  s1 = (uint16_t*)src_1;
  s2 = (uint16_t*)src_2;
  d = (uint16_t*)dst;

  for(i = 0; i < num; i++)
    {
    *d = 
      (((*s1 & RGB16_LOWER_MASK) + 
        (*s2 & RGB16_LOWER_MASK)) >> 1) & RGB16_LOWER_MASK;
    *d |= 
      (((*s1 & RGB16_MIDDLE_MASK) + 
        (*s2 & RGB16_MIDDLE_MASK)) >> 1) & RGB16_MIDDLE_MASK;
    *d |= 
      (((*s1 & RGB16_UPPER_MASK) + 
        (*s2 & RGB16_UPPER_MASK)) >> 1) & RGB16_UPPER_MASK;
    s1++;
    s2++;
    d++;
    }
  
  }

static void average_8_c(uint8_t * src_1, uint8_t * src_2, 
                    uint8_t * dst, int num)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    *dst = (*src_1 + *src_2) >> 1;
    src_1++;
    src_2++;
    dst++;
    }
  }

static void average_16_c(uint8_t * src_1, uint8_t * src_2, 
                     uint8_t * dst, int num)
  {
  int i;
  uint16_t * s1, *s2, *d;

  s1 = (uint16_t*)src_1;
  s2 = (uint16_t*)src_2;
  d = (uint16_t*)dst;

  for(i = 0; i < num; i++)
    {
    *d = (*s1 + *s2) >> 1;
    s1++;
    s2++;
    d++;
    }

  }

static void average_f_c(uint8_t * src_1, uint8_t * src_2, 
                        uint8_t * dst, int num)
  {
  int i;
  float * s1, *s2, *d;

  s1 = (float*)src_1;
  s2 = (float*)src_2;
  d = (float*)dst;

  for(i = 0; i < num; i++)
    {
    *d = (*s1 + *s2) * 0.5;
    s1++;
    s2++;
    d++;
    }
  }


/* */

/* Interpolating */

static void interpolate_rgb15_c(uint8_t * src_1, uint8_t * src_2, 
                            uint8_t * dst, int num, float fac)
  {
  int i;
  uint16_t * s1, *s2, *d;
  int fac_i = (int)(fac * 0x10000 + 0.5);
  int anti_fac = 0x10000 - fac_i;
  
  s1 = (uint16_t*)src_1;
  s2 = (uint16_t*)src_2;
  d = (uint16_t*)dst;

  for(i = 0; i < num; i++)
    {
    *d = 
      (((*s1 & RGB15_LOWER_MASK)*fac_i + 
        (*s2 & RGB15_LOWER_MASK)*anti_fac) >> 16) & RGB15_LOWER_MASK;
    *d |= 
      (((*s1 & RGB15_MIDDLE_MASK)*fac_i + 
        (*s2 & RGB15_MIDDLE_MASK)*anti_fac) >> 16) & RGB15_MIDDLE_MASK;
    *d |= 
      (((*s1 & RGB15_UPPER_MASK)*fac_i + 
        (*s2 & RGB15_UPPER_MASK)*anti_fac) >> 16) & RGB15_UPPER_MASK;
    s1++;
    s2++;
    d++;
    }
  
  }

static void interpolate_rgb16_c(uint8_t * src_1, uint8_t * src_2, 
                        uint8_t * dst, int num, float fac)
  {
  int i;
  uint16_t * s1, *s2, *d;
  int fac_i = (int)(fac * 0x10000 + 0.5);
  int anti_fac = 0x10000 - fac_i;

  s1 = (uint16_t*)src_1;
  s2 = (uint16_t*)src_2;
  d = (uint16_t*)dst;

  for(i = 0; i < num; i++)
    {
    *d = 
      (((*s1 & RGB16_LOWER_MASK)*fac_i + 
        (*s2 & RGB16_LOWER_MASK)*anti_fac) >> 16) & RGB16_LOWER_MASK;
    *d |= 
      (((*s1 & RGB16_MIDDLE_MASK)*fac_i + 
        (*s2 & RGB16_MIDDLE_MASK)*anti_fac) >> 16) & RGB16_MIDDLE_MASK;
    *d |= 
      (((*s1 & RGB16_UPPER_MASK)*fac_i + 
        (*s2 & RGB16_UPPER_MASK)*anti_fac) >> 16) & RGB16_UPPER_MASK;
    s1++;
    s2++;
    d++;
    }
  
  }

static void interpolate_8_c(uint8_t * src_1, uint8_t * src_2, 
                    uint8_t * dst, int num, float fac)
  {
  int i;
  int fac_i = (int)(fac * 0x10000 + 0.5);
  int anti_fac = 0x10000 - fac_i;
  for(i = 0; i < num; i++)
    {
    *dst = (*src_1 * fac_i + *src_2 * anti_fac) >> 16;
    src_1++;
    src_2++;
    dst++;
    }
  }

static void interpolate_16_c(uint8_t * src_1, uint8_t * src_2, 
                             uint8_t * dst, int num, float fac)
  {
  int i;
  uint16_t * s1, *s2, *d;
  int fac_i = (int)(fac * 0x8000 + 0.5);
  int anti_fac = 0x8000 - fac_i;

  s1 = (uint16_t*)src_1;
  s2 = (uint16_t*)src_2;
  d = (uint16_t*)dst;

  for(i = 0; i < num; i++)
    {
    *d = (*s1 * fac_i + *s2 * anti_fac) >> 15;
    s1++;
    s2++;
    d++;
    }

  }

static void interpolate_f_c(uint8_t * src_1, uint8_t * src_2, 
                            uint8_t * dst, int num, float fac)
  {
  int i;
  float * s1, *s2, *d;
  float anti_fac = 1.0 - fac;
  s1 = (float*)src_1;
  s2 = (float*)src_2;
  d = (float*)dst;

  for(i = 0; i < num; i++)
    {
    *d = *s1 * fac + *s2 * anti_fac;
    s1++;
    s2++;
    d++;
    }
  }



void gavl_dsp_init_c(gavl_dsp_funcs_t * funcs, 
                     int quality)
  {
  funcs->sad_rgb15 = sad_rgb15_c;
  funcs->sad_rgb16 = sad_rgb16_c;
  funcs->sad_8     = sad_8_c;
  funcs->sad_16    = sad_16_c;
  funcs->sad_f     = sad_f_c;

  funcs->average_rgb15 = average_rgb15_c;
  funcs->average_rgb16 = average_rgb16_c;
  funcs->average_8     = average_8_c;
  funcs->average_16    = average_16_c;
  funcs->average_f     = average_f_c;

  funcs->interpolate_rgb15 = interpolate_rgb15_c;
  funcs->interpolate_rgb16 = interpolate_rgb16_c;
  funcs->interpolate_8     = interpolate_8_c;
  funcs->interpolate_16    = interpolate_16_c;
  funcs->interpolate_f     = interpolate_f_c;
  }
