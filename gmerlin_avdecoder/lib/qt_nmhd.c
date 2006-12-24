/*****************************************************************
 
  qt_nmhd.c
 
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

int bgav_qt_nmhd_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_nmhd_t * ret)
  {
  READ_VERSION_AND_FLAGS;
  return 1;
  }

void bgav_qt_nmhd_free(qt_nmhd_t * g)
  {

  }

void bgav_qt_nmhd_dump(int indent, qt_nmhd_t * g)
  {
  bgav_diprintf(indent, "nmhd\n");

  bgav_diprintf(indent+2, "version:       %d\n", g->version);
  bgav_diprintf(indent+2, "flags:         %08x\n", g->flags);
  bgav_diprintf(indent, "end of nmhd\n");
  }
