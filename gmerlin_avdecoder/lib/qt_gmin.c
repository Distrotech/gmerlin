/*****************************************************************
 
  qt_gmin.c
 
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

#if 0
  int version;
  uint32_t flags;
  uint16_t graphics_mode;
  uint16_t opcolor[3];
  uint16_t balance;
  uint16_t reserved;
#endif

int bgav_qt_gmin_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_gmin_t * ret)
  {
  READ_VERSION_AND_FLAGS;
  return bgav_input_read_16_be(input, &(ret->graphics_mode)) &&
    bgav_input_read_16_be(input, &(ret->opcolor[0])) &&
    bgav_input_read_16_be(input, &(ret->opcolor[1])) &&
    bgav_input_read_16_be(input, &(ret->opcolor[2])) &&
    bgav_input_read_16_be(input, &(ret->balance)) &&
    bgav_input_read_16_be(input, &(ret->reserved));
  }

void bgav_qt_gmin_free(qt_gmin_t * g)
  {

  }

void bgav_qt_gmin_dump(int indent, qt_gmin_t * g)
  {
  bgav_diprintf(indent, "gmin\n");

  bgav_diprintf(indent+2, "version:       %d\n", g->version);
  bgav_diprintf(indent+2, "flags:         %08x\n", g->flags);
  bgav_diprintf(indent+2, "graphics_mode: %d\n", g->graphics_mode);
  bgav_diprintf(indent+2, "opcolors:      %d %d %d\n",
               g->opcolor[0], g->opcolor[1], g->opcolor[2]);
  bgav_diprintf(indent+2, "balance:       %d\n", g->balance);
  bgav_diprintf(indent+2, "reserved:      %d\n", g->reserved);
  bgav_diprintf(indent, "end of gmin\n");
  }
