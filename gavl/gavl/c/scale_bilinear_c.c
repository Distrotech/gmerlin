/*****************************************************************

  scale_bilinear_c.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <stdio.h>
#include <gavl/gavl.h>
#include <video.h>
#include <scale.h>

/* Packed formats for 15/16 colors (idea from libvisual) */

typedef struct {
	uint16_t b:5, g:6, r:5;
} color_16;

typedef struct {
	uint16_t b:5, g:5, r:5;
} color_15;

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
  dst[0] = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]) >> 16;

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint8_x_3_x_bilinear_c
#define TYPE uint8_t
#define SCALE \
  dst[0] = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]) >> 16;\
  dst[1] = (ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1]) >> 16;\
  dst[2] = (ctx->table_h.pixels[i].factor_i[0] * src_1[2] + ctx->table_h.pixels[i].factor_i[1] * src_2[2]) >> 16;

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint8_x_4_x_bilinear_c
#define TYPE uint8_t
#define SCALE \
  dst[0] = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]) >> 16;\
  dst[1] = (ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1]) >> 16;\
  dst[2] = (ctx->table_h.pixels[i].factor_i[0] * src_1[2] + ctx->table_h.pixels[i].factor_i[1] * src_2[2]) >> 16;\
  dst[3] = (ctx->table_h.pixels[i].factor_i[0] * src_1[3] + ctx->table_h.pixels[i].factor_i[1] * src_2[3]) >> 16;
 
#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint16_x_1_x_bilinear_c
#define TYPE uint16_t
#define INIT uint32_t tmp;
#define SCALE                                                           \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]); \
  dst[0] = tmp >> 16;

#include "scale_bilinear_x.h"


#define FUNC_NAME scale_uint16_x_3_x_bilinear_c
#define TYPE uint16_t
#define INIT uint32_t tmp;
#define SCALE                                                           \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]); \
  dst[0] = tmp >> 16; \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1]); \
  dst[1] = tmp >> 16; \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[2] + ctx->table_h.pixels[i].factor_i[1] * src_2[2]); \
  dst[2] = tmp >> 16;

#include "scale_bilinear_x.h"

#define FUNC_NAME scale_uint16_x_4_x_bilinear_c
#define TYPE uint16_t
#define INIT uint32_t tmp;
#define SCALE                                                           \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[0] + ctx->table_h.pixels[i].factor_i[1] * src_2[0]); \
  dst[0] = tmp >> 16; \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[1] + ctx->table_h.pixels[i].factor_i[1] * src_2[1]); \
  dst[1] = tmp >> 16; \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[2] + ctx->table_h.pixels[i].factor_i[1] * src_2[2]); \
  dst[2] = tmp >> 16; \
  tmp = (ctx->table_h.pixels[i].factor_i[0] * src_1[3] + ctx->table_h.pixels[i].factor_i[1] * src_2[3]); \
  dst[3] = tmp >> 16;

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
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];

#define NO_UINT8

#define SCALE                                                           \
  dst->r = (fac_1 * src_1->r + fac_2 * src_2->r) >> 16;\
  dst->g = (fac_1 * src_1->g + fac_2 * src_2->g) >> 16;\
  dst->b = (fac_1 * src_1->b + fac_2 * src_2->b) >> 16;

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_rgb_16_y_bilinear_c
#define TYPE color_16
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define NO_UINT8

#define SCALE                                           \
  dst->r = (fac_1 * src_1->r + fac_2 * src_2->r) >> 16;\
  dst->g = (fac_1 * src_1->g + fac_2 * src_2->g) >> 16;\
  dst->b = (fac_1 * src_1->b + fac_2 * src_2->b) >> 16;

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint8_x_1_y_bilinear_c
#define TYPE uint8_t
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define SCALE \
  dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]) >> 16;

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint8_x_3_y_bilinear_c
#define TYPE uint8_t
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define SCALE \
  dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]) >> 16;\
  dst[1] = (fac_1 * src_1[1] + fac_2 * src_2[1]) >> 16;\
  dst[2] = (fac_1 * src_1[2] + fac_2 * src_2[2]) >> 16;

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint8_x_4_y_bilinear_c
#define TYPE uint8_t
#define INIT int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define SCALE \
  dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]) >> 16;\
  dst[1] = (fac_1 * src_1[1] + fac_2 * src_2[1]) >> 16;\
  dst[2] = (fac_1 * src_1[2] + fac_2 * src_2[2]) >> 16;\
  dst[3] = (fac_1 * src_1[3] + fac_2 * src_2[3]) >> 16;
 
