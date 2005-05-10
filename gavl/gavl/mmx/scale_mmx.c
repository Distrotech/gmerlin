/*****************************************************************

  colorspace_mmx.c 

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

#include <stdio.h>
#include <config.h>
#include <gavl.h>
#include <video.h>
#include <colorspace.h>
#include <attributes.h>
#include <scale.h>

#ifdef MMXEXT
#define MOVQ_R2M(reg,mem) movntq_r2m(reg, mem)
#else
#define MOVQ_R2M(reg,mem) movq_r2m(reg, mem)
#endif

#include "mmx.h"
#include "interpolate.h"

#define SCALE_FUNC_HEAD(a) \
  for(i = 0; i < s->table[plane].num_coeffs_h; i+=a)       \
    {

#define SCALE_FUNC_TAIL \
    }

/* Bilinear in x direction */

#define LOAD_FACTORS_BILINEAR_X \
  tmp.uw[0] = (s->table[plane].coeffs_h[i].factor[0] >> 1);\
  tmp.uw[1] = tmp.uw[0];                                   \
  tmp.uw[2] = tmp.uw[0];                                   \
  tmp.uw[3] = tmp.uw[0];                                   \
  movq_m2r(tmp, mm4);

#define LOAD_FACTORS_BILINEAR_Y \
  tmp.uw[0] = (s->table[plane].coeffs_v[scanline].factor[0] >> 1);\
  tmp.uw[1] = tmp.uw[0];                                   \
  tmp.uw[2] = tmp.uw[0];                                   \
  tmp.uw[3] = tmp.uw[0];                                   \
  movq_m2r(tmp, mm4);

#ifdef SLOW_FUNCTIONS

static void scale_x_15_16_bilinear_mmx(gavl_video_scaler_t * s,
                                   uint8_t * _src,
                                   int src_stride,
                                   uint8_t * _dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  uint16_t * src;
  uint16_t * dst;
  uint16_t * src_1;
  uint16_t * src_2;

  INTERPOLATE_INIT_TEMP

  src = (uint16_t*)(_src + scanline * src_stride);
  dst = (uint16_t*)_dst;
  
  SCALE_FUNC_HEAD(1)
    src_1 = src + s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 1;

    LOAD_FACTORS_BILINEAR_X
    INTERPOLATE_1D_LOAD_SRC_1_15
    INTERPOLATE_1D_LOAD_SRC_2_15
    INTERPOLATE_1D
    INTERPOLATE_WRITE_15
    
    dst++;
  SCALE_FUNC_TAIL
  emms();
  }

static void scale_x_16_16_bilinear_mmx(gavl_video_scaler_t * s,
                                   uint8_t * _src,
                                   int src_stride,
                                   uint8_t * _dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  uint16_t * src;
  uint16_t * dst;
  uint16_t * src_1;
  uint16_t * src_2;

  INTERPOLATE_INIT_TEMP

  src = (uint16_t*)(_src + scanline * src_stride);
  dst = (uint16_t*)_dst;
  
  SCALE_FUNC_HEAD(1)
    src_1 = src + s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 1;

    LOAD_FACTORS_BILINEAR_X
    INTERPOLATE_1D_LOAD_SRC_1_16
    INTERPOLATE_1D_LOAD_SRC_2_16
    INTERPOLATE_1D
    INTERPOLATE_WRITE_16
    
    dst++;
  SCALE_FUNC_TAIL
  emms();
  }

static void scale_x_24_24_bilinear_mmx(gavl_video_scaler_t * s,
                                     uint8_t * _src,
                                     int src_stride,
                                     uint8_t * dst,
                                     int plane,
                                     int scanline)
  {
  int i;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;

  INTERPOLATE_INIT_TEMP

  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(1)
    src_1 = src + 3 * s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 3;

    LOAD_FACTORS_BILINEAR_X
    INTERPOLATE_1D_LOAD_SRC_1_24
    INTERPOLATE_1D_LOAD_SRC_2_24
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGB24

    dst += 3;
  
  SCALE_FUNC_TAIL
  emms();
  }
