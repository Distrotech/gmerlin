/*****************************************************************
 
  chapterlist.c
 
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

#include <avdec_private.h>

bgav_chapter_list_t * bgav_chapter_list_create(int timescale,
                                               int num_chapters)
  {
  bgav_chapter_list_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->timescale = timescale;
  ret->num_chapters = num_chapters;
  ret->chapters = calloc(ret->num_chapters, sizeof(*ret->chapters));
  return ret;
  }

void bgav_chapter_list_dump(bgav_chapter_list_t * list)
  {
  int i;
  char time_string[GAVL_TIME_STRING_LEN];
  gavl_time_t t;
  
  bgav_dprintf("============ Chapter list =============\n");
  for(i = 0; i < list->num_chapters; i++)
    {
    t = gavl_time_unscale(list->timescale, 
                          list->chapters[i].time);
    gavl_time_prettyprint(t, time_string);
    bgav_dprintf("Chapter %d\n", i+1);
    bgav_dprintf("  Name: %s\n", list->chapters[i].name);
    bgav_dprintf("  Time: %" PRId64 " [%s]\n", list->chapters[i].time, time_string);
    }
  }

void bgav_chapter_list_destroy(bgav_chapter_list_t * list)
  {
  int i;
  for(i = 0; i < list->num_chapters; i++)
    if(list->chapters[i].name) free(list->chapters[i].name);
  if(list->chapters) free(list->chapters);
  }

static bgav_chapter_list_t * get_chapter_list(bgav_t * bgav, int track)
  {
  if(track >= bgav->tt->num_tracks || track < 0)
    return 0;
  return bgav->tt->tracks[track].chapter_list;
  }

int bgav_get_num_chapters(bgav_t * bgav, int track, int * timescale)
  {
  bgav_chapter_list_t * list;
  list = get_chapter_list(bgav, track);
  if(!list)
    {
    *timescale = 0;
    return 0;
    }
  *timescale = list->timescale;
  return list->num_chapters;
  }

const char *
bgav_get_chapter_name(bgav_t * bgav, int track, int chapter)
  {
  bgav_chapter_list_t * list;
  list = get_chapter_list(bgav, track);
  if(!list || (chapter < 0) || (chapter >= list->num_chapters))
    return (char*)0;
  return list->chapters[chapter].name;
  }

int64_t bgav_get_chapter_time(bgav_t * bgav, int track, int chapter)
  {
  bgav_chapter_list_t * list;
  list = get_chapter_list(bgav, track);
  if(!list || (chapter < 0) || (chapter >= list->num_chapters))
    return 0;
  return list->chapters[chapter].time;
  }
