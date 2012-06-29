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

#include <stdlib.h>

#include <avdec_private.h>

const gavl_chapter_list_t * bgav_get_chapter_list(bgav_t * bgav, int track)
  {
  if(track >= bgav->tt->num_tracks || track < 0)
    return NULL;
  return bgav->tt->tracks[track].chapter_list;
  }

int bgav_get_num_chapters(bgav_t * bgav, int track, int * timescale)
  {
  const gavl_chapter_list_t * list;
  list = bgav_get_chapter_list(bgav, track);
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
  const gavl_chapter_list_t * list;
  list = bgav_get_chapter_list(bgav, track);
  if(!list || (chapter < 0) || (chapter >= list->num_chapters))
    return NULL;
  return list->chapters[chapter].name;
  }

int64_t bgav_get_chapter_time(bgav_t * bgav, int track, int chapter)
  {
  const gavl_chapter_list_t * list;
  list = bgav_get_chapter_list(bgav, track);
  if(!list || (chapter < 0) || (chapter >= list->num_chapters))
    return 0;
  return list->chapters[chapter].time;
  }
