/*****************************************************************

  colorspace_scanline_mmx.c

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

#define SCANLINE
#include "../colorspace_macros.h"

#include <config.h>
#include <gavl.h>
#include <video.h>
#include <colorspace.h>

#include <attributes.h>
#include "mmx.h"

#include "_rgb_rgb_mmx.c"
#include "_rgb_yuv_mmx.c"
#include "_yuv_rgb_mmx.c"
#include "_yuv_yuv_mmx.c"
