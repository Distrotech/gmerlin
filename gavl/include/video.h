/*****************************************************************

  video.h

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

#ifndef _GAVL_VIDEO_H_
#define _GAVL_VIDEO_H_

/* Private structures for the video converter */

#include "config.h"

  
struct gavl_video_options_s
  {
  /*
   *  Quality setting from 1 to 5 (0 means undefined).
   *  3 means Standard C routines or accellerated version with
   *  equal quality. Lower numbers mean accellerated versions with lower
   *  quality.
   */
  int quality;         

  /* Explicit accel_flags are mainly for colorspace_test.c */
  int accel_flags;     /* CPU Acceleration flags */

  int conversion_flags;
  
  gavl_alpha_mode_t alpha_mode;

  gavl_scale_mode_t scale_mode;
  int scale_order;

  gavl_deinterlace_mode_t deinterlace_mode;
  gavl_deinterlace_drop_mode_t deinterlace_drop_mode;
    
  /* Background color (Floating point and 16 bit int)background_float[3]; */
  float background_float[3];

  uint16_t background_16[3];
  
  /* Source and destination rectangles */

  gavl_rectangle_f_t src_rect;
  gavl_rectangle_i_t dst_rect;
  int have_rectangles;
  
  int keep_aspect;
  };

void gavl_video_options_copy(gavl_video_options_t * dst,
                             const gavl_video_options_t * src);

typedef struct gavl_video_convert_context_s gavl_video_convert_context_t;

typedef void (*gavl_video_func_t)(gavl_video_convert_context_t * ctx);

struct gavl_video_convert_context_s
  {
  gavl_video_frame_t * input_frame;
  gavl_video_frame_t * output_frame;
  gavl_video_options_t * options;

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;

  gavl_video_scaler_t * scaler;

  struct gavl_video_convert_context_s * next;
  gavl_video_func_t func;
  };

struct gavl_video_converter_s
  {
  gavl_video_options_t options;

  gavl_video_convert_context_t * first_context;
  gavl_video_convert_context_t * last_context;
  int num_contexts;
  };


/* Find conversion functions */

gavl_video_func_t
gavl_find_pixelformat_converter(const gavl_video_options_t * opt,
                               gavl_pixelformat_t input_pixelformat,
                               gavl_pixelformat_t output_pixelformat,
                               int width, int height);

/* Check if a pixelformat can be converted by simple scaling */

int gavl_pixelformat_can_scale(gavl_pixelformat_t in_csp, gavl_pixelformat_t out_csp);

/*
 *  Return a pixelformat (or GAVL_PIXELFORMAT_NONE) as an intermediate pixelformat
 *  for which the conversion quality can be improved. E.g. instead of
 *  RGB -> YUV420P, we can do RGB -> YUV444P -> YUV420P with proper chroma scaling
 */

gavl_pixelformat_t gavl_pixelformat_get_intermediate(gavl_pixelformat_t in_csp,
                                                   gavl_pixelformat_t out_csp);

#endif // _GAVL_VIDEO_H_
