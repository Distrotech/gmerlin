/*****************************************************************
 
  qt_dinf.c
 
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

#include <string.h>
#include <stdlib.h>

#include <avdec_private.h>
#include <stdio.h>

#include <qt.h>

int bgav_qt_dinf_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_dinf_t * ret)
  {
  qt_atom_header_t ch; /* Child header */
  
  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('d', 'r', 'e', 'f'):
        if(!bgav_qt_dref_read(&ch, input, &(ret->dref)))
          return 0;
        ret->has_dref = 1;
        break;
      default:
        bgav_qt_atom_skip_unknown(input, &ch,h->fourcc);
        break;
      }
    }
  return 1;
  }
     
void bgav_qt_dinf_free(qt_dinf_t * dinf)
  {
  if(dinf->has_dref)
    bgav_qt_dref_free(&dinf->dref);
  }

void bgav_qt_dinf_dump(int indent, qt_dinf_t * dinf)
  {
  bgav_diprintf(indent, "dinf\n");
  if(dinf->has_dref)
    bgav_qt_dref_dump(indent+2, &dinf->dref);
  bgav_diprintf(indent, "end of dinf\n");
  }
