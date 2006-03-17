/*****************************************************************
 
  qt_utils.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <avdec_private.h>
#include <qt.h>

int bgav_qt_read_fixed32(bgav_input_context_t * ctx,
                         float * ret)
  {
  uint32_t a, b, c, d;
  uint8_t data[4];
  
  if(bgav_input_read_data(ctx, data, 4) < 4)
    return 0;

  a = data[0];
  b = data[1];
  c = data[2];
  d = data[3];
  
  a = (a << 8) + b;
  b = (c << 8) + d;
  
  if(b)
    *ret = (float)a + (float)b / 65536;
  else
    *ret = a;
  return 1;
  }

int bgav_qt_read_fixed16(bgav_input_context_t * ctx,
                         float * ret)
  {
  unsigned char data[2];
  if(bgav_input_read_data(ctx, data, 2) < 2)
    return 0;
  if(data[1])
    *ret = (float)data[0] + (float)data[1] / 256;
  else
    *ret = (float)data[0];
  return 1;
  }
