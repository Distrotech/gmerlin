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
  int index[MAX_INTERPOL_POINTS];
  int factor[MAX_INTERPOL_POINTS];
  } gavl_video_scale_coeff_t;

typedef void (*gavl_video_scale_scanline_func)(gavl_video_scaler_t*,
                                               uint8_t * src_plane,
                                               int src_stride,
                                               uint8_t * dst_line,
                                               int plane,
                                               int scanline);

/* Table for a whole plane */

typedef struct
  {
  int coeffs_h_alloc;
  int coeffs_v_alloc;
  
  int num_coeffs_h;
  int num_coeffs_v;
  
  gavl_video_scale_coeff_t * coeffs_h;
  gavl_video_scale_coeff_t * coeffs_v;
  gavl_video_scale_scanline_func scanline_func;
  } gavl_video_scale_table_t;

struct gavl_video_scaler_s
  {
  int src_x;
  int src_y;
  int dst_x;
  int dst_y;
  
  gavl_video_scale_table_t table[4];
  int num_planes;

  gavl_video_frame_t * src;
  gavl_video_frame_t * dst;

  gavl_colorspace_t colorspace;
  
  };

/* Scale functions */

typedef struct
  {
  gavl_video_scale_scanline_func scale_15_16;
  gavl_video_scale_scanline_func scale_16_16;
  gavl_video_scale_scanline_func scale_24_24;
  gavl_video_scale_scanline_func scale_24_32;
  gavl_video_scale_scanline_func scale_32_32;
  
  gavl_video_scale_scanline_func scale_8;
  gavl_video_scale_scanline_func scale_yuy2;
  gavl_video_scale_scanline_func scale_uyvy;
  
  } gavl_scale_funcs_t;

void gavl_init_scale_funcs_c(gavl_scale_funcs_t * tab,
                             gavl_scale_mode_t scale_mode,
                             int scale_x, int scale_y);
