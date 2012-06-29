/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <stdlib.h>
#include <string.h>

bgav_track_table_t * bgav_track_table_create(int num_tracks)
  {
  bgav_track_table_t * ret;
  int i;
  
  ret = calloc(1, sizeof(*ret));
  if(num_tracks)
    {
    ret->tracks = calloc(num_tracks, sizeof(*(ret->tracks)));
    /* Set all durations to undefined */
    for(i = 0; i < num_tracks; i++)
      ret->tracks[i].duration = GAVL_TIME_UNDEFINED;
    ret->num_tracks = num_tracks;
    ret->cur = ret->tracks;
    }
  ret->refcount = 1;
  return ret;
  }

bgav_track_t * bgav_track_table_append_track(bgav_track_table_t * t)
  {
  int track_index;
  track_index = (int)(t->cur - t->tracks);
  
  t->tracks = realloc(t->tracks, sizeof(*t->tracks) * (t->num_tracks+1));
  memset(&t->tracks[t->num_tracks], 0, sizeof(t->tracks[t->num_tracks]));
  t->tracks[t->num_tracks].duration = GAVL_TIME_UNDEFINED;
  t->num_tracks++;

  t->cur = t->tracks + track_index;
  
  return &t->tracks[t->num_tracks-1];
  }

void bgav_track_table_ref(bgav_track_table_t * t)
  {
  t->refcount++;
  }
     
void bgav_track_table_unref(bgav_track_table_t * t)
  {
  int i;
  t->refcount--;
  if(t->refcount)
    return;
  for(i = 0; i < t->num_tracks; i++)
    {
    bgav_track_free(&t->tracks[i]);
    }
  free(t->tracks);
  free(t);
  }

void bgav_track_table_select_track(bgav_track_table_t * t, int track)
  {
  t->cur = &t->tracks[track];
  bgav_track_mute(t->cur);
  }

void bgav_track_table_dump(bgav_track_table_t * t)
  {
  }

void bgav_track_table_merge_metadata(bgav_track_table_t*t,
                                     bgav_metadata_t * m)
  {
  int i;
  for(i = 0; i < t->num_tracks; i++)
    {
    gavl_metadata_merge2(&t->tracks[i].metadata, m);
    }
  }

void bgav_track_table_remove_unsupported(bgav_track_table_t * t)
  {
  int i;
  for(i = 0; i < t->num_tracks; i++)
    {
    bgav_track_remove_unsupported(&t->tracks[i]);
    }
  }
