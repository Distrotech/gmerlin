/*****************************************************************

  colorspace_hq.c 

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

#include <gavl/gavl.h>
#include <video.h>
#include <colorspace.h>

#define DO_ROUND
#define HQ

#include "../c/colorspace_tables.h"
#include "../c/colorspace_macros.h"

#include "../c/_rgb_rgb_c.c"
#include "../c/_rgb_yuv_c.c"
#include "../c/_yuv_rgb_c.c"
#include "../c/_yuv_yuv_c.c"
