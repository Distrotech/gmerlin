#include <gtk/gtk.h>
#include <gui_gtk/gtkutils.h>
#include "transcoder_window.h"


int main(int argc, char ** argv)
  {
  

  transcoder_window_t * win;

  bg_gtk_init(&argc, &argv);
  
  win = transcoder_window_create();
  
  transcoder_window_run(win);

  transcoder_window_destroy(win);

  return 0;
  }
