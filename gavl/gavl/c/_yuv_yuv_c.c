/*****************************************************************

  _yuv_yuv_c.c

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

#include "c_macros.h"

#include "../_video_copy.c"

/*****************************************************
 *
 * C YUV <-> YUV Conversions
 *
 ******************************************************/

/* YUY2 <-> UYVY */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define FUNC_NAME   uyvy_to_yuy2_c
#define CONVERT     dst[0] = src[1];\
                    dst[1] = src[0];\
                    dst[2] = src[3];\
                    dst[3] = src[2];\

#include "../csp_packed_packed.h"

/* YUY2 -> */

/* yuy2_to_yuv_420_p_c */

#define FUNC_NAME      yuy2_to_yuv_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     2
#define CONVERT_YUV \
    dst_y[0] = src[0];\
    *dst_u   = src[1];\
    dst_y[1] = src[2];\
    *dst_v   = src[3];

#define CONVERT_Y \
    dst_y[0] = src[0];\
    dst_y[1] = src[2];

#include "../csp_packed_planar.h"

/* yuy2_to_yuv_410_p_c */

#define FUNC_NAME      yuy2_to_yuv_410_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB     4
#define CONVERT_YUV \
    dst_y[0] = src[0];\
    *dst_u   = src[1];\
    dst_y[1] = src[2];\
    *dst_v   = src[3];\
    dst_y[2] = src[4];\
    dst_y[3] = src[6];

#define CONVERT_Y \
    dst_y[0] = src[0];\
    dst_y[1] = src[2];\
    dst_y[2] = src[4];\
    dst_y[3] = src[6];

#include "../csp_packed_planar.h"

/* yuy2_to_yuv_422_p_c */

#define FUNC_NAME      yuy2_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = src[0];\
    *dst_u   = src[1];\
    dst_y[1] = src[2];\
    *dst_v   = src[3];

#include "../csp_packed_planar.h"

/* yuy2_to_yuv_422_p_16_c */

#define FUNC_NAME      yuy2_to_yuv_422_p_16_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_16(src[0]);                 \
  *dst_u   = UV_8_TO_16(src[1]);                \
  dst_y[1] = Y_8_TO_16(src[2]);                 \
  *dst_v   = UV_8_TO_16(src[3]);

#include "../csp_packed_planar.h"


/* yuy2_to_yuv_411_p_c */

#define FUNC_NAME      yuy2_to_yuv_411_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = src[0];\
    *dst_u   = src[1];\
    dst_y[1] = src[2];\
    *dst_v   = src[3];\
    dst_y[2] = src[4];\
    dst_y[3] = src[6];

#include "../csp_packed_planar.h"

/* yuy2_to_yuv_444_p_c */

#define FUNC_NAME      yuy2_to_yuv_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = src[0]; \
    dst_u[0] = src[1]; \
    dst_v[0] = src[3]; \
    dst_y[1] = src[2]; \
    dst_u[1] = src[1]; \
    dst_v[1] = src[3];

#include "../csp_packed_planar.h"

/* yuy2_to_yuv_444_p_16_c */

#define FUNC_NAME      yuy2_to_yuv_444_p_16_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_16(src[0]);                 \
  dst_u[0] = UV_8_TO_16(src[1]);                \
  dst_v[0] = UV_8_TO_16(src[3]);                \
  dst_y[1] = Y_8_TO_16(src[2]);               \
  dst_u[1] = UV_8_TO_16(src[1]);                \
  dst_v[1] = UV_8_TO_16(src[3]);

#include "../csp_packed_planar.h"

/* yuy2_to_yuvj_420_p_c */

#define FUNC_NAME      yuy2_to_yuvj_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     2
#define CONVERT_YUV \
    dst_y[0] = Y_8_TO_YJ_8(src[0]);\
    *dst_u   = UV_8_TO_UVJ_8(src[1]);\
    dst_y[1] = Y_8_TO_YJ_8(src[2]);\
    *dst_v   = UV_8_TO_UVJ_8(src[3]);

#define CONVERT_Y \
    dst_y[0] = Y_8_TO_YJ_8(src[0]);\
    dst_y[1] = Y_8_TO_YJ_8(src[2]);

#include "../csp_packed_planar.h"

/* yuy2_to_yuvj_422_p_c */

#define FUNC_NAME      yuy2_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = Y_8_TO_YJ_8(src[0]);\
    *dst_u   = UV_8_TO_UVJ_8(src[1]);\
    dst_y[1] = Y_8_TO_YJ_8(src[2]);\
    *dst_v   = UV_8_TO_UVJ_8(src[3]);

#include "../csp_packed_planar.h"

/* yuy2_to_yuvj_444_p_c */

#define FUNC_NAME      yuy2_to_yuvj_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = Y_8_TO_YJ_8(src[0]); \
    dst_u[0] = UV_8_TO_UVJ_8(src[1]); \
    dst_v[0] = UV_8_TO_UVJ_8(src[3]); \
    dst_y[1] = Y_8_TO_YJ_8(src[2]); \
    dst_u[1] = UV_8_TO_UVJ_8(src[1]); \
    dst_v[1] = UV_8_TO_UVJ_8(src[3]);

#include "../csp_packed_planar.h"


/* UYVY -> */

/* uyvy_to_yuv_420_p_c */

#define FUNC_NAME      uyvy_to_yuv_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     2
#define CONVERT_YUV \
    dst_y[0] = src[1];\
    *dst_u   = src[0];\
    dst_y[1] = src[3];\
    *dst_v   = src[2];

#define CONVERT_Y \
    dst_y[0] = src[1];\
    dst_y[1] = src[3];

#include "../csp_packed_planar.h"

/* uyvy_to_yuv_410_p_c */

#define FUNC_NAME      uyvy_to_yuv_410_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB     4
#define CONVERT_YUV \
    dst_y[0] = src[1];\
    *dst_u   = src[0];\
    dst_y[1] = src[3];\
    *dst_v   = src[2];\
    dst_y[2] = src[5];\
    dst_y[3] = src[7];\


#define CONVERT_Y \
    dst_y[0] = src[1];\
    dst_y[1] = src[3];\
    dst_y[2] = src[5];\
    dst_y[3] = src[7];

#include "../csp_packed_planar.h"

/* uyvy_to_yuv_422_p_c */

#define FUNC_NAME      uyvy_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = src[1];\
    *dst_u   = src[0];\
    dst_y[1] = src[3];\
    *dst_v   = src[2];

#include "../csp_packed_planar.h"

/* uyvy_to_yuv_422_p_16_c */

#define FUNC_NAME      uyvy_to_yuv_422_p_16_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_16(src[1]);                 \
  *dst_u   = UV_8_TO_16(src[0]);                 \
  dst_y[1] = Y_8_TO_16(src[3]);                 \
  *dst_v   = UV_8_TO_16(src[2]);

#include "../csp_packed_planar.h"


/* uyvy_to_yuv_411_p_c */

#define FUNC_NAME      uyvy_to_yuv_411_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = src[1];\
    *dst_u   = src[0];\
    dst_y[1] = src[3];\
    *dst_v   = src[2];\
    dst_y[2] = src[5];\
    dst_y[3] = src[7];

#include "../csp_packed_planar.h"

/* uyvy_to_yuv_444_p_c */

#define FUNC_NAME      uyvy_to_yuv_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = src[1]; \
    dst_u[0] = src[0]; \
    dst_v[0] = src[2]; \
    dst_y[1] = src[3]; \
    dst_u[1] = src[0]; \
    dst_v[1] = src[2];

#include "../csp_packed_planar.h"

/* uyvy_to_yuv_444_p_16_c */

#define FUNC_NAME      uyvy_to_yuv_444_p_16_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_16(src[1]);                  \
  dst_u[0] = UV_8_TO_16(src[0]);                 \
  dst_v[0] = UV_8_TO_16(src[2]);                 \
  dst_y[1] = Y_8_TO_16(src[3]);                  \
  dst_u[1] = UV_8_TO_16(src[0]);                 \
  dst_v[1] = UV_8_TO_16(src[2]);

#include "../csp_packed_planar.h"


/* uyvy_to_yuvj_420_p_c */

#define FUNC_NAME      uyvy_to_yuvj_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     2
#define CONVERT_YUV \
    dst_y[0] = Y_8_TO_YJ_8(src[1]);\
    *dst_u   = UV_8_TO_UVJ_8(src[0]);\
    dst_y[1] = Y_8_TO_YJ_8(src[3]);\
    *dst_v   = UV_8_TO_UVJ_8(src[2]);

#define CONVERT_Y \
    dst_y[0] = Y_8_TO_YJ_8(src[1]);\
    dst_y[1] = Y_8_TO_YJ_8(src[3]);

#include "../csp_packed_planar.h"

/* uyvy_to_yuvj_422_p_c */

