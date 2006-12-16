/*****************************************************************
 
  qt_gmhd.c
 
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

int bgav_qt_gmhd_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_gmhd_t * ret)
  {
  qt_atom_header_t ch; /* Child header */
  
  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('g', 'm', 'i', 'n'):
        if(!bgav_qt_gmin_read(&ch, input, &(ret->gmin)))
          return 0;
        ret->has_gmin = 1;
        break;
      case BGAV_MK_FOURCC('t', 'e', 'x', 't'):
        ret->has_text = 1;
        bgav_qt_atom_skip(input, &ch);
        break;
      default:
        bgav_qt_atom_skip_unknown(input, &ch, h->fourcc);
        break;
      }
    }
  return 1;

  }

void bgav_qt_gmhd_free(qt_gmhd_t * g)
  {
  if(g->has_gmin)
    bgav_qt_gmin_free(&g->gmin);

  }

void bgav_qt_gmhd_dump(int indent, qt_gmhd_t * g)
  {
  bgav_diprintf(indent, "gmhd\n");
  if(g->has_gmin)
    bgav_qt_gmin_dump(indent+2, &g->gmin);
  bgav_diprintf(indent, "end of gmhd\n");
  }