#endif // SLOW_FUNCTIONS

static void scale_x_24_32_bilinear_mmx(gavl_video_scaler_t * s,
                                   uint8_t * _src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;

  INTERPOLATE_INIT_TEMP

  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(1)
    src_1 = src + 4 * s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 4;

    LOAD_FACTORS_BILINEAR_X
    INTERPOLATE_1D_LOAD_SRC_1_32
    INTERPOLATE_1D_LOAD_SRC_2_32
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGB32

    dst += 4;
  
  SCALE_FUNC_TAIL
  emms();
  }

static void scale_x_32_32_bilinear_mmx(gavl_video_scaler_t * s,
                                   uint8_t * _src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  uint8_t * src;
  uint8_t * src_1;
  uint8_t * src_2;

  INTERPOLATE_INIT_TEMP

  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(1)
    src_1 = src + 4 * s->table[plane].coeffs_h[i].index;
    src_2 = src_1 + 4;

    LOAD_FACTORS_BILINEAR_X
    INTERPOLATE_1D_LOAD_SRC_1_32
    INTERPOLATE_1D_LOAD_SRC_2_32
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGBA32

    dst += 4;
  
  SCALE_FUNC_TAIL
  emms();
  }

#ifdef SLOW_FUNCTIONS

static void scale_x_8_bilinear_mmx(gavl_video_scaler_t * s,
                               uint8_t * _src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i;
  uint8_t * src;
  INTERPOLATE_INIT_TEMP
  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(4)
    tmp.uw[0] = *(src + s->table[plane].coeffs_h[i].index);
    tmp.uw[1] = *(src + s->table[plane].coeffs_h[i+1].index);
    tmp.uw[2] = *(src + s->table[plane].coeffs_h[i+2].index);
    tmp.uw[3] = *(src + s->table[plane].coeffs_h[i+3].index);
    movq_m2r(tmp, mm0);

    tmp.uw[0] = *(src + s->table[plane].coeffs_h[i].index+1);
    tmp.uw[1] = *(src + s->table[plane].coeffs_h[i+1].index+1);
    tmp.uw[2] = *(src + s->table[plane].coeffs_h[i+2].index+1);
    tmp.uw[3] = *(src + s->table[plane].coeffs_h[i+3].index+1);
    movq_m2r(tmp, mm1);

    tmp.uw[0] = *(src + s->table[plane].coeffs_h[i].factor[0]);
    tmp.uw[1] = *(src + s->table[plane].coeffs_h[i+1].factor[0]);
    tmp.uw[2] = *(src + s->table[plane].coeffs_h[i+2].factor[0]);
    tmp.uw[3] = *(src + s->table[plane].coeffs_h[i+3].factor[0]);
    movq_m2r(tmp, mm4);
    psrlw_i2r(1, mm4);
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGBA32    
    
    dst+=4;
  SCALE_FUNC_TAIL
  emms();
  }


