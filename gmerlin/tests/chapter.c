#include <stdio.h>
#include <gtk/gtk.h>

#include <parameter.h>
#include <streaminfo.h>
#include <utils.h>

#include <gui_gtk/chapterdialog.h>
#include <gui_gtk/gtkutils.h>

static bg_chapter_list_t * create_chapter_list()
  {
  int i;
  bg_chapter_list_t * ret;

  ret = bg_chapter_list_load("chapters.xml");
  if(ret)
    return ret;

  ret = bg_chapter_list_create(6);

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
  bg_chapter_list_t * cl;
  gavl_time_t duration = 3600;
  duration *= GAVL_TIME_SCALE;

  bg_gtk_init(&argc, &argv, (char*)0);
  
  cl = create_chapter_list();
  bg_gtk_chapter_dialog_show(&cl, duration);

  if(cl)
    {
    bg_chapter_list_save(cl, "chapters.xml");
    bg_chapter_list_destroy(cl);
    }
  else
    remove("chapters.xml");
  return 0;
  }
