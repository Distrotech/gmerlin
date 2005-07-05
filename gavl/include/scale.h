/*****************************************************************

  scale.h

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

#include "video.h"

#define MAX_INTERPOL_POINTS 2 /* Must be increased to 4 for
                                 bicubic interpolation */

typedef struct
  {
  int index; /* Index of the first row/column */
  struct
    {
    float fac_f; /* Scaling coefficient with high precision */
    int fac_i;   /* Scaling with lower precision requested by
                    the initialization function for a plane */
    }
  factor[MAX_INTERPOL_POINTS];
  } gavl_video_scale_coeff_1D_t;

typedef void (*gavl_video_scale_scanline_func)(gavl_video_scaler_t*,
                                               uint8_t * src_plane,
                                               int src_stride,
                                               uint8_t * dst_line,
                                               int plane,
                                               int scanline);

typedef void (*gavl_video_scale_init_plane_func)(gavl_video_scaler_t*,
                                                 int plane);

/* Table for a whole plane */

typedef struct
  {
  int coeffs_h_alloc;
  int coeffs_v_alloc;

  int num_coeffs_h;
  int num_coeffs_v;
  
  gavl_video_scale_coeff_1D_t * coeffs_h;
  gavl_video_scale_coeff_1D_t * coeffs_v;
  gavl_video_scale_scanline_func scanline_func;
  
  
  int byte_advance;
  } gavl_video_scale_table_t;

void gavl_video_scale_table_init_int(gavl_video_scale_table_t table,
                                     int bits);

struct gavl_video_scaler_s
  {
  gavl_video_options_t opt;
  
  gavl_video_scale_table_t table[GAVL_MAX_PLANES];
  gavl_rectangle_t src_rect[GAVL_MAX_PLANES];
  gavl_rectangle_t dst_rect[GAVL_MAX_PLANES];
  
  int num_planes;

  gavl_video_frame_t * src;
  gavl_video_frame_t * dst;

  gavl_colorspace_t colorspace;

  /* Needed for copying only */
  gavl_video_format_t copy_format;
  
  int action; /* What to do */
  };

/* Scale functions */

typedef struct
  {
  gavl_video_scale_scanline_func scale_15_16;
  gavl_video_scale_scanline_func scale_16_16;
  gavl_video_scale_scanline_func scale_24_24;
  gavl_video_scale_scanline_func scale_24_32;
  gavl_video_scale_scanline_func scale_32_32;
  gavl_video_scale_scanline_func scale_uint16_x_1;
  gavl_video_scale_scanline_func scale_uint16_x_3;
  gavl_video_scale_scanline_func scale_uint16_x_4;
  gavl_video_scale_scanline_func scale_float_x_3;
  gavl_video_scale_scanline_func scale_float_x_4;
  gavl_video_scale_scanline_func scale_8;
  gavl_video_scale_scanline_func scale_yuy2;
  gavl_video_scale_scanline_func scale_uyvy;

  /* Bits needed for the integer scaling coefficient
     (0 when float is used) */
  
  int bits_15_16;
  int bits_16_16;
  int bits_24_24;
  int bits_24_32;
  int bits_32_32;
  int bits_uint16_x_1;
  int bits_uint16_x_3;
  int bits_uint16_x_4;
  int bits_float_x_3;
  int bits_float_x_4;
  int bits_8;
  int bits_yuy2;
  int bits_uyvy;

  
  } gavl_scale_funcs_t;

void gavl_init_scale_funcs_c(gavl_scale_funcs_t * tab,
                             gavl_scale_mode_t scale_mode,
                             int scale_x, int scale_y);
#ifdef ARCH_X86

void gavl_init_scale_funcs_mmxext(gavl_scale_funcs_t * tab,
                                  gavl_scale_mode_t scale_mode,
                                  int scale_x, int scale_y, int min_scanline_width);
void gavl_init_scale_funcs_mmx(gavl_scale_funcs_t * tab,
                               gavl_scale_mode_t scale_mode,
                               int scale_x, int scale_y, int min_scanline_width);
#endif
