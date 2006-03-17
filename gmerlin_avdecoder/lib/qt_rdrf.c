/*****************************************************************
 
  qt_rdrf.c
 
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

#if 0
  uint32_t flags;
  uint32_t fourcc;
  uint32_t data_ref_size;
  uint8_t * data_ref;
#endif

static void bgav_qt_rdrf_dump(qt_rdrf_t * r)
  {
  fprintf(stderr, "rdrf:\n");
  fprintf(stderr, "fourcc: ");
  bgav_dump_fourcc(r->fourcc);
  fprintf(stderr, "\ndata_ref_size: %d\n", r->data_ref_size);

  if(r->fourcc == BGAV_MK_FOURCC('u', 'r', 'l', ' '))
    {
    fprintf(stderr, "URL: ");
    fwrite(r->data_ref, 1, r->data_ref_size, stderr);
    fprintf(stderr, "\n");
    }
  else
    {
    fprintf(stderr, "Unknown data, hexdump follows");
    bgav_hexdump(r->data_ref, r->data_ref_size, 16);
    }
  }

int bgav_qt_rdrf_read(qt_atom_header_t * h,
                      bgav_input_context_t * input, qt_rdrf_t * ret)
  {
  if(!bgav_input_read_32_le(input, &(ret->flags)) ||
     !bgav_input_read_fourcc(input, &(ret->fourcc)) ||
     !bgav_input_read_fourcc(input, &(ret->data_ref_size)))
    return 0;

  ret->data_ref = malloc(ret->data_ref_size);
  if(bgav_input_read_data(input, ret->data_ref, ret->data_ref_size) <
     ret->data_ref_size)
    return 0;
  bgav_qt_rdrf_dump(ret);
  return 1;
  }

void bgav_qt_rdrf_free(qt_rdrf_t * r)
  {
  if(r->data_ref)
    free(r->data_ref);
  }

