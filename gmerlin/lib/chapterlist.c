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

#include <string.h>

#include <parameter.h>
#include <streaminfo.h>
#include <utils.h>

bg_chapter_list_t * bg_chapter_list_create(int num_chapters)
  {
  bg_chapter_list_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->chapters = calloc(num_chapters, sizeof(*(ret->chapters)));
  ret->num_chapters = num_chapters;
  return ret;
  }

void bg_chapter_list_destroy(bg_chapter_list_t * list)
  {
  int i;
  for(i = 0; i < list->num_chapters; i++)
    {
    if(list->chapters[i].name)
      free(list->chapters[i].name);
    }
  free(list->chapters);
  }

void bg_chapter_list_insert(bg_chapter_list_t * list, int index,
                            int64_t time, const char * name)
  {
  int chapters_to_add;
  /* Add a chapter behind the last one */
  if(index >= list->num_chapters)
    {
    chapters_to_add = index + 1 - list->num_chapters;
    list->chapters =
      realloc(list->chapters,
              (chapters_to_add + list->num_chapters)*sizeof(*list->chapters));
    memset(list->chapters + list->num_chapters,
           0, sizeof(*list->chapters) * chapters_to_add);
    list->chapters[index].name = bg_strdup(list->chapters[index].name, name);
    list->chapters[index].time = time;
    list->num_chapters = index + 1;
    }
  else
    {
    list->chapters = realloc(list->chapters,
                             (list->num_chapters + 1)*
                             sizeof(*list->chapters));
    
    memmove(list->chapters + index + 1, list->chapters + index,
            (list->num_chapters - index) * sizeof(*list->chapters));

    list->chapters[index].name = bg_strdup((char*)0, name);
    list->chapters[index].time = time;
    list->num_chapters++;
    }
  
  }

void bg_chapter_list_delete(bg_chapter_list_t * list, int index)
  {
  if((index < 0) || (index >= list->num_chapters))
    return;

  if(list->chapters[index].name)
    free(list->chapters[index].name);

  if(index < list->num_chapters-1)
    {
    memmove(list->chapters + index, list->chapters + index + 1,
            sizeof(*list->chapters) * (list->num_chapters - index));
    }

  list->num_chapters--;
  
  }

void bg_chapter_list_set_default_names(bg_chapter_list_t * list)
  {
  int i;
  for(i = 0; i < list->num_chapters; i++)
    {
    if(!list->chapters[i].name)
      list->chapters[i].name = bg_sprintf("Chapter %d", i+1);
    }
  }

int bg_chapter_list_get_current(bg_chapter_list_t * list,
                                 gavl_time_t time)
  {
  int i;
  for(i = 0; i < list->num_chapters-1; i++)
    {
    if(time < list->chapters[i+1].time)
      return i;
    }
  return list->num_chapters-1;
  }

int bg_chapter_list_changed(bg_chapter_list_t * list,
                            gavl_time_t time, int * current_chapter)
  {
  int ret = 0;
  while(*current_chapter < list->num_chapters-1)
    {
    if(time >= list->chapters[(*current_chapter)+1].time)
      {
      (*current_chapter)++;
      ret = 1;
      }
    else
      break;
    }
  return ret;
  }
