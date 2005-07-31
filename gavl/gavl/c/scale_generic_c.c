/*****************************************************************

  scale_generic_c.c

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

#define RECLIP(a,idx) \
  if(a < ctx->min_values[idx]) a = ctx->min_values[idx];    \
  if(a > ctx->max_values[idx]) a = ctx->max_values[idx]

#define RECLIP_FLOAT(a) if(a < 0.0) a = 0.0; if(a > 1.0) a = 1.0

/* x-Direction */

#define FUNC_NAME scale_rgb_15_x_generic_c
#define TYPE color_15

#define SCALE_INIT ctx->tmp[0] = 0; ctx->tmp[1] = 0; ctx->tmp[2] = 0;

#define SCALE_ACCUM                                                    \
  ctx->tmp[0] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src->r;   \
  ctx->tmp[1] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src->g;   \
  ctx->tmp[2] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src->b;

#define SCALE_FINISH                                                    \
  RECLIP(ctx->tmp[0],0);                                                     \
  RECLIP(ctx->tmp[1],1);                                                     \
  RECLIP(ctx->tmp[2],2);                                                     \
  dst->r = ctx->tmp[0] >> 8;                                                 \
  dst->g = ctx->tmp[1] >> 8;                                                 \
  dst->b = ctx->tmp[2] >> 8;

#include "scale_generic_x.h"

#define FUNC_NAME scale_rgb_16_x_generic_c
#define TYPE color_16

#define SCALE_INIT ctx->tmp[0] = 0; ctx->tmp[1] = 0; ctx->tmp[2] = 0;

#define SCALE_ACCUM                                                    \
  ctx->tmp[0] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src->r;   \
  ctx->tmp[1] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src->g;   \
  ctx->tmp[2] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src->b;

#define SCALE_FINISH                                                    \
  RECLIP(ctx->tmp[0],0);                                                     \
  RECLIP(ctx->tmp[1],1);                                                     \
  RECLIP(ctx->tmp[2],2);                                                     \
  dst->r = ctx->tmp[0] >> 8;                                                 \
  dst->g = ctx->tmp[1] >> 8;                                                 \
  dst->b = ctx->tmp[2] >> 8;

#include "scale_generic_x.h"

#define FUNC_NAME scale_uint8_x_1_x_generic_c
#define TYPE uint8_t

#define SCALE_INIT ctx->tmp[0] = 0;

#define SCALE_ACCUM ctx->tmp[0] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[0];

#define SCALE_FINISH   \
  RECLIP(ctx->tmp[0],ctx->plane);               \
  dst[0] = ctx->tmp[0] >> 8;

#include "scale_generic_x.h"

#define FUNC_NAME scale_uint8_x_3_x_generic_c
#define TYPE uint8_t

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;

#define SCALE_ACCUM \
  ctx->tmp[0] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[0];\
  ctx->tmp[1] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[1];\
  ctx->tmp[2] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[2];

#define SCALE_FINISH   \
  RECLIP(ctx->tmp[0],0);                        \
  dst[0] = ctx->tmp[0] >> 8;                    \
  RECLIP(ctx->tmp[1],1);                        \
  dst[1] = ctx->tmp[1] >> 8;                    \
  RECLIP(ctx->tmp[2],2);                        \
  dst[2] = ctx->tmp[2] >> 8;


#include "scale_generic_x.h"

#define FUNC_NAME scale_uint8_x_4_x_generic_c
#define TYPE uint8_t

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;ctx->tmp[3] = 0;

#define SCALE_ACCUM \
  ctx->tmp[0] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[0];\
  ctx->tmp[1] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[1];\
  ctx->tmp[2] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[2];\
  ctx->tmp[3] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[3];


#define SCALE_FINISH   \
  RECLIP(ctx->tmp[0],0);                        \
  dst[0] = ctx->tmp[0] >> 8;                    \
  RECLIP(ctx->tmp[1],1);                        \
  dst[1] = ctx->tmp[1] >> 8;                    \
  RECLIP(ctx->tmp[2],2);                        \
  dst[2] = ctx->tmp[2] >> 8;                    \
  RECLIP(ctx->tmp[3],3);                        \
  dst[3] = ctx->tmp[3] >> 8;

#include "scale_generic_x.h"

#define FUNC_NAME scale_uint16_x_1_x_generic_c
#define TYPE uint16_t

#define SCALE_INIT ctx->tmp[0] = 0;

#define SCALE_ACCUM                             \
  ctx->tmp[0] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[0];

