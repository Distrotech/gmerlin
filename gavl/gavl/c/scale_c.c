/*****************************************************************

  scale_c.c

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

#define SCALE_FUNC_HEAD \
  for(i = 0; i < ctx->dst_rect.w; i++)       \
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


static void scale_x_15_16_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  color_15 * src;
  color_15 * dst;
  color_15 * src_1;
  color_15 * src_2;
  src = (color_15*)(ctx->src + ctx->scanline * ctx->src_stride);
  dst = (color_15*)(ctx->dst);
  
  SCALE_FUNC_HEAD
    src_1 = src + ctx->table_h.pixels[i].index;
    src_2 = src_1 + 1;
    BILINEAR_1D_PACKED(src_1, src_2, ctx->table_h.pixels[i].factor[0].fac_i,
                       ctx->table_h.pixels[i].factor[1].fac_i, dst);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_x_16_16_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  color_16 * src;
  color_16 * dst;
  color_16 * src_1;
  color_16 * src_2;
  src = (color_16*)(ctx->src + ctx->scanline * ctx->src_stride);
  dst = (color_16*)(ctx->dst);
  
  SCALE_FUNC_HEAD
    src_1 = src + ctx->table_h.pixels[i].index;
    src_2 = src_1 + 1;
    BILINEAR_1D_PACKED(src_1, src_2, ctx->table_h.pixels[i].factor[0].fac_i,
                       ctx->table_h.pixels[i].factor[1].fac_i, dst);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_x_24_24_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = ctx->src + ctx->scanline * ctx->src_stride;
  
  SCALE_FUNC_HEAD
    src_1 = src + 3 * ctx->table_h.pixels[i].index;
    src_2 = src_1 + 3;
    BILINEAR_1D_3(src_1, src_2,
                  ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i,
                  ctx->dst);
    ctx->dst += 3;
  SCALE_FUNC_TAIL
  }

static void scale_x_24_32_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = ctx->src + ctx->scanline * ctx->src_stride;

  SCALE_FUNC_HEAD
    src_1 = src + 4 * ctx->table_h.pixels[i].index;
    src_2 = src_1 + 4;
    BILINEAR_1D_3(src_1, src_2, ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, ctx->dst);
    ctx->dst += 4;
  
  SCALE_FUNC_TAIL
  }

static void scale_x_32_32_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = ctx->src + ctx->scanline * ctx->src_stride;

  SCALE_FUNC_HEAD
    src_1 = src + 4 * ctx->table_h.pixels[i].index;
    src_2 = src_1 + 4;
    BILINEAR_1D_4(src_1, src_2,
                  ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, ctx->dst);
    ctx->dst += 4;
  
  SCALE_FUNC_TAIL
  }

static void scale_x_8_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = ctx->src + ctx->scanline * ctx->src_stride;

  SCALE_FUNC_HEAD
    src_1 = src + ctx->table_h.pixels[i].index;
    src_2 = src_1 + 1;
    BILINEAR_1D_1(src_1, src_2, ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, ctx->dst);
    ctx->dst++;
  SCALE_FUNC_TAIL
  }

static void scale_x_8_bilinear_advance(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;
  src = ctx->src + ctx->scanline * ctx->src_stride;

  SCALE_FUNC_HEAD
    src_1 = src + ctx->src_advance * ctx->table_h.pixels[i].index;
    src_2 = src_1 + ctx->src_advance;
    BILINEAR_1D_1(src_1, src_2, ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, ctx->dst);
    ctx->dst+=ctx->dst_advance;
  SCALE_FUNC_TAIL
  }

/* Bilinear y */

static void scale_y_15_16_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  color_15 * dst;
  color_15 * src_1;
  color_15 * src_2;

  src_1 = (color_15*)(ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride);
  src_2 = (color_15*)((uint8_t*)src_1 + ctx->src_stride);

  dst = (color_15*)(ctx->dst);
  
  SCALE_FUNC_HEAD
    BILINEAR_1D_PACKED(src_1, src_2, ctx->table_v.pixels[ctx->scanline].factor[0].fac_i,
                       ctx->table_v.pixels[ctx->scanline].factor[1].fac_i, dst);
    src_1++;
    src_2++;
    dst += ctx->dst_advance;
  SCALE_FUNC_TAIL
  }

