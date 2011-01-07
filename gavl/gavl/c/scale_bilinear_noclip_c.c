/*****************************************************************
 * gavl - a general purpose audio/video processing library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <stdio.h>
#include <gavl/gavl.h>
#include <video.h>
#include <scale.h>

#include <scale_macros.h>

/* x-Direction */

#define FUNC_NAME scale_rgb_15_x_bilinear_c
#define TYPE color_15
#define SCALE \
  dst->r = (ctx->table_h.pixels[i].factor_i[0] * src_1->r + ctx->table_h.pixels[i].factor_i[1] * src_2->r) >> 16;\
  dst->g = (ctx->table_h.pixels[i].factor_i[0] * src_1->g + ctx->table_h.pixels[i].factor_i[1] * src_2->g) >> 16;\
  dst->b = (ctx->table_h.pixels[i].factor_i[0] * src_1->b + ctx->table_h.pixels[i].factor_i[1] * src_2->b) >> 16;

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_rgb_16_x_bilinear_c
#define TYPE color_16
#define SCALE \
  dst->r = (ctx->table_h.pixels[i].factor_i[0] * src_1->r + ctx->table_h.pixels[i].factor_i[1] * src_2->r) >> 16;\
  dst->g = (ctx->table_h.pixels[i].factor_i[0] * src_1->g + ctx->table_h.pixels[i].factor_i[1] * src_2->g) >> 16;\
  dst->b = (ctx->table_h.pixels[i].factor_i[0] * src_1->b + ctx->table_h.pixels[i].factor_i[1] * src_2->b) >> 16;

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint8_x_1_x_bilinear_c
#define TYPE uint8_t
#define SCALE \
  dst[0] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0], 16);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint8_x_2_x_bilinear_c
#define TYPE uint8_t
#define SCALE \
  dst[0] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0], 16);\
  dst[1] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1], 16);

#include "scale_bilinear_x.h"


#define FUNC_NAME scale_uint8_x_3_x_bilinear_c
#define TYPE uint8_t
#define SCALE \
  dst[0] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0], 16);\
  dst[1] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1], 16);\
  dst[2] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[2] + ctx->table_h.pixels[i].factor_i[1] * src_2[2], 16);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint8_x_4_x_bilinear_c
#define TYPE uint8_t
#define SCALE \
  dst[0] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0], 16);\
  dst[1] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1], 16);\
  dst[2] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[2] + ctx->table_h.pixels[i].factor_i[1] * src_2[2], 16);\
  dst[3] = DOWNSHIFT(ctx->table_h.pixels[i].factor_i[0] * src_1[3] + ctx->table_h.pixels[i].factor_i[1] * src_2[3], 16);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint16_x_1_x_bilinear_c
#define TYPE uint16_t

#ifdef HQ
#define INIT int64_t tmp;
#else
#define INIT uint32_t tmp;
#endif

#define SCALE                                                           \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]); \
  dst[0] = DOWNSHIFT(tmp, 16);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint16_x_2_x_bilinear_c
#define TYPE uint16_t

#ifdef HQ
#define INIT int64_t tmp;
#else
#define INIT uint32_t tmp;
#endif

#define SCALE                                                           \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]); \
  dst[0] = DOWNSHIFT(tmp, 16); \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1]); \
  dst[1] = DOWNSHIFT(tmp, 16);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint16_x_3_x_bilinear_c
#define TYPE uint16_t

#ifdef HQ
#define INIT int64_t tmp;
#else
#define INIT uint32_t tmp;
#endif

#define SCALE                                                           \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]); \
  dst[0] = DOWNSHIFT(tmp, 16); \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1]); \
  dst[1] = DOWNSHIFT(tmp, 16); \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[2] + ctx->table_h.pixels[i].factor_i[1] * src_2[2]); \
  dst[2] = DOWNSHIFT(tmp, 16);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint16_x_4_x_bilinear_c
#define TYPE uint16_t

#ifdef HQ
#define INIT int64_t tmp;
#else
#define INIT uint32_t tmp;
#endif

