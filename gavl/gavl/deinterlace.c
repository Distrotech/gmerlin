/*****************************************************************

  deinterlace.c

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gavl/gavl.h>
#include <video.h>

#include <deinterlace.h>
#include <accel.h>

gavl_video_deinterlacer_t * gavl_video_deinterlacer_create()
  {
  gavl_video_deinterlacer_t * ret;
  ret = calloc(1, sizeof(*ret));
  gavl_video_options_set_defaults(&ret->opt);

  ret->src_field = gavl_video_frame_create(NULL);
  ret->dst_field = gavl_video_frame_create(NULL);
  return ret;
  }

void gavl_video_deinterlacer_destroy(gavl_video_deinterlacer_t * d)
  {
  gavl_video_frame_destroy(d->src_field);
  gavl_video_frame_destroy(d->dst_field);
  free(d);
  }

gavl_video_options_t *
gavl_video_deinterlacer_get_options(gavl_video_deinterlacer_t * d)
  {
  return &(d->opt);
  }

gavl_video_deinterlace_func find_deinterlacer(gavl_video_options_t * opt,
                                              const gavl_video_format_t * format)
  {
  switch(opt->deinterlace_mode)
    {
    case GAVL_DEINTERLACE_NONE:
      return (gavl_video_deinterlace_func)0;
      break;
    case GAVL_DEINTERLACE_COPY:
      return gavl_find_deinterlacer_copy_c(opt, format);
      break;
    case GAVL_DEINTERLACE_SCALE:
      return (gavl_video_deinterlace_func)0;
      break;
    }
  return (gavl_video_deinterlace_func)0;
  }


int gavl_video_deinterlacer_init(gavl_video_deinterlacer_t * d,
                                 const gavl_video_format_t * src_format)
  {
  gavl_video_format_copy(&(d->format), src_format);
  gavl_video_format_copy(&(d->half_height_format), src_format);

  d->half_height_format.image_height /= 2;
  d->half_height_format.frame_height /= 2;

  d->func = find_deinterlacer(&(d->opt), src_format);
  return 1;
  }

void gavl_video_deinterlacer_deinterlace(gavl_video_deinterlacer_t * d,
                                         gavl_video_frame_t * input_frame,
                                         gavl_video_frame_t * output_frame)
  {
  d->func(d, input_frame, output_frame);
  }
