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

/* YUV 420 P -> YUY2 */

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

/* YUV 422 P -> YUY2 */

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

/* YUV 422 P -> YUY2 */

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

#define FUNC_NAME     yuv_420_p_to_yuv_444_p
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

#define FUNC_NAME     yuv_422_p_to_yuv_444_p
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

#define FUNC_NAME     yuv_444_p_to_yuv_420_p
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


#define FUNC_NAME     yuv_444_p_to_yuv_422_p
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

#ifdef SCANLINE
void gavl_init_yuv_yuv_scanline_funcs_c(gavl_colorspace_function_table_t * tab)
#else     
void gavl_init_yuv_yuv_funcs_c(gavl_colorspace_function_table_t * tab)
#endif
  {
  tab->yuy2_to_yuv_420_p      = yuy2_to_yuv_420_p_c;
  tab->yuy2_to_yuv_422_p      = yuy2_to_yuv_422_p_c;
  tab->yuy2_to_yuv_444_p      = yuy2_to_yuv_444_p_c;

  tab->yuv_420_p_to_yuy2      = yuv_420_p_to_yuy2_c;
  tab->yuv_422_p_to_yuy2      = yuv_422_p_to_yuy2_c;
  tab->yuv_444_p_to_yuy2      = yuv_444_p_to_yuy2_c;

  tab->yuv_420_p_to_yuv_444_p = yuv_420_p_to_yuv_444_p;
  tab->yuv_422_p_to_yuv_444_p = yuv_422_p_to_yuv_444_p;

  tab->yuv_444_p_to_yuv_420_p = yuv_444_p_to_yuv_420_p;
  tab->yuv_444_p_to_yuv_422_p = yuv_444_p_to_yuv_422_p;
  
  tab->yuv_420_p_to_yuv_422_p = yuv_420_p_to_yuv_422_p_generic;
  tab->yuv_422_p_to_yuv_420_p = yuv_422_p_to_yuv_420_p_generic;
  }
