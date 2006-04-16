#include <gtk/gtk.h>
#include <gui_gtk/scrolltext.h>
#include <gui_gtk/gtkutils.h>

float fg_color[] = { 1.0, 0.5, 0.0 };
float bg_color[] = { 0.0, 0.0, 0.0 };


int main(int argc, char ** argv)
  {
  GtkWidget * win;
  bg_gtk_scrolltext_t * scrolltext;
    
  bg_gtk_init(&argc, &argv, (char*)0);
  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  scrolltext = bg_gtk_scrolltext_create(226, 18);
  bg_gtk_scrolltext_set_font(scrolltext, "Sans Bold 10");
  bg_gtk_scrolltext_set_text(scrolltext,
                             "Beneide nicht das Glueck derer, die in einem Narrenparadies leben, denn nur ein Narr kann das fuer Glueck halten", fg_color, bg_color);

  
  gtk_container_add(GTK_CONTAINER(win),
                    bg_gtk_scrolltext_get_widget(scrolltext));
    
  gtk_widget_show(win);
  gtk_main();
  return 0;
  }
