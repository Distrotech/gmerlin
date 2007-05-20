/*****************************************************************

  deinterlace.h

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

#ifndef _GAVL_DEINTERLACE_H_
#define _GAVL_DEINTERLACE_H_

/* Private structures for the deinterlacer */

#include "config.h"

typedef void (*gavl_video_deinterlace_func)(gavl_video_deinterlacer_t*,
                                            gavl_video_frame_t*in,
                                            gavl_video_frame_t*out);

typedef void (*gavl_video_deinterlace_blend_func)(uint8_t * t,
                                                  uint8_t * m,
                                                  uint8_t * b,
                                                  uint8_t * dst,
                                                  int num);

typedef struct
  {
  gavl_video_deinterlace_blend_func func_packed_15;
  gavl_video_deinterlace_blend_func func_packed_16;
  gavl_video_deinterlace_blend_func func_rgb_24_4;
  gavl_video_deinterlace_blend_func func_8;
  gavl_video_deinterlace_blend_func func_16;
  gavl_video_deinterlace_blend_func func_float;
  
  } gavl_video_deinterlace_blend_func_table_t;


struct gavl_video_deinterlacer_s
  {
  gavl_video_options_t opt;
  gavl_video_format_t format;
  gavl_video_format_t half_height_format;
  gavl_video_deinterlace_func func;
  
  gavl_video_frame_t * src_field;
  gavl_video_frame_t * dst_field;
  
  gavl_video_scaler_t * scaler;
  
  gavl_video_deinterlace_blend_func blend_func;
  
  int num_planes;
  int line_width;

  int sub_h;
  int sub_v;
  
  
  };

/* Find conversion function */

void gavl_deinterlacer_init_scale(gavl_video_deinterlacer_t * d,
                                  const gavl_video_format_t * src_format);


void gavl_deinterlacer_init_blend(gavl_video_deinterlacer_t * deint,
                                  const gavl_video_format_t * src_format);

gavl_video_deinterlace_func
gavl_find_deinterlacer_copy(const gavl_video_options_t * opt,
                            const gavl_video_format_t * format);

void
gavl_find_deinterlacer_blend_funcs_c(gavl_video_deinterlace_blend_func_table_t * tab,
                                     const gavl_video_options_t * opt,
                                     const gavl_video_format_t * format);

#ifdef HAVE_MMX
void
gavl_find_deinterlacer_blend_funcs_mmx(gavl_video_deinterlace_blend_func_table_t * tab,
                                       const gavl_video_options_t * opt,
                                      const gavl_video_format_t * format);

void
gavl_find_deinterlacer_blend_funcs_mmxext(gavl_video_deinterlace_blend_func_table_t * tab,
                                     const gavl_video_options_t * opt,
                                         const gavl_video_format_t * format);


#endif



#endif // _GAVL_DEINTERLACE_H_
