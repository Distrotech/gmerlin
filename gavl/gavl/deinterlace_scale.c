/*****************************************************************

  deinterlace_scale.c

  Copyright (c) 2007 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <gavl/gavl.h>
#include <video.h>
#include <deinterlace.h>

static void deinterlace_scale(gavl_video_deinterlacer_t * d,
                              const gavl_video_frame_t * in,
                              gavl_video_frame_t * out)
  { 
  gavl_video_scaler_scale(d->scaler, in, out);
  }

void gavl_deinterlacer_init_scale(gavl_video_deinterlacer_t * d,
                                  const gavl_video_format_t * src_format)
  {
  gavl_video_options_t * scaler_opt;
  gavl_video_format_t in_format;
  gavl_video_format_t out_format;
  
  if(!d->scaler)
    d->scaler = gavl_video_scaler_create();
  scaler_opt = gavl_video_scaler_get_options(d->scaler);
  gavl_video_options_copy(scaler_opt, &d->opt);
  
  gavl_video_format_copy(&in_format, src_format);
  gavl_video_format_copy(&out_format, src_format);
  
  if(in_format.interlace_mode == GAVL_INTERLACE_NONE)
    in_format.interlace_mode = GAVL_INTERLACE_TOP_FIRST;
  out_format.interlace_mode = GAVL_INTERLACE_NONE;
  
  gavl_video_scaler_init(d->scaler, 
                         &in_format,
                         &out_format);
  
  d->func = deinterlace_scale;
  }
