/*****************************************************************
 
  qt_stss.c
 
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

   uint32_t num_entries;
   uint32_t * entries;
   } qt_stss_t;
*/

int bgav_qt_stss_read(qt_atom_header_t * h,
                      bgav_input_context_t * input, qt_stss_t * ret)
  {
  uint32_t i;
  READ_VERSION_AND_FLAGS;
  memcpy(&(ret->h), h, sizeof(*h));
  
  if(!bgav_input_read_32_be(input, &(ret->num_entries)))
    return 0;

  ret->entries = calloc(ret->num_entries, sizeof(*(ret->entries)));
  
  for(i = 0; i < ret->num_entries; i++)
    {
    if(!bgav_input_read_32_be(input, &(ret->entries[i])))
      return 0;
    }
  return 1;
  }

void bgav_qt_stss_free(qt_stss_t * c)
  {
  if(c->entries)
    free(c->entries);
  }
