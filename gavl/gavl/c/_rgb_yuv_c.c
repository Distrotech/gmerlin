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

static int r_to_yj[0x100];
static int g_to_yj[0x100];
static int b_to_yj[0x100];

static int r_to_uj[0x100];
static int g_to_uj[0x100];
static int b_to_uj[0x100];

static int r_to_vj[0x100];
static int g_to_vj[0x100];
static int b_to_vj[0x100];

static int rgbyuv_tables_initialized = 0;

static void _init_rgb_to_yuv_c()
  {
  int i;
  if(rgbyuv_tables_initialized)
    return;

  rgbyuv_tables_initialized = 1;
  
  for(i = 0; i < 0x100; i++)
    {
    r_to_y[i] = (int)((0.29900*219.0/255.0)*0x10000 * i + 16 * 0x10000);
    g_to_y[i] = (int)((0.58700*219.0/255.0)*0x10000 * i);
    b_to_y[i] = (int)((0.11400*219.0/255.0)*0x10000 * i);
    
    r_to_u[i] = (int)(-(0.16874*224.0/255.0)*0x10000 * i);
    g_to_u[i] = (int)(-(0.33126*224.0/255.0)*0x10000 * i);
    b_to_u[i] = (int)( (0.50000*224.0/255.0)*0x10000 * i + 0x800000);
    
    r_to_v[i] = (int)( (0.50000*224.0/255.0)*0x10000 * i);
    g_to_v[i] = (int)(-(0.41869*224.0/255.0)*0x10000 * i);
    b_to_v[i] = (int)(-(0.08131*224.0/255.0)*0x10000 * i + 0x800000);

    r_to_yj[i] = (int)((0.29900)*0x10000 * i);
    g_to_yj[i] = (int)((0.58700)*0x10000 * i);
    b_to_yj[i] = (int)((0.11400)*0x10000 * i);
    
    r_to_uj[i] = (int)(-(0.16874)*0x10000 * i);
    g_to_uj[i] = (int)(-(0.33126)*0x10000 * i);
    b_to_uj[i] = (int)( (0.50000)*0x10000 * i + 0x800000);
    
    r_to_vj[i] = (int)( (0.50000)*0x10000 * i);
    g_to_vj[i] = (int)(-(0.41869)*0x10000 * i);
    b_to_vj[i] = (int)(-(0.08131)*0x10000 * i + 0x800000);
 
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

#define RGB_TO_YUVJ(r,g,b,y,u,v) y=(r_to_yj[r]+g_to_yj[g]+b_to_yj[b])>>16;\
                                 u=(r_to_uj[r]+g_to_uj[g]+b_to_uj[b])>>16;\
                                 v=(r_to_vj[r]+g_to_vj[g]+b_to_vj[b])>>16;

#define RGB_TO_Y(r,g,b,y) y=(r_to_y[r]+g_to_y[g]+b_to_y[b])>>16;

#define RGB_TO_YJ(r,g,b,y) y=(r_to_yj[r]+g_to_yj[g]+b_to_yj[b])>>16;

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

#define RGBA_TO_YUVJ(src_r, src_g, src_b, src_a, dst_y, dst_u, dst_v) \
anti_alpha = 0xFF - src_a;\
r_tmp=((src_a*src_r)+(anti_alpha*background_r))>>8;\
g_tmp=((src_a*src_g)+(anti_alpha*background_g))>>8;\
b_tmp=((src_a*src_b)+(anti_alpha*background_b))>>8;\
dst_y=(r_to_yj[r_tmp]+g_to_yj[g_tmp]+b_to_yj[b_tmp])>>16;\
dst_u=(r_to_uj[r_tmp]+g_to_uj[g_tmp]+b_to_uj[b_tmp])>>16;\
dst_v=(r_to_vj[r_tmp]+g_to_vj[g_tmp]+b_to_vj[b_tmp])>>16;

#define RGBA_TO_YJ(src_r, src_g, src_b, src_a, dst_y) \
anti_alpha = 0xFF - src_a;\
r_tmp=((src_a*src_r)+(anti_alpha*background_r))>>8;\
g_tmp=((src_a*src_g)+(anti_alpha*background_g))>>8;\
b_tmp=((src_a*src_b)+(anti_alpha*background_b))>>8;\
dst_y=(r_to_yj[r_tmp]+g_to_yj[g_tmp]+b_to_yj[b_tmp])>>16;


/****************************************************
 * Conversions to YUY2
 ****************************************************/

/* rgb_15_to_yuy2_c */

#define FUNC_NAME   rgb_15_to_yuy2_c
#define IN_TYPE     uint16_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  2
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(RGB15_TO_R(src[0]),\
               RGB15_TO_G(src[0]),\
               RGB15_TO_B(src[0]),dst[0],dst[1],dst[3])\
    RGB_TO_Y(RGB15_TO_R(src[1]),RGB15_TO_G(src[1]),RGB15_TO_B(src[1]),dst[2])\

#include "../csp_packed_packed.h"

/* bgr_15_to_yuy2_c */

#define FUNC_NAME   bgr_15_to_yuy2_c
#define IN_TYPE     uint16_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  2
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(BGR15_TO_R(src[0]),\
               BGR15_TO_G(src[0]),\
               BGR15_TO_B(src[0]),dst[0],dst[1],dst[3])\
    RGB_TO_Y(BGR15_TO_R(src[1]),BGR15_TO_G(src[1]),BGR15_TO_B(src[1]),dst[2])\

#include "../csp_packed_packed.h"

/* rgb_16_to_yuy2_c */

#define FUNC_NAME   rgb_16_to_yuy2_c
#define IN_TYPE     uint16_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  2
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(RGB16_TO_R(src[0]),\
               RGB16_TO_G(src[0]),\
               RGB16_TO_B(src[0]),dst[0],dst[1],dst[3])\
    RGB_TO_Y(RGB16_TO_R(src[1]),RGB16_TO_G(src[1]),RGB16_TO_B(src[1]),dst[2])\

#include "../csp_packed_packed.h"

/* bgr_16_to_yuy2_c */

#define FUNC_NAME   bgr_16_to_yuy2_c
#define IN_TYPE     uint16_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  2
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(BGR16_TO_R(src[0]),\
               BGR16_TO_G(src[0]),\
               BGR16_TO_B(src[0]),dst[0],dst[1],dst[3])\
    RGB_TO_Y(BGR16_TO_R(src[1]),BGR16_TO_G(src[1]),BGR16_TO_B(src[1]),dst[2])\

#include "../csp_packed_packed.h"

/* rgb_24_to_yuy2_c */

#define FUNC_NAME   rgb_24_to_yuy2_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  6
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(src[0],src[1],src[2],dst[0],dst[1],dst[3]) \
    RGB_TO_Y(src[3],src[4],src[5],dst[2])

#include "../csp_packed_packed.h"

/* bgr_24_to_yuy2_c */

#define FUNC_NAME   bgr_24_to_yuy2_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  6
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(src[2],src[1],src[0],dst[0],dst[1],dst[3]) \
    RGB_TO_Y(src[5],src[4],src[3],dst[2])

#include "../csp_packed_packed.h"

/* rgb_32_to_yuy2_c */

#define FUNC_NAME   rgb_32_to_yuy2_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(src[0],src[1],src[2],dst[0],dst[1],dst[3]) \
    RGB_TO_Y(src[4],src[5],src[6],dst[2])

#include "../csp_packed_packed.h"

/* bgr_32_to_yuy2_c */

#define FUNC_NAME   bgr_32_to_yuy2_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(src[2],src[1],src[0],dst[0],dst[1],dst[3]) \
    RGB_TO_Y(src[6],src[5],src[4],dst[2])

#include "../csp_packed_packed.h"

/****************************************************
 * Conversions to UYVY
 ****************************************************/

/* rgb_15_to_uyvy_c */

#define FUNC_NAME   rgb_15_to_uyvy_c
#define IN_TYPE     uint16_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  2
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(RGB15_TO_R(src[0]),\
               RGB15_TO_G(src[0]),\
               RGB15_TO_B(src[0]),dst[1],dst[0],dst[2])\
    RGB_TO_Y(RGB15_TO_R(src[1]),RGB15_TO_G(src[1]),RGB15_TO_B(src[1]),dst[3])\

#include "../csp_packed_packed.h"

/* bgr_15_to_uyvy_c */

#define FUNC_NAME   bgr_15_to_uyvy_c
#define IN_TYPE     uint16_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  2
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(BGR15_TO_R(src[0]),\
               BGR15_TO_G(src[0]),\
               BGR15_TO_B(src[0]),dst[1],dst[0],dst[2])\
    RGB_TO_Y(BGR15_TO_R(src[1]),BGR15_TO_G(src[1]),BGR15_TO_B(src[1]),dst[3])\

#include "../csp_packed_packed.h"

/* rgb_16_to_uyvy_c */

#define FUNC_NAME   rgb_16_to_uyvy_c
#define IN_TYPE     uint16_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  2
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(RGB16_TO_R(src[0]),\
               RGB16_TO_G(src[0]),\
               RGB16_TO_B(src[0]),dst[1],dst[0],dst[2])\
    RGB_TO_Y(RGB16_TO_R(src[1]),RGB16_TO_G(src[1]),RGB16_TO_B(src[1]),dst[3])\

#include "../csp_packed_packed.h"

/* bgr_16_to_uyvy_c */

#define FUNC_NAME   bgr_16_to_uyvy_c
#define IN_TYPE     uint16_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  2
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(BGR16_TO_R(src[0]),\
               BGR16_TO_G(src[0]),\
               BGR16_TO_B(src[0]),dst[1],dst[0],dst[2])\
    RGB_TO_Y(BGR16_TO_R(src[1]),BGR16_TO_G(src[1]),BGR16_TO_B(src[1]),dst[3])\

#include "../csp_packed_packed.h"

/* rgb_24_to_uyvy_c */

#define FUNC_NAME   rgb_24_to_uyvy_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  6
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(src[0],src[1],src[2],dst[1],dst[0],dst[2]) \
    RGB_TO_Y(src[3],src[4],src[5],dst[3])

#include "../csp_packed_packed.h"

/* bgr_24_to_uyvy_c */

#define FUNC_NAME   bgr_24_to_uyvy_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  6
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(src[2],src[1],src[0],dst[1],dst[0],dst[2]) \
    RGB_TO_Y(src[5],src[4],src[3],dst[3])

#include "../csp_packed_packed.h"

/* rgb_32_to_uyvy_c */

#define FUNC_NAME   rgb_32_to_uyvy_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(src[0],src[1],src[2],dst[1],dst[0],dst[2]) \
    RGB_TO_Y(src[4],src[5],src[6],dst[3])

#include "../csp_packed_packed.h"

/* bgr_32_to_uyvy_c */

#define FUNC_NAME   bgr_32_to_uyvy_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGB_TO_YUV(src[2],src[1],src[0],dst[1],dst[0],dst[2]) \
    RGB_TO_Y(src[6],src[5],src[4],dst[3])

#include "../csp_packed_packed.h"

/****************************************************
 * Conversions to YUV 422 P
 ****************************************************/

/* rgb_15_to_yuv_422_p_c */

#define FUNC_NAME      rgb_15_to_yuv_422_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUV(RGB15_TO_R(src[0]), \
           RGB15_TO_G(src[0]), \
           RGB15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_15_to_yuv_422_p_c */

#define FUNC_NAME      bgr_15_to_yuv_422_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUV(BGR15_TO_R(src[0]), \
           BGR15_TO_G(src[0]), \
           BGR15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_16_to_yuv_422_p_c */

#define FUNC_NAME      rgb_16_to_yuv_422_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUV(RGB16_TO_R(src[0]), \
           RGB16_TO_G(src[0]), \
           RGB16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_16_to_yuv_422_p_c */

#define FUNC_NAME      bgr_16_to_yuv_422_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUV(BGR16_TO_R(src[0]), \
           BGR16_TO_G(src[0]), \
           BGR16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_24_to_yuv_422_p_c */

#define FUNC_NAME      rgb_24_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     6
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_24_to_yuv_422_p_c */

#define FUNC_NAME      bgr_24_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     6
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[5],src[4],src[3],dst_y[1])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_32_to_yuv_422_p_c */

#define FUNC_NAME      rgb_32_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_32_to_yuv_422_p_c */

#define FUNC_NAME      bgr_32_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/****************************************************
 * Conversions to YUV 411 P
 ****************************************************/

/* rgb_15_to_yuv_411_p_c */

#define FUNC_NAME      rgb_15_to_yuv_411_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
RGB_TO_YUV(RGB15_TO_R(src[0]), \
           RGB15_TO_G(src[0]), \
           RGB15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(RGB15_TO_R(src[2]), \
         RGB15_TO_G(src[2]), \
         RGB15_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(RGB15_TO_R(src[3]), \
         RGB15_TO_G(src[3]), \
         RGB15_TO_B(src[3]), \
         dst_y[3])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_15_to_yuv_411_p_c */

#define FUNC_NAME      bgr_15_to_yuv_411_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
RGB_TO_YUV(BGR15_TO_R(src[0]), \
           BGR15_TO_G(src[0]), \
           BGR15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(BGR15_TO_R(src[2]), \
         BGR15_TO_G(src[2]), \
         BGR15_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(BGR15_TO_R(src[3]), \
         BGR15_TO_G(src[3]), \
         BGR15_TO_B(src[3]), \
         dst_y[3])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_16_to_yuv_411_p_c */

#define FUNC_NAME      rgb_16_to_yuv_411_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
RGB_TO_YUV(RGB16_TO_R(src[0]), \
           RGB16_TO_G(src[0]), \
           RGB16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(RGB16_TO_R(src[2]), \
         RGB16_TO_G(src[2]), \
         RGB16_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(RGB16_TO_R(src[3]), \
         RGB16_TO_G(src[3]), \
         RGB16_TO_B(src[3]), \
         dst_y[3])


#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_16_to_yuv_411_p_c */

#define FUNC_NAME      bgr_16_to_yuv_411_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
RGB_TO_YUV(BGR16_TO_R(src[0]), \
           BGR16_TO_G(src[0]), \
           BGR16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(BGR16_TO_R(src[2]), \
         BGR16_TO_G(src[2]), \
         BGR16_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(BGR16_TO_R(src[3]), \
         BGR16_TO_G(src[3]), \
         BGR16_TO_B(src[3]), \
         dst_y[3]) \

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_24_to_yuv_411_p_c */

#define FUNC_NAME      rgb_24_to_yuv_411_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     12
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1]) \
    RGB_TO_Y(src[6],src[7],src[8],dst_y[2]) \
    RGB_TO_Y(src[9],src[10],src[11],dst_y[3])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_24_to_yuv_411_p_c */

#define FUNC_NAME      bgr_24_to_yuv_411_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     12
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[5],src[4],src[3],dst_y[1]) \
    RGB_TO_Y(src[8],src[7],src[6],dst_y[2]) \
    RGB_TO_Y(src[11],src[10],src[9],dst_y[3])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_32_to_yuv_411_p_c */

#define FUNC_NAME      rgb_32_to_yuv_411_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1]) \
    RGB_TO_Y(src[8],src[9],src[10],dst_y[2]) \
    RGB_TO_Y(src[12],src[13],src[14],dst_y[3])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_32_to_yuv_411_p_c */

#define FUNC_NAME      bgr_32_to_yuv_411_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1]) \
    RGB_TO_Y(src[10],src[9],src[8],dst_y[2]) \
    RGB_TO_Y(src[14],src[13],src[12],dst_y[3])

#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/****************************************************
 * Conversions to YUV 444 P
 ****************************************************/

/* rgb_15_to_yuv_444_p_c */

#define FUNC_NAME      rgb_15_to_yuv_444_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
RGB_TO_YUV(RGB15_TO_R(src[0]), \
           RGB15_TO_G(src[0]), \
           RGB15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_15_to_yuv_444_p_c */

#define FUNC_NAME      bgr_15_to_yuv_444_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
RGB_TO_YUV(BGR15_TO_R(src[0]), \
           BGR15_TO_G(src[0]), \
           BGR15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v)
// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_16_to_yuv_444_p_c */

#define FUNC_NAME      rgb_16_to_yuv_444_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
RGB_TO_YUV(RGB16_TO_R(src[0]), \
           RGB16_TO_G(src[0]), \
           RGB16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_16_to_yuv_444_p_c */

#define FUNC_NAME      bgr_16_to_yuv_444_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
RGB_TO_YUV(BGR16_TO_R(src[0]), \
           BGR16_TO_G(src[0]), \
           BGR16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_24_to_yuv_444_p_c */

#define FUNC_NAME      rgb_24_to_yuv_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     3
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_24_to_yuv_444_p_c */

#define FUNC_NAME      bgr_24_to_yuv_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     3
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_32_to_yuv_444_p_c */

#define FUNC_NAME      rgb_32_to_yuv_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_32_to_yuv_444_p_c */

#define FUNC_NAME      bgr_32_to_yuv_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/****************************************************
 * Conversions to YUV 420 P
 ****************************************************/

/* rgb_15_to_yuv_420_p_c */

#define FUNC_NAME      rgb_15_to_yuv_420_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUV(RGB15_TO_R(src[0]), \
           RGB15_TO_G(src[0]), \
           RGB15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1])

#define CONVERT_Y \
RGB_TO_Y(RGB15_TO_R(src[0]), \
         RGB15_TO_G(src[0]), \
         RGB15_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_Y(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* bgr_15_to_yuv_420_p_c */

#define FUNC_NAME      bgr_15_to_yuv_420_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUV(BGR15_TO_R(src[0]), \
           BGR15_TO_G(src[0]), \
           BGR15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1])

#define CONVERT_Y \
RGB_TO_Y(BGR15_TO_R(src[0]), \
         BGR15_TO_G(src[0]), \
         BGR15_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_Y(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1])
#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* rgb_16_to_yuv_420_p_c */

#define FUNC_NAME      rgb_16_to_yuv_420_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUV(RGB16_TO_R(src[0]), \
           RGB16_TO_G(src[0]), \
           RGB16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1])

#define CONVERT_Y    \
RGB_TO_Y(RGB16_TO_R(src[0]), \
         RGB16_TO_G(src[0]), \
         RGB16_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_Y(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* bgr_16_to_yuv_420_p_c */

#define FUNC_NAME      bgr_16_to_yuv_420_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUV(BGR16_TO_R(src[0]), \
           BGR16_TO_G(src[0]), \
           BGR16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1])

#define CONVERT_Y \
RGB_TO_Y(BGR16_TO_R(src[0]), \
         BGR16_TO_G(src[0]), \
         BGR16_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_Y(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* rgb_24_to_yuv_420_p_c */

#define FUNC_NAME      rgb_24_to_yuv_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     6
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1])

#define CONVERT_Y      \
    RGB_TO_Y(src[0],src[1],src[2],dst_y[0]) \
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* bgr_24_to_yuv_420_p_c */

#define FUNC_NAME      bgr_24_to_yuv_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     6
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[5],src[4],src[3],dst_y[1])

#define CONVERT_Y      \
    RGB_TO_Y(src[2],src[1],src[0],dst_y[0]) \
    RGB_TO_Y(src[5],src[4],src[3],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* rgb_32_to_yuv_420_p_c */

#define FUNC_NAME      rgb_32_to_yuv_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1])

#define CONVERT_Y      \
    RGB_TO_Y(src[0],src[1],src[2], \
               dst_y[0]) \
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* bgr_32_to_yuv_420_p_c */

#define FUNC_NAME      bgr_32_to_yuv_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1])

#define CONVERT_Y      \
    RGB_TO_Y(src[2],src[1],src[0], \
             dst_y[0]) \
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/****************************************************
 * Conversions to YUV 410 P
 ****************************************************/

/* rgb_15_to_yuv_410_p_c */

#define FUNC_NAME      rgb_15_to_yuv_410_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
RGB_TO_YUV(RGB15_TO_R(src[0]), \
           RGB15_TO_G(src[0]), \
           RGB15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(RGB15_TO_R(src[2]), \
         RGB15_TO_G(src[2]), \
         RGB15_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(RGB15_TO_R(src[3]), \
         RGB15_TO_G(src[3]), \
         RGB15_TO_B(src[3]), \
         dst_y[3])

#define CONVERT_Y \
RGB_TO_Y(RGB15_TO_R(src[0]), \
         RGB15_TO_G(src[0]), \
         RGB15_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_Y(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(RGB15_TO_R(src[2]), \
         RGB15_TO_G(src[2]), \
         RGB15_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(RGB15_TO_R(src[3]), \
         RGB15_TO_G(src[3]), \
         RGB15_TO_B(src[3]), \
         dst_y[3])

#define CHROMA_SUB     4

#include "../csp_packed_planar.h"

/* bgr_15_to_yuv_410_p_c */

#define FUNC_NAME      bgr_15_to_yuv_410_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
RGB_TO_YUV(BGR15_TO_R(src[0]), \
           BGR15_TO_G(src[0]), \
           BGR15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(BGR15_TO_R(src[2]), \
         BGR15_TO_G(src[2]), \
         BGR15_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(BGR15_TO_R(src[3]), \
         BGR15_TO_G(src[3]), \
         BGR15_TO_B(src[3]), \
         dst_y[3])

#define CONVERT_Y \
RGB_TO_Y(BGR15_TO_R(src[0]), \
         BGR15_TO_G(src[0]), \
         BGR15_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_Y(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(BGR15_TO_R(src[2]), \
         BGR15_TO_G(src[2]), \
         BGR15_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(BGR15_TO_R(src[3]), \
         BGR15_TO_G(src[3]), \
         BGR15_TO_B(src[3]), \
         dst_y[3])

#define CHROMA_SUB     4

#include "../csp_packed_planar.h"

/* rgb_16_to_yuv_410_p_c */

#define FUNC_NAME      rgb_16_to_yuv_410_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
RGB_TO_YUV(RGB16_TO_R(src[0]), \
           RGB16_TO_G(src[0]), \
           RGB16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(RGB16_TO_R(src[2]), \
         RGB16_TO_G(src[2]), \
         RGB16_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(RGB16_TO_R(src[3]), \
         RGB16_TO_G(src[3]), \
         RGB16_TO_B(src[3]), \
         dst_y[3])

#define CONVERT_Y    \
RGB_TO_Y(RGB16_TO_R(src[0]), \
         RGB16_TO_G(src[0]), \
         RGB16_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_Y(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(RGB16_TO_R(src[2]), \
         RGB16_TO_G(src[2]), \
         RGB16_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(RGB16_TO_R(src[3]), \
         RGB16_TO_G(src[3]), \
         RGB16_TO_B(src[3]), \
         dst_y[3])


#define CHROMA_SUB     4

#include "../csp_packed_planar.h"

/* bgr_16_to_yuv_410_p_c */

#define FUNC_NAME      bgr_16_to_yuv_410_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
RGB_TO_YUV(BGR16_TO_R(src[0]), \
           BGR16_TO_G(src[0]), \
           BGR16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_Y(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(BGR16_TO_R(src[2]), \
         BGR16_TO_G(src[2]), \
         BGR16_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(BGR16_TO_R(src[3]), \
         BGR16_TO_G(src[3]), \
         BGR16_TO_B(src[3]), \
         dst_y[3])

#define CONVERT_Y \
RGB_TO_Y(BGR16_TO_R(src[0]), \
         BGR16_TO_G(src[0]), \
         BGR16_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_Y(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1]) \
RGB_TO_Y(BGR16_TO_R(src[2]), \
         BGR16_TO_G(src[2]), \
         BGR16_TO_B(src[2]), \
         dst_y[2]) \
RGB_TO_Y(BGR16_TO_R(src[3]), \
         BGR16_TO_G(src[3]), \
         BGR16_TO_B(src[3]), \
         dst_y[3])

#define CHROMA_SUB     4

#include "../csp_packed_planar.h"

/* rgb_24_to_yuv_410_p_c */

#define FUNC_NAME      rgb_24_to_yuv_410_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     12
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1]) \
    RGB_TO_Y(src[6],src[7],src[8],dst_y[2]) \
    RGB_TO_Y(src[9],src[10],src[11],dst_y[3])
         

#define CONVERT_Y      \
    RGB_TO_Y(src[0],src[1],src[2],dst_y[0]) \
    RGB_TO_Y(src[3],src[4],src[5],dst_y[1]) \
    RGB_TO_Y(src[6],src[7],src[8],dst_y[2]) \
    RGB_TO_Y(src[9],src[10],src[11],dst_y[3])

#define CHROMA_SUB     4

#include "../csp_packed_planar.h"

/* bgr_24_to_yuv_410_p_c */

#define FUNC_NAME      bgr_24_to_yuv_410_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     12
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[5],src[4],src[3],dst_y[1]) \
    RGB_TO_Y(src[8],src[7],src[6],dst_y[2]) \
    RGB_TO_Y(src[11],src[10],src[9],dst_y[3])

#define CONVERT_Y      \
    RGB_TO_Y(src[2],src[1],src[0],dst_y[0]) \
    RGB_TO_Y(src[5],src[4],src[3],dst_y[1]) \
    RGB_TO_Y(src[8],src[7],src[6],dst_y[2]) \
    RGB_TO_Y(src[11],src[10],src[9],dst_y[3])

#define CHROMA_SUB     4

#include "../csp_packed_planar.h"

/* rgb_32_to_yuv_410_p_c */

#define FUNC_NAME      rgb_32_to_yuv_410_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGB_TO_YUV(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1]) \
    RGB_TO_Y(src[8],src[9],src[10],dst_y[2]) \
    RGB_TO_Y(src[12],src[13],src[14],dst_y[3])

#define CONVERT_Y      \
    RGB_TO_Y(src[0],src[1],src[2],dst_y[0]) \
    RGB_TO_Y(src[4],src[5],src[6],dst_y[1]) \
    RGB_TO_Y(src[8],src[9],src[10],dst_y[2]) \
    RGB_TO_Y(src[12],src[13],src[14],dst_y[3])

#define CHROMA_SUB     4

#include "../csp_packed_planar.h"

/* bgr_32_to_yuv_410_p_c */

#define FUNC_NAME      bgr_32_to_yuv_410_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGB_TO_YUV(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1]) \
    RGB_TO_Y(src[10],src[9],src[8],dst_y[2]) \
    RGB_TO_Y(src[14],src[13],src[12],dst_y[3])

#define CONVERT_Y      \
    RGB_TO_Y(src[2],src[1],src[0], \
             dst_y[0]) \
    RGB_TO_Y(src[6],src[5],src[4],dst_y[1]) \
    RGB_TO_Y(src[10],src[9],src[8],dst_y[2]) \
    RGB_TO_Y(src[14],src[13],src[12],dst_y[3])

#define CHROMA_SUB     4

#include "../csp_packed_planar.h"

/* RGBA -> ... */

/* rgba_32_to_yuy2_c */

#define FUNC_NAME   rgba_32_to_yuy2_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGBA_TO_YUV(src[0],src[1],src[2],src[3],dst[0],dst[1],dst[3]) \
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst[2])
#define INIT   INIT_RGBA32

#include "../csp_packed_packed.h"

/* rgba_32_to_uyvy_c */

#define FUNC_NAME   rgba_32_to_uyvy_c
#define IN_TYPE     uint8_t
#define OUT_TYPE    uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define CONVERT     \
    RGBA_TO_YUV(src[0],src[1],src[2],src[3],dst[1],dst[0],dst[2]) \
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst[3])
#define INIT   INIT_RGBA32

#include "../csp_packed_packed.h"

/* rgba_32_to_yuv_422_p_c */

#define FUNC_NAME      rgba_32_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGBA_TO_YUV(src[0],src[1],src[2],src[3], \
               dst_y[0],*dst_u,*dst_v) \
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1])
#define INIT   INIT_RGBA32

// #define CONVERT_Y      
#define CHROMA_SUB     1
#include "../csp_packed_planar.h"

/* rgba_32_to_yuv_411_p_c */

#define FUNC_NAME      rgba_32_to_yuv_411_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGBA_TO_YUV(src[0],src[1],src[2],src[3], \
               dst_y[0],*dst_u,*dst_v) \
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1]) \
    RGBA_TO_Y(src[8],src[9],src[10],src[11],dst_y[2]) \
    RGBA_TO_Y(src[12],src[13],src[14],src[15],dst_y[3])

#define INIT   INIT_RGBA32

// #define CONVERT_Y      
#define CHROMA_SUB     1


#include "../csp_packed_planar.h"

/* rgba_32_to_yuv_420_p_c */

#define FUNC_NAME      rgba_32_to_yuv_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGBA_TO_YUV(src[0],src[1],src[2],src[3], \
               dst_y[0],*dst_u,*dst_v) \
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1])
#define INIT   INIT_RGBA32

#define CONVERT_Y      \
    RGBA_TO_Y(src[0],src[1],src[2],src[3], \
               dst_y[0]) \
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* rgba_32_to_yuv_410_p_c */

#define FUNC_NAME      rgba_32_to_yuv_410_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CONVERT_YUV    \
    RGBA_TO_YUV(src[0],src[1],src[2],src[3], \
               dst_y[0],*dst_u,*dst_v) \
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1]) \
    RGBA_TO_Y(src[8],src[9],src[10],src[11],dst_y[2]) \
    RGBA_TO_Y(src[12],src[13],src[14],src[15],dst_y[3])

#define INIT   INIT_RGBA32

#define CONVERT_Y      \
    RGBA_TO_Y(src[0],src[1],src[2],src[3],dst_y[0]) \
    RGBA_TO_Y(src[4],src[5],src[6],src[7],dst_y[1]) \
    RGBA_TO_Y(src[8],src[9],src[10],src[11],dst_y[2]) \
    RGBA_TO_Y(src[12],src[13],src[14],src[15],dst_y[3])

#define CHROMA_SUB     4

#include "../csp_packed_planar.h"



/* rgba_32_to_yuv_444_p_c */

#define FUNC_NAME      rgba_32_to_yuv_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGBA_TO_YUV(src[0],src[1],src[2],src[3], \
               dst_y[0],*dst_u,*dst_v)
#define INIT   INIT_RGBA32

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* JPEG */

/****************************************************
 * Conversions to YUVJ 422 P
 ****************************************************/

/* rgb_15_to_yuvj_422_p_c */

#define FUNC_NAME      rgb_15_to_yuvj_422_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUVJ(RGB15_TO_R(src[0]), \
           RGB15_TO_G(src[0]), \
           RGB15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_YJ(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1])

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_15_to_yuvj_422_p_c */

#define FUNC_NAME      bgr_15_to_yuvj_422_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUVJ(BGR15_TO_R(src[0]), \
           BGR15_TO_G(src[0]), \
           BGR15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_YJ(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1])

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_16_to_yuvj_422_p_c */

#define FUNC_NAME      rgb_16_to_yuvj_422_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUVJ(RGB16_TO_R(src[0]), \
           RGB16_TO_G(src[0]), \
           RGB16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_YJ(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1])

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_16_to_yuvj_422_p_c */

#define FUNC_NAME      bgr_16_to_yuvj_422_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUVJ(BGR16_TO_R(src[0]), \
           BGR16_TO_G(src[0]), \
           BGR16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_YJ(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1])

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_24_to_yuvj_422_p_c */

#define FUNC_NAME      rgb_24_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     6
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_YJ(src[3],src[4],src[5],dst_y[1])

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_24_to_yuvj_422_p_c */

#define FUNC_NAME      bgr_24_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     6
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_YJ(src[5],src[4],src[3],dst_y[1])

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_32_to_yuvj_422_p_c */

#define FUNC_NAME      rgb_32_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_YJ(src[4],src[5],src[6],dst_y[1])

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_32_to_yuvj_422_p_c */

#define FUNC_NAME      bgr_32_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_YJ(src[6],src[5],src[4],dst_y[1])

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/****************************************************
 * Conversions to YUV 444 P
 ****************************************************/

/* rgb_15_to_yuvj_444_p_c */

#define FUNC_NAME      rgb_15_to_yuvj_444_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
RGB_TO_YUVJ(RGB15_TO_R(src[0]), \
           RGB15_TO_G(src[0]), \
           RGB15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_15_to_yuvj_444_p_c */

#define FUNC_NAME      bgr_15_to_yuvj_444_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
RGB_TO_YUVJ(BGR15_TO_R(src[0]), \
           BGR15_TO_G(src[0]), \
           BGR15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v)
// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_16_to_yuvj_444_p_c */

#define FUNC_NAME      rgb_16_to_yuvj_444_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
RGB_TO_YUVJ(RGB16_TO_R(src[0]), \
           RGB16_TO_G(src[0]), \
           RGB16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_16_to_yuvj_444_p_c */

#define FUNC_NAME      bgr_16_to_yuvj_444_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
RGB_TO_YUVJ(BGR16_TO_R(src[0]), \
           BGR16_TO_G(src[0]), \
           BGR16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_24_to_yuvj_444_p_c */

#define FUNC_NAME      rgb_24_to_yuvj_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     3
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_24_to_yuvj_444_p_c */

#define FUNC_NAME      bgr_24_to_yuvj_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     3
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgb_32_to_yuvj_444_p_c */

#define FUNC_NAME      rgb_32_to_yuvj_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* bgr_32_to_yuvj_444_p_c */

#define FUNC_NAME      bgr_32_to_yuvj_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v)

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"


/****************************************************
 * Conversions to YUV 420 P
 ****************************************************/

/* rgb_15_to_yuvj_420_p_c */

#define FUNC_NAME      rgb_15_to_yuvj_420_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUVJ(RGB15_TO_R(src[0]), \
           RGB15_TO_G(src[0]), \
           RGB15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_YJ(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1])

#define CONVERT_Y \
RGB_TO_YJ(RGB15_TO_R(src[0]), \
         RGB15_TO_G(src[0]), \
         RGB15_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_YJ(RGB15_TO_R(src[1]), \
         RGB15_TO_G(src[1]), \
         RGB15_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* bgr_15_to_yuvj_420_p_c */

#define FUNC_NAME      bgr_15_to_yuvj_420_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUVJ(BGR15_TO_R(src[0]), \
           BGR15_TO_G(src[0]), \
           BGR15_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_YJ(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1])

#define CONVERT_Y \
RGB_TO_YJ(BGR15_TO_R(src[0]), \
         BGR15_TO_G(src[0]), \
         BGR15_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_YJ(BGR15_TO_R(src[1]), \
         BGR15_TO_G(src[1]), \
         BGR15_TO_B(src[1]), \
         dst_y[1])
#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* rgb_16_to_yuvj_420_p_c */

#define FUNC_NAME      rgb_16_to_yuvj_420_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUVJ(RGB16_TO_R(src[0]), \
           RGB16_TO_G(src[0]), \
           RGB16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_YJ(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1])

#define CONVERT_Y    \
RGB_TO_YJ(RGB16_TO_R(src[0]), \
         RGB16_TO_G(src[0]), \
         RGB16_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_YJ(RGB16_TO_R(src[1]), \
         RGB16_TO_G(src[1]), \
         RGB16_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* bgr_16_to_yuvj_420_p_c */

#define FUNC_NAME      bgr_16_to_yuvj_420_p_c
#define IN_TYPE        uint16_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
RGB_TO_YUVJ(BGR16_TO_R(src[0]), \
           BGR16_TO_G(src[0]), \
           BGR16_TO_B(src[0]), \
           dst_y[0],*dst_u,*dst_v) \
RGB_TO_YJ(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1])

#define CONVERT_Y \
RGB_TO_YJ(BGR16_TO_R(src[0]), \
         BGR16_TO_G(src[0]), \
         BGR16_TO_B(src[0]), \
         dst_y[0]) \
RGB_TO_YJ(BGR16_TO_R(src[1]), \
         BGR16_TO_G(src[1]), \
         BGR16_TO_B(src[1]), \
         dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* rgb_24_to_yuvj_420_p_c */

#define FUNC_NAME      rgb_24_to_yuvj_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     6
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_YJ(src[3],src[4],src[5],dst_y[1])

#define CONVERT_Y      \
    RGB_TO_YJ(src[0],src[1],src[2],dst_y[0]) \
    RGB_TO_YJ(src[3],src[4],src[5],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* bgr_24_to_yuvj_420_p_c */

#define FUNC_NAME      bgr_24_to_yuvj_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     6
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_YJ(src[5],src[4],src[3],dst_y[1])

#define CONVERT_Y      \
    RGB_TO_YJ(src[2],src[1],src[0],dst_y[0]) \
    RGB_TO_YJ(src[5],src[4],src[3],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* rgb_32_to_yuvj_420_p_c */

#define FUNC_NAME      rgb_32_to_yuvj_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[0],src[1],src[2], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_YJ(src[4],src[5],src[6],dst_y[1])

#define CONVERT_Y      \
    RGB_TO_YJ(src[0],src[1],src[2], \
               dst_y[0]) \
    RGB_TO_YJ(src[4],src[5],src[6],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* bgr_32_to_yuvj_420_p_c */

#define FUNC_NAME      bgr_32_to_yuvj_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGB_TO_YUVJ(src[2],src[1],src[0], \
               dst_y[0],*dst_u,*dst_v) \
    RGB_TO_YJ(src[6],src[5],src[4],dst_y[1])

#define CONVERT_Y      \
    RGB_TO_YJ(src[2],src[1],src[0], \
             dst_y[0]) \
    RGB_TO_YJ(src[6],src[5],src[4],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* RGBA -> ... */

/* rgba_32_to_yuvj_422_p_c */

#define FUNC_NAME      rgba_32_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGBA_TO_YUVJ(src[0],src[1],src[2],src[3], \
               dst_y[0],*dst_u,*dst_v) \
    RGBA_TO_YJ(src[4],src[5],src[6],src[7],dst_y[1])
#define INIT   INIT_RGBA32

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"

/* rgba_32_to_yuvj_420_p_c */

#define FUNC_NAME      rgba_32_to_yuvj_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CONVERT_YUV    \
    RGBA_TO_YUVJ(src[0],src[1],src[2],src[3], \
               dst_y[0],*dst_u,*dst_v) \
    RGBA_TO_YJ(src[4],src[5],src[6],src[7],dst_y[1])
#define INIT   INIT_RGBA32

#define CONVERT_Y      \
    RGBA_TO_YJ(src[0],src[1],src[2],src[3], \
               dst_y[0]) \
    RGBA_TO_YJ(src[4],src[5],src[6],src[7],dst_y[1])

#define CHROMA_SUB     2

#include "../csp_packed_planar.h"

/* rgba_32_to_yuv_444_p_c */

#define FUNC_NAME      rgba_32_to_yuvj_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CONVERT_YUV    \
    RGBA_TO_YUVJ(src[0],src[1],src[2],src[3], \
               dst_y[0],*dst_u,*dst_v)
#define INIT   INIT_RGBA32

// #define CONVERT_Y      
#define CHROMA_SUB     1

#include "../csp_packed_planar.h"


#ifdef SCANLINE
void gavl_init_rgb_yuv_scanline_funcs_c(gavl_colorspace_function_table_t * tab)
#else     
void gavl_init_rgb_yuv_funcs_c(gavl_colorspace_function_table_t * tab)
#endif
  {
  _init_rgb_to_yuv_c();

  tab->rgb_15_to_yuy2 =      rgb_15_to_yuy2_c;
  tab->rgb_15_to_uyvy =      rgb_15_to_uyvy_c;
  tab->rgb_15_to_yuv_420_p = rgb_15_to_yuv_420_p_c;
  tab->rgb_15_to_yuv_410_p = rgb_15_to_yuv_410_p_c;
  tab->rgb_15_to_yuv_422_p = rgb_15_to_yuv_422_p_c;
  tab->rgb_15_to_yuv_411_p = rgb_15_to_yuv_411_p_c;
  tab->rgb_15_to_yuv_444_p = rgb_15_to_yuv_444_p_c;
  tab->rgb_15_to_yuvj_420_p = rgb_15_to_yuvj_420_p_c;
  tab->rgb_15_to_yuvj_422_p = rgb_15_to_yuvj_422_p_c;
  tab->rgb_15_to_yuvj_444_p = rgb_15_to_yuvj_444_p_c;
  
  tab->bgr_15_to_yuy2 =      bgr_15_to_yuy2_c;
  tab->bgr_15_to_uyvy =      bgr_15_to_uyvy_c;
  tab->bgr_15_to_yuv_420_p = bgr_15_to_yuv_420_p_c;
  tab->bgr_15_to_yuv_410_p = bgr_15_to_yuv_410_p_c;
  tab->bgr_15_to_yuv_422_p = bgr_15_to_yuv_422_p_c;
  tab->bgr_15_to_yuv_411_p = bgr_15_to_yuv_411_p_c;
  tab->bgr_15_to_yuv_444_p = bgr_15_to_yuv_444_p_c;
  tab->bgr_15_to_yuvj_420_p = bgr_15_to_yuvj_420_p_c;
  tab->bgr_15_to_yuvj_422_p = bgr_15_to_yuvj_422_p_c;
  tab->bgr_15_to_yuvj_444_p = bgr_15_to_yuvj_444_p_c;

  tab->rgb_16_to_yuy2 =      rgb_16_to_yuy2_c;
  tab->rgb_16_to_uyvy =      rgb_16_to_uyvy_c;
  tab->rgb_16_to_yuv_420_p = rgb_16_to_yuv_420_p_c;
  tab->rgb_16_to_yuv_410_p = rgb_16_to_yuv_410_p_c;
  tab->rgb_16_to_yuv_422_p = rgb_16_to_yuv_422_p_c;
  tab->rgb_16_to_yuv_411_p = rgb_16_to_yuv_411_p_c;
  tab->rgb_16_to_yuv_444_p = rgb_16_to_yuv_444_p_c;
  tab->rgb_16_to_yuvj_420_p = rgb_16_to_yuvj_420_p_c;
  tab->rgb_16_to_yuvj_422_p = rgb_16_to_yuvj_422_p_c;
  tab->rgb_16_to_yuvj_444_p = rgb_16_to_yuvj_444_p_c;

  tab->bgr_16_to_yuy2 =      bgr_16_to_yuy2_c;
  tab->bgr_16_to_uyvy =      bgr_16_to_uyvy_c;
  tab->bgr_16_to_yuv_420_p = bgr_16_to_yuv_420_p_c;
  tab->bgr_16_to_yuv_410_p = bgr_16_to_yuv_410_p_c;
  tab->bgr_16_to_yuv_422_p = bgr_16_to_yuv_422_p_c;
  tab->bgr_16_to_yuv_411_p = bgr_16_to_yuv_411_p_c;
  tab->bgr_16_to_yuv_444_p = bgr_16_to_yuv_444_p_c;
  tab->bgr_16_to_yuvj_420_p = bgr_16_to_yuvj_420_p_c;
  tab->bgr_16_to_yuvj_422_p = bgr_16_to_yuvj_422_p_c;
  tab->bgr_16_to_yuvj_444_p = bgr_16_to_yuvj_444_p_c;

  tab->rgb_24_to_yuy2 =      rgb_24_to_yuy2_c;
  tab->rgb_24_to_uyvy =      rgb_24_to_uyvy_c;
  tab->rgb_24_to_yuv_420_p = rgb_24_to_yuv_420_p_c;
  tab->rgb_24_to_yuv_410_p = rgb_24_to_yuv_410_p_c;
  tab->rgb_24_to_yuv_422_p = rgb_24_to_yuv_422_p_c;
  tab->rgb_24_to_yuv_411_p = rgb_24_to_yuv_411_p_c;
  tab->rgb_24_to_yuv_444_p = rgb_24_to_yuv_444_p_c;
  tab->rgb_24_to_yuvj_420_p = rgb_24_to_yuvj_420_p_c;
  tab->rgb_24_to_yuvj_422_p = rgb_24_to_yuvj_422_p_c;
  tab->rgb_24_to_yuvj_444_p = rgb_24_to_yuvj_444_p_c;

  tab->bgr_24_to_yuy2 =      bgr_24_to_yuy2_c;
  tab->bgr_24_to_uyvy =      bgr_24_to_uyvy_c;
  tab->bgr_24_to_yuv_420_p = bgr_24_to_yuv_420_p_c;
  tab->bgr_24_to_yuv_410_p = bgr_24_to_yuv_410_p_c;
  tab->bgr_24_to_yuv_422_p = bgr_24_to_yuv_422_p_c;
  tab->bgr_24_to_yuv_411_p = bgr_24_to_yuv_411_p_c;
  tab->bgr_24_to_yuv_444_p = bgr_24_to_yuv_444_p_c;
  tab->bgr_24_to_yuvj_420_p = bgr_24_to_yuvj_420_p_c;
  tab->bgr_24_to_yuvj_422_p = bgr_24_to_yuvj_422_p_c;
  tab->bgr_24_to_yuvj_444_p = bgr_24_to_yuvj_444_p_c;

  tab->rgb_32_to_yuy2 =      rgb_32_to_yuy2_c;
  tab->rgb_32_to_uyvy =      rgb_32_to_uyvy_c;
  tab->rgb_32_to_yuv_420_p = rgb_32_to_yuv_420_p_c;
  tab->rgb_32_to_yuv_410_p = rgb_32_to_yuv_410_p_c;
  tab->rgb_32_to_yuv_422_p = rgb_32_to_yuv_422_p_c;
  tab->rgb_32_to_yuv_411_p = rgb_32_to_yuv_411_p_c;
  tab->rgb_32_to_yuv_444_p = rgb_32_to_yuv_444_p_c;
  tab->rgb_32_to_yuvj_420_p = rgb_32_to_yuvj_420_p_c;
  tab->rgb_32_to_yuvj_422_p = rgb_32_to_yuvj_422_p_c;
  tab->rgb_32_to_yuvj_444_p = rgb_32_to_yuvj_444_p_c;

  tab->bgr_32_to_yuy2 =      bgr_32_to_yuy2_c;
  tab->bgr_32_to_uyvy =      bgr_32_to_uyvy_c;
  tab->bgr_32_to_yuv_420_p = bgr_32_to_yuv_420_p_c;
  tab->bgr_32_to_yuv_410_p = bgr_32_to_yuv_410_p_c;
  tab->bgr_32_to_yuv_422_p = bgr_32_to_yuv_422_p_c;
  tab->bgr_32_to_yuv_411_p = bgr_32_to_yuv_411_p_c;
  tab->bgr_32_to_yuv_444_p = bgr_32_to_yuv_444_p_c;
  tab->bgr_32_to_yuvj_420_p = bgr_32_to_yuvj_420_p_c;
  tab->bgr_32_to_yuvj_422_p = bgr_32_to_yuvj_422_p_c;
  tab->bgr_32_to_yuvj_444_p = bgr_32_to_yuvj_444_p_c;

  tab->rgba_32_to_yuy2 =      rgba_32_to_yuy2_c;
  tab->rgba_32_to_uyvy =      rgba_32_to_uyvy_c;
  tab->rgba_32_to_yuv_420_p = rgba_32_to_yuv_420_p_c;
  tab->rgba_32_to_yuv_410_p = rgba_32_to_yuv_410_p_c;
  tab->rgba_32_to_yuv_422_p = rgba_32_to_yuv_422_p_c;
  tab->rgba_32_to_yuv_411_p = rgba_32_to_yuv_411_p_c;
  tab->rgba_32_to_yuv_444_p = rgba_32_to_yuv_444_p_c;
  tab->rgba_32_to_yuvj_420_p = rgba_32_to_yuvj_420_p_c;
  tab->rgba_32_to_yuvj_422_p = rgba_32_to_yuvj_422_p_c;
  tab->rgba_32_to_yuvj_444_p = rgba_32_to_yuvj_444_p_c;
  }
