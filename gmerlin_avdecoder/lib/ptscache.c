/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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


#include <avdec_private.h>
#include <ptscache.h>

static int get_min_index(bgav_pts_cache_t * c)
  {
  int i;
  int ret = -1;
  int64_t min;
  for(i = 0; i < PTS_CACHE_SIZE; i++)
    {
    if(c->entries[i].used)
      {
      if((ret < 0) || (c->entries[i].pts < min))
        {
        ret = i;
        min = c->entries[i].pts;
        }
      }
    }
  return ret;
  }

void bgav_pts_cache_push(bgav_pts_cache_t * c,
                         int64_t pts,
                         int duration,
                         gavl_timecode_t tc,
                         int * index,
                         bgav_pts_cache_entry_t ** e)
  {
  int i;
  for(i = 0; i < PTS_CACHE_SIZE; i++)
    {
    if(!c->entries[i].used)
      break;
    }
  if(i == PTS_CACHE_SIZE)
    i = get_min_index(c);
  
  c->entries[i].used      = 1;
  c->entries[i].pts       = pts;
  c->entries[i].duration  = duration;
  c->entries[i].tc        = tc;
  
  if(index)
    *index = i;
  if(e)
    *e = &c->entries[i];
  }

void bgav_pts_cache_clear(bgav_pts_cache_t * c)
  {
  int i;
  for(i = 0; i < PTS_CACHE_SIZE; i++)
    c->entries[i].used = 0;
  }

/* Get the smallest timestamp */
int64_t bgav_pts_cache_get_first(bgav_pts_cache_t * c, int * duration,
                                 gavl_timecode_t * tc)
  {
  int i = get_min_index(c);

  if(i < 0)
    return BGAV_TIMESTAMP_UNDEFINED;
  else
    {
    c->entries[i].used = 0;
    *duration = c->entries[i].duration;
    if(tc)
      *tc = c->entries[i].tc;
    return c->entries[i].pts;
    }
  }