#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint16_x_1_y_bilinear_c
#define TYPE uint16_t
#define INIT uint32_t tmp; \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define NO_UINT8

#define SCALE                                  \
  tmp = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[0] = tmp >> 16;

#include "scale_bilinear_y.h"


#define FUNC_NAME scale_uint16_x_3_y_bilinear_c
#define TYPE uint16_t
#define INIT uint32_t tmp;                      \
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define NO_UINT8


#define SCALE                                  \
  tmp = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[0] = tmp >> 16; \
  tmp = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[1] = tmp >> 16; \
  tmp = (fac_1 * src_1[2] + fac_2 * src_2[2]); \
  dst[2] = tmp >> 16;

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_uint16_x_4_y_bilinear_c
#define TYPE uint16_t
#define INIT uint32_t tmp;\
  int fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define NO_UINT8

#define SCALE                                  \
  tmp = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[0] = tmp >> 16; \
  tmp = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[1] = tmp >> 16; \
  tmp = (fac_1 * src_1[2] + fac_2 * src_2[2]); \
  dst[2] = tmp >> 16; \
  tmp = (fac_1 * src_1[3] + fac_2 * src_2[3]); \
  dst[3] = tmp >> 16;

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_float_x_3_y_bilinear_c
#define TYPE float
#define INIT float fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_f[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_f[1];
#define NO_UINT8
  
#define SCALE                                                           \
  dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]); \
  dst[1] = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[2] = (fac_1 * src_1[2] + fac_2 * src_2[2]);

#include "scale_bilinear_y.h"

#define FUNC_NAME scale_float_x_4_y_bilinear_c
#define TYPE float
#define INIT float fac_1, fac_2;\
  fac_1 = ctx->table_v.pixels[ctx->scanline].factor_f[0];\
  fac_2 = ctx->table_v.pixels[ctx->scanline].factor_f[1];
#define NO_UINT8

#define SCALE                                     \
dst[0] = (fac_1 * src_1[0] + fac_2 * src_2[0]);   \
  dst[1] = (fac_1 * src_1[1] + fac_2 * src_2[1]); \
  dst[2] = (fac_1 * src_1[2] + fac_2 * src_2[2]); \
  dst[3] = (fac_1 * src_1[3] + fac_2 * src_2[3]); \

#include "scale_bilinear_y.h"

/* xy-Direction */

#define FUNC_NAME scale_rgb_15_xy_bilinear_c
#define TYPE color_15
#define INIT int fac_v_1, fac_v_2;                              \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0]; \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];

#define SCALE \
  dst->r = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11->r + ctx->table_h.pixels[i].factor_i[1] * src_12->r)+ \
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21->r + ctx->table_h.pixels[i].factor_i[1] * src_22->r))>>16; \
  dst->g = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11->g + ctx->table_h.pixels[i].factor_i[1] * src_12->g)+ \
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21->g + ctx->table_h.pixels[i].factor_i[1] * src_22->g))>>16; \
  dst->b = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11->b + ctx->table_h.pixels[i].factor_i[1] * src_12->b) + \
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21->b + ctx->table_h.pixels[i].factor_i[1] * src_22->b))>>16;

#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_rgb_16_xy_bilinear_c
#define TYPE color_16
#define INIT int fac_v_1, fac_v_2;                                  \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];