static void scale_y_16_16_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  color_16 * dst;
  color_16 * src_1;
  color_16 * src_2;

  src_1 = (color_16*)(ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride);
  src_2 = (color_16*)((uint8_t*)src_1 + ctx->src_stride);

  dst = (color_16*)(ctx->dst);
  
  SCALE_FUNC_HEAD
    BILINEAR_1D_PACKED(src_1, src_2,
                       ctx->table_v.pixels[ctx->scanline].factor[0].fac_i,
                       ctx->table_v.pixels[ctx->scanline].factor[1].fac_i,
                       dst);
    src_1++;
    src_2++;
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_y_24_24_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride;
  src_2 = src_1 + ctx->src_stride;
  
  SCALE_FUNC_HEAD
    BILINEAR_1D_3(src_1, src_2,
                  ctx->table_v.pixels[ctx->scanline].factor[0].fac_i,
                  ctx->table_v.pixels[ctx->scanline].factor[1].fac_i,
                  ctx->dst);
    src_1 += 3;
    src_2 += 3;
    ctx->dst += 3;
  SCALE_FUNC_TAIL
  }

static void scale_y_24_32_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = ctx->src + ctx->table_v.pixels[ctx->scanline].index * ctx->src_stride;
  src_2 = src_1 + ctx->src_stride;
  
  SCALE_FUNC_HEAD
    BILINEAR_1D_3(src_1, src_2,
                  ctx->table_v.pixels[ctx->scanline].factor[0].fac_i,
                  ctx->table_v.pixels[ctx->scanline].factor[1].fac_i,
                  ctx->dst);
    src_1 += 4;
    src_2 += 4;
    ctx->dst += 4;
  SCALE_FUNC_TAIL
  }

static void scale_y_32_32_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = src + ctx->table_v.pixels[ctx->scanline].index * src_stride;
  src_2 = src_1 + src_stride;
  
  SCALE_FUNC_HEAD
    BILINEAR_1D_4(src_1, src_2,
                  ctx->table_v.pixels[ctx->scanline].factor[0].fac_i,
                  ctx->table_v.pixels[ctx->scanline].factor[1].fac_i,
                  ctx->dst);
    src_1 += 4;
    src_2 += 4;
    ctx->dst += 4;
  SCALE_FUNC_TAIL
  }

static void scale_y_8_bilinear_c(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = src + ctx->table_v.pixels[ctx->scanline].index * src_stride;
  src_2 = src_1 + src_stride;
  
  SCALE_FUNC_HEAD
    BILINEAR_1D_1(src_1, src_2,
                  ctx->table_v.pixels[ctx->scanline].factor[0].fac_i,
                  ctx->table_v.pixels[ctx->scanline].factor[1].fac_i,
                  ctx->dst);
    src_1++;
    src_2++;
    ctx->dst++;
  SCALE_FUNC_TAIL
  }

static void scale_y_8_bilinear_advance(gavl_video_scale_context_t * ctx)
  {
  int i;
  int tmp;
  uint8_t * src_1;
  uint8_t * src_2;
  
  src_1 = src + ctx->table_v.pixels[ctx->scanline].index * src_stride;
  src_2 = src_1 + src_stride;
  
  SCALE_FUNC_HEAD
    BILINEAR_1D_1(src_1, src_2,
                  ctx->table_v.pixels[ctx->scanline].factor[0].fac_i,
                  ctx->table_v.pixels[ctx->scanline].factor[1].fac_i,
                  ctx->dst);
    src_1 += ctx->src_advance;
    src_2 += ctx->src_advance;
    ctx->dst+=ctx->dst_advance;
  SCALE_FUNC_TAIL
  }

/* Bilinear x and y */

static void scale_xy_15_16_bilinear_c(gavl_video_scale_context_t * ctx)
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
  
  src_start_1 = (color_15*)(src + ctx->table_v.pixels[ctx->scanline].index * src_stride);
  src_start_2 = (color_15*)((uint8_t*)src_start_1 + src_stride);
  dst = (color_15*)(_dst);

  factor_v1 = ctx->table_v.pixels[ctx->scanline].factor[0].fac_i;
  factor_v2 = ctx->table_v.pixels[ctx->scanline].factor[1].fac_i;
  
  SCALE_FUNC_HEAD
    src_11 = src_start_1 + ctx->table_h.pixels[i].index;
    src_12 = src_11 + 1;

    src_21 = src_start_2 + ctx->table_h.pixels[i].index;
    src_22 = src_21 + 1;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_PACKED(src_11, src_12, src_21, src_22,
                       ctx->table_h.pixels[i].factor[0].fac_i,
                       ctx->table_h.pixels[i].factor[1].fac_i, dst);
    dst++;
  SCALE_FUNC_TAIL
  }

