/*****************************************************************

  mpv_common.h

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

/* Common defintions and routines for driving mpeg2enc */

#define FORMAT_MPEG1 0
#define FORMAT_VCD   1
#define FORMAT_MPEG2 3
#define FORMAT_SVCD  4
#define FORMAT_DVD   8

typedef struct
  {
  int format;       /* -f */
  int bitrate;      /* -b */
  int video_buffer; /* -V */
  int bframes;      /* -R */
  
  } bg_mpv_common_t;

bg_parameter_info_t * bg_mpv_get_parameters();

/* Must pass a bg_mpv_common_t for parameters */
void bg_mpv_set_parameter(void * data, char * name, bg_parameter_value_t * val);

char * bg_mpv_make_commandline(bg_mpv_common_t * com, const char * filename);

