/*****************************************************************
 
  audio.c
 
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <audio.h>
#include <interleave.h>
#include <sampleformat.h>
#include <mix.h>

void gavl_audio_options_copy(gavl_audio_options_t * dst,
                             const gavl_audio_options_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

#if 0
static gavl_audio_convert_context_t *
create_resample_context(gavl_audio_options_t * opt,
                        const gavl_audio_format_t * in_format,
                        const gavl_audio_format_t * out_format)
  {
  gavl_audio_convert_context_t * ret;
  ret = calloc(1, sizeof(*ret));
  fprintf(stderr, "Resampling required but not supported\n");
  return ret;
  }
#endif

void gavl_audio_default_options(gavl_audio_options_t * opt)
  {
  memset(opt, 0, sizeof(*opt));
  
  opt->conversion_flags =
    GAVL_AUDIO_FRONT_TO_REAR_COPY |
    GAVL_AUDIO_STEREO_TO_MONO_MIX;
  opt->accel_flags = GAVL_ACCEL_C;
  }

