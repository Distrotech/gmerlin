/*****************************************************************
 
  qt_stbl.c
 
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
#include <stdlib.h>

#include <avdec_private.h>

#include <qt.h>


/*

typedef struct
  {
  qt_atom_header_t h;

  qt_stts_t stts;
  qt_stss_t stss;
  qt_stsd_t stsd;
  qt_stsz_t stsz;
  qt_stsc_t stsc;
  qt_stco_t stco;
  } qt_stbl_t;
*/


int bgav_qt_stbl_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_stbl_t * ret, qt_minf_t * minf)
  {
  qt_atom_header_t ch; /* Child header */
  memcpy(&(ret->h), h, sizeof(*h));

  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('s', 't', 't', 's'):
        if(!bgav_qt_stts_read(&ch, input, &(ret->stts)))
          return 0;
        break;
      case BGAV_MK_FOURCC('s', 't', 's', 's'):
        if(!bgav_qt_stss_read(&ch, input, &(ret->stss)))
          return 0;
        break;
      case BGAV_MK_FOURCC('s', 't', 's', 'd'):
        /* Read stsd */
        if(!bgav_qt_stsd_read(&ch, input, &(ret->stsd)))
          return 0;
        break;
      case BGAV_MK_FOURCC('s', 't', 's', 'z'):
        if(!bgav_qt_stsz_read(&ch, input, &(ret->stsz)))
          return 0;
        break;
      case BGAV_MK_FOURCC('s', 't', 's', 'c'):
        if(!bgav_qt_stsc_read(&ch, input, &(ret->stsc)))
          return 0;
        break;
      case BGAV_MK_FOURCC('s', 't', 'c', 'o'):
        if(!bgav_qt_stco_read(&ch, input, &(ret->stco)))
          return 0;
        break;
      case BGAV_MK_FOURCC('c', 'o', '6', '4'):
        if(!bgav_qt_stco_read_64(&ch, input, &(ret->stco)))
          return 0;
        break;
      default:
        bgav_qt_atom_skip(input, &ch);
        break;
      }
    }
  return 1;
  }

void bgav_qt_stbl_free(qt_stbl_t * c)
  {
  bgav_qt_stts_free(&(c->stts));
  bgav_qt_stss_free(&(c->stss));
  bgav_qt_stsd_free(&(c->stsd));
  bgav_qt_stsz_free(&(c->stsz));
  bgav_qt_stsc_free(&(c->stsc));
  bgav_qt_stco_free(&(c->stco));
  }

void bgav_qt_stbl_dump(qt_stbl_t * c)
  {
  bgav_qt_stts_dump(&(c->stts));
  bgav_qt_stss_dump(&(c->stss));
  bgav_qt_stsd_dump(&(c->stsd));
  bgav_qt_stsz_dump(&(c->stsz));
  bgav_qt_stsc_dump(&(c->stsc));
  bgav_qt_stco_dump(&(c->stco));
  }
