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

static int y_to_rgb[0x100];
static int v_to_r[0x100];
static int u_to_g[0x100];
static int v_to_g[0x100];
static int u_to_b[0x100];

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
    
    y_to_rgb[i] = (int)(1.164*(i-16)) * 0x10000;
    
    v_to_r[i]   = (int)( 1.596  * (i - 0x80) * 0x10000);
    u_to_g[i]   = (int)(-0.392  * (i - 0x80) * 0x10000);
    v_to_g[i]   = (int)(-0.813  * (i - 0x80) * 0x10000);
    u_to_b[i]   = (int)( 2.017 * (i - 0x80) * 0x10000);
    }
  }

#define RECLIP(color) (uint8_t)((color>0xFF)?0xff:((color<0)?0:color))

#define YUV_2_RGB(y,u,v,r,g,b) i_tmp=(y_to_rgb[y]+v_to_r[v])>>16;\
                               r=RECLIP(i_tmp);\
                               i_tmp=(y_to_rgb[y]+u_to_g[u]+v_to_g[v])>>16;\
                               g=RECLIP(i_tmp);\
                               i_tmp=(y_to_rgb[y]+u_to_b[u])>>16;\
                               b=RECLIP(i_tmp);

#define PACK_RGB16(r,g,b,pixel) pixel=((((((r<<5)&0xff00)|g)<<6)&0xfff00)|b)>>3
#define PACK_BGR16(r,g,b,pixel) pixel=((((((b<<5)&0xff00)|g)<<6)&0xfff00)|r)>>3

#define PACK_RGB15(r,g,b,pixel) pixel=((((((r<<5)&0xff00)|g)<<5)&0xfff00)|b)>>3
#define PACK_BGR15(r,g,b,pixel) pixel=((((((b<<5)&0xff00)|g)<<5)&0xfff00)|r)>>3

static void yuy2_to_rgb_15_c(gavl_video_convert_context_t * ctx)
  {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  int i_tmp;
  
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 2, 1)
  CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PACKED

  YUV_2_RGB(src[0], src[1], src[3], r, g, b)
  PACK_RGB15(r, g, b, dst[0]);

  YUV_2_RGB(src[2], src[1], src[3], r, g, b)
  PACK_RGB15(r, g, b, dst[1]);
    
  SCANLINE_LOOP_END_PACKED_PACKED(4, 2)
     
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void yuy2_to_bgr_15_c(gavl_video_convert_context_t * ctx)
  {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  int i_tmp;

  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 2, 1)
    CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PACKED

  YUV_2_RGB(src[0], src[1], src[3], r, g, b)
  PACK_BGR15(r, g, b, dst[0]);

  YUV_2_RGB(src[2], src[1], src[3], r, g, b)
  PACK_BGR15(r, g, b, dst[1]);
    
  SCANLINE_LOOP_END_PACKED_PACKED(4, 2)

  CONVERSION_FUNC_END_PACKED_PACKED
  }
                             
static void yuy2_to_rgb_16_c(gavl_video_convert_context_t * ctx)
  {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  int i_tmp;

  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 2, 1)
    CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PACKED

  YUV_2_RGB(src[0], src[1], src[3], r, g, b)
  PACK_RGB16(r, g, b, dst[0]);

  YUV_2_RGB(src[2], src[1], src[3], r, g, b)
  PACK_RGB16(r, g, b, dst[1]);
    
  SCANLINE_LOOP_END_PACKED_PACKED(4, 2)

  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void yuy2_to_bgr_16_c(gavl_video_convert_context_t * ctx)
  {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  int i_tmp;

  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint16_t, 2, 1)
    CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PACKED

  YUV_2_RGB(src[0], src[1], src[3], r, g, b)
  PACK_BGR16(r, g, b, dst[0]);

  YUV_2_RGB(src[2], src[1], src[3], r, g, b)
  PACK_BGR16(r, g, b, dst[1]);
    
  SCANLINE_LOOP_END_PACKED_PACKED(4, 2)
     
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void yuy2_to_rgb_24_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  YUV_2_RGB(src[0], src[1], src[3], dst[0], dst[1], dst[2])
  YUV_2_RGB(src[2], src[1], src[3], dst[3], dst[4], dst[5])
      
  SCANLINE_LOOP_END_PACKED_PACKED(4, 6)
     
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void yuy2_to_bgr_24_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
  YUV_2_RGB(src[0], src[1], src[3], dst[2], dst[1], dst[0])
  YUV_2_RGB(src[2], src[1], src[3], dst[5], dst[4], dst[3])

  SCANLINE_LOOP_END_PACKED_PACKED(4, 6)
  
  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void yuy2_to_rgb_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  YUV_2_RGB(src[0], src[1], src[3], dst[0], dst[1], dst[2])
  YUV_2_RGB(src[2], src[1], src[3], dst[4], dst[5], dst[6])
      
  SCANLINE_LOOP_END_PACKED_PACKED(4, 8)

  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void yuy2_to_bgr_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED

  YUV_2_RGB(src[0], src[1], src[3], dst[2], dst[1], dst[0])
  YUV_2_RGB(src[2], src[1], src[3], dst[6], dst[5], dst[4])

  SCANLINE_LOOP_END_PACKED_PACKED(4, 8)

  CONVERSION_FUNC_END_PACKED_PACKED
  }

