/*****************************************************************
 
  qt_minf.c
 
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

int bgav_qt_minf_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_minf_t * ret)
  {
  qt_atom_header_t ch; /* Child header */
  memcpy(&(ret->h), h, sizeof(*h));

  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('h', 'd', 'l', 'r'):
        if(!bgav_qt_hdlr_read(&ch, input, &(ret->hdlr)))
          return 0;
        break;
      case BGAV_MK_FOURCC('s', 't', 'b', 'l'):
        if(!bgav_qt_stbl_read(&ch, input, &(ret->stbl), ret))
          return 0;
        break;
      case BGAV_MK_FOURCC('v', 'm', 'h', 'd'):
        ret->has_vmhd = 1;
        bgav_qt_atom_skip(input, &ch);
        break;
      case BGAV_MK_FOURCC('s', 'm', 'h', 'd'):
        ret->has_smhd = 1;
        bgav_qt_atom_skip(input, &ch);
      case BGAV_MK_FOURCC('d', 'i', 'n', 'f'):
      default:
        bgav_qt_atom_skip(input, &ch);
      }
    }
  return 1;
  }

void bgav_qt_minf_free(qt_minf_t * h)
  {
  //  bgav_qt_vmhd_free(&(h->vmhd));
  //  bgav_qt_dinf_free(&(h->dinf));
  bgav_qt_hdlr_free(&(h->hdlr));
  bgav_qt_stbl_free(&(h->stbl));
  }

void bgav_qt_minf_dump(qt_minf_t * h)
  {
  bgav_dprintf( "minf\n");
  bgav_qt_hdlr_dump(&(h->hdlr));
  bgav_qt_stbl_dump(&(h->stbl));
  bgav_dprintf( "end of minf\n");
  }
