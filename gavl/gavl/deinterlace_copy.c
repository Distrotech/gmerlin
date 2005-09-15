/*****************************************************************

  deinterlace_copy.c

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

#include <stdio.h>

#include <gavl/gavl.h>
#include <video.h>
#include <deinterlace.h>

static void deinterlace_copy(gavl_video_deinterlacer_t * d,
                             gavl_video_frame_t * input_frame,
                             gavl_video_frame_t * output_frame)
  {
  fprintf(stderr, "DEINTERLACE_COPY\n");
  /* Src field */
  gavl_video_frame_get_field(d->format.pixelformat,
                             input_frame,
                             d->src_field,
                             (d->opt.deinterlace_drop_mode ==
                              GAVL_DEINTERLACE_DROP_TOP) ? 1 : 0);

  /* Dst field (even) */
  gavl_video_frame_get_field(d->format.pixelformat,
                             output_frame,
                             d->dst_field, 0);
  gavl_video_frame_copy(&d->half_height_format,
                        d->dst_field, d->src_field);

  /* Dst field (odd) */
  gavl_video_frame_get_field(d->format.pixelformat,
                             output_frame,
                             d->dst_field, 1);
  gavl_video_frame_copy(&d->half_height_format,
                        d->dst_field, d->src_field);
  
  }

gavl_video_deinterlace_func
gavl_find_deinterlacer_copy_c(const gavl_video_options_t * opt,
                              const gavl_video_format_t * format)
  {
  return deinterlace_copy;
  }