static void yuy2_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PACKED_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PACKED_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PACKED
  YUV_2_RGB(src[0], src[1], src[3], dst[0], dst[1], dst[2])
  dst[3] = 0xFF;
  YUV_2_RGB(src[2], src[1], src[3], dst[4], dst[5], dst[6])
  dst[7] = 0xFF;
  SCANLINE_LOOP_END_PACKED_PACKED(4, 8)
     
  CONVERSION_FUNC_END_PACKED_PACKED
  }
  
static void yuv_420_p_to_rgb_15_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint16_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_RGB15(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_RGB15(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)

#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_RGB15(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_RGB15(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)
#endif

  
  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_420_p_to_bgr_15_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint16_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_BGR15(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_BGR15(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)

#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_BGR15(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_BGR15(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)
#endif

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_420_p_to_rgb_16_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint16_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_RGB16(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_RGB16(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)

#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_RGB16(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_RGB16(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)
#endif

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_420_p_to_bgr_16_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint16_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_BGR16(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_BGR16(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)

#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint16_t)

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_BGR16(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_BGR16(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)
#endif

     
  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_420_p_to_rgb_24_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[3], dst[4], dst[5])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 6)

#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[3], dst[4], dst[5])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 6)
#endif
     
  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_420_p_to_bgr_24_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[5], dst[4], dst[3])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 6)

#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[5], dst[4], dst[3])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 6)
#endif

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_420_p_to_rgb_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 8)

#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 8)
#endif

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_420_p_to_bgr_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
    SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[6], dst[5], dst[4])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 8)

#ifndef SCANLINE
  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint8_t)

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[6], dst[5], dst[4])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 8)
#endif

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_420_p_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 2)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  dst[3] = 0xff;
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])
  dst[7] = 0xff;

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 8)

#ifndef SCANLINE

  CONVERSION_FUNC_MIDDLE_420_TO_PACKED(uint8_t, uint8_t)
     

  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  dst[3] = 0xff;
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])
  dst[7] = 0xff;

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1,  8)
#endif

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_rgb_15_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint16_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_RGB15(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_RGB15(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)
  
  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_bgr_15_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint16_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_BGR15(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_BGR15(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_rgb_16_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint16_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_RGB16(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_RGB16(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_bgr_16_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint16_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint16_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], r, g, b)
  PACK_BGR16(r, g, b, dst[0]);

  YUV_2_RGB(src_y[1], src_u[0], src_v[0], r, g, b)
  PACK_BGR16(r, g, b, dst[1]);

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 2)

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_rgb_24_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[3], dst[4], dst[5])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 6)

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_bgr_24_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[5], dst[4], dst[3])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 6)

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_rgb_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])

  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 8)

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_bgr_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PLANAR

  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[2], dst[1], dst[0])
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[6], dst[5], dst[4])
  
  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 8)

  CONVERSION_FUNC_END_PLANAR_PACKED
  }

static void yuv_422_p_to_rgba_32_c(gavl_video_convert_context_t * ctx)
  {
  int i_tmp;
  CONVERSION_FUNC_START_PLANAR_PACKED(uint8_t, uint8_t, 2, 1)
    CONVERSION_LOOP_START_PLANAR_PACKED(uint8_t, uint8_t)
  SCANLINE_LOOP_START_PLANAR
  YUV_2_RGB(src_y[0], src_u[0], src_v[0], dst[0], dst[1], dst[2])
  dst[3] = 0xff;
  YUV_2_RGB(src_y[1], src_u[0], src_v[0], dst[4], dst[5], dst[6])
  dst[7] = 0xff;
  SCANLINE_LOOP_END_PLANAR_PACKED(2, 1, 8)
     
  CONVERSION_FUNC_END_PLANAR_PACKED
  }

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
  }

#undef RECLIP

#undef YUV_2_RGB
#undef PACK_RGB16
#undef PACK_BGR16

#undef PACK_RGB15
#undef PACK_BGR15
