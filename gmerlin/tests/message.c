#include <stdio.h>
#include <gtk/gtk.h>
#include <gui_gtk/message.h>
#include <gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  bg_gtk_init(&argc, &argv);
  bg_gtk_message("Switch coffemachine to backwards\rOr not?", BG_GTK_MESSAGE_ERROR);
  return 0;
  }
