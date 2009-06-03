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

#include "../colorspace_macros.h"
#include "c_macros.h"

#include "../_video_copy.c"

/*****************************************************
 *
 * C YUV <-> YUV Conversions
 *
 ******************************************************/

static void yuy2_to_yuv_420_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 2)

  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)

    SCANLINE_LOOP_START_PACKED

    dst_y[0] = src[0];
    *dst_u   = src[1];
    dst_y[1] = src[2];
    *dst_v   = src[3];

    SCANLINE_LOOP_END_PACKED_PLANAR(4, 2, 1)

#ifndef SCANLINE
      
  CONVERSION_FUNC_MIDDLE_PACKED_TO_420(uint8_t, uint8_t)

    SCANLINE_LOOP_START_PACKED

    dst_y[0] = src[0];
    dst_y[1] = src[2];

    SCANLINE_LOOP_END_PACKED_TO_420_Y(4, 2)

#endif
      
  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void yuy2_to_yuv_422_p_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PACKED_PLANAR(uint8_t, uint8_t, 2, 1)

  CONVERSION_LOOP_START_PACKED_PLANAR(uint8_t, uint8_t)

    SCANLINE_LOOP_START_PACKED

    dst_y[0] = src[0];
    *dst_u   = src[1];
    dst_y[1] = src[2];
    *dst_v   = src[3];

    SCANLINE_LOOP_END_PACKED_PLANAR(4, 2, 1)

  CONVERSION_FUNC_END_PACKED_PLANAR
  }

static void yuv_420_p_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 2)

  CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)

    SCANLINE_LOOP_START_PLANAR

    dst[0] = src_y[0];
    dst[1] = *src_u;
    dst[2] = src_y[1];
    dst[3] = *src_v;

    SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 4)

#ifndef SCANLINE
      
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint8_t)

    SCANLINE_LOOP_START_PLANAR

    dst[0] = src_y[0];
    dst[1] = *src_u;
    dst[2] = src_y[1];
    dst[3] = *src_v;

    SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 4)

#endif
      
  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_yuy2_c(gavl_video_convert_context_t * ctx)
  {
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 1)

  CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)

    SCANLINE_LOOP_START_PLANAR

    dst[0] = src_y[0];
    dst[1] = *src_u;
    dst[2] = src_y[1];
    dst[3] = *src_v;

    SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 4)
      
  CONVERSION_FUNC_END_PLANAR_PACKED

  }

#ifdef SCANLINE
void gavl_init_yuv_yuv_scanline_funcs_c(gavl_colorspace_function_table_t * tab)
#else     
void gavl_init_yuv_yuv_funcs_c(gavl_colorspace_function_table_t * tab)
#endif
  {
  tab->yuy2_to_yuv_420_p      = yuy2_to_yuv_420_p_c;
  tab->yuy2_to_yuv_422_p      = yuy2_to_yuv_422_p_c;

  tab->yuv_420_p_to_yuy2      = yuv_420_p_to_yuy2_c;
  tab->yuv_422_p_to_yuy2      = yuv_422_p_to_yuy2_c;

  tab->yuv_420_p_to_yuv_422_p = yuv_420_p_to_yuv_422_p_generic;
  tab->yuv_422_p_to_yuv_420_p = yuv_422_p_to_yuv_420_p_generic;
  }
