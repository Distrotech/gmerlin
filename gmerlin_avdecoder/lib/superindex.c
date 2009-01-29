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

void bgav_superindex_set_size(bgav_superindex_t * ret, int size)
  {
  if(size > ret->entries_alloc)
    {
    ret->entries_alloc = size;
    ret->entries = realloc(ret->entries, ret->entries_alloc * sizeof(*(ret->entries)));
    memset(ret->entries + ret->num_entries, 0,
           sizeof(*ret->entries) * (ret->entries_alloc - ret->num_entries));
    }
  ret->num_entries = size;
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

  if(keyframe)
    idx->entries[idx->num_entries].flags = PACKET_FLAG_KEY;
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

void bgav_superindex_set_durations(bgav_superindex_t * idx,
                                   bgav_stream_t * s)
  {
  int i;
  int last_pos;
  if(idx->entries[s->first_index_position].duration)
    return;
  
  i = s->first_index_position+1;
  while(idx->entries[i].stream_id != s->stream_id)
    i++;
  
  last_pos = s->first_index_position;
  
  while(i <= s->last_index_position)
    {
    if(idx->entries[i].stream_id == s->stream_id)
      {
      idx->entries[last_pos].duration = idx->entries[i].time - idx->entries[last_pos].time;
      last_pos = i;
      }
    i++;
    }
  idx->entries[s->last_index_position].duration = s->duration -
    idx->entries[s->last_index_position].time;
  }

void bgav_superindex_set_coding_types(bgav_superindex_t * idx,
                                      bgav_stream_t * s)
  {
  int i;
  int64_t max_time = BGAV_TIMESTAMP_UNDEFINED;
  for(i = 0; i < idx->num_entries; i++)
    {
    if(idx->entries[i].stream_id == s->stream_id)
      {
      if(max_time == BGAV_TIMESTAMP_UNDEFINED)
        {
        if(idx->entries[i].flags & PACKET_FLAG_KEY)
          idx->entries[i].flags |= BGAV_CODING_TYPE_I;
        else
          idx->entries[i].flags |= BGAV_CODING_TYPE_P;
        max_time = idx->entries[i].time;
        }
      else if(idx->entries[i].time > max_time)
        {
        if(idx->entries[i].flags & PACKET_FLAG_KEY)
          idx->entries[i].flags |= BGAV_CODING_TYPE_I;
        else
          idx->entries[i].flags |= BGAV_CODING_TYPE_P;
        max_time = idx->entries[i].time;
        }
      else
        idx->entries[i].flags |= BGAV_CODING_TYPE_B;
      }
    }
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
       (idx->entries[i].flags & PACKET_FLAG_KEY) &&
       (idx->entries[i].time <= time_scaled))
      {
      break;
      }
    i--;
    }
  
  if(i < s->first_index_position)
    i = s->first_index_position;
  s->in_time = idx->entries[i].time;

  /* Handle audio preroll */
  if((s->type == BGAV_STREAM_AUDIO) && s->data.audio.preroll)
    {
    while(i >= s->first_index_position)
      {
      if((idx->entries[i].stream_id == s->stream_id) &&
         (idx->entries[i].flags & PACKET_FLAG_KEY) &&
         (s->in_time - idx->entries[i].time >= s->data.audio.preroll))
        {
        break;
        }
      i--;
      }
    }

  if(i < s->first_index_position)
    i = s->first_index_position;

  s->index_position = i;
  s->in_time = idx->entries[i].time;
  }

void bgav_superindex_dump(bgav_superindex_t * idx)
  {
  int i;
  bgav_dprintf( "superindex %d entries:\n", idx->num_entries);
  for(i = 0; i < idx->num_entries; i++)
    {
#if 0
    bgav_dprintf( "  No: %d ID: %d K: %d Offset: %" PRId64 " T: %" PRId64 " D: %d S: %d\n", 
            i,
            idx->entries[i].stream_id,
            idx->entries[i].keyframe,
            idx->entries[i].offset,
            idx->entries[i].time,
            idx->entries[i].duration,
            idx->entries[i].size);
#else
    bgav_dprintf( "  ID: %d K: %d O: %" PRId64 " T: %" PRId64 " D: %d S: %d", 
                  idx->entries[i].stream_id,
                  !!(idx->entries[i].flags & PACKET_FLAG_KEY),
                  idx->entries[i].offset,
                  idx->entries[i].time,
                  idx->entries[i].duration,
                  idx->entries[i].size);
    if(idx->entries[i].flags & 0xff)
      bgav_dprintf(" PT: %c\n", idx->entries[i].flags & 0xff);
    else
      bgav_dprintf("\n");
#endif
    }
  }
  
void bgav_superindex_merge_fileindex(bgav_superindex_t * idx, bgav_stream_t * s)
  {
  int64_t pts;
  int i;
  /* Set all times to undefined */
  for(i = s->first_index_position; i <= s->last_index_position; i++)
    {
    if(idx->entries[i].stream_id == s->stream_id)
      idx->entries[i].time = BGAV_TIMESTAMP_UNDEFINED;
    }

  /* Set pts for all packets, in which frames start */
  
  for(i = 0; i < s->file_index->num_entries; i++)
    {
    if(!i || (s->file_index->entries[i-1].position !=
              s->file_index->entries[i].position))
      {
      idx->entries[s->file_index->entries[i].position].time =
        s->file_index->entries[i].time;
      }
    }
  
  /* Set pts for all packets, in which no frames start */
  pts = s->duration;
  for(i = s->last_index_position; i >= s->first_index_position; i--)
    {
    if(idx->entries[i].stream_id != s->stream_id)
      continue;
    
    if(idx->entries[i].time == BGAV_TIMESTAMP_UNDEFINED)
      idx->entries[i].time = pts;
    else
      pts = idx->entries[i].time;
    }

  /* Recalculate durations */
  idx->entries[0].duration = 0;
  bgav_superindex_set_durations(idx, s);

  //  bgav_superindex_dump(idx);
  }