#define SCALE_FINISH                            \
  RECLIP(ctx->tmp[0],ctx->plane);                    \
  dst[0] = ctx->tmp[0] >> 16;

#include "scale_generic_x.h"

#define FUNC_NAME scale_uint16_x_3_x_generic_c
#define TYPE uint16_t

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;

#define SCALE_ACCUM                                                     \
  ctx->tmp[0] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[0];   \
  ctx->tmp[1] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[1];   \
  ctx->tmp[2] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[2];

#define SCALE_FINISH                            \
  RECLIP(ctx->tmp[0],0);                             \
  dst[0] = ctx->tmp[0] >> 16;                        \
  RECLIP(ctx->tmp[1],1);                             \
  dst[1] = ctx->tmp[1] >> 16;                        \
  RECLIP(ctx->tmp[2],2);                             \
  dst[2] = ctx->tmp[2] >> 16;

#include "scale_generic_x.h"

#define FUNC_NAME scale_uint16_x_4_x_generic_c
#define TYPE uint16_t

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;

#define SCALE_ACCUM                                                     \
  ctx->tmp[0] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[0];   \
  ctx->tmp[1] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[1];   \
  ctx->tmp[2] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[2];   \
  ctx->tmp[3] += (int64_t)ctx->table_h.pixels[i].factor[j].fac_i * src[3];

#define SCALE_FINISH                            \
  RECLIP(ctx->tmp[0],0);                             \
  dst[0] = ctx->tmp[0] >> 16;                        \
  RECLIP(ctx->tmp[1],1);                             \
  dst[1] = ctx->tmp[1] >> 16;                        \
  RECLIP(ctx->tmp[2],2);                             \
  dst[2] = ctx->tmp[2] >> 16;                        \
  RECLIP(ctx->tmp[3],3);                             \
  dst[3] = ctx->tmp[3] >> 16;

#include "scale_generic_x.h"

#define FUNC_NAME scale_float_x_3_x_generic_c
#define TYPE float

#define SCALE_INIT dst[0] = 0.0; dst[1] = 0.0; dst[2] = 0.0;

#define SCALE_ACCUM \
  dst[0] += ctx->table_h.pixels[i].factor[j].fac_f * src[0];\
  dst[1] += ctx->table_h.pixels[i].factor[j].fac_f * src[1];\
  dst[2] += ctx->table_h.pixels[i].factor[j].fac_f * src[2];

#define SCALE_FINISH                                                    \
  RECLIP_FLOAT(dst[0]);                                                 \
  RECLIP_FLOAT(dst[1]);                                                 \
  RECLIP_FLOAT(dst[2]);                                                 \

#include "scale_generic_x.h"

#define FUNC_NAME scale_float_x_4_x_generic_c
#define TYPE float

#define SCALE_INIT dst[0] = 0.0; dst[1] = 0.0; dst[2] = 0.0; dst[3] = 0.0;

#define SCALE_ACCUM \
  dst[0] += ctx->table_h.pixels[i].factor[j].fac_f * src[0];    \
  dst[1] += ctx->table_h.pixels[i].factor[j].fac_f * src[1];    \
  dst[2] += ctx->table_h.pixels[i].factor[j].fac_f * src[2];    \
  dst[3] += ctx->table_h.pixels[i].factor[j].fac_f * src[3]; 

#define SCALE_FINISH                                                    \
  RECLIP_FLOAT(dst[0]);                                                 \
  RECLIP_FLOAT(dst[1]);                                                 \
  RECLIP_FLOAT(dst[2]);                                                 \
  RECLIP_FLOAT(dst[3]);

#include "scale_generic_x.h"

/* y-Direction */

#define FUNC_NAME scale_rgb_15_y_generic_c
#define TYPE color_15

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;

#define SCALE_ACCUM                             \
  ctx->tmp[0] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src->r;\
  ctx->tmp[1] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src->g;\
  ctx->tmp[2] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src->b;

#define SCALE_FINISH                            \
  RECLIP(ctx->tmp[0], 0);                                       \
  dst->r = ctx->tmp[0] >> 8;                                    \
  RECLIP(ctx->tmp[1], 1);                                       \
  dst->g = ctx->tmp[1] >> 8;                                    \
  RECLIP(ctx->tmp[2], 2);                                       \
  dst->b = ctx->tmp[2] >> 8;

#include "scale_generic_y.h"

#define FUNC_NAME scale_rgb_16_y_generic_c
#define TYPE color_16

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;

#define SCALE_ACCUM                             \
  ctx->tmp[0] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src->r;\
  ctx->tmp[1] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src->g;\
  ctx->tmp[2] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src->b;

