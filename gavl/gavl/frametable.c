/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <gavl/gavl.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

gavl_frame_table_t * gavl_frame_table_create()
  {
  gavl_frame_table_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void gavl_frame_table_destroy(gavl_frame_table_t * tab)
  {
  if(tab->entries)
    free(tab->entries);
  if(tab->timecodes)
    free(tab->timecodes);
  free(tab);
  }
  
void gavl_frame_table_append_entry(gavl_frame_table_t * t, int64_t duration)
  {
  if(t->num_entries && (t->entries[t->num_entries-1].duration == duration))
    {
    t->entries[t->num_entries-1].num_frames++;
    return;
    }
  
  if(t->entries_alloc <= t->num_entries)
    {
    t->entries_alloc = t->num_entries + 128;
    t->entries = realloc(t->entries,
                         t->entries_alloc * sizeof(*t->entries));
    memset(t->entries + t->num_entries, 0,
           (t->entries_alloc-t->num_entries) * sizeof(*t->entries));
    }
  t->entries[t->num_entries].duration = duration;
  t->entries[t->num_entries].num_frames = 1;
  t->num_entries++;
  }

void gavl_frame_table_alloc_timecodes(gavl_frame_table_t * t, int num)
  {
  if(t->timecodes_alloc < num)
    {
    t->timecodes_alloc = num + 128;
    t->timecodes = realloc(t->timecodes,
                           t->timecodes_alloc * sizeof(*t->timecodes));
    }
  memset(t->timecodes + t->num_timecodes, 0,
         (t->timecodes_alloc-t->num_timecodes) *
         sizeof(*t->timecodes));
  }

int64_t gavl_frame_table_frame_to_time(gavl_frame_table_t * t,
                                       int frame, int * duration)
  {
  int i = 0;
  int64_t ret = t->offset;
  int counter = 0;
  
  while(1)
    {
    if(i >= t->num_entries)
      {
      if(duration)
        *duration = 0;
      return GAVL_TIME_UNDEFINED;
      }
    else if(frame - counter < t->entries[i].num_frames)
      {
      if(duration)
        *duration = t->entries[i].duration;
      
      ret += (frame-counter)*t->entries[i].duration;
      break;
      }

    counter += t->entries[i].num_frames;
    ret += t->entries[i].num_frames * t->entries[i].duration;
    i++;
    }
      
  return ret;
  }

int gavl_frame_table_time_to_frame(gavl_frame_table_t * t,
                                   int64_t time,
                                   int64_t * start_time)
  {
  int i = 0;
  int ret = 0;
  int64_t counter = t->offset;
  int off;
  
  if(time < counter)
    return -1;

  while(1)
    {
    if(i >= t->num_entries)
      {
      if(start_time)
        *start_time = GAVL_TIME_UNDEFINED;
      return -1;
      }
    else if(time - counter < t->entries[i].num_frames*t->entries[i].duration)
      {
      off = (time - counter) / t->entries[i].duration;
      ret += off;
      if(start_time)
        *start_time = counter + off * t->entries[i].duration;
      break;
      }
    
    ret += t->entries[i].num_frames;
    counter += t->entries[i].num_frames * t->entries[i].duration;
    i++;
    }
  return ret;
  }

int gavl_frame_table_num_frames(gavl_frame_table_t * t)
  {
  int i, ret = 0;
  for(i = 0; i < t->num_entries; i++)
    {
    ret += t->entries[i].num_frames;
    }
  return ret;
  }

void gavl_frame_table_dump(gavl_frame_table_t * t)
  {
  int i;
  fprintf(stderr, "Entries: %d, total frames: %d, offset: %"PRId64"\n",
          t->num_entries, gavl_frame_table_num_frames(t), t->offset);
  
  for(i = 0; i < t->num_entries; i++)
    {
    fprintf(stderr, "  Frames: %d, duration: %"PRId64"\n",
            t->entries[i].num_frames, t->entries[i].duration);
    }
  }