#define SCALE                                                           \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]); \
  dst[0] = DOWNSHIFT(tmp, 16); \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1]); \
  dst[1] = DOWNSHIFT(tmp, 16); \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[2] + ctx->table_h.pixels[i].factor_i[1] * src_2[2]); \
  dst[2] = DOWNSHIFT(tmp, 16); \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[3] + ctx->table_h.pixels[i].factor_i[1] * src_2[3]); \
  dst[3] = DOWNSHIFT(tmp, 16);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_float_x_1_x_bilinear_c
#define TYPE float
#define SCALE                                                           \
  dst[0] = (ctx->table_h.pixels[i].factor_f[0] * src_1[0] + ctx->table_h.pixels[i].factor_f[1] * src_2[0]);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_float_x_2_x_bilinear_c
#define TYPE float
#define SCALE                                                           \
  dst[0] = (ctx->table_h.pixels[i].factor_f[0] * src_1[0] + ctx->table_h.pixels[i].factor_f[1] * src_2[0]); \
  dst[1] = (ctx->table_h.pixels[i].factor_f[0] * src_1[1] + ctx->table_h.pixels[i].factor_f[1] * src_2[1]);

#include "scale_bilinear_x.h"



#define FUNC_NAME scale_float_x_3_x_bilinear_c
#define TYPE float
#define SCALE                                                           \
  dst[0] = (ctx->table_h.pixels[i].factor_f[0] * src_1[0] + ctx->table_h.pixels[i].factor_f[1] * src_2[0]); \
  dst[1] = (ctx->table_h.pixels[i].factor_f[0] * src_1[1] + ctx->table_h.pixels[i].factor_f[1] * src_2[1]); \
  dst[2] = (ctx->table_h.pixels[i].factor_f[0] * src_1[2] + ctx->table_h.pixels[i].factor_f[1] * src_2[2]);

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_float_x_4_x_bilinear_c
#define TYPE float
#define SCALE                                                           \
  dst[0] = (ctx->table_h.pixels[i].factor_f[0] * src_1[0] + ctx->table_h.pixels[i].factor_f[1] * src_2[0]); \
  dst[1] = (ctx->table_h.pixels[i].factor_f[0] * src_1[1] + ctx->table_h.pixels[i].factor_f[1] * src_2[1]); \
  dst[2] = (ctx->table_h.pixels[i].factor_f[0] * src_1[2] + ctx->table_h.pixels[i].factor_f[1] * src_2[2]); \
  dst[3] = (ctx->table_h.pixels[i].factor_f[0] * src_1[3] + ctx->table_h.pixels[i].factor_f[1] * src_2[3]); \

#include "scale_bilinear_x.h"


/* y-Direction */

#define FUNC_NAME scale_rgb_15_y_bilinear_c
#define TYPE color_15
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];

#define NO_UINT8

#define SCALE                                                           \
  dst->r = (fac_1 * src_1->r + fac_2 * src_2->r) >> 16;\
  dst->g = (fac_1 * src_1->g + fac_2 * src_2->g) >> 16;\
  dst->b = (fac_1 * src_1->b + fac_2 * src_2->b) >> 16;

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_rgb_16_y_bilinear_c
#define TYPE color_16
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#define NO_UINT8