#define SCALE_FINISH                            \
  RECLIP(ctx->tmp[0], 0);                                       \
  dst->r = ctx->tmp[0] >> 8;                                    \
  RECLIP(ctx->tmp[1], 1);                                       \
  dst->g = ctx->tmp[1] >> 8;                                    \
  RECLIP(ctx->tmp[2], 2);                                       \
  dst->b = ctx->tmp[2] >> 8;

#include "scale_generic_y.h"

#define FUNC_NAME scale_uint8_x_1_y_generic_c
#define TYPE uint8_t

#define SCALE_INIT ctx->tmp[0] = 0;

#define SCALE_ACCUM \
  ctx->tmp[0] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[0];

#define SCALE_FINISH                            \
  RECLIP(ctx->tmp[0], ctx->plane);                      \
  dst[0] = ctx->tmp[0] >> 8;

#include "scale_generic_y.h"

#define FUNC_NAME scale_uint8_x_3_y_generic_c
#define TYPE uint8_t

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;

#define SCALE_ACCUM \
  ctx->tmp[0] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[0];\
  ctx->tmp[1] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[1];\
  ctx->tmp[2] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[2];

#define SCALE_FINISH                                    \
  RECLIP(ctx->tmp[0], 0);                               \
  dst[0] = ctx->tmp[0] >> 8;                            \
  RECLIP(ctx->tmp[1], 1);                               \
  dst[1] = ctx->tmp[1] >> 8;                            \
  RECLIP(ctx->tmp[2], 2);                               \
  dst[2] = ctx->tmp[2] >> 8;

#include "scale_generic_y.h"

#define FUNC_NAME scale_uint8_x_4_y_generic_c
#define TYPE uint8_t

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;ctx->tmp[3] = 0;

#define SCALE_ACCUM \
  ctx->tmp[0] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[0];\
  ctx->tmp[1] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[1];\
  ctx->tmp[2] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[2];\
  ctx->tmp[3] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[3];

#define SCALE_FINISH                                    \
  RECLIP(ctx->tmp[0], 0);                               \
  dst[0] = ctx->tmp[0] >> 8;                            \
  RECLIP(ctx->tmp[1], 1);                               \
  dst[1] = ctx->tmp[1] >> 8;                            \
  RECLIP(ctx->tmp[2], 2);                               \
  dst[2] = ctx->tmp[2] >> 8;                            \
  RECLIP(ctx->tmp[3], 3);                               \
  dst[3] = ctx->tmp[3] >> 8;


#include "scale_generic_y.h"

#define FUNC_NAME scale_uint16_x_1_y_generic_c
#define TYPE uint16_t

#define SCALE_INIT ctx->tmp[0] = 0;

#define SCALE_ACCUM                             \
  ctx->tmp[0] += (int64_t)ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[0];

#define SCALE_FINISH \
  RECLIP(ctx->tmp[0], ctx->plane);                      \
  dst[0] = ctx->tmp[0] >> 16;

#include "scale_generic_y.h"

#define FUNC_NAME scale_uint16_x_3_y_generic_c
#define TYPE uint16_t

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;

#define SCALE_ACCUM                             \
  ctx->tmp[0] += (int64_t)ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[0];\
  ctx->tmp[1] += (int64_t)ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[1];\
  ctx->tmp[2] += (int64_t)ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[2];

#define SCALE_FINISH \
  RECLIP(ctx->tmp[0], 0);                      \
  dst[0] = ctx->tmp[0] >> 16;                  \
  RECLIP(ctx->tmp[1], 1);                      \
  dst[1] = ctx->tmp[1] >> 16;                  \
  RECLIP(ctx->tmp[2], 2);                      \
  dst[2] = ctx->tmp[2] >> 16;

#include "scale_generic_y.h"

#define FUNC_NAME scale_uint16_x_4_y_generic_c
#define TYPE uint16_t

#define SCALE_INIT ctx->tmp[0] = 0;ctx->tmp[1] = 0;ctx->tmp[2] = 0;ctx->tmp[3] = 0;

#define SCALE_ACCUM                             \
  ctx->tmp[0] += (int64_t)ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[0];\
  ctx->tmp[1] += (int64_t)ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[1];\
  ctx->tmp[2] += (int64_t)ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[2];\
  ctx->tmp[3] += (int64_t)ctx->table_v.pixels[ctx->scanline].factor[j].fac_i * src[3];

