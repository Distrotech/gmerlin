/*****************************************************************

  _rgb_yuv_c.c  

  Copyright (c) 2001-2002 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/


#include <stdlib.h>

static int r_to_y[0x100];
static int g_to_y[0x100];
static int b_to_y[0x100];

static int r_to_u[0x100];
static int g_to_u[0x100];
static int b_to_u[0x100];

static int r_to_v[0x100];
static int g_to_v[0x100];
static int b_to_v[0x100];

static int rgbyuv_tables_initialized = 0;

static void _init_rgb_to_yuv_c()
  {
  int i;
  if(rgbyuv_tables_initialized)
    return;

  rgbyuv_tables_initialized = 1;
  
  for(i = 0; i < 0x100; i++)
    {
    r_to_y[i] = (int)(0.257*0x10000 * i + 16 * 0x10000);
    g_to_y[i] = (int)(0.504*0x10000 * i);
    b_to_y[i] = (int)(0.098*0x10000 * i);
    
    r_to_u[i] = (int)(-0.148*0x10000 * i);
    g_to_u[i] = (int)(-0.291*0x10000 * i);
    b_to_u[i] = (int)( 0.439*0x10000 * i + 0x800000);
    
    r_to_v[i] = (int)( 0.439*0x10000 * i);
    g_to_v[i] = (int)(-0.368*0x10000 * i);
    b_to_v[i] = (int)(-0.071*0x10000 * i + 0x800000);
    }
  }

/* Masks for BGR16 and RGB16 formats */

#define RGB16_LOWER_MASK  0x001f
#define RGB16_MIDDLE_MASK 0x07e0
#define RGB16_UPPER_MASK  0xf800

/* Extract unsigned char RGB values from 15 bit pixels */

#define RGB16_TO_R(pixel) ((pixel & RGB16_UPPER_MASK)>>8)
#define RGB16_TO_G(pixel) ((pixel & RGB16_MIDDLE_MASK)>>3) 
#define RGB16_TO_B(pixel) ((pixel & RGB16_LOWER_MASK)<<3)

#define BGR16_TO_B(pixel) RGB16_TO_R(pixel)
#define BGR16_TO_G(pixel) RGB16_TO_G(pixel)
#define BGR16_TO_R(pixel) RGB16_TO_B(pixel)

/* Masks for BGR16 and RGB16 formats */

#define RGB15_LOWER_MASK  0x001f
#define RGB15_MIDDLE_MASK 0x03e0
#define RGB15_UPPER_MASK  0x7C00

/* Extract unsigned char RGB values from 16 bit pixels */

#define RGB15_TO_R(pixel) ((pixel & RGB15_UPPER_MASK)>>7)
#define RGB15_TO_G(pixel) ((pixel & RGB15_MIDDLE_MASK)>>2) 
#define RGB15_TO_B(pixel) ((pixel & RGB15_LOWER_MASK)<<3)

#define BGR15_TO_B(pixel) RGB15_TO_R(pixel)
#define BGR15_TO_G(pixel) RGB15_TO_G(pixel) 
#define BGR15_TO_R(pixel) RGB15_TO_B(pixel)

#define RGB_TO_YUV(r,g,b,y,u,v) y=(r_to_y[r]+g_to_y[g]+b_to_y[b])>>16;\
                                u=(r_to_u[r]+g_to_u[g]+b_to_u[b])>>16;\
                                v=(r_to_v[r]+g_to_v[g]+b_to_v[b])>>16;

#define RGB_TO_Y(r,g,b,y) y=(r_to_y[r]+g_to_y[g]+b_to_y[b])>>16;

/********************************************************************
 * Alpha Stuff
 ********************************************************************/

#define INIT_RGBA32 int anti_alpha;\
                    int background_r=ctx->options->background_red>>8;\
                    int background_g=ctx->options->background_green>>8;\
                    int background_b=ctx->options->background_blue>>8;\
                    uint8_t r_tmp;\
                    uint8_t g_tmp;\
                    uint8_t b_tmp;

#define RGBA_TO_YUV(src_r, src_g, src_b, src_a, dst_y, dst_u, dst_v) \
anti_alpha = 0xFF - src_a;\
r_tmp=((src_a*src_r)+(anti_alpha*background_r))>>8;\
g_tmp=((src_a*src_g)+(anti_alpha*background_g))>>8;\
b_tmp=((src_a*src_b)+(anti_alpha*background_b))>>8;\
dst_y=(r_to_y[r_tmp]+g_to_y[g_tmp]+b_to_y[b_tmp])>>16;\
dst_u=(r_to_u[r_tmp]+g_to_u[g_tmp]+b_to_u[b_tmp])>>16;\
dst_v=(r_to_v[r_tmp]+g_to_v[g_tmp]+b_to_v[b_tmp])>>16;