#define SCALE                                           \
  dst->r = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11->r + ctx->table_h.pixels[i].factor_i[1] * src_12->r)+ \
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21->r + ctx->table_h.pixels[i].factor_i[1] * src_22->r))>>16; \
  dst->g = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11->g + ctx->table_h.pixels[i].factor_i[1] * src_12->g)+ \
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21->g + ctx->table_h.pixels[i].factor_i[1] * src_22->g))>>16; \
  dst->b = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11->b + ctx->table_h.pixels[i].factor_i[1] * src_12->b) + \
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21->b + ctx->table_h.pixels[i].factor_i[1] * src_22->b))>>16;

#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_uint8_x_1_xy_bilinear_c
#define TYPE uint8_t
#define INIT int fac_v_1, fac_v_2;                                  \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define SCALE                                           \
  dst[0] = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11[0] + ctx->table_h.pixels[i].factor_i[1] * src_12[0]) +\
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21[0] + ctx->table_h.pixels[i].factor_i[1] * src_22[0]))>>16;



#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_uint8_x_3_xy_bilinear_c
#define TYPE uint8_t
#define INIT int fac_v_1, fac_v_2;                                  \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define SCALE                                           \
  dst[0] = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11[0] + ctx->table_h.pixels[i].factor_i[1] * src_12[0]) +\
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21[0] + ctx->table_h.pixels[i].factor_i[1] * src_22[0]))>>16;\
  dst[1] = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11[1] + ctx->table_h.pixels[i].factor_i[1] * src_12[1]) +\
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21[1] + ctx->table_h.pixels[i].factor_i[1] * src_22[1]))>>16;\
  dst[2] = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11[2] + ctx->table_h.pixels[i].factor_i[1] * src_12[2]) +\
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21[2] + ctx->table_h.pixels[i].factor_i[1] * src_22[2]))>>16;
  

#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_uint8_x_4_xy_bilinear_c
#define TYPE uint8_t
#define INIT int fac_v_1, fac_v_2;                                  \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];
#define SCALE                                           \
  dst[0] = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11[0] + ctx->table_h.pixels[i].factor_i[1] * src_12[0]) +\
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21[0] + ctx->table_h.pixels[i].factor_i[1] * src_22[0]))>>16;\
  dst[1] = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11[1] + ctx->table_h.pixels[i].factor_i[1] * src_12[1]) +\
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21[1] + ctx->table_h.pixels[i].factor_i[1] * src_22[1]))>>16;\
  dst[2] = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11[2] + ctx->table_h.pixels[i].factor_i[1] * src_12[2]) +\
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21[2] + ctx->table_h.pixels[i].factor_i[1] * src_22[2]))>>16;\
  dst[3] = (fac_v_1*(ctx->table_h.pixels[i].factor_i[0] * src_11[3] + ctx->table_h.pixels[i].factor_i[1] * src_12[3]) +\
            fac_v_2*(ctx->table_h.pixels[i].factor_i[0] * src_21[3] + ctx->table_h.pixels[i].factor_i[1] * src_22[3]))>>16;

#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_uint16_x_1_xy_bilinear_c
#define TYPE uint16_t
#define INIT uint64_t tmp;                      \
  uint64_t fac_v_1, fac_v_2;                                             \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];

#define SCALE                                  \
  tmp = (fac_v_1*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_11[0] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_12[0])+ \
         fac_v_2*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_21[0] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_22[0])); \
  dst[0] = tmp >> 32;

#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_uint16_x_3_xy_bilinear_c
#define TYPE uint16_t
#define INIT uint64_t tmp;                      \
  uint64_t fac_v_1, fac_v_2;                                             \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];

#define SCALE                                  \
  tmp = (fac_v_1*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_11[0] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_12[0])+ \
         fac_v_2*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_21[0] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_22[0])); \
  dst[0] = tmp >> 32;                                                   \
  tmp = (fac_v_1*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_11[1] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_12[1])+ \
         fac_v_2*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_21[1] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_22[1])); \
  dst[1] = tmp >> 32;\
  tmp = (fac_v_1*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_11[2] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_12[2])+ \
         fac_v_2*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_21[2] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_22[2])); \
  dst[2] = tmp >> 32;