#define SCALE                                           \
  dst->r = (fac_1 * src_1->r + fac_2 * src_2->r) >> 16;\
  dst->g = (fac_1 * src_1->g + fac_2 * src_2->g) >> 16;\
  dst->b = (fac_1 * src_1->b + fac_2 * src_2->b) >> 16;

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint8_x_1_y_bilinear_c
#define TYPE uint8_t
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#define SCALE \
  dst[0] = DOWNSHIFT(fac_1 * src_1[0] + fac_2 * src_2[0], 16);

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint8_x_2_y_bilinear_c
#define TYPE uint8_t
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#define SCALE \
  dst[0] = DOWNSHIFT(fac_1 * src_1[0] + fac_2 * src_2[0], 16);\
  dst[1] = DOWNSHIFT(fac_1 * src_1[1] + fac_2 * src_2[1], 16);

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint8_x_3_y_bilinear_c
#define TYPE uint8_t
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#define SCALE \
  dst[0] = DOWNSHIFT(fac_1 * src_1[0] + fac_2 * src_2[0], 16);\
  dst[1] = DOWNSHIFT(fac_1 * src_1[1] + fac_2 * src_2[1], 16);\
  dst[2] = DOWNSHIFT(fac_1 * src_1[2] + fac_2 * src_2[2], 16);

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint8_x_4_y_bilinear_c
#define TYPE uint8_t
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#define SCALE \
  dst[0] = DOWNSHIFT(fac_1 * src_1[0] + fac_2 * src_2[0], 16);\
  dst[1] = DOWNSHIFT(fac_1 * src_1[1] + fac_2 * src_2[1], 16);\
  dst[2] = DOWNSHIFT(fac_1 * src_1[2] + fac_2 * src_2[2], 16);\
  dst[3] = DOWNSHIFT(fac_1 * src_1[3] + fac_2 * src_2[3], 16);\
 
#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint16_x_1_y_bilinear_c
#define TYPE uint16_t
#ifdef HQ
#define INIT int64_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#else
#define INIT uint32_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#endif

#define NO_UINT8

#define SCALE                                  \
  tmp = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[0] = DOWNSHIFT(tmp, 16);

#include "scale_bilinear_y.h"


#define FUNC_NAME scale_uint16_x_2_y_bilinear_c
#define TYPE uint16_t

#ifdef HQ
#define INIT int64_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#else
#define INIT uint32_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#endif

#define NO_UINT8


#define SCALE                                  \
  tmp = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[0] = DOWNSHIFT(tmp, 16); \
  tmp = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[1] = DOWNSHIFT(tmp, 16);

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint16_x_3_y_bilinear_c
#define TYPE uint16_t

#ifdef HQ
#define INIT int64_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#else
#define INIT uint32_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#endif

#define NO_UINT8


#define SCALE                                  \
  tmp = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[0] = DOWNSHIFT(tmp, 16); \
  tmp = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[1] = DOWNSHIFT(tmp, 16); \
  tmp = (fac_1 * src_1[2] + fac_2 * src_2[2]); \
  dst[2] = DOWNSHIFT(tmp, 16);

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint16_x_4_y_bilinear_c
#define TYPE uint16_t

#ifdef HQ
#define INIT int64_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#else
#define INIT uint32_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_i[1];
#endif

#define NO_UINT8

#define SCALE                                  \
  tmp = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[0] = DOWNSHIFT(tmp, 16); \
  tmp = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[1] = DOWNSHIFT(tmp, 16); \
  tmp = (fac_1 * src_1[2] + fac_2 * src_2[2]); \
  dst[2] = DOWNSHIFT(tmp, 16); \
  tmp = (fac_1 * src_1[3] + fac_2 * src_2[3]); \
  dst[3] = DOWNSHIFT(tmp, 16);

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_float_x_1_y_bilinear_c
#define TYPE float
#define INIT float fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_f[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_f[1];
#define NO_UINT8
  
#define SCALE                                                           \
  dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]);

#include "scale_bilinear_y.h"


#define FUNC_NAME scale_float_x_2_y_bilinear_c
#define TYPE float
#define INIT float fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_f[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_f[1];
#define NO_UINT8
  
#define SCALE                                                           \
  dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[1] = (fac_1 * src_1[1] + fac_2 * src_2[1]);

#include "scale_bilinear_y.h"


#define FUNC_NAME scale_float_x_3_y_bilinear_c
#define TYPE float
#define INIT float fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_f[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_f[1];
#define NO_UINT8
  
#define SCALE                                                           \
  dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[1] = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[2] = (fac_1 * src_1[2] + fac_2 * src_2[2]);

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_float_x_4_y_bilinear_c
#define TYPE float
#define INIT float fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[scanline].factor_f[0];\
  fac_2 = ctx->table_v.pixels[scanline].factor_f[1];