static void scale_x_8_bilinear_advance(gavl_video_scaler_t * s,
                                       uint8_t * _src,
                                       int src_stride,
                                       uint8_t * dst,
                                       int plane,
                                       int scanline,
                                       int advance)
  {
  int i;
  uint8_t * src;
  INTERPOLATE_INIT_TEMP
  src = _src + scanline * src_stride;

  SCALE_FUNC_HEAD(4)
    tmp.uw[0] = *(src + advance * s->table[plane].coeffs_h[i].index);
    tmp.uw[1] = *(src + advance * s->table[plane].coeffs_h[i+1].index);
    tmp.uw[2] = *(src + advance * s->table[plane].coeffs_h[i+2].index);
    tmp.uw[3] = *(src + advance * s->table[plane].coeffs_h[i+3].index);
    movq_m2r(tmp, mm0);

    tmp.uw[0] = *(src + advance * (s->table[plane].coeffs_h[i].index + 1));
    tmp.uw[1] = *(src + advance * (s->table[plane].coeffs_h[i+1].index + 1));
    tmp.uw[2] = *(src + advance * (s->table[plane].coeffs_h[i+2].index + 1));
    tmp.uw[3] = *(src + advance * (s->table[plane].coeffs_h[i+3].index + 1));
    movq_m2r(tmp, mm1);

    tmp.uw[0] = s->table[plane].coeffs_h[i].factor[0];
    tmp.uw[1] = s->table[plane].coeffs_h[i+1].factor[0];
    tmp.uw[2] = s->table[plane].coeffs_h[i+2].factor[0];
    tmp.uw[3] = s->table[plane].coeffs_h[i+3].factor[0];
    movq_m2r(tmp, mm4);
    psrlw_i2r(1, mm4);
    INTERPOLATE_1D
    MOVQ_R2M(mm7, tmp);
    dst[0] = tmp.ub[0];
    dst[advance] = tmp.ub[2];
    dst[2*advance] = tmp.ub[4];
    dst[3*advance] = tmp.ub[6];
    
    dst+=4 * advance;
  SCALE_FUNC_TAIL
  }


static void scale_x_yuy2_bilinear_mmx(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  scale_x_8_bilinear_advance(s,
                             src,
                             src_stride,
                             dst,
                             0,
                             scanline, 2);
  
  scale_x_8_bilinear_advance(s,
                             src+1,
                             src_stride,
                             dst+1,
                             1,
                             scanline, 4);
  
  scale_x_8_bilinear_advance(s,
                             src+3,
                             src_stride,
                             dst+3,
                             1,
                             scanline, 4);
  emms();
  }

static void scale_x_uyvy_bilinear_mmx(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  scale_x_8_bilinear_advance(s,
                             src+1,
                             src_stride,
                             dst+1,
                             0,
                             scanline, 2);

  scale_x_8_bilinear_advance(s,
                             src,
                             src_stride,
                             dst,
                             1,
                             scanline, 4);

  scale_x_8_bilinear_advance(s,
                             src+2,
                             src_stride,
                             dst+2,
                             1,
                             scanline, 4);

  emms();
  }



/* Bilinear y */

static void scale_y_15_16_bilinear_mmx(gavl_video_scaler_t * s,
                                     uint8_t * src,
                                     int src_stride,
                                     uint8_t * _dst,
                                     int plane,
                                     int scanline)
  {
  int i;
  uint16_t * dst;
  uint16_t * src_1;
  uint16_t * src_2;
  uint16_t * src_start_1;
  uint16_t * src_start_2;

  INTERPOLATE_INIT_TEMP
    
  src_start_1 = (uint16_t*)(src + s->table[plane].coeffs_v[scanline].index * src_stride);
  src_start_2 = (uint16_t*)((uint8_t*)src_start_1 + src_stride);
  dst = (uint16_t*)_dst;

  LOAD_FACTORS_BILINEAR_Y
  
  SCALE_FUNC_HEAD(1)
    src_1 = src_start_1 + i;
    src_2 = src_start_2 + i;

    INTERPOLATE_1D_LOAD_SRC_1_15
    INTERPOLATE_1D_LOAD_SRC_2_15
    INTERPOLATE_1D
    INTERPOLATE_WRITE_15
    
    dst++;
  SCALE_FUNC_TAIL
  emms();
  }

static void scale_y_16_16_bilinear_mmx(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * _dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  uint16_t * dst;
  uint16_t * src_1;
  uint16_t * src_2;
  uint16_t * src_start_1;
  uint16_t * src_start_2;

  INTERPOLATE_INIT_TEMP
  src_start_1 = (uint16_t*)(src + s->table[plane].coeffs_v[scanline].index * src_stride);
  src_start_2 = (uint16_t*)((uint8_t*)src_start_1 + src_stride);

  dst = (uint16_t*)_dst;

  LOAD_FACTORS_BILINEAR_Y
  
  SCALE_FUNC_HEAD(1)
    src_1 = src_start_1 + i;
    src_2 = src_start_2 + i;

    INTERPOLATE_1D_LOAD_SRC_1_16
    INTERPOLATE_1D_LOAD_SRC_2_16
    INTERPOLATE_1D
    INTERPOLATE_WRITE_16
    
    dst++;
  SCALE_FUNC_TAIL
  emms();
  }

