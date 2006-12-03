/*****************************************************************

  mpa_common.h

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

/* Common defintions and routines for driving mp2enc */

typedef struct
  {
  int bitrate;      /* -b (kbps) */
  int layer;        /* -l */
  int vcd; /* -V */

  gavl_audio_format_t format;
  bg_subprocess_t * mp2enc;
  
  char * error_msg;
  sigset_t oldset;
  } bg_mpa_common_t;

bg_parameter_info_t * bg_mpa_get_parameters();

/* Must pass a bg_mpa_common_t for parameters */
void bg_mpa_set_parameter(void * data, char * name,
                          bg_parameter_value_t * val);

void bg_mpa_set_format(bg_mpa_common_t * com, const gavl_audio_format_t * format);
void bg_mpa_get_format(bg_mpa_common_t * com, gavl_audio_format_t * format);

int bg_mpa_start(bg_mpa_common_t * com, const char * filename);

int bg_mpa_write_audio_frame(bg_mpa_common_t * com, gavl_audio_frame_t * frame);

int bg_mpa_close(bg_mpa_common_t * com);

const char * bg_mpa_get_extension(bg_mpa_common_t * mpa);
