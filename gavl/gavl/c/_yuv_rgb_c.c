/*****************************************************************

  _yuv_rgb_c.c

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

/* Standard quantization */

static int y_to_rgb[0x100];
static int v_to_r[0x100];
static int u_to_g[0x100];
static int v_to_g[0x100];
static int u_to_b[0x100];

/* JPEG Quantization */

static int yj_to_rgb[0x100];
static int vj_to_r[0x100];
static int uj_to_g[0x100];
static int vj_to_g[0x100];
static int uj_to_b[0x100];

static int yuv2rgb_tables_initialized = 0;

static void _init_yuv2rgb_c()
  {
  int i;
  if(yuv2rgb_tables_initialized)
    return;
  yuv2rgb_tables_initialized = 1;
  
  for(i = 0; i < 0x100; i++)
    {
    // YCbCr -> R'G'B' according to CCIR 601
    
    y_to_rgb[i] = (int)(255.0/219.0*(i-16)) * 0x10000;
    
    v_to_r[i]   = (int)( 1.40200*255.0/224.0 * (i - 0x80) * 0x10000);
    u_to_g[i]   = (int)(-0.34414*255.0/224.0 * (i - 0x80) * 0x10000);
    v_to_g[i]   = (int)(-0.71414*255.0/224.0 * (i - 0x80) * 0x10000);
    u_to_b[i]   = (int)( 1.77200*255.0/224.0 * (i - 0x80) * 0x10000);

    /* JPEG Quantization */
    yj_to_rgb[i] = (int)(i * 0x10000);
    
    vj_to_r[i]   = (int)( 1.40200 * (i - 0x80) * 0x10000);
    uj_to_g[i]   = (int)(-0.34414 * (i - 0x80) * 0x10000);
    vj_to_g[i]   = (int)(-0.71414 * (i - 0x80) * 0x10000);
    uj_to_b[i]   = (int)( 1.77200 * (i - 0x80) * 0x10000);
    }
  }

#define RECLIP(color) (uint8_t)((color>0xFF)?0xff:((color<0)?0:color))

#define YUV_2_RGB(y,u,v,r,g,b) i_tmp=(y_to_rgb[y]+v_to_r[v])>>16;\
                               r=RECLIP(i_tmp);\
                               i_tmp=(y_to_rgb[y]+u_to_g[u]+v_to_g[v])>>16;\
                               g=RECLIP(i_tmp);\
                               i_tmp=(y_to_rgb[y]+u_to_b[u])>>16;\
                               b=RECLIP(i_tmp);

#define YUVJ_2_RGB(y,u,v,r,g,b) i_tmp=(yj_to_rgb[y]+vj_to_r[v])>>16;\
                                r=RECLIP(i_tmp);\
                                i_tmp=(yj_to_rgb[y]+uj_to_g[u]+vj_to_g[v])>>16;\
                                g=RECLIP(i_tmp);\
                                i_tmp=(yj_to_rgb[y]+uj_to_b[u])>>16;\
                                b=RECLIP(i_tmp);

#define PACK_RGB16(r,g,b,pixel) pixel=((((((r<<5)&0xff00)|g)<<6)&0xfff00)|b)>>3
#define PACK_BGR16(r,g,b,pixel) pixel=((((((b<<5)&0xff00)|g)<<6)&0xfff00)|r)>>3

#define PACK_RGB15(r,g,b,pixel) pixel=((((((r<<5)&0xff00)|g)<<5)&0xfff00)|b)>>3
#define PACK_BGR15(r,g,b,pixel) pixel=((((((b<<5)&0xff00)|g)<<5)&0xfff00)|r)>>3

/* YUY2 -> */