static void scale_y_24_24_bilinear_mmx(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  uint8_t * src_start_1;
  uint8_t * src_start_2;
  uint8_t * src_1;
  uint8_t * src_2;

  INTERPOLATE_INIT_TEMP

  src_start_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  LOAD_FACTORS_BILINEAR_Y
  
  SCALE_FUNC_HEAD(1)
    src_1 = src_start_1 + 3 * i;
    src_2 = src_start_2 + 3 * i;

    INTERPOLATE_1D_LOAD_SRC_1_24
    INTERPOLATE_1D_LOAD_SRC_2_24
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGB24
    
    dst += 3;
  SCALE_FUNC_TAIL
  emms();
  }
#endif // SLOW_FUNCTIONS

static void scale_y_24_32_bilinear_mmx(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  uint8_t * src_start_1;
  uint8_t * src_start_2;
  uint8_t * src_1;
  uint8_t * src_2;

  INTERPOLATE_INIT_TEMP

  src_start_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  LOAD_FACTORS_BILINEAR_Y
  
  SCALE_FUNC_HEAD(1)
    src_1 = src_start_1 + 4 * i;
    src_2 = src_start_2 + 4 * i;

    INTERPOLATE_1D_LOAD_SRC_1_32
    INTERPOLATE_1D_LOAD_SRC_2_32
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGB32

    dst += 4;
  SCALE_FUNC_TAIL
  emms();
  }

static void scale_y_32_32_bilinear_mmx(gavl_video_scaler_t * s,
                                   uint8_t * src,
                                   int src_stride,
                                   uint8_t * dst,
                                   int plane,
                                   int scanline)
  {
  int i;
  uint8_t * src_start_1;
  uint8_t * src_start_2;
  uint8_t * src_1;
  uint8_t * src_2;
  
  INTERPOLATE_INIT_TEMP

  src_start_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_start_2 = src_start_1 + src_stride;

  LOAD_FACTORS_BILINEAR_Y
  
  SCALE_FUNC_HEAD(1)
    src_1 = src_start_1 + 4 * i;
    src_2 = src_start_2 + 4 * i;

    INTERPOLATE_1D_LOAD_SRC_1_32
    INTERPOLATE_1D_LOAD_SRC_2_32
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGBA32
    
    dst += 4;
  SCALE_FUNC_TAIL
  emms();
  }

static void scale_y_8_bilinear_mmx(gavl_video_scaler_t * s,
                               uint8_t * src,
                               int src_stride,
                               uint8_t * dst,
                               int plane,
                               int scanline)
  {
  int i;
  uint8_t * src_1;
  uint8_t * src_2;

  INTERPOLATE_INIT_TEMP

  src_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_2 = src_1 + src_stride;

  tmp.uw[0] = (s->table[plane].coeffs_v[scanline].factor[0] >> 1);
  tmp.uw[1] = tmp.uw[0];
  tmp.uw[2] = tmp.uw[0];
  tmp.uw[3] = tmp.uw[0];
  movq_m2r(tmp, mm4);
  
  SCALE_FUNC_HEAD(4)
    
    
    INTERPOLATE_1D_LOAD_SRC_1_32
    INTERPOLATE_1D_LOAD_SRC_2_32
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGBA32
    
    src_1+=4;
    src_2+=4;

    dst+=4;
  SCALE_FUNC_TAIL
  emms();
  }

