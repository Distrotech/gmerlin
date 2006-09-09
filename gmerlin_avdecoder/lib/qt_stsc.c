/*****************************************************************
 
  qt_stsc.c
 
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
  uint32_t num_entries;

  struct
    {
    uint32_t first_chunk;
    uint32_t samples_per_chunk;
    uint32_t sample_description_id;
    } *
  entries;
  } qt_stsc_t;
  
*/

int bgav_qt_stsc_read(qt_atom_header_t * h,
                      bgav_input_context_t * input, qt_stsc_t * ret)
  {
  uint32_t i;
  READ_VERSION_AND_FLAGS;
  memcpy(&(ret->h), h, sizeof(*h));
  
  if(!bgav_input_read_32_be(input, &(ret->num_entries)))
    return 0;
  
  ret->entries = calloc(ret->num_entries, sizeof(*(ret->entries)));
  for(i = 0; i < ret->num_entries; i++)
    {
    if(!bgav_input_read_32_be(input, &(ret->entries[i].first_chunk)) ||
       !bgav_input_read_32_be(input, &(ret->entries[i].samples_per_chunk)) ||
       !bgav_input_read_32_be(input, &(ret->entries[i].sample_description_id)))
      return 0;
    }
  return 1;
  }

void bgav_qt_stsc_free(qt_stsc_t * c)
  {
  if(c->entries)
    free(c->entries);
  }

void bgav_qt_stsc_dump(int indent, qt_stsc_t * c)
  {
  int i;
  bgav_diprintf(indent,  "stsc\n");
  bgav_diprintf(indent+2,  "num_entries: %d\n", c->num_entries);

  for(i = 0; i < c->num_entries; i++)
    {
    bgav_diprintf(indent+2, "chunk: %d samples: %d id: %d\n",
            c->entries[i].first_chunk, c->entries[i].samples_per_chunk,
            c->entries[i].sample_description_id);
    }
  bgav_diprintf(indent,  "end of stsc\n");
  
  }

