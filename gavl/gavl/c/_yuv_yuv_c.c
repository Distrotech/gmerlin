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

/*
 *  Tables, which map MPEG quantized YUV values to JPEG quantized and
 *  vice versa
 */

static uint8_t yj_2_y[256] =
{
  0x10, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1c,
  0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2a,
  0x2b, 0x2c, 0x2d, 0x2e, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x34, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
  0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x52, 0x53,
  0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x5f, 0x60, 0x61,
  0x62, 0x63, 0x64, 0x65, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d,
  0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x89, 0x8a,
  0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x95, 0x96, 0x97, 0x98,
  0x99, 0x9a, 0x9b, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
  0xa7, 0xa8, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4,
  0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc0, 0xc1,
  0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd,
  0xde, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb
};
 
static uint8_t y_2_yj[256] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10, 0x11,
  0x12, 0x13, 0x14, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x24,
  0x25, 0x26, 0x27, 0x28, 0x29, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x32, 0x33, 0x34, 0x35, 0x36,
  0x37, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x47, 0x48, 0x49,
  0x4a, 0x4b, 0x4c, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b,
  0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6b, 0x6c, 0x6d, 0x6e,
  0x6f, 0x70, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x80, 0x81,
  0x82, 0x83, 0x84, 0x85, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93,
  0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa3, 0xa4, 0xa5, 0xa6,
  0xa7, 0xa8, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb9,
  0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
  0xcc, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdc, 0xdd, 0xde,
  0xdf, 0xe0, 0xe1, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf1,
  0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
 
static uint8_t uvj_2_uv[256] =
{
  0x10, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
  0x1e, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
  0x2c, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
  0x3a, 0x3b, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x51, 0x52, 0x53, 0x54, 0x55,
  0x56, 0x57, 0x58, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x5f, 0x60, 0x61, 0x62, 0x63,
  0x64, 0x65, 0x66, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6d, 0x6e, 0x6f, 0x70, 0x71,
  0x72, 0x73, 0x74, 0x75, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8a, 0x8b, 0x8c, 0x8d,
  0x8e, 0x8f, 0x90, 0x91, 0x92, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x99, 0x9a, 0x9b,
  0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa7, 0xa8, 0xa9,
  0xaa, 0xab, 0xac, 0xad, 0xae, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc4, 0xc5,
  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd3,
  0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe1,
  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0
};
 
static uint8_t uv_2_uvj[256] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x11,
  0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x21, 0x22, 0x23,
  0x24, 0x25, 0x26, 0x27, 0x28, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x32, 0x33, 0x34, 0x35,
  0x36, 0x37, 0x38, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
  0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6b, 0x6c,
  0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7c, 0x7d, 0x7e,
  0x7f, 0x80, 0x81, 0x82, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8c, 0x8d, 0x8e, 0x8f, 0x90,
  0x91, 0x92, 0x93, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2,
  0xa3, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb5,
  0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd6, 0xd7, 0xd8, 0xd9,
  0xda, 0xdb, 0xdc, 0xdd, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe7, 0xe8, 0xe9, 0xea, 0xeb,
  0xec, 0xed, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};



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
    dst_y[0] = y_2_yj[src[0]];\
    *dst_u   = uv_2_uvj[src[1]];\
    dst_y[1] = y_2_yj[src[2]];\
    *dst_v   = uv_2_uvj[src[3]];

#define CONVERT_Y \
    dst_y[0] = y_2_yj[src[0]];\
    dst_y[1] = y_2_yj[src[2]];

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
    dst_y[0] = y_2_yj[src[0]];\
    *dst_u   = uv_2_uvj[src[1]];\
    dst_y[1] = y_2_yj[src[2]];\
    *dst_v   = uv_2_uvj[src[3]];

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
    dst_y[0] = y_2_yj[src[0]]; \
    dst_u[0] = uv_2_uvj[src[1]]; \
    dst_v[0] = uv_2_uvj[src[3]]; \
    dst_y[1] = y_2_yj[src[2]]; \
    dst_u[1] = uv_2_uvj[src[1]]; \
    dst_v[1] = uv_2_uvj[src[3]];

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
    dst_y[0] = y_2_yj[src[1]];\
    *dst_u   = uv_2_uvj[src[0]];\
    dst_y[1] = y_2_yj[src[3]];\
    *dst_v   = uv_2_uvj[src[2]];

