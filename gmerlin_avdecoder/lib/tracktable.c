/*****************************************************************
 
  track.c
 
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

#include <avdec_private.h>
#include <stdlib.h>

bgav_track_table_t * bgav_track_table_create(int num_tracks)
  {
  bgav_track_table_t * ret;
  int i;
  
  ret = calloc(1, sizeof(*ret));
  ret->tracks = calloc(num_tracks, sizeof(*(ret->tracks)));

  /* Set all durations to undefined */

  for(i = 0; i < num_tracks; i++)
    ret->tracks[i].duration = GAVL_TIME_UNDEFINED;
  
  ret->num_tracks = num_tracks;
  ret->current_track = ret->tracks;
  ret->refcount = 1;
  return ret;
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
    bgav_track_free(&(t->tracks[i]));
    }
  free(t->tracks);
  free(t);
  }

void bgav_track_table_select_track(bgav_track_table_t * t, int track)
  {
  t->current_track = &(t->tracks[track]);
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
    bgav_metadata_merge2(&(t->tracks[i].metadata), m);
    }
  }

void bgav_track_table_remove_unsupported(bgav_track_table_t * t)
  {
  int i;
  for(i = 0; i < t->num_tracks; i++)
    {
    bgav_track_remove_unsupported(&(t->tracks[i]));
    }
  }
