/*****************************************************************
 
  alsa_common.h
 
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

#include <gavl/gavl.h>
#include <alsa/asoundlib.h>

/* For reading, the sample format, num_channels and samplerate must be set */

snd_pcm_t * bg_alsa_open_read(const char * card, gavl_audio_format_t * format);

/* For writing, the complete format must be set, values will be changed if not compatible */

snd_pcm_t * bg_alsa_open_write(const char * card, gavl_audio_format_t * format);

/* Builds a parameter array for all available cards */

void bg_alsa_create_card_parameters(bg_parameter_info_t * ret);