#define NO_UINT8

#define SCALE                                     \
dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]);   \
  dst[1] = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[2] = (fac_1 * src_1[2] + fac_2 * src_2[2]); \
  dst[3] = (fac_1 * src_1[3] + fac_2 * src_2[3]); \

#include "scale_bilinear_y.h"

void gavl_init_scale_funcs_bilinear_noclip_c(gavl_scale_funcs_t * tab)
  {
  //  fprintf(stderr, "gavl_init_scale_funcs_bilinear_c\n");
  tab->funcs_x.scale_rgb_15 =     scale_rgb_15_x_bilinear_c;
  tab->funcs_x.scale_rgb_16 =     scale_rgb_16_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_1_advance =  scale_uint8_x_1_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_1_noadvance =  scale_uint8_x_1_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_2 =  scale_uint8_x_2_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_3 =  scale_uint8_x_3_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_4 =  scale_uint8_x_4_x_bilinear_c;
  tab->funcs_x.scale_uint16_x_1 = scale_uint16_x_1_x_bilinear_c;
  tab->funcs_x.scale_uint16_x_2 = scale_uint16_x_2_x_bilinear_c;
  tab->funcs_x.scale_uint16_x_3 = scale_uint16_x_3_x_bilinear_c;
  tab->funcs_x.scale_uint16_x_4 = scale_uint16_x_4_x_bilinear_c;
  tab->funcs_x.scale_float_x_1 =  scale_float_x_1_x_bilinear_c;
  tab->funcs_x.scale_float_x_2 =  scale_float_x_2_x_bilinear_c;
  tab->funcs_x.scale_float_x_3 =  scale_float_x_3_x_bilinear_c;
  tab->funcs_x.scale_float_x_4 =  scale_float_x_4_x_bilinear_c;
  tab->funcs_x.bits_rgb_15 = 16;
  tab->funcs_x.bits_rgb_16 = 16;
  tab->funcs_x.bits_uint8_advance  = 16;
  tab->funcs_x.bits_uint8_noadvance  = 16;
  tab->funcs_x.bits_uint16 = 16;

  tab->funcs_y.scale_rgb_15 =     scale_rgb_15_y_bilinear_c;
  tab->funcs_y.scale_rgb_16 =     scale_rgb_16_y_bilinear_c;
  tab->funcs_y.scale_uint8_x_1_noadvance =  scale_uint8_x_1_y_bilinear_c;
  tab->funcs_y.scale_uint8_x_1_advance =  scale_uint8_x_1_y_bilinear_c;
  tab->funcs_y.scale_uint8_x_2 =  scale_uint8_x_2_y_bilinear_c;
  tab->funcs_y.scale_uint8_x_3 =  scale_uint8_x_3_y_bilinear_c;
  tab->funcs_y.scale_uint8_x_4 =  scale_uint8_x_4_y_bilinear_c;
  tab->funcs_y.scale_uint16_x_1 = scale_uint16_x_1_y_bilinear_c;
  tab->funcs_y.scale_uint16_x_2 = scale_uint16_x_2_y_bilinear_c;
  tab->funcs_y.scale_uint16_x_3 = scale_uint16_x_3_y_bilinear_c;
  tab->funcs_y.scale_uint16_x_4 = scale_uint16_x_4_y_bilinear_c;
  tab->funcs_y.scale_float_x_1 =  scale_float_x_1_y_bilinear_c;
  tab->funcs_y.scale_float_x_2 =  scale_float_x_2_y_bilinear_c;
  tab->funcs_y.scale_float_x_3 =  scale_float_x_3_y_bilinear_c;
  tab->funcs_y.scale_float_x_4 =  scale_float_x_4_y_bilinear_c;
  tab->funcs_y.bits_rgb_15 = 16;
  tab->funcs_y.bits_rgb_16 = 16;
  tab->funcs_y.bits_uint8_advance  = 16;
  tab->funcs_y.bits_uint8_noadvance  = 16;
  tab->funcs_y.bits_uint16 = 16;
  
  }