/* yuy2_to_rgb_15_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 2
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_rgb_15_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], r, g, b) \
  PACK_RGB15(r, g, b, dst[0]); \
  YUV_2_RGB(src[2], src[1], src[3], r, g, b) \
  PACK_RGB15(r, g, b, dst[1]);

#define INIT \
  uint8_t r; \
  uint8_t g; \
  uint8_t b; \
  int i_tmp;

#include "../csp_packed_packed.h"

/* yuy2_to_bgr_15_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 2
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_bgr_15_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], r, g, b) \
  PACK_BGR15(r, g, b, dst[0]); \
  YUV_2_RGB(src[2], src[1], src[3], r, g, b) \
  PACK_BGR15(r, g, b, dst[1]);

#define INIT \
  uint8_t r; \
  uint8_t g; \
  uint8_t b; \
  int i_tmp;

#include "../csp_packed_packed.h"


/* yuy2_to_rgb_16_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 2
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_rgb_16_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], r, g, b) \
  PACK_RGB16(r, g, b, dst[0]); \
  YUV_2_RGB(src[2], src[1], src[3], r, g, b) \
  PACK_RGB16(r, g, b, dst[1]);

#define INIT \
  uint8_t r; \
  uint8_t g; \
  uint8_t b; \
  int i_tmp;

#include "../csp_packed_packed.h"

/* yuy2_to_bgr_16_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 2
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_bgr_16_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], r, g, b) \
  PACK_BGR16(r, g, b, dst[0]); \
  YUV_2_RGB(src[2], src[1], src[3], r, g, b) \
  PACK_BGR16(r, g, b, dst[1]);

#define INIT \
  uint8_t r; \
  uint8_t g; \
  uint8_t b; \
  int i_tmp;

#include "../csp_packed_packed.h"

/* yuy2_to_rgb_24_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 6
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_rgb_24_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], dst[0], dst[1], dst[2]) \
  YUV_2_RGB(src[2], src[1], src[3], dst[3], dst[4], dst[5])

#define INIT \
  int i_tmp;

#include "../csp_packed_packed.h"

/* yuy2_to_rgb_24_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 6
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_bgr_24_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], dst[2], dst[1], dst[0]) \
  YUV_2_RGB(src[2], src[1], src[3], dst[5], dst[4], dst[3])

#define INIT \
  int i_tmp;

#include "../csp_packed_packed.h"

/* yuy2_to_rgb_32_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 8
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_rgb_32_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], dst[0], dst[1], dst[2]) \
  YUV_2_RGB(src[2], src[1], src[3], dst[4], dst[5], dst[6])

#define INIT \
  int i_tmp;

#include "../csp_packed_packed.h"

/* yuy2_to_bgr_32_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 8
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_bgr_32_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], dst[2], dst[1], dst[0]) \
  YUV_2_RGB(src[2], src[1], src[3], dst[6], dst[5], dst[4])

#define INIT \
  int i_tmp;

#include "../csp_packed_packed.h"

/* yuy2_to_rgba_32_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 8
#define NUM_PIXELS  2
#define FUNC_NAME yuy2_to_rgba_32_c
#define CONVERT  \
  YUV_2_RGB(src[0], src[1], src[3], dst[0], dst[1], dst[2]) \
  dst[3] = 0xFF;\
  YUV_2_RGB(src[2], src[1], src[3], dst[4], dst[5], dst[6]) \
  dst[7] = 0xFF;

#define INIT \
  int i_tmp;

#include "../csp_packed_packed.h"

/*************************************************
  YUV420P ->
 *************************************************/

/* yuv_420_p_to_rgb_15_c */

#define FUNC_NAME yuv_420_p_to_rgb_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[0]); \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_420_p_to_bgr_15_c */

#define FUNC_NAME yuv_420_p_to_bgr_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[0]); \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_420_p_to_rgb_16_c */

#define FUNC_NAME yuv_420_p_to_rgb_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[0]); \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_420_p_to_bgr_16_c */

#define FUNC_NAME yuv_420_p_to_bgr_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[0]); \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_420_p_to_rgb_24_c */

#define FUNC_NAME yuv_420_p_to_rgb_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   6
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])\
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[3], dst[4], dst[5])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_420_p_to_bgr_24_c */

#define FUNC_NAME yuv_420_p_to_bgr_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   6
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0]) \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[5], dst[4], dst[3])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_420_p_to_rgb_32_c */

#define FUNC_NAME yuv_420_p_to_rgb_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_420_p_to_bgr_32_c */

#define FUNC_NAME yuv_420_p_to_bgr_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0]) \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[6], dst[5], dst[4])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_420_p_to_rgba_32_c */

#define FUNC_NAME yuv_420_p_to_rgba_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  dst[3] = 0xff;\
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6]) \
  dst[7] = 0xff;

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/*************************************************
  YUV422P ->
 *************************************************/

/* yuv_422_p_to_rgb_15_c */