#define RGBA_TO_Y(src_r, src_g, src_b, src_a, dst_y) \
anti_alpha = 0xFF - src_a;\
r_tmp=((src_a*src_r)+(anti_alpha*background_r))>>8;\
g_tmp=((src_a*src_g)+(anti_alpha*background_g))>>8;\
b_tmp=((src_a*src_b)+(anti_alpha*background_b))>>8;\
dst_y=(r_to_y[r_tmp]+g_to_y[g_tmp]+b_to_y[b_tmp])>>16;

static void rgb_15_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(RGB15_TO_R(src[0]),RGB15_TO_G(src[0]),RGB15_TO_B(src[0]),dst[0],dst[1],dst[3])
    RGB_TO_Y(RGB15_TO_R(src[1]),RGB15_TO_G(src[1]),RGB15_TO_B(src[1]),dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(2, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void bgr_15_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(BGR15_TO_R(src[0]),BGR15_TO_G(src[0]),BGR15_TO_B(src[0]),dst[0],dst[1],dst[3])
    RGB_TO_Y(BGR15_TO_R(src[1]),BGR15_TO_G(src[1]),BGR15_TO_B(src[1]),dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(2, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_16_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(RGB16_TO_R(src[0]),RGB16_TO_G(src[0]),RGB16_TO_B(src[0]),dst[0],dst[1],dst[3])
    RGB_TO_Y(RGB16_TO_R(src[1]),RGB16_TO_G(src[1]),RGB16_TO_B(src[1]),dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(2, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void bgr_16_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint16_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(BGR16_TO_R(src[0]),BGR16_TO_G(src[0]),BGR16_TO_B(src[0]),dst[0],dst[1],dst[3])
    RGB_TO_Y(BGR16_TO_R(src[1]),BGR16_TO_G(src[1]),BGR16_TO_B(src[1]),dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(2, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_24_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[0],src[1],src[2],dst[0],dst[1],dst[3])
    RGB_TO_Y(src[3],src[4],src[5],dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(6, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void bgr_24_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[2],src[1],src[0],dst[0],dst[1],dst[3])
    RGB_TO_Y(src[5],src[4],src[3],dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(6, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_32_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[0],src[1],src[2],dst[0],dst[1],dst[3])
    RGB_TO_Y(src[4],src[5],src[6],dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(8, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void bgr_32_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[2],src[1],src[0],dst[0],dst[1],dst[3])
    RGB_TO_Y(src[6],src[5],src[4],dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(8, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgb_15_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint16_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(RGB15_TO_R(src[0]),RGB15_TO_G(src[0]),RGB15_TO_B(src[0]),
               dst_y[0],*dst_u,*dst_v)
    RGB_TO_Y(RGB15_TO_R(src[1]),RGB15_TO_G(src[1]),RGB15_TO_B(src[0]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(2, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void bgr_15_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint16_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(BGR15_TO_R(src[0]),BGR15_TO_G(src[0]),BGR15_TO_B(src[0]),
               dst_y[0],*dst_u,*dst_v)
    RGB_TO_Y(BGR15_TO_R(src[1]),BGR15_TO_G(src[1]),BGR15_TO_B(src[0]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(2, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void rgb_16_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint16_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(RGB16_TO_R(src[0]),RGB16_TO_G(src[0]),RGB16_TO_B(src[0]),
               dst_y[0],*dst_u,*dst_v)
    RGB_TO_Y(RGB16_TO_R(src[1]),RGB16_TO_G(src[1]),RGB16_TO_B(src[0]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(2, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void bgr_16_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint16_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(BGR16_TO_R(src[0]),BGR16_TO_G(src[0]),BGR16_TO_B(src[0]),
               dst_y[0],*dst_u,*dst_v)
    RGB_TO_Y(BGR16_TO_R(src[1]),BGR16_TO_G(src[1]),BGR16_TO_B(src[0]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(2, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void rgb_24_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[0],src[1],src[2],
               dst_y[0],*dst_u,*dst_v)
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(6, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void bgr_24_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[2],src[1],src[0],
               dst_y[0],*dst_u,*dst_v)
    RGB_TO_Y(src[5],src[4],src[3],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(6, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void rgb_32_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[0],src[1],src[2],
               dst_y[0],*dst_u,*dst_v)
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(8, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void bgr_32_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[2],src[1],src[0],
               dst_y[0],*dst_u,*dst_v)
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(8, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }


static void rgb_15_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint16_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint16_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
  RGB_TO_YUV(RGB15_TO_R(src[0]),RGB15_TO_G(src[0]),RGB15_TO_B(src[0]),
             dst_y[0],dst_u[0],dst_v[0])
  RGB_TO_Y(RGB15_TO_R(src[1]),RGB15_TO_G(src[1]),RGB15_TO_B(src[1]),dst_y[1])
  SCANLINE_LOOP_END_PACKED_PLANAR(2, 2, 1)
#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint16_t, uint8_t)
      
  SCANLINE_LOOP_START_PACKED
  RGB_TO_Y(RGB15_TO_R(*src),RGB15_TO_G(*src),RGB15_TO_B(*src),*dst_y)
  RGB_TO_Y(RGB15_TO_R(src[1]),RGB15_TO_G(src[1]),RGB15_TO_B(src[1]),dst_y[1])
  SCANLINE_LOOP_END_PACKED_TO_420_Y(2, 2)
#endif

  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void bgr_15_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint16_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(BGR15_TO_R(src[0]),BGR15_TO_G(src[0]),BGR15_TO_B(src[0]),
               dst_y[0],dst_u[0],dst_v[0])
    RGB_TO_Y(BGR15_TO_R(src[1]),BGR15_TO_G(src[1]),BGR15_TO_B(src[1]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(2, 2, 1)
#ifndef SCANLINE
    CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint16_t, uint8_t)
      
    SCANLINE_LOOP_START_PACKED
    RGB_TO_Y(BGR15_TO_R(*src),BGR15_TO_G(*src),BGR15_TO_B(*src),*dst_y)
    RGB_TO_Y(BGR15_TO_R(src[1]),BGR15_TO_G(src[1]),BGR15_TO_B(src[1]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_TO_420_Y(2, 2)
#endif

  CONVERSION_FUNC_END_PACKED_PLANAR

  }

static void rgb_16_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint16_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(RGB16_TO_R(src[0]),RGB16_TO_G(src[0]),RGB16_TO_B(src[0]),
               dst_y[0],dst_u[0],dst_v[0])
    RGB_TO_Y(RGB16_TO_R(src[1]),RGB16_TO_G(src[1]),RGB16_TO_B(src[1]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(2, 2, 1)
#ifndef SCANLINE
    CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint16_t, uint8_t)
      
    SCANLINE_LOOP_START_PACKED
    RGB_TO_Y(RGB16_TO_R(*src),RGB16_TO_G(*src),RGB16_TO_B(*src),*dst_y)
    RGB_TO_Y(RGB16_TO_R(src[1]),RGB16_TO_G(src[1]),RGB16_TO_B(src[1]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_TO_420_Y(2, 2)
#endif

  CONVERSION_FUNC_END_PACKED_PLANAR

  }

static void bgr_16_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint16_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint16_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(BGR16_TO_R(src[0]),BGR16_TO_G(src[0]),BGR16_TO_B(src[0]),
               dst_y[0],dst_u[0],dst_v[0])
    RGB_TO_Y(BGR16_TO_R(src[1]),BGR16_TO_G(src[1]),BGR16_TO_B(src[1]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(2, 2, 1)
#ifndef SCANLINE
    CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint16_t, uint8_t)
      
    SCANLINE_LOOP_START_PACKED
    RGB_TO_Y(BGR16_TO_R(*src),BGR16_TO_G(*src),BGR16_TO_B(*src),*dst_y)
    RGB_TO_Y(BGR16_TO_R(src[1]),BGR16_TO_G(src[1]),BGR16_TO_B(src[1]),dst_y[1])
    SCANLINE_LOOP_END_PACKED_TO_420_Y(2, 2)
#endif
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void rgb_24_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[0],src[1],src[2],dst_y[0],dst_u[0],dst_v[0])
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(6, 2, 1)
#ifndef SCANLINE
    CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint8_t, uint8_t)
      
    SCANLINE_LOOP_START_PACKED
    RGB_TO_Y(src[0],src[1],src[2],*dst_y)
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1])
    SCANLINE_LOOP_END_PACKED_TO_420_Y(6, 2)
#endif
  CONVERSION_FUNC_END_PACKED_PLANAR

  }

static void bgr_24_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
  RGB_TO_YUV(src[2],src[1],src[0],dst_y[0],dst_u[0],dst_v[0])
  RGB_TO_Y(src[5],src[4],src[3],dst_y[1])
  SCANLINE_LOOP_END_PACKED_PLANAR(6, 2, 1)
#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint8_t, uint8_t)
      
  SCANLINE_LOOP_START_PACKED
  RGB_TO_Y(src[2],src[1],src[0],*dst_y)
  RGB_TO_Y(src[5],src[4],src[3],dst_y[1])
  SCANLINE_LOOP_END_PACKED_TO_420_Y(6, 2)
#endif
  CONVERSION_FUNC_END_PACKED_PLANAR

  }

static void rgb_32_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[0],src[1],src[2],dst_y[0],dst_u[0],dst_v[0])
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(8, 2, 1)
#ifndef SCANLINE
    CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint8_t, uint8_t)
      
    SCANLINE_LOOP_START_PACKED
    RGB_TO_Y(src[0],src[1],src[2],*dst_y)
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1])
    SCANLINE_LOOP_END_PACKED_TO_420_Y(8, 2)
#endif
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void bgr_32_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGB_TO_YUV(src[2],src[1],src[0],dst_y[0],dst_u[0],dst_v[0])
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(8, 2, 1)
#ifndef SCANLINE
    CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint8_t, uint8_t)
      
    SCANLINE_LOOP_START_PACKED
    RGB_TO_Y(src[2],src[1],src[0],*dst_y)
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1])
    SCANLINE_LOOP_END_PACKED_TO_420_Y(8, 2)
#endif
  CONVERSION_FUNC_END_PACKED_PLANAR
  }


static void rgba_32_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  INIT_RGBA32
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGBA_TO_YUV(src[0],src[1],src[2],src[3],dst[0],dst[1],dst[3])
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst[2])
    SCANLINE_LOOP_END_PACKED_PACKED(8, 4)
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void rgba_32_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  INIT_RGBA32
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 2)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGBA_TO_YUV(src[0],src[1],src[2],src[3],dst_y[0],dst_u[0],dst_v[0])
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(8, 2, 1)
#ifndef SCANLINE
    CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint8_t, uint8_t)
      
    SCANLINE_LOOP_START_PACKED
    RGBA_TO_Y(src[0],src[1],src[2],src[3],*dst_y)
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1])
    SCANLINE_LOOP_END_PACKED_TO_420_Y(8, 2)
#endif
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void rgba_32_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  INIT_RGBA32
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PACKED
    RGBA_TO_YUV(src[0],src[1],src[2],src[3],
               dst_y[0],*dst_u,*dst_v)
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1])
    SCANLINE_LOOP_END_PACKED_PLANAR(8, 2, 1)
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

#ifdef SCANLINE
void gavl_init_rgb_yuv_scanline_funcs_c(gavl_colorspace_function_table_t * tab)
#else     
void gavl_init_rgb_yuv_funcs_c(gavl_colorspace_function_table_t * tab)
#endif
  {
  _init_rgb_to_yuv_c();


  tab->rgb_15_to_yuy2 =      rgb_15_to_yuy2_c;
  tab->rgb_15_to_yuv_420_p = rgb_15_to_yuv_420_p_c;
  tab->rgb_15_to_yuv_422_p = rgb_15_to_yuv_422_p_c;

  tab->bgr_15_to_yuy2 =      bgr_15_to_yuy2_c;
  tab->bgr_15_to_yuv_420_p = bgr_15_to_yuv_420_p_c;
  tab->bgr_15_to_yuv_422_p = bgr_15_to_yuv_422_p_c;

  tab->rgb_16_to_yuy2 =      rgb_16_to_yuy2_c;
  tab->rgb_16_to_yuv_420_p = rgb_16_to_yuv_420_p_c;
  tab->rgb_16_to_yuv_422_p = rgb_16_to_yuv_422_p_c;

  tab->bgr_16_to_yuy2 =      bgr_16_to_yuy2_c;
  tab->bgr_16_to_yuv_420_p = bgr_16_to_yuv_420_p_c;
  tab->bgr_16_to_yuv_422_p = bgr_16_to_yuv_422_p_c;

  tab->rgb_24_to_yuy2 =      rgb_24_to_yuy2_c;
  tab->rgb_24_to_yuv_420_p = rgb_24_to_yuv_420_p_c;
  tab->rgb_24_to_yuv_422_p = rgb_24_to_yuv_422_p_c;

  tab->bgr_24_to_yuy2 =      bgr_24_to_yuy2_c;
  tab->bgr_24_to_yuv_420_p = bgr_24_to_yuv_420_p_c;
  tab->bgr_24_to_yuv_422_p = bgr_24_to_yuv_422_p_c;

  tab->rgb_32_to_yuy2 =      rgb_32_to_yuy2_c;
  tab->rgb_32_to_yuv_420_p = rgb_32_to_yuv_420_p_c;
  tab->rgb_32_to_yuv_422_p = rgb_32_to_yuv_422_p_c;

  tab->bgr_32_to_yuy2 =      bgr_32_to_yuy2_c;
  tab->bgr_32_to_yuv_420_p = bgr_32_to_yuv_420_p_c;
  tab->bgr_32_to_yuv_422_p = bgr_32_to_yuv_422_p_c;

  tab->rgba_32_to_yuy2 =      rgba_32_to_yuy2_c;
  tab->rgba_32_to_yuv_420_p = rgba_32_to_yuv_420_p_c;
  tab->rgba_32_to_yuv_422_p = rgba_32_to_yuv_422_p_c;
  }
