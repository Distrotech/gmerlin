/*****************************************************************
 
  qt_mdhd.c
 
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

  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t time_scale;
  uint32_t duration;
  uint16_t language;
  uint16_t quality;
  } qt_mdhd_t;
*/

void bgav_qt_mdhd_dump(qt_mdhd_t * m)
  {
  bgav_dprintf( "mdhd:\n");
  bgav_dprintf( "  creation_time:     %d\n", m->creation_time);
  bgav_dprintf( "  modification_time: %d\n", m->modification_time);
  bgav_dprintf( "  time_scale:        %d\n", m->time_scale);
  bgav_dprintf( "  duration:          %d\n", m->duration);
  bgav_dprintf( "  language:          %d\n", m->language);
  bgav_dprintf( "  quality:           %d\n", m->quality);
  }
  
int bgav_qt_mdhd_read(qt_atom_header_t * h, bgav_input_context_t * input,
                      qt_mdhd_t * ret)
  {
  int result;
  READ_VERSION_AND_FLAGS;
  memcpy(&(ret->h), h, sizeof(*h));

  result = bgav_input_read_32_be(input, &(ret->creation_time)) &&
    bgav_input_read_32_be(input, &(ret->modification_time)) &&
    bgav_input_read_32_be(input, &(ret->time_scale)) &&
    bgav_input_read_32_be(input, &(ret->duration)) &&
    bgav_input_read_16_be(input, &(ret->language)) &&
    bgav_input_read_16_be(input, &(ret->quality));

  //  bgav_qt_mdhd_dump(ret);
  return result;
  
  }

void bgav_qt_mdhd_free(qt_mdhd_t * c)
  {
  
  }
