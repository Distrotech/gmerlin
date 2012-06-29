/*****************************************************************
 * gavl - a general purpose audio/video processing library
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

#include <gavl/gavl.h>
#include <gavl/chapterlist.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

gavl_chapter_list_t * gavl_chapter_list_create(int num_chapters)
  {
  gavl_chapter_list_t * ret;
  ret = calloc(1, sizeof(*ret));
  if(num_chapters)
    {
    ret->chapters = calloc(num_chapters, sizeof(*(ret->chapters)));
    ret->num_chapters = num_chapters;
    }
  return ret;
  }

gavl_chapter_list_t * gavl_chapter_list_copy(const gavl_chapter_list_t * list)
  {
  int i;
  gavl_chapter_list_t * ret;

  if(!list || !list->num_chapters)
    return NULL;
  
  ret = gavl_chapter_list_create(list->num_chapters);
  for(i = 0; i < ret->num_chapters; i++)
    {
    ret->chapters[i].time = list->chapters[i].time;

    if(list->chapters[i].name)
      {
      ret->chapters[i].name = malloc(strlen(list->chapters[i].name)+1);
      strcpy(ret->chapters[i].name, list->chapters[i].name);
      }
    }
  ret->timescale = list->timescale;
  return ret;
  }


void gavl_chapter_list_destroy(gavl_chapter_list_t * list)
  {
  int i;
  for(i = 0; i < list->num_chapters; i++)
    {
    if(list->chapters[i].name)
      free(list->chapters[i].name);
    }
  free(list->chapters);
  free(list);
  }

void gavl_chapter_list_insert(gavl_chapter_list_t * list, int index,
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
    list->num_chapters = index + 1;
    }
  else
    {
    list->chapters = realloc(list->chapters,
                             (list->num_chapters + 1)*
                             sizeof(*list->chapters));
    memmove(list->chapters + index + 1, list->chapters + index,
            (list->num_chapters - index) * sizeof(*list->chapters));
    list->num_chapters++;
    }

  if(name)
    {
    list->chapters[index].name = malloc(strlen(name)+1);
    strcpy(list->chapters[index].name, name);
    }
  list->chapters[index].time = time;
  
  }

void gavl_chapter_list_delete(gavl_chapter_list_t * list, int index)
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
  if(!index)
    list->chapters[index].time = 0;
  
  list->num_chapters--;
  
  }


int gavl_chapter_list_get_current(gavl_chapter_list_t * list,
                                gavl_time_t time)
  {
  int i;
  int64_t time_scaled = gavl_time_scale(list->timescale, time);
  for(i = 0; i < list->num_chapters-1; i++)
    {
    if(time_scaled < list->chapters[i+1].time)
      return i;
    }
  return list->num_chapters-1;
  }

int gavl_chapter_list_changed(gavl_chapter_list_t * list,
                              gavl_time_t time, int * current_chapter)
  {
  int ret = 0;
  int64_t time_scaled = gavl_time_scale(list->timescale, time);
  while(*current_chapter < list->num_chapters-1)
    {
    if(time_scaled >= list->chapters[(*current_chapter)+1].time)
      {
      (*current_chapter)++;
      ret = 1;
      }
    else
      break;
    }
  return ret;
  }

void gavl_chapter_list_dump(const gavl_chapter_list_t * list)
  {
  int i;
  char time_string[GAVL_TIME_STRING_LEN];
  gavl_time_t t;
  
  fprintf(stderr, "Chapter list\n");
  fprintf(stderr, "  Timescale: %d\n", list->timescale);
  for(i = 0; i < list->num_chapters; i++)
    {
    t = gavl_time_unscale(list->timescale, 
                          list->chapters[i].time);
    gavl_time_prettyprint(t, time_string);
    fprintf(stderr, "  Chapter %d\n", i+1);
    fprintf(stderr, "    Name: %s\n", list->chapters[i].name);
    fprintf(stderr, "    Time: %" PRId64 " [%s]\n", list->chapters[i].time, time_string);
    }

  }
