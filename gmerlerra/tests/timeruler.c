#include <inttypes.h>
#include <gtk/gtk.h>
#include <gui_gtk/timeruler.h>

#include <gmerlin/gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  GtkWidget * win;
  GtkWidget * box;
  bg_nle_time_ruler_t * r;
    
  bg_gtk_init(&argc, &argv, (char *)0);

  r = bg_nle_time_ruler_create();

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  box = gtk_vbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(box),
                     bg_nle_time_ruler_get_widget(r),
                     FALSE, FALSE, 0);
  gtk_widget_show(box);
  
  gtk_container_add(GTK_CONTAINER(win), box);

  gtk_widget_show(win);
  
  gtk_main();
  }
