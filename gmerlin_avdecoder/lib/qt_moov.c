/*****************************************************************
 
  qt_moov.c
 
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
  int num_tracks;
  qt_trak_t * tracks;
  } qt_moov_t;
*/

int bgav_qt_moov_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_moov_t * ret)
  {
  qt_atom_header_t ch; /* Child header */
  memcpy(&(ret->h), h, sizeof(*h));

  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('c', 'm', 'o', 'v'):
        /* Compressed header */
        return bgav_qt_cmov_read(&ch, input, ret);
        break;
        /* Reference movie */
      case BGAV_MK_FOURCC('r', 'm', 'r', 'a'):
        if(!bgav_qt_rmra_read(&ch, input, &(ret->rmra)))
          return 0;
        //        fprintf(stderr, "Found rmra atom\n");
        ret->has_rmra = 1;
        break;
      case BGAV_MK_FOURCC('t', 'r', 'a', 'k'):
        ret->num_tracks++;
        ret->tracks =
          realloc(ret->tracks, sizeof(*(ret->tracks))*ret->num_tracks);
        memset(&(ret->tracks[ret->num_tracks-1]), 0,
               sizeof(*ret->tracks));
        if(!bgav_qt_trak_read(&ch, input, &(ret->tracks[ret->num_tracks-1])))
          return 0;
        break;
      case BGAV_MK_FOURCC('m', 'v', 'h', 'd'):
        if(!bgav_qt_mvhd_read(&ch, input, &(ret->mvhd)))
          return 0;
        break;
      case BGAV_MK_FOURCC('u', 'd', 't', 'a'):
        if(!bgav_qt_udta_read(&ch, input, &(ret->udta)))
          return 0;
        break;
      default:
        //        fprintf(stderr, "Skipping atom:\n");
        //        bgav_qt_atom_dump_header(&ch);
        bgav_qt_atom_skip(input, &ch);
        break;
      }
    }
  return 1;
  }

void bgav_qt_moov_free(qt_moov_t * c)
  {
  int i;
  if(c->num_tracks)
    {
    for(i = 0; i < c->num_tracks; i++)
      bgav_qt_trak_free(&(c->tracks[i]));
    free(c->tracks);
    }
  bgav_qt_mvhd_free(&(c->mvhd));
  }
