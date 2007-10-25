#include <gtk/gtk.h>
#include <gui_gtk/audio.h>
#include <gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  bg_gtk_vumeter_t * vumeter;
  GtkWidget * window;
  
  bg_gtk_init(&argc, &argv, (char*)0);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  vumeter = bg_gtk_vumeter_create(2, 0);
  
  gtk_container_add(GTK_CONTAINER(window),
                    bg_gtk_vumeter_get_widget(vumeter));
  gtk_widget_show(window);
  gtk_main();

  return 0;
  }
