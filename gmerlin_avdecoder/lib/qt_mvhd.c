/*****************************************************************
 
  qt_mvhd.c
 
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
#include <avdec_private.h>
#include <qt.h>

/*
typedef struct
  {
  qt_atom_header_t h;
  int version;
  uint32_t flags;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t time_scale;
  uint32_t duration;
  float preferred_rate;
  float preferred_volume;
  char reserved[10];
  float matrix[9];
  uint32_t preview_time;
  uint32_t preview_duration;
  uint32_t poster_time;
  uint32_t selection_time;
  uint32_t selection_duration;
  uint32_t current_time;
  uint32_t next_track_id;
  
  } qt_mvhd_t;

*/

int bgav_qt_mvhd_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_mvhd_t * ret)
  {
  int i;
  READ_VERSION_AND_FLAGS;
  memcpy(&(ret->h), h, sizeof(*h));

  if(!bgav_input_read_32_be(input, &(ret->creation_time)) ||
     !bgav_input_read_32_be(input, &(ret->modification_time)) ||
     !bgav_input_read_32_be(input, &(ret->time_scale)) ||
     !bgav_input_read_32_be(input, &(ret->duration)) ||
     !bgav_qt_read_fixed32(input, &(ret->preferred_rate)) ||
     !bgav_qt_read_fixed16(input, &(ret->preferred_volume)) ||
     !(bgav_input_read_data(input, ret->reserved, 10) == 10))
    return 0;

  for(i = 0; i < 9; i++)
    if(!bgav_qt_read_fixed32(input, &(ret->matrix[i])))
      return 0;

  return(bgav_input_read_32_be(input, &(ret->preview_time)) &&
         bgav_input_read_32_be(input, &(ret->preview_duration)) &&
         bgav_input_read_32_be(input, &(ret->poster_time)) &&
         bgav_input_read_32_be(input, &(ret->selection_time)) &&
         bgav_input_read_32_be(input, &(ret->selection_duration)) &&
         bgav_input_read_32_be(input, &(ret->current_time)) &&
         bgav_input_read_32_be(input, &(ret->next_track_id)));
  }

void bgav_qt_mvhd_free(qt_mvhd_t * c)
  {
  
  }
