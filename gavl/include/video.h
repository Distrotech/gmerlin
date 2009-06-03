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

/* Private structures for the video converter */

#include "config.h"

typedef struct
  {
  gavl_video_frame_t * input_frame;
  gavl_video_frame_t * output_frame;
  gavl_video_options_t * options;

  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  } gavl_video_convert_context_t;

typedef void (*gavl_video_func_t)(gavl_video_convert_context_t * ctx);


struct gavl_video_converter_s
  {
  gavl_video_convert_context_t csp_context;
  gavl_video_options_t options;
  gavl_video_func_t csp_func;
  };


/* Find conversion functions */

gavl_video_func_t
gavl_find_colorspace_converter(const gavl_video_options_t * opt,
                               gavl_colorspace_t input_colorspace,
                               gavl_colorspace_t output_colorspace,
                               int width, int height);
