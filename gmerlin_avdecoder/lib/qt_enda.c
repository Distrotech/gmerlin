/*****************************************************************
 
  qt_enda.c
 
  Copyright (c) 2005-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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
#include <stdio.h>

#include <qt.h>


int bgav_qt_enda_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_enda_t * ret)
  {
  int result;
  result = bgav_input_read_16_be(ctx, &ret->littleEndian);
  if(result)
    bgav_qt_atom_skip(ctx, h);
  return result;
  }
  
void bgav_qt_enda_dump(qt_enda_t * f)
  {
  bgav_dprintf( "enda:\n");
  bgav_dprintf( "  littleEndian: %d\n", f->littleEndian);
  }
