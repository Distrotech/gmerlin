/*****************************************************************
 
  qt_mdia.c
 
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

/*
  
typedef struct
  {
  qt_atom_header_t h;

  qt_mdhd_t mdhd;
  qt_hdlr_t hdlr;
  qt_minf_t minf;
  } qt_mdia_t;

*/

int bgav_qt_mdia_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_mdia_t * ret)
  {
  qt_atom_header_t ch; /* Child header */
  memcpy(&(ret->h), h, sizeof(*h));

  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;
#if 0
    fprintf(stderr, "Found ");
    bgav_dump_fourcc(ch.fourcc);
    fprintf(stderr, "\n");
#endif   
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('m', 'd', 'h', 'd'):
        //        fprintf(stderr, "Found mdhd\n");
        if(!bgav_qt_mdhd_read(&ch, input, &(ret->mdhd)))
          return 0;
        break;
      case BGAV_MK_FOURCC('h', 'd', 'l', 'r'):
        //        fprintf(stderr, "Found hdlr\n");
        if(!bgav_qt_hdlr_read(&ch, input, &(ret->hdlr)))
          return 0;
        break;
      case BGAV_MK_FOURCC('m', 'i', 'n', 'f'):
        // fprintf(stderr, "Found minf\n");
        if(!bgav_qt_minf_read(&ch, input, &(ret->minf)))
          return 0;
        break;
      default:
        bgav_qt_atom_skip(input, &ch);
        break;

      }
    }
  return 1;
  }

void bgav_qt_mdia_free(qt_mdia_t * c)
  {
  bgav_qt_mdhd_free(&(c->mdhd));
  bgav_qt_hdlr_free(&(c->hdlr));
  bgav_qt_minf_free(&(c->minf));
  }

void bgav_qt_mdia_dump(qt_mdia_t * c)
  {
  fprintf(stderr, "mdia\n");
  bgav_qt_mdhd_dump(&(c->mdhd));
  bgav_qt_hdlr_dump(&(c->hdlr));
  bgav_qt_minf_dump(&(c->minf));
  fprintf(stderr, "end of mdia\n");
  }
