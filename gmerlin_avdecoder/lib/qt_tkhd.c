/*****************************************************************
 
  qt_tkhd.c
 
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

  uint32_t version;
  uint32_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  
  uint32_t track_id;
  uint32_t reserved1;
  uint32_t duration;
  uint8_t reserved2[8];
  uint16_t layer;
  uint16_t alternate_group;
  float volume;
  uint16_t reserved3;
  float matrix[9];
  float track_width;
  float track_height;
  } qt_tkhd_t;

*/
  
int bgav_qt_tkhd_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_tkhd_t * ret)
  {
  int i;
  READ_VERSION_AND_FLAGS;
  memcpy(&(ret->h), h, sizeof(*h));
  
  if(!bgav_input_read_32_be(input, &(ret->creation_time)) ||
     !bgav_input_read_32_be(input, &(ret->modification_time)) ||
     !bgav_input_read_32_be(input, &(ret->track_id)) ||
     !bgav_input_read_32_be(input, &(ret->reserved1)) ||
     !bgav_input_read_32_be(input, &(ret->duration)) ||
     (bgav_input_read_data(input, ret->reserved2, 8) < 8) ||
     !bgav_input_read_16_be(input, &(ret->layer)) ||
     !bgav_input_read_16_be(input, &(ret->alternate_group)) ||
     !bgav_qt_read_fixed16(input, &(ret->volume)) ||
     !bgav_input_read_16_be(input, &(ret->reserved3)))
    return 0;

  for(i = 0; i < 9; i++)
    if(!bgav_qt_read_fixed32(input, &(ret->matrix[i])))
      return 0;

  if(!bgav_qt_read_fixed32(input, &(ret->track_width)) ||
     !bgav_qt_read_fixed32(input, &(ret->track_height)))
    return 0;

  return 1;
  }

void bgav_qt_tkhd_free(qt_tkhd_t * c)
  {
  
  }
