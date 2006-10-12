/*****************************************************************
 
  qt_atom.c
 
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

#include <stdio.h>

#define LOG_DOMAIN "qt_atom"

/*
  
  uint64_t size;
  uint64_t start_position; 
  uint32_t fourcc;
  uint8_t version;
  uint32_t flags;
*/



int bgav_qt_atom_read_header(bgav_input_context_t * input,
                             qt_atom_header_t * h)
  {
  uint32_t tmp_32;
  
  h->start_position = input->position;
  
  if(!bgav_input_read_32_be(input, &tmp_32))
    return 0;

  h->size = tmp_32;

  if(!bgav_input_read_fourcc(input, &(h->fourcc)))
    return 0;
  
  if(tmp_32 == 1) /* 64 bit atom */
    {
    if(!bgav_input_read_64_be(input, &(h->size)))
      return 0;
    }
  return 1;
  }

void bgav_qt_atom_skip(bgav_input_context_t * input,
                       qt_atom_header_t * h)
  {
  int64_t bytes_to_skip = h->size - (input->position - h->start_position);
  if(bytes_to_skip > 0)
    bgav_input_skip(input, bytes_to_skip);
  }

void bgav_qt_atom_skip_unknown(bgav_input_context_t * input,
                               qt_atom_header_t * h, uint32_t parent)
  {
  if(!parent)
    bgav_log(input->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
             "Unknown atom [%c%c%c%c] at toplevel",
             (h->fourcc & 0xFF000000) >> 24,
             (h->fourcc & 0x00FF0000) >> 16,
             (h->fourcc & 0x0000FF00) >> 8,
             (h->fourcc & 0x000000FF));
  else
    bgav_log(input->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
             "Unknown atom inside [%c%c%c%c] (fourcc: [%c%c%c%c])",
             (parent & 0xFF000000) >> 24,
             (parent & 0x00FF0000) >> 16,
             (parent & 0x0000FF00) >> 8,
             (parent & 0x000000FF),
             (h->fourcc & 0xFF000000) >> 24,
             (h->fourcc & 0x00FF0000) >> 16,
             (h->fourcc & 0x0000FF00) >> 8,
             (h->fourcc & 0x000000FF));
  bgav_qt_atom_skip(input, h);
  }



void bgav_qt_atom_dump_header(int indent, qt_atom_header_t * h)
  {
  bgav_diprintf(indent, "Size:           %lld\n", h->size);
  bgav_diprintf(indent, "Start Position: %lld\n", h->start_position);
  bgav_diprintf(indent, "Fourcc:         ");
  bgav_dump_fourcc(h->fourcc);
  bgav_dprintf("\n");
  }
