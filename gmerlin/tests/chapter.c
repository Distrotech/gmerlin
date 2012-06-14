/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <stdio.h>
#include <gtk/gtk.h>

#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/utils.h>

#include <gui_gtk/chapterdialog.h>
#include <gui_gtk/gtkutils.h>

static gavl_chapter_list_t * create_chapter_list()
  {
  int i;
  gavl_chapter_list_t * ret;

  ret = bg_chapter_list_load("chapters.xml");
  if(ret)
    return ret;

  ret = gavl_chapter_list_create(6);

  for(i = 0; i < ret->num_chapters; i++)
    {
    ret->chapters[i].name = bg_sprintf("Chapter %c", 'A' + i);
    ret->chapters[i].time = i * 600;
    ret->chapters[i].time *= GAVL_TIME_SCALE;
    }
  return ret;
  }

int main(int argc, char ** argv)
  {
  gavl_chapter_list_t * cl;
  gavl_time_t duration = 3600;
  duration *= GAVL_TIME_SCALE;

  bg_gtk_init(&argc, &argv, NULL, NULL, NULL);;
  
  cl = create_chapter_list();
  bg_gtk_chapter_dialog_show(&cl, duration, NULL);

  if(cl)
    {
    bg_chapter_list_save(cl, "chapters.xml");
    gavl_chapter_list_destroy(cl);
    }
  else
    remove("chapters.xml");
  return 0;
  }
