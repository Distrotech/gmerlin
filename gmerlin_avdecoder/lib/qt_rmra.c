/*****************************************************************
 
  qt_rmra.c
 
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

#include <string.h>
#include <avdec_private.h>
#include <qt.h>

int bgav_qt_rmra_read(qt_atom_header_t * h,
                      bgav_input_context_t * input, qt_rmra_t * ret)
  {
  qt_atom_header_t ch;
  memcpy(&(ret->h), h, sizeof(*h));

  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('r', 'm', 'd', 'a'):
        if(!bgav_qt_rmda_read(&ch, input, &(ret->rmda)))
          return 0;
        break;
      default:
        fprintf(stderr, "Skipping atom inside rmra:\n");
        bgav_qt_atom_dump_header(&ch);
        bgav_qt_atom_skip(input, &ch);
        break;
        
      }
    }
  return 1;
  }

void bgav_qt_rmra_free(qt_rmra_t * r)
  {
  bgav_qt_rmda_free(&(r->rmda));
  }