static void scale_xy_16_16_bilinear_c(gavl_video_scale_context_t * ctx)
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
  
  src_start_1 = (color_16*)(src + ctx->table_v.pixels[ctx->scanline].index * src_stride);
  src_start_2 = (color_16*)((uint8_t*)src_start_1 + src_stride);

  dst = (color_16*)(_dst);

  factor_v1 = ctx->table_v.pixels[ctx->scanline].factor[0].fac_i;
  factor_v2 = ctx->table_v.pixels[ctx->scanline].factor[1].fac_i;
  
  SCALE_FUNC_HEAD
    src_11 = src_start_1 + ctx->table_h.pixels[i].index;
    src_12 = src_11 + 1;

    src_21 = src_start_2 + ctx->table_h.pixels[i].index;
    src_22 = src_21 + 1;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_PACKED(src_11, src_12, src_21, src_22,
                       ctx->table_h.pixels[i].factor[0].fac_i,
                       ctx->table_h.pixels[i].factor[1].fac_i, dst);
    ctx->dst++;
  SCALE_FUNC_TAIL
  }

static void scale_xy_24_24_bilinear_c(gavl_video_scale_context_t * ctx)
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
  
  src_start_1 = src + ctx->table_v.pixels[ctx->scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = ctx->table_v.pixels[ctx->scanline].factor[0].fac_i;
  factor_v2 = ctx->table_v.pixels[ctx->scanline].factor[1].fac_i;
  
  SCALE_FUNC_HEAD
    src_11 = src_start_1 + 3 * ctx->table_h.pixels[i].index;
    src_12 = src_11 + 3;

    src_21 = src_start_2 + 3 * ctx->table_h.pixels[i].index;
    src_22 = src_21 + 3;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_3(src_11, src_12, src_21, src_22,
                  ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, dst);
    dst += 3;
  SCALE_FUNC_TAIL
  }

static void scale_xy_24_32_bilinear_c(gavl_video_scale_context_t * ctx)
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
  
  src_start_1 = src + ctx->table_v.pixels[ctx->scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = ctx->table_v.pixels[ctx->scanline].factor[0].fac_i;
  factor_v2 = ctx->table_v.pixels[ctx->scanline].factor[1].fac_i;
  
  SCALE_FUNC_HEAD
    src_11 = src_start_1 + 4 * ctx->table_h.pixels[i].index;
    src_12 = src_11 + 4;

    src_21 = src_start_2 + 4 * ctx->table_h.pixels[i].index;
    src_22 = src_21 + 4;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_3(src_11, src_12, src_21, src_22,
                  ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, dst);
    dst += 4;
  SCALE_FUNC_TAIL
  }

static void scale_xy_32_32_bilinear_c(gavl_video_scale_context_t * ctx)
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
  
  src_start_1 = src + ctx->table_v.pixels[ctx->scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = ctx->table_v.pixels[ctx->scanline].factor[0].fac_i;
  factor_v2 = ctx->table_v.pixels[ctx->scanline].factor[1].fac_i;
  
  SCALE_FUNC_HEAD
    src_11 = src_start_1 + 4 * ctx->table_h.pixels[i].index;
    src_12 = src_11 + 4;

    src_21 = src_start_2 + 4 * ctx->table_h.pixels[i].index;
    src_22 = src_21 + 4;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_4(src_11, src_12, src_21, src_22,
                  ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, dst);
    dst += 4;
  SCALE_FUNC_TAIL
  

  }

static void scale_xy_8_bilinear_c(gavl_video_scale_context_t * ctx)
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
  
  src_start_1 = src + ctx->table_v.pixels[ctx->scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = ctx->table_v.pixels[ctx->scanline].factor[0].fac_i;
  factor_v2 = ctx->table_v.pixels[ctx->scanline].factor[1].fac_i;
  
  SCALE_FUNC_HEAD
    src_11 = src_start_1 + ctx->table_h.pixels[i].index;
    src_12 = src_11 + 1;

    src_21 = src_start_2 + ctx->table_h.pixels[i].index;
    src_22 = src_21 + 1;
    
    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_1(src_11, src_12, src_21, src_22,
                  ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, dst);
    dst++;
  SCALE_FUNC_TAIL
  
  }