#define FUNC_NAME yuv_422_p_to_rgb_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[0]); \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_bgr_15_c */

#define FUNC_NAME yuv_422_p_to_bgr_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[0]); \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_rgb_16_c */

#define FUNC_NAME yuv_422_p_to_rgb_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[0]); \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_bgr_16_c */

#define FUNC_NAME yuv_422_p_to_bgr_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[0]); \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_rgb_24_c */

#define FUNC_NAME yuv_422_p_to_rgb_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   6
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])\
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[3], dst[4], dst[5])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_bgr_24_c */

#define FUNC_NAME yuv_422_p_to_bgr_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   6
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0]) \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[5], dst[4], dst[3])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_rgb_32_c */

#define FUNC_NAME yuv_422_p_to_rgb_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_bgr_32_c */

#define FUNC_NAME yuv_422_p_to_bgr_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0]) \
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[6], dst[5], dst[4])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_rgba_32_c */

#define FUNC_NAME yuv_422_p_to_rgba_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  dst[3] = 0xff;\
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6]) \
  dst[7] = 0xff;

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/*************************************************
  YUV444P ->
 *************************************************/

/* yuv_444_p_to_rgb_15_c */

#define FUNC_NAME yuv_444_p_to_rgb_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   1
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[0]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_bgr_15_c */

#define FUNC_NAME yuv_444_p_to_bgr_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   1
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[0]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_rgb_16_c */

#define FUNC_NAME yuv_444_p_to_rgb_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   1
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[0]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_bgr_16_c */

#define FUNC_NAME yuv_444_p_to_bgr_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   1
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[0]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_rgb_24_c */

#define FUNC_NAME yuv_444_p_to_rgb_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   3
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_bgr_24_c */

#define FUNC_NAME yuv_444_p_to_bgr_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   3
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_rgb_32_c */

#define FUNC_NAME yuv_444_p_to_rgb_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_bgr_32_c */

#define FUNC_NAME yuv_444_p_to_bgr_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_rgba_32_c */

#define FUNC_NAME yuv_444_p_to_rgba_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  dst[3] = 0xff;

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"


/* JPEG */

/*************************************************
  YUVJ 420 P ->
 *************************************************/

/* yuvj_420_p_to_rgb_15_c */

#define FUNC_NAME yuvj_420_p_to_rgb_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[0]); \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_bgr_15_c */

#define FUNC_NAME yuvj_420_p_to_bgr_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[0]); \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_rgb_16_c */

#define FUNC_NAME yuvj_420_p_to_rgb_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[0]); \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_bgr_16_c */

#define FUNC_NAME yuvj_420_p_to_bgr_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[0]); \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_rgb_24_c */

#define FUNC_NAME yuvj_420_p_to_rgb_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   6
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])\
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[3], dst[4], dst[5])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_bgr_24_c */

#define FUNC_NAME yuvj_420_p_to_bgr_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   6
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0]) \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[5], dst[4], dst[3])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_rgb_32_c */

#define FUNC_NAME yuvj_420_p_to_rgb_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_bgr_32_c */

#define FUNC_NAME yuvj_420_p_to_bgr_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0]) \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[6], dst[5], dst[4])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_rgba_32_c */

#define FUNC_NAME yuvj_420_p_to_rgba_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  dst[3] = 0xff;\
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6]) \
  dst[7] = 0xff;

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/*************************************************
  YUVJ 422 P ->
 *************************************************/

/* yuvj_422_p_to_rgb_15_c */

#define FUNC_NAME yuvj_422_p_to_rgb_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[0]); \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_bgr_15_c */

#define FUNC_NAME yuvj_422_p_to_bgr_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[0]); \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_rgb_16_c */

#define FUNC_NAME yuvj_422_p_to_rgb_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[0]); \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_bgr_16_c */

#define FUNC_NAME yuvj_422_p_to_bgr_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   2
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[0]); \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[1]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_rgb_24_c */

#define FUNC_NAME yuvj_422_p_to_rgb_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   6
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])\
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[3], dst[4], dst[5])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_bgr_24_c */

#define FUNC_NAME yuvj_422_p_to_bgr_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   6
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0]) \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[5], dst[4], dst[3])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_rgb_32_c */

#define FUNC_NAME yuvj_422_p_to_rgb_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_bgr_32_c */

