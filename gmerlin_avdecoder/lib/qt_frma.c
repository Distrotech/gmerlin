/*****************************************************************
 
  qt_frma.c
 
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



int bgav_qt_frma_read(qt_atom_header_t * h, bgav_input_context_t * ctx,
                      qt_frma_t * ret)
  {
  int result;
  result = bgav_input_read_fourcc(ctx, &ret->fourcc);

  if(result)
    {
    bgav_qt_atom_skip(ctx, h);
    }
  return result;
  }
  
void bgav_qt_frma_dump(qt_frma_t * f)
  {
  bgav_dprintf( "frma:\n");
  bgav_dprintf( "  fourcc: ");
  bgav_dump_fourcc(f->fourcc);
  bgav_dprintf( "\n");
  }