#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_uint16_x_4_xy_bilinear_c
#define TYPE uint16_t
#define INIT uint64_t tmp;                                      \
  uint64_t  fac_v_1, fac_v_2;                                             \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_i[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_i[1];

#define SCALE                                   \
  tmp = (fac_v_1*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_11[0] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_12[0])+ \
         fac_v_2*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_21[0] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_22[0])); \
  dst[0] = tmp >> 32;\
  tmp = (fac_v_1*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_11[1] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_12[1])+ \
         fac_v_2*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_21[1] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_22[1])); \
  dst[1] = tmp >> 32;\
  tmp = (fac_v_1*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_11[2] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_12[2])+ \
         fac_v_2*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_21[2] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_22[2])); \
  dst[2] = tmp >> 32;\
  tmp = (fac_v_1*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_11[3] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_12[3])+ \
         fac_v_2*((uint32_t)ctx->table_h.pixels[i].factor_i[0] * src_21[3] + (uint32_t)ctx->table_h.pixels[i].factor_i[1] * src_22[3])); \
  dst[3] = tmp >> 32;

#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_float_x_3_xy_bilinear_c
#define TYPE float
#define INIT float fac_v_1, fac_v_2;                                \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_f[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_f[1];
  
#define SCALE                                           \
  dst[0] = fac_v_1*(ctx->table_h.pixels[i].factor_f[0] * src_11[0] + ctx->table_h.pixels[i].factor_f[1] * src_12[0])+\
           fac_v_2*(ctx->table_h.pixels[i].factor_f[0] * src_21[0] + ctx->table_h.pixels[i].factor_f[1] * src_22[0]);\
  dst[1] = fac_v_1*(ctx->table_h.pixels[i].factor_f[0] * src_11[1] + ctx->table_h.pixels[i].factor_f[1] * src_12[1])+\
           fac_v_2*(ctx->table_h.pixels[i].factor_f[0] * src_21[1] + ctx->table_h.pixels[i].factor_f[1] * src_22[1]);\
  dst[2] = fac_v_1*(ctx->table_h.pixels[i].factor_f[0] * src_11[2] + ctx->table_h.pixels[i].factor_f[1] * src_12[2])+\
           fac_v_2*(ctx->table_h.pixels[i].factor_f[0] * src_21[2] + ctx->table_h.pixels[i].factor_f[1] * src_22[2]);

#include "scale_bilinear_xy.h"

#define FUNC_NAME scale_float_x_4_xy_bilinear_c
#define TYPE float
#define INIT float fac_v_1, fac_v_2;                                \
  fac_v_1 = ctx->table_v.pixels[ctx->scanline].factor_f[0];   \
  fac_v_2 = ctx->table_v.pixels[ctx->scanline].factor_f[1];

#define SCALE                                     \
  dst[0] = fac_v_1*(ctx->table_h.pixels[i].factor_f[0] * src_11[0] + ctx->table_h.pixels[i].factor_f[1] * src_12[0])+\
           fac_v_2*(ctx->table_h.pixels[i].factor_f[0] * src_21[0] + ctx->table_h.pixels[i].factor_f[1] * src_22[0]);\
  dst[1] = fac_v_1*(ctx->table_h.pixels[i].factor_f[0] * src_11[1] + ctx->table_h.pixels[i].factor_f[1] * src_12[1])+\
           fac_v_2*(ctx->table_h.pixels[i].factor_f[0] * src_21[1] + ctx->table_h.pixels[i].factor_f[1] * src_22[1]);\
  dst[2] = fac_v_1*(ctx->table_h.pixels[i].factor_f[0] * src_11[2] + ctx->table_h.pixels[i].factor_f[1] * src_12[2])+\
           fac_v_2*(ctx->table_h.pixels[i].factor_f[0] * src_21[2] + ctx->table_h.pixels[i].factor_f[1] * src_22[2]);\
  dst[3] = fac_v_1*(ctx->table_h.pixels[i].factor_f[0] * src_11[3] + ctx->table_h.pixels[i].factor_f[1] * src_12[3])+\
           fac_v_2*(ctx->table_h.pixels[i].factor_f[0] * src_21[3] + ctx->table_h.pixels[i].factor_f[1] * src_22[3]);