#define FUNC_NAME yuvj_422_p_to_bgr_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0]) \
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[6], dst[5], dst[4])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_rgba_32_c */

#define FUNC_NAME yuvj_422_p_to_rgba_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  dst[3] = 0xff;\
  YUVJ_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6]) \
  dst[7] = 0xff;

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/*************************************************
  YUVJ 444 P ->
 *************************************************/

/* yuvj_444_p_to_rgb_15_c */

#define FUNC_NAME yuvj_444_p_to_rgb_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   1
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB15(r, g, b, dst[0]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_bgr_15_c */

#define FUNC_NAME yuvj_444_p_to_bgr_15_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   1
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR15(r, g, b, dst[0]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_rgb_16_c */

#define FUNC_NAME yuvj_444_p_to_rgb_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   1
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_RGB16(r, g, b, dst[0]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_bgr_16_c */

#define FUNC_NAME yuvj_444_p_to_bgr_16_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint16_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   1
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b) \
  PACK_BGR16(r, g, b, dst[0]);

#define INIT   int i_tmp; \
  uint8_t r; \
  uint8_t g; \
  uint8_t b;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_rgb_24_c */

#define FUNC_NAME yuvj_444_p_to_rgb_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   3
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_bgr_24_c */

#define FUNC_NAME yuvj_444_p_to_bgr_24_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   3
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])


#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_rgb_32_c */

#define FUNC_NAME yuvj_444_p_to_rgb_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_bgr_32_c */

#define FUNC_NAME yuvj_444_p_to_bgr_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_rgba_32_c */

#define FUNC_NAME yuvj_444_p_to_rgba_32_c
#define IN_TYPE uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT \
  YUVJ_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2]) \
  dst[3] = 0xff;

#define INIT   int i_tmp;

#include "../csp_planar_packed.h"


