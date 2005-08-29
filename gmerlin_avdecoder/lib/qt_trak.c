/*****************************************************************
 
  qt_trak.c
 
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
#include <stdio.h>

#include <qt.h>

/*
typedef struct
  {
  qt_atom_header_t h;
  qt_mdia_t mdia;
  } qt_trak_t;
*/

int bgav_qt_trak_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_trak_t * ret)
  {
  qt_atom_header_t ch; /* Child header */
  memcpy(&(ret->h), h, sizeof(*h));
  //  fprintf(stderr, "reading trak\n");
  while(input->position < h->start_position + h->size)
    {
    if(!bgav_qt_atom_read_header(input, &ch))
      return 0;
    switch(ch.fourcc)
      {
      case BGAV_MK_FOURCC('m', 'd', 'i', 'a'):
        if(!bgav_qt_mdia_read(&ch, input, &(ret->mdia)))
          return 0;
        break;
      case BGAV_MK_FOURCC('t', 'k', 'h', 'd'):
        if(!bgav_qt_tkhd_read(&ch, input, &(ret->tkhd)))
          return 0;
        break;
      default:
        bgav_qt_atom_skip(input, &ch);
        break;
      }
    }

  bgav_qt_stsd_finalize(&(ret->mdia.minf.stbl.stsd), ret);
  
  //  fprintf(stderr, "reading trak done\n");
  return 1;
  }

void bgav_qt_trak_free(qt_trak_t * c)
  {
  bgav_qt_mdia_free(&(c->mdia));
  }

void bgav_qt_trak_dump(qt_trak_t * c)
  {
  fprintf(stderr, "trak\n");
  bgav_qt_tkhd_dump(&c->tkhd);
  bgav_qt_mdia_dump(&c->mdia);
  fprintf(stderr, "end of trak\n");
  }

int64_t bgav_qt_trak_samples(qt_trak_t * trak)
  {
  int i;
  int64_t ret = 0;
  
  qt_stts_t * stts = &(trak->mdia.minf.stbl.stts);
  
  for(i = 0; i < stts->num_entries; i++)
    {
    ret += stts->entries[i].count;
    }
  return ret;
  }

int64_t bgav_qt_trak_chunks(qt_trak_t * c)
  {
  return c->mdia.minf.stbl.stco.num_entries;
  }

int64_t bgav_qt_trak_tics(qt_trak_t * c) /* Duration in timescale tics */
  {
  int i;
  int64_t ret = 0;

  for(i = 0; i < c->mdia.minf.stbl.stts.num_entries; i++)
    {
    ret += c->mdia.minf.stbl.stts.entries[i].count * c->mdia.minf.stbl.stts.entries[i].duration;
    }
  return ret;
  }
