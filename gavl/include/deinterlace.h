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

/* Private structures for the video converter */

#include "config.h"

typedef void (*gavl_video_deinterlace_func)(gavl_video_deinterlacer_t*,
                                            gavl_video_frame_t*in,
                                            gavl_video_frame_t*out);

struct gavl_video_deinterlacer_s
  {
  gavl_video_options_t opt;
  gavl_video_format_t format;
  gavl_video_format_t half_height_format;
  gavl_video_deinterlace_func func;

  gavl_video_frame_t * src_field;
  gavl_video_frame_t * dst_field;

  };

/* Find conversion function */

gavl_video_deinterlace_func
gavl_find_deinterlacer_copy_c(const gavl_video_options_t * opt,
                              const gavl_video_format_t * format);


#endif // _GAVL_DEINTERLACE_H_