#define FUNC_NAME      uyvy_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = Y_8_TO_YJ_8(src[1]);\
    *dst_u   = UV_8_TO_UVJ_8(src[0]);\
    dst_y[1] = Y_8_TO_YJ_8(src[3]);\
    *dst_v   = UV_8_TO_UVJ_8(src[2]);

#include "../csp_packed_planar.h"

/* uyvy_to_yuvj_444_p_c */

#define FUNC_NAME      uyvy_to_yuvj_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
    dst_y[0] = Y_8_TO_YJ_8(src[1]); \
    dst_u[0] = UV_8_TO_UVJ_8(src[0]); \
    dst_v[0] = UV_8_TO_UVJ_8(src[2]); \
    dst_y[1] = Y_8_TO_YJ_8(src[3]); \
    dst_u[1] = UV_8_TO_UVJ_8(src[0]); \
    dst_v[1] = UV_8_TO_UVJ_8(src[2]);

#include "../csp_packed_planar.h"

/* -> YUY2 */

/* yuv_420_p_to_yuy2_c */

#define FUNC_NAME     yuv_420_p_to_yuy2_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT       \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = src_y[1];\
    dst[3] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_410_p_to_yuy2_c */

#define FUNC_NAME     yuv_410_p_to_yuy2_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  4
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    4
#define CHROMA_SUB    4
#define CONVERT       \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = src_y[1];\
    dst[3] = *src_v;\
    dst[4] = src_y[2];\
    dst[5] = *src_u;\
    dst[6] = src_y[3];\
    dst[7] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_yuy2_c */

#define FUNC_NAME     yuv_422_p_to_yuy2_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = src_y[1];\
    dst[3] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_422_p_16_to_yuy2_c */

#define FUNC_NAME     yuv_422_p_16_to_yuy2_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
  dst[0] = Y_16_TO_Y_8(src_y[0]);                 \
  dst[1] = UV_16_TO_UV_8(*src_u);                  \
  dst[2] = Y_16_TO_Y_8(src_y[1]);                 \
  dst[3] = UV_16_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"

/* yuv_411_p_to_yuy2_c */

#define FUNC_NAME     yuv_411_p_to_yuy2_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  4
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    4
#define CHROMA_SUB    1
#define CONVERT       \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = src_y[1];\
    dst[3] = *src_v;\
    dst[4] = src_y[2];\
    dst[5] = *src_u;\
    dst[6] = src_y[3];\
    dst[7] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_444_p_to_yuy2_c */

#define FUNC_NAME     yuv_444_p_to_yuy2_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 2
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = src_y[1];\
    dst[3] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_444_p_16_to_yuy2_c */

#define FUNC_NAME     yuv_444_p_16_to_yuy2_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 2
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
  dst[0] = Y_16_TO_Y_8(src_y[0]);                 \
  dst[1] = UV_16_TO_UV_8(*src_u);                   \
  dst[2] = Y_16_TO_Y_8(src_y[1]);                 \
  dst[3] = UV_16_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"


/* yuvj_420_p_to_yuy2_c */

#define FUNC_NAME     yuvj_420_p_to_yuy2_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT       \
  dst[0] = YJ_8_TO_Y_8(src_y[0]);               \
  dst[1] = UVJ_8_TO_UV_8(*src_u);               \
  dst[2] = YJ_8_TO_Y_8(src_y[1]);               \
  dst[3] = UVJ_8_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_yuy2_c */

#define FUNC_NAME     yuvj_422_p_to_yuy2_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
    dst[0] = YJ_8_TO_Y_8(src_y[0]);\
  dst[1] = UVJ_8_TO_UV_8(*src_u);               \
    dst[2] = YJ_8_TO_Y_8(src_y[1]);\
  dst[3] = UVJ_8_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_yuy2_c */

#define FUNC_NAME     yuvj_444_p_to_yuy2_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 2
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
    dst[0] = YJ_8_TO_Y_8(src_y[0]);\
  dst[1] = UVJ_8_TO_UV_8(*src_u);               \
    dst[2] = YJ_8_TO_Y_8(src_y[1]);\
  dst[3] = UVJ_8_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"

/* -> UYVY */

/* yuv_420_p_to_uyvy_c */

#define FUNC_NAME     yuv_420_p_to_uyvy_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT       \
    dst[1] = src_y[0];\
    dst[0] = *src_u;\
    dst[3] = src_y[1];\
    dst[2] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_410_p_to_uyvy_c */

#define FUNC_NAME     yuv_410_p_to_uyvy_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  4
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    4
#define CHROMA_SUB    4
#define CONVERT       \
    dst[1] = src_y[0];\
    dst[0] = *src_u;\
    dst[3] = src_y[1];\
    dst[2] = *src_v;\
    dst[5] = src_y[2];\
    dst[4] = *src_u;\
    dst[7] = src_y[3];\
    dst[6] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_422_p_to_uyvy_c */

#define FUNC_NAME     yuv_422_p_to_uyvy_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
    dst[1] = src_y[0];\
    dst[0] = *src_u;\
    dst[3] = src_y[1];\
    dst[2] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_422_p_16_to_uyvy_c */

#define FUNC_NAME     yuv_422_p_16_to_uyvy_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
  dst[1] = Y_16_TO_Y_8(src_y[0]);                 \
  dst[0] = UV_16_TO_UV_8(*src_u);                  \
  dst[3] = Y_16_TO_Y_8(src_y[1]);                 \
  dst[2] = UV_16_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"


/* yuv_411_p_to_uyvy_c */

#define FUNC_NAME     yuv_411_p_to_uyvy_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  4
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    4
#define CHROMA_SUB    1
#define CONVERT       \
    dst[1] = src_y[0];\
    dst[0] = *src_u;\
    dst[3] = src_y[1];\
    dst[2] = *src_v;\
    dst[5] = src_y[2];\
    dst[4] = *src_u;\
    dst[7] = src_y[3];\
    dst[6] = *src_v;


#include "../csp_planar_packed.h"

/* yuv_444_p_to_uyvy_c */

#define FUNC_NAME     yuv_444_p_to_uyvy_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 2
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
    dst[1] = src_y[0];\
    dst[0] = *src_u;\
    dst[3] = src_y[1];\
    dst[2] = *src_v;

#include "../csp_planar_packed.h"

/* yuv_444_p_16_to_uyvy_c */

#define FUNC_NAME     yuv_444_p_16_to_uyvy_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 2
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
  dst[1] = Y_16_TO_Y_8(src_y[0]);                 \
  dst[0] = UV_16_TO_UV_8(*src_u);                  \
  dst[3] = Y_16_TO_Y_8(src_y[1]);                 \
  dst[2] = UV_16_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_uyvy_c */

#define FUNC_NAME     yuvj_420_p_to_uyvy_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT       \
    dst[1] = YJ_8_TO_Y_8(src_y[0]);\
  dst[0] = UVJ_8_TO_UV_8(*src_u);               \
    dst[3] = YJ_8_TO_Y_8(src_y[1]);\
  dst[2] = UVJ_8_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_uyvy_c */

#define FUNC_NAME     yuvj_422_p_to_uyvy_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
    dst[1] = YJ_8_TO_Y_8(src_y[0]);\
  dst[0] = UVJ_8_TO_UV_8(*src_u);               \
    dst[3] = YJ_8_TO_Y_8(src_y[1]);\
  dst[2] = UVJ_8_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_uyvy_c */

#define FUNC_NAME     yuvj_444_p_to_uyvy_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 2
#define OUT_ADVANCE   4
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT       \
    dst[1] = YJ_8_TO_Y_8(src_y[0]);\
  dst[0] = UVJ_8_TO_UV_8(*src_u);               \
    dst[3] = YJ_8_TO_Y_8(src_y[1]);\
  dst[2] = UVJ_8_TO_UV_8(*src_v);

#include "../csp_planar_packed.h"

/*********************************
  Planar -> planar
 *********************************/

/*********************************
 * 420 -> 444 
 *********************************/

#define FUNC_NAME     yuv_420_p_to_yuv_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_420_p_to_yuv_444_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_8_TO_16(src_y[0]);                 \
  dst_u[0]=UV_8_TO_16(src_u[0]);                \
  dst_v[0]=UV_8_TO_16(src_v[0]);                \
  dst_y[1]=Y_8_TO_16(src_y[1]);                 \
  dst_u[1]=UV_8_TO_16(src_u[0]);                \
  dst_v[1]=UV_8_TO_16(src_v[0]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuv_420_p_to_yuvj_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);\
