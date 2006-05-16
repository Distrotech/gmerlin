/*****************************************************************

  audiooptions.c

  Copyright (c) 2001 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <stdlib.h> /* calloc, free */
#include <string.h> /* calloc, free */

#ifdef DEBUG
#include <stdio.h>  
#endif

#include "gavl.h"
#include "config.h"
#include "audio.h"
#include <accel.h>

#define SET_INT(p) opt->p = p

void gavl_audio_options_set_quality(gavl_audio_options_t * opt, int quality)
  {
  SET_INT(quality);
  }

void gavl_audio_options_set_accel_flags(gavl_audio_options_t * opt,
                                        int accel_flags)
  {
  SET_INT(accel_flags);
  }

void gavl_audio_options_set_conversion_flags(gavl_audio_options_t * opt,
                                             int conversion_flags)
  {
  SET_INT(conversion_flags);
  }

#undef SET_INT

int gavl_audio_options_get_accel_flags(gavl_audio_options_t * opt)
  {
  return opt->accel_flags;
  }

int gavl_audio_options_get_conversion_flags(gavl_audio_options_t * opt)
  {
  return opt->conversion_flags;
  }


void gavl_audio_options_set_defaults(gavl_audio_options_t * opt)
  {
  memset(opt, 0, sizeof(*opt));
  
  opt->conversion_flags =
    GAVL_AUDIO_FRONT_TO_REAR_COPY |
    GAVL_AUDIO_STEREO_TO_MONO_MIX;
  opt->accel_flags = gavl_accel_supported();
  }

gavl_audio_options_t * gavl_audio_options_create()
  {
  gavl_audio_options_t * ret = calloc(1, sizeof(*ret));
  gavl_audio_options_set_defaults(ret);
  return ret;
  }

void gavl_audio_options_copy(gavl_audio_options_t * dst,
                             const gavl_audio_options_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

void gavl_audio_options_destroy(gavl_audio_options_t * opt)
  {
  free(opt);
  }
