/*****************************************************************
 
  textrenderer.h
 
  Copyright (c) 2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/


typedef struct bg_text_renderer_s bg_text_renderer_t;

bg_text_renderer_t * bg_text_renderer_create();
void bg_text_renderer_destroy(bg_text_renderer_t *);

bg_parameter_info_t * bg_text_renderer_get_parameters(void);

void bg_text_renderer_set_parameter(void * data, const char * name,
                                    const bg_parameter_value_t * val);

/* Frame format can be NULL */

void bg_text_renderer_init(bg_text_renderer_t*,
                           const gavl_video_format_t * frame_format,
                           gavl_video_format_t * overlay_format);

void bg_text_renderer_get_frame_format(bg_text_renderer_t * r,
                                       gavl_video_format_t * frame_format);


void bg_text_renderer_render(bg_text_renderer_t*, const char * string, gavl_overlay_t * ovl);
