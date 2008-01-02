/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

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
                                int keyframe, int duration)
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
  idx->entries[idx->num_entries].duration   = duration;

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
                         int64_t time, int scale)
  {
  int i;
  int64_t time_scaled;
  
  time_scaled = gavl_time_rescale(scale, s->timescale, time);
  
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
  bgav_dprintf( "superindex %d entries:\n", idx->num_entries);
  for(i = 0; i < idx->num_entries; i++)
    {
#if 0
    bgav_dprintf( "  No: %d ID: %d K: %d Offset: %" PRId64 " T: %" PRId64 " S: %d\n", 
            i,
            idx->entries[i].stream_id,
            idx->entries[i].keyframe,
            idx->entries[i].offset,
            idx->entries[i].time,
            idx->entries[i].size);
#else
    bgav_dprintf( "  ID: %d K: %d Offset: %" PRId64 " T: %" PRId64 " S: %d\n", 
            idx->entries[i].stream_id,
            idx->entries[i].keyframe,
            idx->entries[i].offset,
            idx->entries[i].time,
            idx->entries[i].size);
#endif
    }
  }

  
