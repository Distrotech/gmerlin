/*****************************************************************
 
  superindex.c
 
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

#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <stdio.h>

#define NUM_ALLOC 1024

bgav_superindex_t * bgav_superindex_create(int size)
  {
  bgav_superindex_t * ret;
  ret = calloc(1, sizeof(*ret));

  if(size)
    {
    ret->entries_alloc = size;
    ret->entries = calloc(ret->entries_alloc, sizeof(*(ret->entries)));
    }

  return ret;
  }

void bgav_superindex_destroy(bgav_superindex_t * idx)
  {
  if(idx->entries)
    free(idx->entries);
  free(idx);
  }

void bgav_superindex_add_packet(bgav_superindex_t * idx,
                                bgav_stream_t * s,
                                int64_t offset,
                                uint32_t size,
                                int stream_id,
                                int64_t timestamp,
                                int keyframe, int samples)
  {
  /* Realloc */
  
  if(idx->num_entries >= idx->entries_alloc)
    {
    idx->entries_alloc += NUM_ALLOC;
    idx->entries = realloc(idx->entries,
                           idx->entries_alloc * sizeof(*idx->entries));
    memset(idx->entries + idx->num_entries, 0,
           NUM_ALLOC * sizeof(*idx->entries));
    }
  /* Set fields */
  idx->entries[idx->num_entries].offset    = offset;
  idx->entries[idx->num_entries].size      = size;
  idx->entries[idx->num_entries].stream_id = stream_id;
  idx->entries[idx->num_entries].time = timestamp;
  idx->entries[idx->num_entries].keyframe  = keyframe;
  idx->entries[idx->num_entries].samples   = samples;

  /* Update indices */
  if(s)
    {
    if(s->first_index_position > idx->num_entries)
      s->first_index_position = idx->num_entries;
    if(s->last_index_position < idx->num_entries)
      s->last_index_position = idx->num_entries;
    }
  
  idx->num_entries++;
  }

void bgav_superindex_seek(bgav_superindex_t * idx,
                         bgav_stream_t * s,
                         gavl_time_t time)
  {
  int i;
  int64_t time_scaled;
  
  time_scaled = (time * s->timescale)/GAVL_TIME_SCALE;
  
  i = s->last_index_position;

  while(i >= s->first_index_position)
    {
    if((idx->entries[i].stream_id == s->stream_id) &&
       (idx->entries[i].keyframe) &&
       (idx->entries[i].time <= time_scaled))
      {
      s->index_position = i;
      s->time_scaled = idx->entries[i].time;
      break;
      }
    i--;
    }
  }

void bgav_superindex_dump(bgav_superindex_t * idx)
  {
  int i;
  fprintf(stderr, "superindex %d entries:\n", idx->num_entries);
  for(i = 0; i < idx->num_entries; i++)
    {
    fprintf(stderr, "  No: %d ID: %d K: %d Offset: %lld T: %lld S: %d\n", 
            i,
            idx->entries[i].stream_id,
            idx->entries[i].keyframe,
            idx->entries[i].offset,
            idx->entries[i].time,
            idx->entries[i].size);
    }
  }

  