static void scale_xy_8_bilinear_advance(gavl_video_scale_context_t * ctx)
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

  src_start_1 = src + ctx->table_v.pixels[ctx->scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  factor_v1 = ctx->table_v.pixels[ctx->scanline].factor[0].fac_i;
  factor_v2 = ctx->table_v.pixels[ctx->scanline].factor[1].fac_i;
  
  SCALE_FUNC_HEAD
    src_11 = src_start_1 + advance * ctx->table_h.pixels[i].index;
    src_12 = src_11 + advance;

    src_21 = src_start_2 + advance * ctx->table_h.pixels[i].index;
    src_22 = src_21 + advance;

    // #define BILINEAR_2D_3(src_c_11, src_c_12, src_c_21, src_c_22, factor_h1, factor_h2, dst_c)
    
    BILINEAR_2D_1(src_11, src_12, src_21, src_22,
                  ctx->table_h.pixels[i].factor[0].fac_i,
                  ctx->table_h.pixels[i].factor[1].fac_i, dst);
    dst+=advance;
  SCALE_FUNC_TAIL
  
  }

void gavl_init_scale_funcs_c(gavl_scale_funcs_t * tab,
                             gavl_scale_mode_t scale_mode,
                             int scale_x, int scale_y)
  {
#if 0
  if(scale_x && scale_y)
    {
    tab->scale_rgb_15 =     scale_xy_rgb_15_bilinear_c;
    tab->scale_rgb_16 =     scale_xy_rgb_16_bilinear_c;
    tab->scale_uint8_x_1 =  scale_xy_uint8_x_1_bilinear_c;
    tab->scale_uint8_x_3 =  scale_xy_uint8_x_3_bilinear_c;
    tab->scale_uint8_x_4 =  scale_xy_uint8_x_4_bilinear_c;
    tab->scale_uint16_x_1 = scale_xy_uint16_x_1_bilinear_c;
    tab->scale_uint16_x_3 = scale_xy_uint16_x_3_bilinear_c;
    tab->scale_uint16_x_4 = scale_xy_uint16_x_4_bilinear_c;
    tab->scale_float_x_3 =  scale_xy_float_x_3_bilinear_c;
    tab->scale_float_x_4 =  scale_xy_float_x_4_bilinear_c;
    }
  else
#endif
    if(scale_x)
    {
    tab->scale_rgb_15 =     scale_x_rgb_15_bilinear_c;
    tab->scale_rgb_16 =     scale_x_rgb_16_bilinear_c;
    tab->scale_uint8_x_1 =  scale_x_uint8_x_1_bilinear_c;
    tab->scale_uint8_x_3 =  scale_x_uint8_x_3_bilinear_c;
    tab->scale_uint8_x_4 =  scale_x_uint8_x_4_bilinear_c;
    tab->scale_uint16_x_1 = scale_x_uint16_x_1_bilinear_c;
    tab->scale_uint16_x_3 = scale_x_uint16_x_3_bilinear_c;
    tab->scale_uint16_x_4 = scale_x_uint16_x_4_bilinear_c;
    tab->scale_float_x_3 =  scale_x_float_x_3_bilinear_c;
    tab->scale_float_x_4 =  scale_x_float_x_4_bilinear_c;
    }
  else if(scale_y)
    {
    tab->scale_rgb_15 =     scale_y_rgb_15_bilinear_c;
    tab->scale_rgb_16 =     scale_y_rgb_16_bilinear_c;
    tab->scale_uint8_x_1 =  scale_y_uint8_x_1_bilinear_c;
    tab->scale_uint8_x_3 =  scale_y_uint8_x_3_bilinear_c;
    tab->scale_uint8_x_4 =  scale_y_uint8_x_4_bilinear_c;
    tab->scale_uint16_x_1 = scale_y_uint16_x_1_bilinear_c;
    tab->scale_uint16_x_3 = scale_y_uint16_x_3_bilinear_c;
    tab->scale_uint16_x_4 = scale_y_uint16_x_4_bilinear_c;
    tab->scale_float_x_3 =  scale_y_float_x_3_bilinear_c;
    tab->scale_float_x_4 =  scale_y_float_x_4_bilinear_c;
    }

  tab->bits_rgb_15 = 8;
  tab->bits_rgb_16 = 8;
  tab->bits_uint8  = 8;
  tab->bits_uint16 = 16;
  }