#define SCALE_FINISH \
  RECLIP(ctx->tmp[0], 0);                      \
  dst[0] = ctx->tmp[0] >> 16;                  \
  RECLIP(ctx->tmp[1], 1);                      \
  dst[1] = ctx->tmp[1] >> 16;                  \
  RECLIP(ctx->tmp[2], 2);                      \
  dst[2] = ctx->tmp[2] >> 16;                  \
  RECLIP(ctx->tmp[3], 3);                      \
  dst[3] = ctx->tmp[3] >> 16;

#include "scale_generic_y.h"

#define FUNC_NAME scale_float_x_3_y_generic_c
#define TYPE float

#define SCALE_INIT dst[0] = 0.0; dst[1] = 0.0; dst[2] = 0.0;

#define SCALE_ACCUM \
  dst[0] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_f * src[0];\
  dst[1] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_f * src[1];\
  dst[2] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_f * src[2];\
  
#define SCALE_FINISH                  \
  RECLIP_FLOAT(dst[0]);        \
  RECLIP_FLOAT(dst[1]);        \
  RECLIP_FLOAT(dst[2]);

#include "scale_generic_y.h"

#define FUNC_NAME scale_float_x_4_y_generic_c
#define TYPE float

#define SCALE_INIT dst[0] = 0.0; dst[1] = 0.0; dst[2] = 0.0;dst[3] = 0.0;

#define SCALE_ACCUM \
  dst[0] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_f * src[0];\
  dst[1] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_f * src[1];\
  dst[2] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_f * src[2];\
  dst[3] += ctx->table_v.pixels[ctx->scanline].factor[j].fac_f * src[3];
  
#define SCALE_FINISH                  \
  RECLIP_FLOAT(dst[0]);        \
  RECLIP_FLOAT(dst[1]);        \
  RECLIP_FLOAT(dst[2]);        \
  RECLIP_FLOAT(dst[3]);

#include "scale_generic_y.h"

void gavl_init_scale_funcs_generic_c(gavl_scale_funcs_t * tab)
  {
  //  fprintf(stderr, "gavl_init_scale_funcs_generic_c\n");
  tab->funcs_x.scale_rgb_15 =     scale_rgb_15_x_generic_c;
  tab->funcs_x.scale_rgb_16 =     scale_rgb_16_x_generic_c;
  tab->funcs_x.scale_uint8_x_1 =  scale_uint8_x_1_x_generic_c;
  tab->funcs_x.scale_uint8_x_3 =  scale_uint8_x_3_x_generic_c;
  tab->funcs_x.scale_uint8_x_4 =  scale_uint8_x_4_x_generic_c;
  tab->funcs_x.scale_uint16_x_1 = scale_uint16_x_1_x_generic_c;
  tab->funcs_x.scale_uint16_x_3 = scale_uint16_x_3_x_generic_c;
  tab->funcs_x.scale_uint16_x_4 = scale_uint16_x_4_x_generic_c;
  tab->funcs_x.scale_float_x_3 =  scale_float_x_3_x_generic_c;
  tab->funcs_x.scale_float_x_4 =  scale_float_x_4_x_generic_c;
  tab->funcs_x.bits_rgb_15 = 8;
  tab->funcs_x.bits_rgb_16 = 8;
  tab->funcs_x.bits_uint8  = 8;
  tab->funcs_x.bits_uint16 = 16;

  tab->funcs_y.scale_rgb_15 =     scale_rgb_15_y_generic_c;
  tab->funcs_y.scale_rgb_16 =     scale_rgb_16_y_generic_c;
  tab->funcs_y.scale_uint8_x_1 =  scale_uint8_x_1_y_generic_c;
  tab->funcs_y.scale_uint8_x_3 =  scale_uint8_x_3_y_generic_c;
  tab->funcs_y.scale_uint8_x_4 =  scale_uint8_x_4_y_generic_c;
  tab->funcs_y.scale_uint16_x_1 = scale_uint16_x_1_y_generic_c;
  tab->funcs_y.scale_uint16_x_3 = scale_uint16_x_3_y_generic_c;
  tab->funcs_y.scale_uint16_x_4 = scale_uint16_x_4_y_generic_c;
  tab->funcs_y.scale_float_x_3 =  scale_float_x_3_y_generic_c;
  tab->funcs_y.scale_float_x_4 =  scale_float_x_4_y_generic_c;
  tab->funcs_y.bits_rgb_15 = 8;
  tab->funcs_y.bits_rgb_16 = 8;
  tab->funcs_y.bits_uint8  = 8;
  tab->funcs_y.bits_uint16 = 16;
  
  }
