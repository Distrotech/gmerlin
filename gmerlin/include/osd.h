/*****************************************************************
 
  osd.h
 
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

typedef struct bg_osd_s bg_osd_t;

bg_osd_t * bg_osd_create();
void bg_osd_destroy(bg_osd_t*); 

bg_parameter_info_t * bg_osd_get_parameters(bg_osd_t*);
void bg_osd_set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val);

void bg_osd_set_overlay(bg_osd_t*, gavl_overlay_t *);

void bg_osd_init(bg_osd_t*,
                 const gavl_video_format_t * format,
                 gavl_video_format_t * overlay_format);

int bg_osd_overlay_valid(bg_osd_t*, gavl_time_t time);

void bg_osd_set_volume_changed(bg_osd_t*, float val, gavl_time_t time);
void bg_osd_set_brightness_changed(bg_osd_t*, float val, gavl_time_t time);
void bg_osd_set_contrast_changed(bg_osd_t*, float val, gavl_time_t time);
void bg_osd_set_saturation_changed(bg_osd_t*, float val, gavl_time_t time);

