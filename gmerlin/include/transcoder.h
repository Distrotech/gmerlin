/*****************************************************************
 
  transcoder.h

  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <transcoder_track.h>

typedef struct bg_transcoder_s bg_transcoder_t;

typedef struct
  {
  float percentage_done;
  gavl_time_t remaining_time; /* Remaining time (Transcoding time, NOT track time!!!) */
  } bg_transcoder_info_t;

bg_transcoder_t * bg_transcoder_create();

const bg_transcoder_info_t * bg_transcoder_get_info(bg_transcoder_t * t);

bg_parameter_info_t * bg_transcoder_get_parameters();

void bg_transcoder_set_parameter(void * priv, char * name, bg_parameter_value_t * val);

int bg_transcoder_init(bg_transcoder_t * t,
                       bg_plugin_registry_t * plugin_reg, bg_transcoder_track_t * track);


const char * bg_transcoder_get_error(bg_transcoder_t * t);

/*
 *  Do one iteration (Will be called as an idle function in the GUI main loop)
 *  If return value is FALSE, we are done
 */

int bg_transcoder_iteration(bg_transcoder_t * t);

void bg_transcoder_destroy(bg_transcoder_t * t);