dst_u[1]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_8_TO_UVJ_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_420_p_to_yuv_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_u[1]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[1]=UVJ_8_TO_UV_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_420_p_to_yuv_444_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_16(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_16(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_16(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_16(src_y[1]);\
dst_u[1]=UVJ_8_TO_UV_16(src_u[0]);\
dst_v[1]=UVJ_8_TO_UV_16(src_v[0]);

#include "../csp_planar_planar.h"


/*********************************
 * 410 -> 444 
 *********************************/

#define FUNC_NAME     yuv_410_p_to_yuv_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 4
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  4
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];\
dst_y[2]=src_y[2];\
dst_u[2]=src_u[0];\
dst_v[2]=src_v[0];\
dst_y[3]=src_y[3];\
dst_u[3]=src_u[0];\
dst_v[3]=src_v[0];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_410_p_to_yuv_444_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 4
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  4
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_8_TO_16(src_y[0]);                 \
  dst_u[0]=UV_8_TO_16(src_u[0]);                \
  dst_v[0]=UV_8_TO_16(src_v[0]);                \
  dst_y[1]=Y_8_TO_16(src_y[1]);                 \
  dst_u[1]=UV_8_TO_16(src_u[0]);                \
  dst_v[1]=UV_8_TO_16(src_v[0]);                \
  dst_y[2]=Y_8_TO_16(src_y[2]);                 \
  dst_u[2]=UV_8_TO_16(src_u[0]);                \
  dst_v[2]=UV_8_TO_16(src_v[0]);                \
  dst_y[3]=Y_8_TO_16(src_y[3]);                 \
  dst_u[3]=UV_8_TO_16(src_u[0]);                \
  dst_v[3]=UV_8_TO_16(src_v[0]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuv_410_p_to_yuvj_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 4
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  4
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=  Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=  Y_8_TO_YJ_8(src_y[1]);\
dst_u[1]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[2]=  Y_8_TO_YJ_8(src_y[2]);\
dst_u[2]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[2]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[3]=  Y_8_TO_YJ_8(src_y[3]);\
dst_u[3]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[3]=UV_8_TO_UVJ_8(src_v[0]);

#include "../csp_planar_planar.h"

/*********************************
 * 420 -> 422 
 *********************************/

#define FUNC_NAME     yuv_420_p_to_yuvj_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_420_p_to_yuv_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_420_p_to_yuv_422_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_16(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_16(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_16(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_16(src_y[1]);

#include "../csp_planar_planar.h"

/*********************************
 * 410 -> 422 
 *********************************/

#define FUNC_NAME      yuv_410_p_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  4
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=  Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=  Y_8_TO_YJ_8(src_y[1]);\
dst_y[2]=  Y_8_TO_YJ_8(src_y[2]);\
dst_u[1]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[3]=  Y_8_TO_YJ_8(src_y[3]);

#include "../csp_planar_planar.h"

#define FUNC_NAME      yuv_410_p_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  4
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

#define FUNC_NAME      yuv_410_p_to_yuv_422_p_16_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  4
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_8_TO_16(src_y[0]);                 \
  dst_u[0]=UV_8_TO_16(src_u[0]);                \
  dst_v[0]=UV_8_TO_16(src_v[0]);                \
  dst_y[1]=Y_8_TO_16(src_y[1]);                 \
  dst_y[2]=Y_8_TO_16(src_y[2]);                 \
  dst_u[1]=UV_8_TO_16(src_u[0]);                \
  dst_v[1]=UV_8_TO_16(src_v[0]);                \
  dst_y[3]=Y_8_TO_16(src_y[3]);

#include "../csp_planar_planar.h"


/*********************************
 * 420 -> 411 
 *********************************/

#define FUNC_NAME     yuv_420_p_to_yuv_411_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_420_p_to_yuv_411_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=  YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=  YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=  YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=  YJ_8_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"

/*********************************
 * 422 -> 420 
 *********************************/

#define FUNC_NAME     yuv_422_p_to_yuvj_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#define CONVERT_Y    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_16_to_yuvj_420_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=Y_16_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_16_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_16_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_16_TO_YJ_8(src_y[1]);

#define CONVERT_Y    \
dst_y[0]=Y_16_TO_YJ_8(src_y[0]);\
dst_y[1]=Y_16_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuvj_422_p_to_yuv_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);

#define CONVERT_Y    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuv_422_p_16_to_yuv_420_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                 \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);

#define CONVERT_Y    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"


/*********************************
 * 422 -> 410 
 *********************************/

#define FUNC_NAME     yuvj_422_p_to_yuv_410_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 4
#define CONVERT_YUV    \
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=YJ_8_TO_Y_8(src_y[3]);

#define CONVERT_Y    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=YJ_8_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_to_yuv_410_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 4
#define CONVERT_YUV    \
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#define CONVERT_Y    \
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_16_to_yuv_410_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 4
#define CONVERT_YUV    \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                 \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                 \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                  \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);                  \
  dst_y[2]=Y_16_TO_Y_8(src_y[2]);                  \
  dst_y[3]=Y_16_TO_Y_8(src_y[3]);

#define CONVERT_Y    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);                \
  dst_y[2]=Y_16_TO_Y_8(src_y[2]);                \
  dst_y[3]=Y_16_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"



/*********************************
 * 411 -> 420 
 *********************************/

#define FUNC_NAME     yuv_411_p_to_yuvj_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=  Y_8_TO_YJ_8(src_y[0]);\
dst_y[1]=  Y_8_TO_YJ_8(src_y[1]);\
dst_y[2]=  Y_8_TO_YJ_8(src_y[2]);\
dst_y[3]=  Y_8_TO_YJ_8(src_y[3]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_u[1]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_8_TO_UVJ_8(src_v[0]);

#define CONVERT_Y    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);\
dst_y[2]=Y_8_TO_YJ_8(src_y[2]);\
dst_y[3]=Y_8_TO_YJ_8(src_y[3]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_411_p_to_yuv_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];

#define CONVERT_Y    \
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"



/*********************************
 * 422 -> 444 
 *********************************/

#define FUNC_NAME     yuv_422_p_to_yuv_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_16_to_yuv_444_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);                 \
  dst_u[1]=UV_16_TO_UV_8(src_u[0]);                \
  dst_v[1]=UV_16_TO_UV_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_to_yuv_444_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_8_TO_16(src_y[0]);                 \
  dst_u[0]=UV_8_TO_16(src_u[0]);                \
  dst_v[0]=UV_8_TO_16(src_v[0]);                \
  dst_y[1]=Y_8_TO_16(src_y[1]);                 \
  dst_u[1]=UV_8_TO_16(src_u[0]);                \
  dst_v[1]=UV_8_TO_16(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_16_to_yuv_444_p_16_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_to_yuvj_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);\
dst_u[1]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_8_TO_UVJ_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_16_to_yuvj_444_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_16_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_16_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_16_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_16_TO_YJ_8(src_y[1]);\
dst_u[1]=UV_16_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_16_TO_UVJ_8(src_v[0]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuvj_422_p_to_yuv_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_u[1]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[1]=UVJ_8_TO_UV_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_422_p_to_yuv_444_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_16(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_16(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_16(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_16(src_y[1]);\
dst_u[1]=UVJ_8_TO_UV_16(src_u[0]);\
dst_v[1]=UVJ_8_TO_UV_16(src_v[0]);

#include "../csp_planar_planar.h"

/*********************************
 * 444 -> 420 
 *********************************/

#define FUNC_NAME     yuv_444_p_to_yuv_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];

#define CONVERT_Y    \
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuv_420_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                 \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);

#define CONVERT_Y    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuv_444_p_to_yuvj_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#define CONVERT_Y    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuvj_420_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=Y_16_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_16_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_16_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_16_TO_YJ_8(src_y[1]);

#define CONVERT_Y    \
dst_y[0]=Y_16_TO_YJ_8(src_y[0]);\
dst_y[1]=Y_16_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuvj_444_p_to_yuv_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);

#define CONVERT_Y    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"

/*********************************
 * 444 -> 410 
 *********************************/

#define FUNC_NAME     yuvj_444_p_to_yuv_410_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 4
#define CONVERT_YUV    \
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=YJ_8_TO_Y_8(src_y[3]);

#define CONVERT_Y    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=YJ_8_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuv_444_p_to_yuv_410_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 4
#define CONVERT_YUV    \
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#define CONVERT_Y    \
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuv_410_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 4
#define CONVERT_YUV    \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                 \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                 \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                  \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);                  \
  dst_y[2]=Y_16_TO_Y_8(src_y[2]);                  \
  dst_y[3]=Y_16_TO_Y_8(src_y[3]);

#define CONVERT_Y    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);                 \
  dst_y[2]=Y_16_TO_Y_8(src_y[2]);                 \
  dst_y[3]=Y_16_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"


/*********************************
 * 444 -> 422 
 *********************************/

#define FUNC_NAME     yuv_444_p_to_yuv_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuv_422_p_16_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_to_yuv_422_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_8_TO_16(src_y[0]);                 \
  dst_u[0]=UV_8_TO_16(src_u[0]);                 \
  dst_v[0]=UV_8_TO_16(src_v[0]);                 \
  dst_y[1]=Y_8_TO_16(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuv_422_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                 \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_to_yuvj_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuvj_422_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_16_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_16_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_16_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_16_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_444_p_to_yuv_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_444_p_to_yuv_422_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_16(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_16(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_16(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_16(src_y[1]);

#include "../csp_planar_planar.h"

/*********************************
 * 444 -> 411 
 *********************************/

#define FUNC_NAME     yuvj_444_p_to_yuv_411_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=YJ_8_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_to_yuv_411_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuv_411_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  4
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);                \
  dst_y[2]=Y_16_TO_Y_8(src_y[2]);                \
  dst_y[3]=Y_16_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"


/*********************************
 * 420 -> 420 
 *********************************/

#define FUNC_NAME     yuv_420_p_to_yuvj_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#define CONVERT_Y    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_420_p_to_yuv_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);

#define CONVERT_Y    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);


#include "../csp_planar_planar.h"

/*********************************
 * 420 -> 410 
 *********************************/

#define FUNC_NAME     yuvj_420_p_to_yuv_410_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 4
#define CONVERT_YUV    \
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=YJ_8_TO_Y_8(src_y[3]);

#define CONVERT_Y    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=YJ_8_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_420_p_to_yuv_410_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 4
#define CONVERT_YUV    \
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#define CONVERT_Y    \
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

/*********************************
 * 420 -> 422 
 *********************************/

#define FUNC_NAME     yuv_420_p_to_yuv_422_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  2
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_u[0]=UV_8_TO_16(src_u[0]);                 \
  dst_v[0]=UV_8_TO_16(src_v[0]);                 \
  dst_y[0]=Y_8_TO_16(src_y[0]);                  \
  dst_y[1]=Y_8_TO_16(src_y[1]);                  \
  dst_y[2]=Y_8_TO_16(src_y[2]);                  \
  dst_y[3]=Y_8_TO_16(src_y[3]);

#define CONVERT_Y    \
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"


/*********************************
 * 410 -> 420 
 *********************************/

#define FUNC_NAME     yuv_410_p_to_yuvj_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  4
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=  Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=  Y_8_TO_YJ_8(src_y[1]);\
dst_y[2]=  Y_8_TO_YJ_8(src_y[2]);\
dst_u[1]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[3]=  Y_8_TO_YJ_8(src_y[3]);

#define CONVERT_Y    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);\
dst_y[2]=Y_8_TO_YJ_8(src_y[2]);\
dst_y[3]=Y_8_TO_YJ_8(src_y[3]);


#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_410_p_to_yuv_420_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  4
#define CHROMA_SUB_OUT 2
#define CONVERT_YUV    \
dst_y[0]=src_y[0];\
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];\
dst_y[3]=src_y[3];

#define CONVERT_Y    \
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

/*********************************
 * 422 -> 422 
 *********************************/

#define FUNC_NAME     yuv_422_p_to_yuvj_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_8_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_16_to_yuvj_422_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_16_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_16_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_16_TO_UVJ_8(src_v[0]);\
dst_y[1]=Y_16_TO_YJ_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_16_to_yuv_422_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                 \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuv_422_p_to_yuv_422_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_8_TO_16(src_y[0]);                 \
  dst_u[0]=UV_8_TO_16(src_u[0]);                 \
  dst_v[0]=UV_8_TO_16(src_v[0]);                 \
  dst_y[1]=Y_8_TO_16(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_422_p_to_yuv_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_422_p_to_yuv_422_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   2
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_16(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_16(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_16(src_v[0]);\
dst_y[1]=YJ_8_TO_Y_16(src_y[1]);

#include "../csp_planar_planar.h"

/*********************************
 * 422 -> 411 
 *********************************/

#define FUNC_NAME     yuvj_422_p_to_yuv_411_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);\
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_y[1]=YJ_8_TO_Y_8(src_y[1]);\
dst_y[2]=YJ_8_TO_Y_8(src_y[2]);\
dst_y[3]=YJ_8_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_to_yuv_411_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_422_p_16_to_yuv_411_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  2
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);                \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_y[1]=Y_16_TO_Y_8(src_y[1]);                 \
  dst_y[2]=Y_16_TO_Y_8(src_y[2]);                 \
  dst_y[3]=Y_16_TO_Y_8(src_y[3]);

#include "../csp_planar_planar.h"

/*********************************
 * 411 -> 422 
 *********************************/

#define FUNC_NAME     yuv_411_p_to_yuvj_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_u[1]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[0]=  Y_8_TO_YJ_8(src_y[0]);\
dst_y[1]=  Y_8_TO_YJ_8(src_y[1]);\
dst_y[2]=  Y_8_TO_YJ_8(src_y[2]);\
dst_y[3]=  Y_8_TO_YJ_8(src_y[3]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_411_p_to_yuv_422_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];\
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_411_p_to_yuv_422_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 2
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_u[0]=UV_8_TO_16(src_u[0]);                \
  dst_v[0]=UV_8_TO_16(src_v[0]);                \
  dst_u[1]=UV_8_TO_16(src_u[0]);                \
  dst_v[1]=UV_8_TO_16(src_v[0]);                \
  dst_y[0]=Y_8_TO_16(src_y[0]);                 \
  dst_y[1]=Y_8_TO_16(src_y[1]);                 \
  dst_y[2]=Y_8_TO_16(src_y[2]);                 \
  dst_y[3]=Y_8_TO_16(src_y[3]);

#include "../csp_planar_planar.h"

/*********************************
 * 411 -> 444 
 *********************************/

#define FUNC_NAME     yuv_411_p_to_yuvj_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 4
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);\
dst_u[1]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[1]=UV_8_TO_UVJ_8(src_v[0]);\
dst_u[2]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[2]=UV_8_TO_UVJ_8(src_v[0]);\
dst_u[3]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[3]=UV_8_TO_UVJ_8(src_v[0]);\
dst_y[0]=  Y_8_TO_YJ_8(src_y[0]);\
dst_y[1]=  Y_8_TO_YJ_8(src_y[1]);\
dst_y[2]=  Y_8_TO_YJ_8(src_y[2]);\
dst_y[3]=  Y_8_TO_YJ_8(src_y[3]);

#include "../csp_planar_planar.h"


#define FUNC_NAME     yuv_411_p_to_yuv_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 4
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_u[0]=src_u[0];\
dst_v[0]=src_v[0];\
dst_u[1]=src_u[0];\
dst_v[1]=src_v[0];\
dst_u[2]=src_u[0];\
dst_v[2]=src_v[0];\
dst_u[3]=src_u[0];\
dst_v[3]=src_v[0];\
dst_y[0]=src_y[0];\
dst_y[1]=src_y[1];\
dst_y[2]=src_y[2];\
dst_y[3]=src_y[3];

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_411_p_to_yuv_444_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   4
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 4
#define NUM_PIXELS     4
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_u[0]=UV_8_TO_16(src_u[0]);                \
  dst_v[0]=UV_8_TO_16(src_v[0]);                \
  dst_u[1]=UV_8_TO_16(src_u[0]);                \
  dst_v[1]=UV_8_TO_16(src_v[0]);                \
  dst_u[2]=UV_8_TO_16(src_u[0]);                \
  dst_v[2]=UV_8_TO_16(src_v[0]);                \
  dst_u[3]=UV_8_TO_16(src_u[0]);                \
  dst_v[3]=UV_8_TO_16(src_v[0]);                \
  dst_y[0]=Y_8_TO_16(src_y[0]);                \
  dst_y[1]=Y_8_TO_16(src_y[1]);                \
  dst_y[2]=Y_8_TO_16(src_y[2]);                \
  dst_y[3]=Y_8_TO_16(src_y[3]);

#include "../csp_planar_planar.h"


/*********************************
 * 444 -> 444 
 *********************************/

#define FUNC_NAME     yuv_444_p_to_yuvj_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   1
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_8_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_8_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_8_TO_UVJ_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuvj_444_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   1
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=Y_16_TO_YJ_8(src_y[0]);\
dst_u[0]=UV_16_TO_UVJ_8(src_u[0]);\
dst_v[0]=UV_16_TO_UVJ_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_to_yuv_444_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   1
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_8_TO_16(src_y[0]);                 \
  dst_u[0]=UV_8_TO_16(src_u[0]);                \
  dst_v[0]=UV_8_TO_16(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuv_444_p_16_to_yuv_444_p_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   1
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
  dst_y[0]=Y_16_TO_Y_8(src_y[0]);                 \
  dst_u[0]=UV_16_TO_UV_8(src_u[0]);                \
  dst_v[0]=UV_16_TO_UV_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_444_p_to_yuv_444_p_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y   1
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_8(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_8(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_8(src_v[0]);

#include "../csp_planar_planar.h"

#define FUNC_NAME     yuvj_444_p_to_yuv_444_p_16_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint16_t
#define IN_ADVANCE_Y   1
#define IN_ADVANCE_UV  1
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB_IN  1
#define CHROMA_SUB_OUT 1
#define CONVERT_YUV    \
dst_y[0]=YJ_8_TO_Y_16(src_y[0]);\
dst_u[0]=UVJ_8_TO_UV_16(src_u[0]);\
dst_v[0]=UVJ_8_TO_UV_16(src_v[0]);

#include "../csp_planar_planar.h"

/********************************
 *  Conversions to yuva 8
 ********************************/

/* yuv_444_p_to_yuya_32_c */

#define FUNC_NAME     yuv_444_p_to_yuva_32_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT    \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = *src_v;\
    dst[3] = 0xff;

#include "../csp_planar_packed.h"

/* yuv_444_p_16_to_yuya_32_c */

#define FUNC_NAME     yuv_444_p_16_to_yuva_32_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT    \
  dst[0] = Y_16_TO_Y_8(src_y[0]);                 \
  dst[1] = UV_16_TO_UV_8(*src_u);                   \
  dst[2] = UV_16_TO_UV_8(*src_v);                   \
  dst[3] = 0xff;

#include "../csp_planar_packed.h"

/* yuvj_444_p_to_yuya_32_c */

#define FUNC_NAME     yuvj_444_p_to_yuva_32_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  1
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   4
#define NUM_PIXELS    1
#define CHROMA_SUB    1
#define CONVERT    \
  dst[0] = YJ_8_TO_Y_8(src_y[0]);                 \
  dst[1] = UVJ_8_TO_UV_8(*src_u);                   \
  dst[2] = UVJ_8_TO_UV_8(*src_v);                   \
  dst[3] = 0xff;

#include "../csp_planar_packed.h"


/* yuv_422_p_to_yuya_32_c */

#define FUNC_NAME     yuv_422_p_to_yuva_32_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT    \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = *src_v;\
    dst[3] = 0xff;\
    dst[4] = src_y[2];\
    dst[5] = *src_u;\
    dst[6] = *src_v;\
    dst[7] = 0xff;

#include "../csp_planar_packed.h"

/* yuv_422_p_16_to_yuva_32_c */

#define FUNC_NAME     yuv_422_p_16_to_yuva_32_c
#define IN_TYPE       uint16_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT    \
  dst[0] = Y_16_TO_Y_8(src_y[0]);                 \
  dst[1] = UV_16_TO_UV_8(*src_u);                   \
  dst[2] = UV_16_TO_UV_8(*src_v);                   \
  dst[3] = 0xff;                                 \
  dst[4] = Y_16_TO_Y_8(src_y[2]);                  \
  dst[5] = UV_16_TO_UV_8(*src_u);                   \
  dst[6] = UV_16_TO_UV_8(*src_v);                   \
  dst[7] = 0xff;

#include "../csp_planar_packed.h"

/* yuvj_422_p_to_yuva_32_c */

#define FUNC_NAME     yuvj_422_p_to_yuva_32_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    1
#define CONVERT    \
  dst[0] = YJ_8_TO_Y_8(src_y[0]);                 \
  dst[1] = UVJ_8_TO_UV_8(*src_u);                   \
  dst[2] = UVJ_8_TO_UV_8(*src_v);                   \
  dst[3] = 0xff;                                 \
  dst[4] = YJ_8_TO_Y_8(src_y[2]);                  \
  dst[5] = UVJ_8_TO_UV_8(*src_u);                   \
  dst[6] = UVJ_8_TO_UV_8(*src_v);                   \
  dst[7] = 0xff;

#include "../csp_planar_packed.h"

/* yuv_411_p_to_yuya_32_c */

#define FUNC_NAME     yuv_411_p_to_yuva_32_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  4
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   16
#define NUM_PIXELS    4
#define CHROMA_SUB    1
#define CONVERT    \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = *src_v;\
    dst[3] = 0xff;\
    dst[4] = src_y[2];\
    dst[5] = *src_u;\
    dst[6] = *src_v;\
    dst[7] = 0xff;\
    dst[8] = src_y[3];\
    dst[9] = *src_u;\
    dst[10] = *src_v;\
    dst[11] = 0xff;\
    dst[12] = src_y[4];\
    dst[13] = *src_u;\
    dst[14] = *src_v;\
    dst[15] = 0xff;

#include "../csp_planar_packed.h"

/* yuv_410_p_to_yuya_32_c */

#define FUNC_NAME     yuv_410_p_to_yuva_32_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  4
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   16
#define NUM_PIXELS    4
#define CHROMA_SUB    4
#define CONVERT    \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = *src_v;\
    dst[3] = 0xff;\
    dst[4] = src_y[2];\
    dst[5] = *src_u;\
    dst[6] = *src_v;\
    dst[7] = 0xff;\
    dst[8] = src_y[3];\
    dst[9] = *src_u;\
    dst[10] = *src_v;\
    dst[11] = 0xff;\
    dst[12] = src_y[4];\
    dst[13] = *src_u;\
    dst[14] = *src_v;\
    dst[15] = 0xff;

#include "../csp_planar_packed.h"


/* yuv_420_p_to_yuya_32_c */

#define FUNC_NAME     yuv_420_p_to_yuva_32_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT    \
    dst[0] = src_y[0];\
    dst[1] = *src_u;\
    dst[2] = *src_v;\
    dst[3] = 0xff;\
    dst[4] = src_y[1];\
    dst[5] = *src_u;\
    dst[6] = *src_v;\
    dst[7] = 0xff;

#include "../csp_planar_packed.h"

/* yuvj_420_p_to_yuya_32_c */

#define FUNC_NAME     yuvj_420_p_to_yuva_32_c
#define IN_TYPE       uint8_t
#define OUT_TYPE      uint8_t
#define IN_ADVANCE_Y  2
#define IN_ADVANCE_UV 1
#define OUT_ADVANCE   8
#define NUM_PIXELS    2
#define CHROMA_SUB    2
#define CONVERT    \
  dst[0] = YJ_8_TO_Y_8(src_y[0]);                 \
  dst[1] = UVJ_8_TO_UV_8(*src_u);                   \
  dst[2] = UVJ_8_TO_UV_8(*src_v);                   \
  dst[3] = 0xff;                                  \
  dst[4] = YJ_8_TO_Y_8(src_y[1]);                 \
  dst[5] = UVJ_8_TO_UV_8(*src_u);                 \
  dst[6] = UVJ_8_TO_UV_8(*src_v);                 \
  dst[7] = 0xff;

#include "../csp_planar_packed.h"

/* yuvy_to_yuya_32_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 8
#define NUM_PIXELS  2
#define FUNC_NAME   uyvy_to_yuva_32_c
#define CONVERT     dst[0] = src[1];\
                    dst[1] = src[0];\
                    dst[2] = src[2];\
                    dst[3] = 0xff;  \
                    dst[4] = src[3];\
                    dst[5] = src[0];\
                    dst[6] = src[2];\
                    dst[7] = 0xff;

#include "../csp_packed_packed.h"

/* yuy2_to_yuya_32_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  4
#define OUT_ADVANCE 8
#define NUM_PIXELS  2
#define FUNC_NAME   yuy2_to_yuva_32_c
#define CONVERT     dst[0] = src[0];\
                    dst[1] = src[1];\
                    dst[2] = src[3];\
                    dst[3] = 0xff;  \
                    dst[4] = src[2];\
                    dst[5] = src[1];\
                    dst[6] = src[3];\
                    dst[7] = 0xff;

#include "../csp_packed_packed.h"

/* YUVA 32 -> */

/* yuva_32_to_yuv_410_p_c */

#define FUNC_NAME      yuva_32_to_yuv_410_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB     4
#define CONVERT_YUV \
  YUVA_32_TO_YUV_8(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);\
  YUVA_32_TO_Y_8(src[4], src[7], dst_y[1]);\
  YUVA_32_TO_Y_8(src[8], src[11], dst_y[2]);\
  YUVA_32_TO_Y_8(src[12], src[15], dst_y[3]);

#define CONVERT_Y                               \
  YUVA_32_TO_Y_8(src[0], src[3], dst_y[0]);\
  YUVA_32_TO_Y_8(src[4], src[7], dst_y[1]);\
  YUVA_32_TO_Y_8(src[8], src[11], dst_y[2]);    \
  YUVA_32_TO_Y_8(src[12], src[15], dst_y[3]);

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_411_p_c */

#define FUNC_NAME      yuva_32_to_yuv_411_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB     1
#define CONVERT_YUV \
  YUVA_32_TO_YUV_8(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);\
  YUVA_32_TO_Y_8(src[4], src[7], dst_y[1]);\
  YUVA_32_TO_Y_8(src[8], src[11], dst_y[2]);\
  YUVA_32_TO_Y_8(src[12], src[15], dst_y[3]);

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_420_p_c */

#define FUNC_NAME      yuva_32_to_yuv_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     2
#define CONVERT_YUV \
  YUVA_32_TO_YUV_8(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);\
  YUVA_32_TO_Y_8(src[4], src[7], dst_y[1]);\

#define CONVERT_Y \
  YUVA_32_TO_Y_8(src[0], src[3], dst_y[0]);\
  YUVA_32_TO_Y_8(src[4], src[7], dst_y[1]);\

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuvj_420_p_c */

#define FUNC_NAME      yuva_32_to_yuvj_420_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     2
#define CONVERT_YUV \
  YUVA_32_TO_YUVJ_8(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);\
  YUVA_32_TO_YJ_8(src[4], src[7], dst_y[1]);

#define CONVERT_Y                               \
  YUVA_32_TO_YJ_8(src[0], src[3], dst_y[0]);\
  YUVA_32_TO_YJ_8(src[4], src[7], dst_y[1]);

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_422_p_c */

#define FUNC_NAME      yuva_32_to_yuv_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  YUVA_32_TO_YUV_8(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);\
  YUVA_32_TO_Y_8(src[4], src[7], dst_y[1]);

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuvj_422_p_c */

#define FUNC_NAME      yuva_32_to_yuvj_422_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  YUVA_32_TO_YUVJ_8(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);\
  YUVA_32_TO_YJ_8(src[4], src[7], dst_y[1]);

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_422_p_16_c */

#define FUNC_NAME      yuva_32_to_yuv_422_p_16_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  YUVA_32_TO_YUV_16(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);\
  YUVA_32_TO_Y_16(src[4], src[7], dst_y[1]);

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_444_p_c */

#define FUNC_NAME      yuva_32_to_yuv_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB     1
#define CONVERT_YUV \
  YUVA_32_TO_YUV_8(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuvj_444_p_c */

#define FUNC_NAME      yuva_32_to_yuvj_444_p_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB     1
#define CONVERT_YUV \
  YUVA_32_TO_YUVJ_8(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_444_p_16_c */

#define FUNC_NAME      yuva_32_to_yuv_444_p_16_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB     1
#define CONVERT_YUV \
  YUVA_32_TO_YUV_16(src[0], src[1], src[2], src[3], dst_y[0], *dst_u, *dst_v);\

#define INIT INIT_YUVA_32

#include "../csp_packed_planar.h"

/* yuva_32_to_yuy2_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define FUNC_NAME   yuva_32_to_yuy2_c
#define CONVERT     \
  YUVA_32_TO_YUV_8(src[0], src[1], src[2], src[3], dst[0], dst[1], dst[3]); \
  YUVA_32_TO_Y_8(src[4], src[7], dst[2]);

#define INIT INIT_YUVA_32

#include "../csp_packed_packed.h"

/* yuva_32_to_uyvy_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define FUNC_NAME   yuva_32_to_uyvy_c
#define CONVERT     \
  YUVA_32_TO_YUV_8(src[0], src[1], src[2], src[3], dst[1], dst[0], dst[2]); \
  YUVA_32_TO_Y_8(src[4], src[7], dst[3]);

#define INIT INIT_YUVA_32

#include "../csp_packed_packed.h"


/* YUVA 32 -> (No Alpha) */

/* yuva_32_to_yuv_410_p_ia_c */

#define FUNC_NAME      yuva_32_to_yuv_410_p_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB     4
#define CONVERT_YUV \
  dst_y[0] = src[0]; \
  dst_u[0] = src[1]; \
  dst_v[0] = src[2]; \
  dst_y[1] = src[4]; \
  dst_y[2] = src[8]; \
  dst_y[3] = src[12];

#define CONVERT_Y                               \
  dst_y[0] = src[0]; \
  dst_y[1] = src[4]; \
  dst_y[2] = src[8]; \
  dst_y[3] = src[12];

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_411_p_ia_c */

#define FUNC_NAME      yuva_32_to_yuv_411_p_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     16
#define OUT_ADVANCE_Y  4
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     4
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = src[0]; \
  dst_u[0] = src[1]; \
  dst_v[0] = src[2]; \
  dst_y[1] = src[4]; \
  dst_y[2] = src[8]; \
  dst_y[3] = src[12];

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_420_p_ia_c */

#define FUNC_NAME      yuva_32_to_yuv_420_p_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     2
#define CONVERT_YUV \
  dst_y[0] = src[0]; \
  dst_u[0] = src[1]; \
  dst_v[0] = src[2]; \
  dst_y[1] = src[4];

#define CONVERT_Y \
  dst_y[0] = src[0]; \
  dst_y[1] = src[4];

#include "../csp_packed_planar.h"

/* yuva_32_to_yuvj_420_p_ia_c */

#define FUNC_NAME      yuva_32_to_yuvj_420_p_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     2
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_YJ_8(src[0]);               \
  dst_u[0] = UV_8_TO_UVJ_8(src[1]);             \
  dst_v[0] = UV_8_TO_UVJ_8(src[2]);             \
  dst_y[1] = Y_8_TO_YJ_8(src[4]);

#define CONVERT_Y                               \
  dst_y[0] = Y_8_TO_YJ_8(src[0]);               \
  dst_y[1] = Y_8_TO_YJ_8(src[4]);

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_422_p_ia_c */

#define FUNC_NAME      yuva_32_to_yuv_422_p_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = src[0]; \
  dst_u[0] = src[1]; \
  dst_v[0] = src[2]; \
  dst_y[1] = src[4];


#include "../csp_packed_planar.h"

/* yuva_32_to_yuvj_422_p_ia_c */

#define FUNC_NAME      yuva_32_to_yuvj_422_p_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_YJ_8(src[0]);               \
  dst_u[0] = UV_8_TO_UVJ_8(src[1]);             \
  dst_v[0] = UV_8_TO_UVJ_8(src[2]);             \
  dst_y[1] = Y_8_TO_YJ_8(src[4]);

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_422_p_16_ia_c */

#define FUNC_NAME      yuva_32_to_yuv_422_p_16_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE     8
#define OUT_ADVANCE_Y  2
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     2
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_16(src[0]);               \
  dst_u[0] = UV_8_TO_16(src[1]);             \
  dst_v[0] = UV_8_TO_16(src[2]);             \
  dst_y[1] = Y_8_TO_16(src[4]);

#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_444_p_ia_c */

#define FUNC_NAME      yuva_32_to_yuv_444_p_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = src[0];             \
  dst_u[0] = src[1];             \
  dst_v[0] = src[2];             \

#include "../csp_packed_planar.h"

/* yuva_32_to_yuvj_444_p_ia_c */

#define FUNC_NAME      yuva_32_to_yuvj_444_p_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint8_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_YJ_8(src[0]);               \
  dst_u[0] = UV_8_TO_UVJ_8(src[1]);             \
  dst_v[0] = UV_8_TO_UVJ_8(src[2]);             \


#include "../csp_packed_planar.h"

/* yuva_32_to_yuv_444_p_16_ia_c */

#define FUNC_NAME      yuva_32_to_yuv_444_p_16_ia_c
#define IN_TYPE        uint8_t
#define OUT_TYPE       uint16_t
#define IN_ADVANCE     4
#define OUT_ADVANCE_Y  1
#define OUT_ADVANCE_UV 1
#define NUM_PIXELS     1
#define CHROMA_SUB     1
#define CONVERT_YUV \
  dst_y[0] = Y_8_TO_16(src[0]);               \
  dst_u[0] = UV_8_TO_16(src[1]);             \
  dst_v[0] = UV_8_TO_16(src[2]);             \

#include "../csp_packed_planar.h"

/* yuva_32_to_yuy2_ia_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define FUNC_NAME   yuva_32_to_yuy2_ia_c
#define CONVERT     \
  dst[0] = src[0]; /* Y */\
  dst[1] = src[1]; /* U */\
  dst[2] = src[4]; /* Y */\
  dst[3] = src[2]; /* V */


#include "../csp_packed_packed.h"

/* yuva_32_to_uyvy_ia_c */

#define IN_TYPE  uint8_t
#define OUT_TYPE uint8_t
#define IN_ADVANCE  8
#define OUT_ADVANCE 4
#define NUM_PIXELS  2
#define FUNC_NAME   yuva_32_to_uyvy_ia_c
#define CONVERT     \
  dst[0] = src[1]; /* U */\
  dst[1] = src[0]; /* Y */\
  dst[2] = src[2]; /* V */\
  dst[3] = src[4]; /* Y */


#include "../csp_packed_packed.h"

void gavl_init_yuv_yuv_funcs_c(gavl_pixelformat_function_table_t * tab, const gavl_video_options_t * opt)
  {
  if(opt->alpha_mode == GAVL_ALPHA_BLEND_COLOR)
    {
    tab->yuva_32_to_yuv_420_p = yuva_32_to_yuv_420_p_c;
    tab->yuva_32_to_yuvj_420_p = yuva_32_to_yuvj_420_p_c;
    tab->yuva_32_to_yuv_422_p = yuva_32_to_yuv_422_p_c;
    tab->yuva_32_to_yuvj_422_p = yuva_32_to_yuvj_422_p_c;
    tab->yuva_32_to_yuv_422_p_16 = yuva_32_to_yuv_422_p_16_c;
    tab->yuva_32_to_yuv_410_p = yuva_32_to_yuv_410_p_c;
    tab->yuva_32_to_yuv_411_p = yuva_32_to_yuv_411_p_c;
    tab->yuva_32_to_yuv_444_p = yuva_32_to_yuv_444_p_c;
    tab->yuva_32_to_yuvj_444_p = yuva_32_to_yuvj_444_p_c;
    tab->yuva_32_to_yuv_444_p_16 = yuva_32_to_yuv_444_p_16_c;
    tab->yuva_32_to_yuy2 = yuva_32_to_yuy2_c;
    tab->yuva_32_to_uyvy = yuva_32_to_uyvy_c;
    }
  else if(opt->alpha_mode == GAVL_ALPHA_IGNORE)
    {
    tab->yuva_32_to_yuv_420_p = yuva_32_to_yuv_420_p_ia_c;
    tab->yuva_32_to_yuvj_420_p = yuva_32_to_yuvj_420_p_ia_c;
    tab->yuva_32_to_yuv_422_p = yuva_32_to_yuv_422_p_ia_c;
    tab->yuva_32_to_yuvj_422_p = yuva_32_to_yuvj_422_p_ia_c;
    tab->yuva_32_to_yuv_422_p_16 = yuva_32_to_yuv_422_p_16_ia_c;
    tab->yuva_32_to_yuv_410_p = yuva_32_to_yuv_410_p_ia_c;
    tab->yuva_32_to_yuv_411_p = yuva_32_to_yuv_411_p_ia_c;
    tab->yuva_32_to_yuv_444_p = yuva_32_to_yuv_444_p_ia_c;
    tab->yuva_32_to_yuvj_444_p = yuva_32_to_yuvj_444_p_ia_c;
    tab->yuva_32_to_yuv_444_p_16 = yuva_32_to_yuv_444_p_16_ia_c;
    tab->yuva_32_to_yuy2 = yuva_32_to_yuy2_ia_c;
    tab->yuva_32_to_uyvy = yuva_32_to_uyvy_ia_c;
    }

  tab->uyvy_to_yuy2            = uyvy_to_yuy2_c;
  tab->yuy2_to_yuv_420_p       = yuy2_to_yuv_420_p_c;
  tab->yuy2_to_yuv_410_p       = yuy2_to_yuv_410_p_c;
  tab->yuy2_to_yuv_422_p       = yuy2_to_yuv_422_p_c;
  tab->yuy2_to_yuv_422_p_16       = yuy2_to_yuv_422_p_16_c;
  tab->yuy2_to_yuv_411_p       = yuy2_to_yuv_411_p_c;
  tab->yuy2_to_yuv_444_p       = yuy2_to_yuv_444_p_c;
  tab->yuy2_to_yuv_444_p_16       = yuy2_to_yuv_444_p_16_c;

  tab->yuy2_to_yuvj_420_p      = yuy2_to_yuvj_420_p_c;
  tab->yuy2_to_yuvj_422_p      = yuy2_to_yuvj_422_p_c;
  tab->yuy2_to_yuvj_444_p      = yuy2_to_yuvj_444_p_c;
  
  tab->yuv_420_p_to_yuy2       = yuv_420_p_to_yuy2_c;
  tab->yuv_422_p_to_yuy2       = yuv_422_p_to_yuy2_c;
  tab->yuv_422_p_16_to_yuy2       = yuv_422_p_16_to_yuy2_c;

  tab->yuv_444_p_to_yuy2       = yuv_444_p_to_yuy2_c;
  tab->yuv_444_p_16_to_yuy2       = yuv_444_p_16_to_yuy2_c;

  tab->yuvj_420_p_to_yuy2      = yuvj_420_p_to_yuy2_c;
  tab->yuvj_422_p_to_yuy2      = yuvj_422_p_to_yuy2_c;
  tab->yuvj_444_p_to_yuy2      = yuvj_444_p_to_yuy2_c;

  tab->uyvy_to_yuv_420_p       = uyvy_to_yuv_420_p_c;
  tab->uyvy_to_yuv_410_p       = uyvy_to_yuv_410_p_c;
  tab->uyvy_to_yuv_422_p       = uyvy_to_yuv_422_p_c;
  tab->uyvy_to_yuv_422_p_16       = uyvy_to_yuv_422_p_16_c;
  tab->uyvy_to_yuv_411_p       = uyvy_to_yuv_411_p_c;
  tab->uyvy_to_yuv_444_p       = uyvy_to_yuv_444_p_c;
  tab->uyvy_to_yuv_444_p_16       = uyvy_to_yuv_444_p_16_c;

  tab->uyvy_to_yuvj_420_p      = uyvy_to_yuvj_420_p_c;
  tab->uyvy_to_yuvj_422_p      = uyvy_to_yuvj_422_p_c;
  tab->uyvy_to_yuvj_444_p      = uyvy_to_yuvj_444_p_c;
  
  tab->yuv_420_p_to_uyvy       = yuv_420_p_to_uyvy_c;
  tab->yuv_422_p_to_uyvy       = yuv_422_p_to_uyvy_c;
  tab->yuv_422_p_16_to_uyvy       = yuv_422_p_16_to_uyvy_c;
  tab->yuv_444_p_to_uyvy       = yuv_444_p_to_uyvy_c;
  tab->yuv_444_p_16_to_uyvy       = yuv_444_p_16_to_uyvy_c;

  tab->yuvj_420_p_to_uyvy      = yuvj_420_p_to_uyvy_c;
  tab->yuvj_422_p_to_uyvy      = yuvj_422_p_to_uyvy_c;
  tab->yuvj_444_p_to_uyvy      = yuvj_444_p_to_uyvy_c;
  
  tab->yuv_420_p_to_yuv_444_p  = yuv_420_p_to_yuv_444_p_c;
  tab->yuv_420_p_to_yuv_444_p_16  = yuv_420_p_to_yuv_444_p_16_c;
  tab->yuv_420_p_to_yuvj_444_p = yuv_420_p_to_yuvj_444_p_c;
  tab->yuvj_420_p_to_yuv_444_p = yuvj_420_p_to_yuv_444_p_c;
  tab->yuvj_420_p_to_yuv_444_p_16 = yuvj_420_p_to_yuv_444_p_16_c;

  tab->yuv_422_p_to_yuv_444_p  = yuv_422_p_to_yuv_444_p_c;

  tab->yuv_422_p_to_yuv_444_p_16  = yuv_422_p_to_yuv_444_p_16_c;
  tab->yuv_422_p_16_to_yuv_444_p  = yuv_422_p_16_to_yuv_444_p_c;
  tab->yuv_422_p_16_to_yuvj_444_p  = yuv_422_p_16_to_yuvj_444_p_c;

  tab->yuv_422_p_to_yuvj_444_p = yuv_422_p_to_yuvj_444_p_c;
  tab->yuvj_422_p_to_yuv_444_p = yuvj_422_p_to_yuv_444_p_c;
  tab->yuvj_422_p_to_yuv_444_p_16 = yuvj_422_p_to_yuv_444_p_16_c;
  
  tab->yuv_444_p_to_yuv_420_p  = yuv_444_p_to_yuv_420_p_c;
  tab->yuv_444_p_to_yuvj_420_p = yuv_444_p_to_yuvj_420_p_c;
  tab->yuvj_444_p_to_yuv_420_p = yuvj_444_p_to_yuv_420_p_c;

  tab->yuv_444_p_to_yuv_422_p  = yuv_444_p_to_yuv_422_p_c;
  tab->yuv_444_p_to_yuv_422_p_16  = yuv_444_p_to_yuv_422_p_16_c;

  tab->yuv_444_p_16_to_yuv_422_p_16  = yuv_444_p_16_to_yuv_422_p_16_c;
  tab->yuv_444_p_16_to_yuv_422_p  = yuv_444_p_16_to_yuv_422_p_c;
  tab->yuv_444_p_16_to_yuvj_422_p  = yuv_444_p_16_to_yuvj_422_p_c;

  tab->yuv_444_p_16_to_yuv_410_p  = yuv_444_p_16_to_yuv_410_p_c;
  tab->yuv_444_p_16_to_yuv_411_p  = yuv_444_p_16_to_yuv_411_p_c;
  tab->yuv_444_p_16_to_yuv_420_p  = yuv_444_p_16_to_yuv_420_p_c;
  tab->yuv_444_p_16_to_yuvj_420_p  = yuv_444_p_16_to_yuvj_420_p_c;
  
  tab->yuv_444_p_to_yuvj_422_p = yuv_444_p_to_yuvj_422_p_c;
  tab->yuvj_444_p_to_yuv_422_p = yuvj_444_p_to_yuv_422_p_c;
  tab->yuvj_444_p_to_yuv_422_p_16 = yuvj_444_p_to_yuv_422_p_16_c;
  tab->yuvj_444_p_to_yuv_411_p = yuvj_444_p_to_yuv_411_p_c;
  tab->yuvj_444_p_to_yuv_410_p = yuvj_444_p_to_yuv_410_p_c;

  tab->yuv_444_p_to_yuv_411_p = yuv_444_p_to_yuv_411_p_c;
  tab->yuv_444_p_to_yuv_410_p = yuv_444_p_to_yuv_410_p_c;
  
  tab->yuv_420_p_to_yuv_422_p  = yuv_420_p_to_yuv_422_p_generic;
  tab->yuv_420_p_to_yuv_422_p_16  = yuv_420_p_to_yuv_422_p_16_c;
  tab->yuv_420_p_to_yuv_411_p  = yuv_420_p_to_yuv_411_p_c;
  tab->yuv_420_p_to_yuv_410_p  = yuv_420_p_to_yuv_410_p_c;
  tab->yuv_420_p_to_yuvj_422_p = yuv_420_p_to_yuvj_422_p_c;
  tab->yuvj_420_p_to_yuv_422_p = yuvj_420_p_to_yuv_422_p_c;
  tab->yuvj_420_p_to_yuv_422_p_16 = yuvj_420_p_to_yuv_422_p_16_c;
  tab->yuvj_420_p_to_yuv_411_p = yuvj_420_p_to_yuv_411_p_c;
  tab->yuvj_420_p_to_yuv_410_p = yuvj_420_p_to_yuv_410_p_c;
  
  
  tab->yuv_410_p_to_yuv_411_p  = yuv_410_p_to_yuv_411_p_generic;
  tab->yuv_410_p_to_yuy2       = yuv_410_p_to_yuy2_c;
  tab->yuv_410_p_to_uyvy       = yuv_410_p_to_uyvy_c;
  tab->yuv_410_p_to_yuv_444_p  = yuv_410_p_to_yuv_444_p_c;
  tab->yuv_410_p_to_yuv_444_p_16  = yuv_410_p_to_yuv_444_p_16_c;
  tab->yuv_410_p_to_yuvj_444_p = yuv_410_p_to_yuvj_444_p_c;
  tab->yuv_410_p_to_yuv_420_p  = yuv_410_p_to_yuv_420_p_c;
  tab->yuv_410_p_to_yuvj_420_p = yuv_410_p_to_yuvj_420_p_c;
  tab->yuv_410_p_to_yuvj_422_p = yuv_410_p_to_yuvj_422_p_c;
  tab->yuv_410_p_to_yuv_422_p  = yuv_410_p_to_yuv_422_p_c;
  tab->yuv_410_p_to_yuv_422_p_16  = yuv_410_p_to_yuv_422_p_16_c;
  tab->yuv_410_p_to_yuv_420_p  = yuv_410_p_to_yuv_420_p_c;
  
  tab->yuv_422_p_to_yuv_420_p  = yuv_422_p_to_yuv_420_p_generic;
  tab->yuv_422_p_to_yuvj_420_p = yuv_422_p_to_yuvj_420_p_c;
  tab->yuvj_422_p_to_yuv_420_p = yuvj_422_p_to_yuv_420_p_c;
  tab->yuv_422_p_to_yuv_411_p  = yuv_422_p_to_yuv_411_p_c;
  tab->yuvj_422_p_to_yuv_411_p = yuvj_422_p_to_yuv_411_p_c;
  tab->yuv_422_p_to_yuv_410_p  = yuv_422_p_to_yuv_410_p_c;
  tab->yuvj_422_p_to_yuv_410_p = yuvj_422_p_to_yuv_410_p_c;
  tab->yuv_422_p_to_yuv_422_p_16  = yuv_422_p_to_yuv_422_p_16_c;

  
  tab->yuv_422_p_16_to_yuv_420_p = yuv_422_p_16_to_yuv_420_p_c;
  tab->yuv_422_p_16_to_yuvj_420_p = yuv_422_p_16_to_yuvj_420_p_c;
  tab->yuv_422_p_16_to_yuv_410_p = yuv_422_p_16_to_yuv_410_p_c;
  tab->yuv_422_p_16_to_yuv_411_p = yuv_422_p_16_to_yuv_411_p_c;
  tab->yuv_422_p_16_to_yuv_422_p = yuv_422_p_16_to_yuv_422_p_c;
  tab->yuv_422_p_16_to_yuvj_422_p = yuv_422_p_16_to_yuvj_422_p_c;

  tab->yuv_422_p_16_to_yuv_444_p_16 = yuv_422_p_16_to_yuv_444_p_16_c;

  
  tab->yuv_411_p_to_yuv_410_p  = yuv_411_p_to_yuv_410_p_generic;
  tab->yuv_411_p_to_yuy2       = yuv_411_p_to_yuy2_c;
  tab->yuv_411_p_to_uyvy       = yuv_411_p_to_uyvy_c;
  tab->yuv_411_p_to_yuv_420_p  = yuv_411_p_to_yuv_420_p_c;
  tab->yuv_411_p_to_yuvj_420_p = yuv_411_p_to_yuvj_420_p_c;
  tab->yuv_411_p_to_yuv_444_p  = yuv_411_p_to_yuv_444_p_c;
  tab->yuv_411_p_to_yuv_444_p_16  = yuv_411_p_to_yuv_444_p_16_c;
  tab->yuv_411_p_to_yuvj_444_p = yuv_411_p_to_yuvj_444_p_c;
  tab->yuv_411_p_to_yuv_422_p  = yuv_411_p_to_yuv_422_p_c;
  tab->yuv_411_p_to_yuv_422_p_16  = yuv_411_p_to_yuv_422_p_16_c;
  tab->yuv_411_p_to_yuvj_422_p = yuv_411_p_to_yuvj_422_p_c;
  
  
  tab->yuv_420_p_to_yuvj_420_p = yuv_420_p_to_yuvj_420_p_c;
  tab->yuvj_420_p_to_yuv_420_p = yuvj_420_p_to_yuv_420_p_c;

  tab->yuv_422_p_to_yuvj_422_p = yuv_422_p_to_yuvj_422_p_c;
  tab->yuvj_422_p_to_yuv_422_p = yuvj_422_p_to_yuv_422_p_c;
  tab->yuvj_422_p_to_yuv_422_p_16 = yuvj_422_p_to_yuv_422_p_16_c;

  tab->yuv_444_p_to_yuvj_444_p = yuv_444_p_to_yuvj_444_p_c;
  tab->yuvj_444_p_to_yuv_444_p = yuvj_444_p_to_yuv_444_p_c;
  tab->yuvj_444_p_to_yuv_444_p_16 = yuvj_444_p_to_yuv_444_p_16_c;

  tab->yuv_444_p_to_yuv_444_p_16 = yuv_444_p_to_yuv_444_p_16_c;
  tab->yuv_444_p_16_to_yuv_444_p = yuv_444_p_16_to_yuv_444_p_c;
  tab->yuv_444_p_16_to_yuvj_444_p = yuv_444_p_16_to_yuvj_444_p_c;
  
  tab->yuv_444_p_to_yuva_32  = yuv_444_p_to_yuva_32_c;
  tab->yuv_422_p_to_yuva_32  = yuv_422_p_to_yuva_32_c;

  tab->yuv_444_p_16_to_yuva_32  = yuv_444_p_16_to_yuva_32_c;
  tab->yuvj_444_p_to_yuva_32  = yuvj_444_p_to_yuva_32_c;
  tab->yuv_422_p_16_to_yuva_32  = yuv_422_p_16_to_yuva_32_c;
  tab->yuvj_422_p_to_yuva_32  = yuvj_422_p_to_yuva_32_c;

  tab->yuv_411_p_to_yuva_32  = yuv_411_p_to_yuva_32_c;
  tab->yuv_410_p_to_yuva_32  = yuv_410_p_to_yuva_32_c;
  tab->yuv_420_p_to_yuva_32  = yuv_420_p_to_yuva_32_c;
  tab->yuvj_420_p_to_yuva_32  = yuvj_420_p_to_yuva_32_c;
  tab->uyvy_to_yuva_32  = uyvy_to_yuva_32_c;
  tab->yuy2_to_yuva_32  = yuy2_to_yuva_32_c;
  


  
  }