static void scale_y_yuv_packed_bilinear_mmx(gavl_video_scaler_t * s,
                                  uint8_t * src,
                                  int src_stride,
                                  uint8_t * dst,
                                  int plane,
                                  int scanline)
  {
  int i;
  uint8_t * src_1;
  uint8_t * src_2;

  INTERPOLATE_INIT_TEMP

  src_1 = src + s->table[plane].coeffs_v[scanline].index * src_stride;
  src_2 = src_1 + src_stride;

  tmp.uw[0] = (s->table[plane].coeffs_v[scanline].factor[0] >> 1);
  tmp.uw[1] = tmp.uw[0];
  tmp.uw[2] = tmp.uw[0];
  tmp.uw[3] = tmp.uw[0];
  
  movq_m2r(tmp, mm4);
    
  SCALE_FUNC_HEAD(2)
    
    
    INTERPOLATE_1D_LOAD_SRC_1_32
    INTERPOLATE_1D_LOAD_SRC_2_32
    INTERPOLATE_1D
    INTERPOLATE_WRITE_RGBA32
    
    src_1+=4;
    src_2+=4;

    dst+=4;
  SCALE_FUNC_TAIL
  emms();
  }

#ifdef MMXEXT
void gavl_init_scale_funcs_mmxext(gavl_scale_funcs_t * tab,
                                  gavl_scale_mode_t scale_mode,
                                  int scale_x, int scale_y, int min_scanline_width)
#else
void gavl_init_scale_funcs_mmx(gavl_scale_funcs_t * tab,
                               gavl_scale_mode_t scale_mode,
                               int scale_x, int scale_y, int min_scanline_width)
  
#endif
  {
  //  fprintf(stderr, "gavl_init_scale_funcs_mmx %d %d\n", scale_x, scale_y);
  switch(scale_mode)
    {
    case GAVL_SCALE_AUTO:
    case GAVL_SCALE_NEAREST:
      break;
    case GAVL_SCALE_BILINEAR:
      if(scale_x && scale_y)
        {
#if 0
        tab->scale_15_16 = scale_xy_15_16_bilinear_mmx;
        tab->scale_16_16 = scale_xy_16_16_bilinear_mmx;
        tab->scale_24_24 = scale_xy_24_24_bilinear_mmx;
        tab->scale_24_32 = scale_xy_24_32_bilinear_mmx;
        tab->scale_32_32 = scale_xy_32_32_bilinear_mmx;
        
        tab->scale_8     = scale_xy_8_bilinear_mmx;
        tab->scale_yuy2  = scale_xy_yuy2_bilinear_mmx;
        tab->scale_uyvy  = scale_xy_uyvy_bilinear_mmx;
#endif
        }
      else if(scale_x)
        {
#ifdef SLOW_FUNCTIONS
        tab->scale_15_16 = scale_x_15_16_bilinear_mmx;
        tab->scale_16_16 = scale_x_16_16_bilinear_mmx;
        tab->scale_24_24 = scale_x_24_24_bilinear_mmx;
        tab->scale_yuy2  = scale_x_yuy2_bilinear_mmx;
        tab->scale_uyvy  = scale_x_uyvy_bilinear_mmx;
        if(!(min_scanline_width % 4))
          {
          tab->scale_8     = scale_x_8_bilinear_mmx;
          }
#endif
        tab->scale_32_32 = scale_x_32_32_bilinear_mmx;
        tab->scale_24_32 = scale_x_24_32_bilinear_mmx;
        }
      else if(scale_y)
        {
#ifdef SLOW_FUNCTIONS
        tab->scale_15_16 = scale_y_15_16_bilinear_mmx;
        tab->scale_16_16 = scale_y_16_16_bilinear_mmx;
        tab->scale_24_24 = scale_y_24_24_bilinear_mmx;
#endif
        tab->scale_24_32 = scale_y_24_32_bilinear_mmx;
        tab->scale_32_32 = scale_y_32_32_bilinear_mmx;
        if(!(min_scanline_width % 4))
          {
          tab->scale_8     = scale_y_8_bilinear_mmx;
          }

        tab->scale_yuy2  = scale_y_yuv_packed_bilinear_mmx;
        tab->scale_uyvy  = scale_y_yuv_packed_bilinear_mmx;
        }
      break;
    case GAVL_SCALE_NONE:
      break;
    }
  }