#define CONVERT_Y \
    dst_y[0] = y_2_yj[src[1]];\
    dst_y[1] = y_2_yj[src[3]];

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
    dst_y[0] = y_2_yj[src[1]];\
    *dst_u   = uv_2_uvj[src[0]];\
    dst_y[1] = y_2_yj[src[3]];\
    *dst_v   = uv_2_uvj[src[2]];

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
    dst_y[0] = y_2_yj[src[1]]; \
    dst_u[0] = uv_2_uvj[src[0]]; \
    dst_v[0] = uv_2_uvj[src[2]]; \
    dst_y[1] = y_2_yj[src[3]]; \
    dst_u[1] = uv_2_uvj[src[0]]; \
    dst_v[1] = uv_2_uvj[src[2]];

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
    dst[0] = yj_2_y[src_y[0]];\
    dst[1] = uvj_2_uv[*src_u];\
    dst[2] = yj_2_y[src_y[1]];\
    dst[3] = uvj_2_uv[*src_v];

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
    dst[0] = yj_2_y[src_y[0]];\
    dst[1] = uvj_2_uv[*src_u];\
    dst[2] = yj_2_y[src_y[1]];\
    dst[3] = uvj_2_uv[*src_v];

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
    dst[0] = yj_2_y[src_y[0]];\
    dst[1] = uvj_2_uv[*src_u];\
    dst[2] = yj_2_y[src_y[1]];\
    dst[3] = uvj_2_uv[*src_v];

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
    dst[1] = yj_2_y[src_y[0]];\
    dst[0] = uvj_2_uv[*src_u];\
    dst[3] = yj_2_y[src_y[1]];\
    dst[2] = uvj_2_uv[*src_v];

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
    dst[1] = yj_2_y[src_y[0]];\
    dst[0] = uvj_2_uv[*src_u];\
    dst[3] = yj_2_y[src_y[1]];\
    dst[2] = uvj_2_uv[*src_v];

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
    dst[1] = yj_2_y[src_y[0]];\
    dst[0] = uvj_2_uv[*src_u];\
    dst[3] = yj_2_y[src_y[1]];\
    dst[2] = uvj_2_uv[*src_v];

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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=y_2_yj[src_y[1]];\
dst_u[1]=uv_2_uvj[src_u[0]];\
dst_v[1]=uv_2_uvj[src_v[0]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_u[1]=uvj_2_uv[src_u[0]];\
dst_v[1]=uvj_2_uv[src_v[0]];

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
dst_y[0]=  y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=  y_2_yj[src_y[1]];\
dst_u[1]=uv_2_uvj[src_u[0]];\
dst_v[1]=uv_2_uvj[src_v[0]];\
dst_y[2]=  y_2_yj[src_y[2]];\
dst_u[2]=uv_2_uvj[src_u[0]];\
dst_v[2]=uv_2_uvj[src_v[0]];\
dst_y[3]=  y_2_yj[src_y[3]];\
dst_u[3]=uv_2_uvj[src_u[0]];\
dst_v[3]=uv_2_uvj[src_v[0]];

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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=y_2_yj[src_y[1]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=yj_2_y[src_y[1]];

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
dst_y[0]=  y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=  y_2_yj[src_y[1]];\
dst_y[2]=  y_2_yj[src_y[2]];\
dst_u[1]=uv_2_uvj[src_u[0]];\
dst_v[1]=uv_2_uvj[src_v[0]];\
dst_y[3]=  y_2_yj[src_y[3]];

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
dst_y[0]=  yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=  yj_2_y[src_y[1]];\
dst_y[2]=  yj_2_y[src_y[2]];\
dst_y[3]=  yj_2_y[src_y[3]];

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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=y_2_yj[src_y[1]];

#define CONVERT_Y    \
dst_y[0]=y_2_yj[src_y[0]];\
dst_y[1]=y_2_yj[src_y[1]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=yj_2_y[src_y[1]];

#define CONVERT_Y    \
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];

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
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_y[2]=yj_2_y[src_y[2]];\
dst_y[3]=yj_2_y[src_y[3]];

#define CONVERT_Y    \
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_y[2]=yj_2_y[src_y[2]];\
dst_y[3]=yj_2_y[src_y[3]];

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
dst_y[0]=  y_2_yj[src_y[0]];\
dst_y[1]=  y_2_yj[src_y[1]];\
dst_y[2]=  y_2_yj[src_y[2]];\
dst_y[3]=  y_2_yj[src_y[3]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_u[1]=uv_2_uvj[src_u[0]];\
dst_v[1]=uv_2_uvj[src_v[0]];

#define CONVERT_Y    \
dst_y[0]=y_2_yj[src_y[0]];\
dst_y[1]=y_2_yj[src_y[1]];\
dst_y[2]=y_2_yj[src_y[2]];\
dst_y[3]=y_2_yj[src_y[3]];

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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=y_2_yj[src_y[1]];\
dst_u[1]=uv_2_uvj[src_u[0]];\
dst_v[1]=uv_2_uvj[src_v[0]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_u[1]=uvj_2_uv[src_u[0]];\
dst_v[1]=uvj_2_uv[src_v[0]];

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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=y_2_yj[src_y[1]];

#define CONVERT_Y    \
dst_y[0]=y_2_yj[src_y[0]];\
dst_y[1]=y_2_yj[src_y[1]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=yj_2_y[src_y[1]];

#define CONVERT_Y    \
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];

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
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_y[2]=yj_2_y[src_y[2]];\
dst_y[3]=yj_2_y[src_y[3]];

#define CONVERT_Y    \
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_y[2]=yj_2_y[src_y[2]];\
dst_y[3]=yj_2_y[src_y[3]];

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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=y_2_yj[src_y[1]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=yj_2_y[src_y[1]];

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
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_y[2]=yj_2_y[src_y[2]];\
dst_y[3]=yj_2_y[src_y[3]];

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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=y_2_yj[src_y[1]];

#define CONVERT_Y    \
dst_y[0]=y_2_yj[src_y[0]];\
dst_y[1]=y_2_yj[src_y[1]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=yj_2_y[src_y[1]];

#define CONVERT_Y    \
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];


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
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_y[2]=yj_2_y[src_y[2]];\
dst_y[3]=yj_2_y[src_y[3]];

#define CONVERT_Y    \
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_y[2]=yj_2_y[src_y[2]];\
dst_y[3]=yj_2_y[src_y[3]];

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
dst_y[0]=  y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=  y_2_yj[src_y[1]];\
dst_y[2]=  y_2_yj[src_y[2]];\
dst_u[1]=uv_2_uvj[src_u[0]];\
dst_v[1]=uv_2_uvj[src_v[0]];\
dst_y[3]=  y_2_yj[src_y[3]];

#define CONVERT_Y    \
dst_y[0]=y_2_yj[src_y[0]];\
dst_y[1]=y_2_yj[src_y[1]];\
dst_y[2]=y_2_yj[src_y[2]];\
dst_y[3]=y_2_yj[src_y[3]];


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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_y[1]=y_2_yj[src_y[1]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[1]=yj_2_y[src_y[1]];

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
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];\
dst_y[0]=yj_2_y[src_y[0]];\
dst_y[1]=yj_2_y[src_y[1]];\
dst_y[2]=yj_2_y[src_y[2]];\
dst_y[3]=yj_2_y[src_y[3]];

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
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_u[1]=uv_2_uvj[src_u[0]];\
dst_v[1]=uv_2_uvj[src_v[0]];\
dst_y[0]=  y_2_yj[src_y[0]];\
dst_y[1]=  y_2_yj[src_y[1]];\
dst_y[2]=  y_2_yj[src_y[2]];\
dst_y[3]=  y_2_yj[src_y[3]];

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
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];\
dst_u[1]=uv_2_uvj[src_u[0]];\
dst_v[1]=uv_2_uvj[src_v[0]];\
dst_u[2]=uv_2_uvj[src_u[0]];\
dst_v[2]=uv_2_uvj[src_v[0]];\
dst_u[3]=uv_2_uvj[src_u[0]];\
dst_v[3]=uv_2_uvj[src_v[0]];\
dst_y[0]=  y_2_yj[src_y[0]];\
dst_y[1]=  y_2_yj[src_y[1]];\
dst_y[2]=  y_2_yj[src_y[2]];\
dst_y[3]=  y_2_yj[src_y[3]];

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
dst_y[0]=y_2_yj[src_y[0]];\
dst_u[0]=uv_2_uvj[src_u[0]];\
dst_v[0]=uv_2_uvj[src_v[0]];

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
dst_y[0]=yj_2_y[src_y[0]];\
dst_u[0]=uvj_2_uv[src_u[0]];\
dst_v[0]=uvj_2_uv[src_v[0]];

#include "../csp_planar_planar.h"


#ifdef SCANLINE
void gavl_init_yuv_yuv_scanline_funcs_c(gavl_colorspace_function_table_t * tab)
#else     
void gavl_init_yuv_yuv_funcs_c(gavl_colorspace_function_table_t * tab)
#endif
  {
  tab->uyvy_to_yuy2            = uyvy_to_yuy2_c;
  
  tab->yuy2_to_yuv_420_p       = yuy2_to_yuv_420_p_c;
  tab->yuy2_to_yuv_410_p       = yuy2_to_yuv_410_p_c;
  tab->yuy2_to_yuv_422_p       = yuy2_to_yuv_422_p_c;
  tab->yuy2_to_yuv_411_p       = yuy2_to_yuv_411_p_c;
  tab->yuy2_to_yuv_444_p       = yuy2_to_yuv_444_p_c;

  tab->yuy2_to_yuvj_420_p      = yuy2_to_yuvj_420_p_c;
  tab->yuy2_to_yuvj_422_p      = yuy2_to_yuvj_422_p_c;
  tab->yuy2_to_yuvj_444_p      = yuy2_to_yuvj_444_p_c;
  
  tab->yuv_420_p_to_yuy2       = yuv_420_p_to_yuy2_c;
  tab->yuv_422_p_to_yuy2       = yuv_422_p_to_yuy2_c;
  tab->yuv_444_p_to_yuy2       = yuv_444_p_to_yuy2_c;

  tab->yuvj_420_p_to_yuy2      = yuvj_420_p_to_yuy2_c;
  tab->yuvj_422_p_to_yuy2      = yuvj_422_p_to_yuy2_c;
  tab->yuvj_444_p_to_yuy2      = yuvj_444_p_to_yuy2_c;

  tab->uyvy_to_yuv_420_p       = uyvy_to_yuv_420_p_c;
  tab->uyvy_to_yuv_410_p       = uyvy_to_yuv_410_p_c;
  tab->uyvy_to_yuv_422_p       = uyvy_to_yuv_422_p_c;
  tab->uyvy_to_yuv_411_p       = uyvy_to_yuv_411_p_c;
  tab->uyvy_to_yuv_444_p       = uyvy_to_yuv_444_p_c;

  tab->uyvy_to_yuvj_420_p      = uyvy_to_yuvj_420_p_c;
  tab->uyvy_to_yuvj_422_p      = uyvy_to_yuvj_422_p_c;
  tab->uyvy_to_yuvj_444_p      = uyvy_to_yuvj_444_p_c;
  
  tab->yuv_420_p_to_uyvy       = yuv_420_p_to_uyvy_c;
  tab->yuv_422_p_to_uyvy       = yuv_422_p_to_uyvy_c;
  tab->yuv_444_p_to_uyvy       = yuv_444_p_to_uyvy_c;

  tab->yuvj_420_p_to_uyvy      = yuvj_420_p_to_uyvy_c;
  tab->yuvj_422_p_to_uyvy      = yuvj_422_p_to_uyvy_c;
  tab->yuvj_444_p_to_uyvy      = yuvj_444_p_to_uyvy_c;
  
  tab->yuv_420_p_to_yuv_444_p  = yuv_420_p_to_yuv_444_p_c;
  tab->yuv_420_p_to_yuvj_444_p = yuv_420_p_to_yuvj_444_p_c;
  tab->yuvj_420_p_to_yuv_444_p = yuvj_420_p_to_yuv_444_p_c;

  tab->yuv_422_p_to_yuv_444_p  = yuv_422_p_to_yuv_444_p_c;
  tab->yuv_422_p_to_yuvj_444_p = yuv_422_p_to_yuvj_444_p_c;
  tab->yuvj_422_p_to_yuv_444_p = yuvj_422_p_to_yuv_444_p_c;
  
  tab->yuv_444_p_to_yuv_420_p  = yuv_444_p_to_yuv_420_p_c;
  tab->yuv_444_p_to_yuvj_420_p = yuv_444_p_to_yuvj_420_p_c;
  tab->yuvj_444_p_to_yuv_420_p = yuvj_444_p_to_yuv_420_p_c;

  tab->yuv_444_p_to_yuv_422_p  = yuv_444_p_to_yuv_422_p_c;
  tab->yuv_444_p_to_yuvj_422_p = yuv_444_p_to_yuvj_422_p_c;
  tab->yuvj_444_p_to_yuv_422_p = yuvj_444_p_to_yuv_422_p_c;
  tab->yuvj_444_p_to_yuv_411_p = yuvj_444_p_to_yuv_411_p_c;
  tab->yuvj_444_p_to_yuv_410_p = yuvj_444_p_to_yuv_410_p_c;

  tab->yuv_444_p_to_yuv_411_p = yuv_444_p_to_yuv_411_p_c;
  tab->yuv_444_p_to_yuv_410_p = yuv_444_p_to_yuv_410_p_c;
  
  tab->yuv_420_p_to_yuv_422_p  = yuv_420_p_to_yuv_422_p_generic;
  tab->yuv_420_p_to_yuv_411_p  = yuv_420_p_to_yuv_411_p_c;
  tab->yuv_420_p_to_yuv_410_p  = yuv_420_p_to_yuv_410_p_c;
  tab->yuv_420_p_to_yuvj_422_p = yuv_420_p_to_yuvj_422_p_c;
  tab->yuvj_420_p_to_yuv_422_p = yuvj_420_p_to_yuv_422_p_c;
  tab->yuvj_420_p_to_yuv_411_p = yuvj_420_p_to_yuv_411_p_c;
  tab->yuvj_420_p_to_yuv_410_p = yuvj_420_p_to_yuv_410_p_c;

  tab->yuv_410_p_to_yuv_411_p  = yuv_410_p_to_yuv_411_p_generic;
  tab->yuv_410_p_to_yuy2       = yuv_410_p_to_yuy2_c;
  tab->yuv_410_p_to_uyvy       = yuv_410_p_to_uyvy_c;
  tab->yuv_410_p_to_yuv_444_p  = yuv_410_p_to_yuv_444_p_c;
  tab->yuv_410_p_to_yuvj_444_p = yuv_410_p_to_yuvj_444_p_c;
  tab->yuv_410_p_to_yuv_420_p  = yuv_410_p_to_yuv_420_p_c;
  tab->yuv_410_p_to_yuvj_420_p = yuv_410_p_to_yuvj_420_p_c;
  tab->yuv_410_p_to_yuvj_422_p = yuv_410_p_to_yuvj_422_p_c;
  tab->yuv_410_p_to_yuv_422_p  = yuv_410_p_to_yuv_422_p_c;
  tab->yuv_410_p_to_yuv_420_p  = yuv_410_p_to_yuv_420_p_c;
  
  tab->yuv_422_p_to_yuv_420_p  = yuv_422_p_to_yuv_420_p_generic;
  tab->yuv_422_p_to_yuvj_420_p = yuv_422_p_to_yuvj_420_p_c;
  tab->yuvj_422_p_to_yuv_420_p = yuvj_422_p_to_yuv_420_p_c;
  tab->yuv_422_p_to_yuv_411_p  = yuv_422_p_to_yuv_411_p_c;
  tab->yuvj_422_p_to_yuv_411_p = yuvj_422_p_to_yuv_411_p_c;
  tab->yuv_422_p_to_yuv_410_p  = yuv_422_p_to_yuv_410_p_c;
  tab->yuvj_422_p_to_yuv_410_p = yuvj_422_p_to_yuv_410_p_c;

  tab->yuv_411_p_to_yuv_410_p  = yuv_411_p_to_yuv_410_p_generic;
  tab->yuv_411_p_to_yuy2       = yuv_411_p_to_yuy2_c;
  tab->yuv_411_p_to_uyvy       = yuv_411_p_to_uyvy_c;
  tab->yuv_411_p_to_yuv_420_p  = yuv_411_p_to_yuv_420_p_c;
  tab->yuv_411_p_to_yuvj_420_p = yuv_411_p_to_yuvj_420_p_c;
  tab->yuv_411_p_to_yuv_444_p  = yuv_411_p_to_yuv_444_p_c;
  tab->yuv_411_p_to_yuvj_444_p = yuv_411_p_to_yuvj_444_p_c;
  tab->yuv_411_p_to_yuv_422_p  = yuv_411_p_to_yuv_422_p_c;
  tab->yuv_411_p_to_yuvj_422_p = yuv_411_p_to_yuvj_422_p_c;
  
  
  tab->yuv_420_p_to_yuvj_420_p = yuv_420_p_to_yuvj_420_p_c;
  tab->yuvj_420_p_to_yuv_420_p = yuvj_420_p_to_yuv_420_p_c;

  tab->yuv_422_p_to_yuvj_422_p = yuv_422_p_to_yuvj_422_p_c;
  tab->yuvj_422_p_to_yuv_422_p = yuvj_422_p_to_yuv_422_p_c;

  tab->yuv_444_p_to_yuvj_444_p = yuv_444_p_to_yuvj_444_p_c;
  tab->yuvj_444_p_to_yuv_444_p = yuvj_444_p_to_yuv_444_p_c;

  }