#ifdef SCANLINE
void gavl_init_yuv_rgb_scanline_funcs_c(gavl_colorspace_function_table_t * tab)
#else
void gavl_init_yuv_rgb_funcs_c(gavl_colorspace_function_table_t * tab)
#endif
  {
  _init_yuv2rgb_c();
  
  tab->yuy2_to_rgb_15 = yuy2_to_rgb_15_c;
  tab->yuy2_to_bgr_15 = yuy2_to_bgr_15_c;
  tab->yuy2_to_rgb_16 = yuy2_to_rgb_16_c;
  tab->yuy2_to_bgr_16 = yuy2_to_bgr_16_c;
  tab->yuy2_to_rgb_24 = yuy2_to_rgb_24_c;
  tab->yuy2_to_bgr_24 = yuy2_to_bgr_24_c;
  tab->yuy2_to_rgb_32 = yuy2_to_rgb_32_c;
  tab->yuy2_to_bgr_32 = yuy2_to_bgr_32_c;
  tab->yuy2_to_rgba_32 = yuy2_to_rgba_32_c;
  
  tab->yuv_420_p_to_rgb_15 = yuv_420_p_to_rgb_15_c;
  tab->yuv_420_p_to_bgr_15 = yuv_420_p_to_bgr_15_c;
  tab->yuv_420_p_to_rgb_16 = yuv_420_p_to_rgb_16_c;
  tab->yuv_420_p_to_bgr_16 = yuv_420_p_to_bgr_16_c;
  tab->yuv_420_p_to_rgb_24 = yuv_420_p_to_rgb_24_c;
  tab->yuv_420_p_to_bgr_24 = yuv_420_p_to_bgr_24_c;
  tab->yuv_420_p_to_rgb_32 = yuv_420_p_to_rgb_32_c;
  tab->yuv_420_p_to_bgr_32 = yuv_420_p_to_bgr_32_c;
  tab->yuv_420_p_to_rgba_32 = yuv_420_p_to_rgba_32_c;

  tab->yuv_422_p_to_rgb_15 = yuv_422_p_to_rgb_15_c;
  tab->yuv_422_p_to_bgr_15 = yuv_422_p_to_bgr_15_c;
  tab->yuv_422_p_to_rgb_16 = yuv_422_p_to_rgb_16_c;
  tab->yuv_422_p_to_bgr_16 = yuv_422_p_to_bgr_16_c;
  tab->yuv_422_p_to_rgb_24 = yuv_422_p_to_rgb_24_c;
  tab->yuv_422_p_to_bgr_24 = yuv_422_p_to_bgr_24_c;
  tab->yuv_422_p_to_rgb_32 = yuv_422_p_to_rgb_32_c;
  tab->yuv_422_p_to_bgr_32 = yuv_422_p_to_bgr_32_c;
  tab->yuv_422_p_to_rgba_32 = yuv_422_p_to_rgba_32_c;

  tab->yuv_444_p_to_rgb_15 = yuv_444_p_to_rgb_15_c;
  tab->yuv_444_p_to_bgr_15 = yuv_444_p_to_bgr_15_c;
  tab->yuv_444_p_to_rgb_16 = yuv_444_p_to_rgb_16_c;
  tab->yuv_444_p_to_bgr_16 = yuv_444_p_to_bgr_16_c;
  tab->yuv_444_p_to_rgb_24 = yuv_444_p_to_rgb_24_c;
  tab->yuv_444_p_to_bgr_24 = yuv_444_p_to_bgr_24_c;
  tab->yuv_444_p_to_rgb_32 = yuv_444_p_to_rgb_32_c;
  tab->yuv_444_p_to_bgr_32 = yuv_444_p_to_bgr_32_c;
  tab->yuv_444_p_to_rgba_32 = yuv_444_p_to_rgba_32_c;

  tab->yuvj_420_p_to_rgb_15 = yuvj_420_p_to_rgb_15_c;
  tab->yuvj_420_p_to_bgr_15 = yuvj_420_p_to_bgr_15_c;
  tab->yuvj_420_p_to_rgb_16 = yuvj_420_p_to_rgb_16_c;
  tab->yuvj_420_p_to_bgr_16 = yuvj_420_p_to_bgr_16_c;
  tab->yuvj_420_p_to_rgb_24 = yuvj_420_p_to_rgb_24_c;
  tab->yuvj_420_p_to_bgr_24 = yuvj_420_p_to_bgr_24_c;
  tab->yuvj_420_p_to_rgb_32 = yuvj_420_p_to_rgb_32_c;
  tab->yuvj_420_p_to_bgr_32 = yuvj_420_p_to_bgr_32_c;
  tab->yuvj_420_p_to_rgba_32 = yuvj_420_p_to_rgba_32_c;

  tab->yuvj_422_p_to_rgb_15 = yuvj_422_p_to_rgb_15_c;
  tab->yuvj_422_p_to_bgr_15 = yuvj_422_p_to_bgr_15_c;
  tab->yuvj_422_p_to_rgb_16 = yuvj_422_p_to_rgb_16_c;
  tab->yuvj_422_p_to_bgr_16 = yuvj_422_p_to_bgr_16_c;
  tab->yuvj_422_p_to_rgb_24 = yuvj_422_p_to_rgb_24_c;
  tab->yuvj_422_p_to_bgr_24 = yuvj_422_p_to_bgr_24_c;
  tab->yuvj_422_p_to_rgb_32 = yuvj_422_p_to_rgb_32_c;
  tab->yuvj_422_p_to_bgr_32 = yuvj_422_p_to_bgr_32_c;
  tab->yuvj_422_p_to_rgba_32 = yuvj_422_p_to_rgba_32_c;

  tab->yuvj_444_p_to_rgb_15 = yuvj_444_p_to_rgb_15_c;
  tab->yuvj_444_p_to_bgr_15 = yuvj_444_p_to_bgr_15_c;
  tab->yuvj_444_p_to_rgb_16 = yuvj_444_p_to_rgb_16_c;
  tab->yuvj_444_p_to_bgr_16 = yuvj_444_p_to_bgr_16_c;
  tab->yuvj_444_p_to_rgb_24 = yuvj_444_p_to_rgb_24_c;
  tab->yuvj_444_p_to_bgr_24 = yuvj_444_p_to_bgr_24_c;
  tab->yuvj_444_p_to_rgb_32 = yuvj_444_p_to_rgb_32_c;
  tab->yuvj_444_p_to_bgr_32 = yuvj_444_p_to_bgr_32_c;
  tab->yuvj_444_p_to_rgba_32 = yuvj_444_p_to_rgba_32_c;


  }

#undef RECLIP

#undef YUV_2_RGB
#undef PACK_RGB16
#undef PACK_BGR16

#undef PACK_RGB15
#undef PACK_BGR15