#include "scale_bilinear_xy.h"

void gavl_init_scale_funcs_bilinear_c(gavl_scale_funcs_t * tab)
  {
  //  fprintf(stderr, "gavl_init_scale_funcs_bilinear_c\n");
#if 1
  tab->funcs_xy.scale_rgb_15 =     scale_rgb_15_xy_bilinear_c;
  tab->funcs_xy.scale_rgb_16 =     scale_rgb_16_xy_bilinear_c;
  tab->funcs_xy.scale_uint8_x_1_advance = scale_uint8_x_1_xy_bilinear_c;
  tab->funcs_xy.scale_uint8_x_1_noadvance = scale_uint8_x_1_xy_bilinear_c;
  tab->funcs_xy.scale_uint8_x_3 =  scale_uint8_x_3_xy_bilinear_c;
  tab->funcs_xy.scale_uint8_x_4 =  scale_uint8_x_4_xy_bilinear_c;
  tab->funcs_xy.scale_uint16_x_1 = scale_uint16_x_1_xy_bilinear_c;
  tab->funcs_xy.scale_uint16_x_3 = scale_uint16_x_3_xy_bilinear_c;
  tab->funcs_xy.scale_uint16_x_4 = scale_uint16_x_4_xy_bilinear_c;
  tab->funcs_xy.scale_float_x_3 =  scale_float_x_3_xy_bilinear_c;
  tab->funcs_xy.scale_float_x_4 =  scale_float_x_4_xy_bilinear_c;

  tab->funcs_xy.bits_rgb_15 = 8;
  tab->funcs_xy.bits_rgb_16 = 8;
  tab->funcs_xy.bits_uint8_advance  = 8;
  tab->funcs_xy.bits_uint8_noadvance  = 8;
  tab->funcs_xy.bits_uint16 = 16;
#endif
  tab->funcs_x.scale_rgb_15 =     scale_rgb_15_x_bilinear_c;
  tab->funcs_x.scale_rgb_16 =     scale_rgb_16_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_1_advance =  scale_uint8_x_1_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_1_noadvance =  scale_uint8_x_1_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_3 =  scale_uint8_x_3_x_bilinear_c;
  tab->funcs_x.scale_uint8_x_4 =  scale_uint8_x_4_x_bilinear_c;
  tab->funcs_x.scale_uint16_x_1 = scale_uint16_x_1_x_bilinear_c;
  tab->funcs_x.scale_uint16_x_3 = scale_uint16_x_3_x_bilinear_c;
  tab->funcs_x.scale_uint16_x_4 = scale_uint16_x_4_x_bilinear_c;
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
  tab->funcs_y.scale_uint8_x_3 =  scale_uint8_x_3_y_bilinear_c;
  tab->funcs_y.scale_uint8_x_4 =  scale_uint8_x_4_y_bilinear_c;
  tab->funcs_y.scale_uint16_x_1 = scale_uint16_x_1_y_bilinear_c;
  tab->funcs_y.scale_uint16_x_3 = scale_uint16_x_3_y_bilinear_c;
  tab->funcs_y.scale_uint16_x_4 = scale_uint16_x_4_y_bilinear_c;
  tab->funcs_y.scale_float_x_3 =  scale_float_x_3_y_bilinear_c;
  tab->funcs_y.scale_float_x_4 =  scale_float_x_4_y_bilinear_c;
  tab->funcs_y.bits_rgb_15 = 16;
  tab->funcs_y.bits_rgb_16 = 16;
  tab->funcs_y.bits_uint8_advance  = 16;
  tab->funcs_y.bits_uint8_noadvance  = 16;
  tab->funcs_y.bits_uint16 = 16;
  
  }
