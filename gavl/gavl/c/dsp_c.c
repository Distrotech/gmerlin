/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <gavl/gavl.h>
#include <gavl/gavldsp.h>
#include <dsp.h>
#include <bswap.h>
#include <math.h>

#include "colorspace_tables.h"
#include "colorspace_macros.h"

/* Sum of absolute differences */
static int sad_rgb15_c(const uint8_t * src_1, const uint8_t * src_2, 
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

static int sad_rgb16_c(const uint8_t * src_1, const uint8_t * src_2, 
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

static int sad_8_c(const uint8_t * src_1, const uint8_t * src_2, 
                   int stride_1, int stride_2, 
                   int w, int h)
  {
  int ret = 0, i, j;
  const uint8_t * s1, *s2;
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

static int sad_16_c(const uint8_t * src_1, const uint8_t * src_2, 
                    int stride_1, int stride_2, 
                    int w, int h)
  {
  int ret = 0, i, j;
  const uint16_t * s1, *s2;
  for(i = 0; i < h; i++)
    {
    s1 = (const uint16_t*)src_1;
    s2 = (const uint16_t*)src_2;

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

static float sad_f_c(const uint8_t * src_1, const uint8_t * src_2, 
                     int stride_1, int stride_2, 
                     int w, int h)
  {
  float ret = 0.0;
  int i, j;
  const float * s1, *s2;
  for(i = 0; i < h; i++)
    {
    s1 = (const float*)src_1;
    s2 = (const float*)src_2;

    for(j = 0; j < w; j++)
      {
      ret += fabs((*s1)-(*s2));
      s1++;
      s2++;
      }
    src_1 += stride_1;
    src_2 += stride_2;
    }
  return ret;
  }

/* Averaging */
static void average_rgb15_c(const uint8_t * src_1, const uint8_t * src_2, 
                        uint8_t * dst, int num)
  {
  int i;
  const uint16_t * s1, *s2;
  uint16_t *d;

  s1 = (const uint16_t*)src_1;
  s2 = (const uint16_t*)src_2;
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

static void average_rgb16_c(const uint8_t * src_1, const uint8_t * src_2, 
                        uint8_t * dst, int num)
  {
  int i;
  const uint16_t * s1, *s2;
  uint16_t *d;

  s1 = (const uint16_t*)src_1;
  s2 = (const uint16_t*)src_2;
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

static void average_8_c(const uint8_t * src_1, const uint8_t * src_2, 
                    uint8_t * dst, int num)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    *dst = (*src_1 + *src_2 + 1) >> 1;
    src_1++;
    src_2++;
    dst++;
    }
  }

static void average_16_c(const uint8_t * src_1, const uint8_t * src_2, 
                     uint8_t * dst, int num)
  {
  int i;
  const uint16_t * s1, *s2;
  uint16_t *d;

  s1 = (const uint16_t*)src_1;
  s2 = (const uint16_t*)src_2;
  d = (uint16_t*)dst;

  for(i = 0; i < num; i++)
    {
    *d = (*s1 + *s2 + 1) >> 1;
    s1++;
    s2++;
    d++;
    }

  }

static void average_f_c(const uint8_t * src_1, const uint8_t * src_2, 
                        uint8_t * dst, int num)
  {
  int i;
  const float * s1, *s2;
  float *d;

  s1 = (const float*)src_1;
  s2 = (const float*)src_2;
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

static void interpolate_rgb15_c(const uint8_t * src_1, const uint8_t * src_2, 
                            uint8_t * dst, int num, float fac)
  {
  int i;
  const uint16_t * s1, *s2;
  uint16_t *d;
  int fac_i = (int)(fac * 0x10000 + 0.5);
  int anti_fac = 0x10000 - fac_i;
  
  s1 = (const uint16_t*)src_1;
  s2 = (const uint16_t*)src_2;
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

static void interpolate_rgb16_c(const uint8_t * src_1, const uint8_t * src_2, 
                        uint8_t * dst, int num, float fac)
  {
  int i;
  const uint16_t * s1, *s2;
  uint16_t *d;
  int fac_i = (int)(fac * 0x10000 + 0.5);
  int anti_fac = 0x10000 - fac_i;

  s1 = (const uint16_t*)src_1;
  s2 = (const uint16_t*)src_2;
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

static void interpolate_8_c(const uint8_t * src_1, const uint8_t * src_2, 
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

static void interpolate_16_c(const uint8_t * src_1, const uint8_t * src_2, 
                             uint8_t * dst, int num, float fac)
  {
  int i;
  const uint16_t * s1, *s2;
  uint16_t *d;
  int fac_i = (int)(fac * 0x8000 + 0.5);
  int anti_fac = 0x8000 - fac_i;

  s1 = (const uint16_t*)src_1;
  s2 = (const uint16_t*)src_2;
  d = (uint16_t*)dst;

  for(i = 0; i < num; i++)
    {
    *d = (*s1 * fac_i + *s2 * anti_fac) >> 15;
    s1++;
    s2++;
    d++;
    }

  }

static void interpolate_f_c(const uint8_t * src_1, const uint8_t * src_2, 
                            uint8_t * dst, int num, float fac)
  {
  int i;
  const float * s1, *s2;
  float *d;
  float anti_fac = 1.0 - fac;
  s1 = (const float*)src_1;
  s2 = (const float*)src_2;
  d = (float*)dst;

  for(i = 0; i < num; i++)
    {
    *d = *s1 * fac + *s2 * anti_fac;
    s1++;
    s2++;
    d++;
    }
  }

static void bswap_16_c(void * data, int len)
  {
  int i;
  uint16_t * ptr = (uint16_t*)data;

  for(i = 0; i < len; i++)
    {
    *ptr = bswap_16(*ptr);
    ptr++;
    }
  
  }

static void bswap_32_c(void * data, int len)
  {
  int i;
  uint32_t * ptr = (uint32_t*)data;
  for(i = 0; i < len; i++)
    {
    *ptr = bswap_32(*ptr);
    ptr++;
    }
  }

static void bswap_64_c(void * data, int len)
  {
  int i;
  uint64_t * ptr = (uint64_t*)data;
  for(i = 0; i < len; i++)
    {
    *ptr = bswap_64(*ptr);
    ptr++;
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
  
  funcs->bswap_16          = bswap_16_c;
  funcs->bswap_32          = bswap_32_c;
  funcs->bswap_64          = bswap_64_c;
  
  }
