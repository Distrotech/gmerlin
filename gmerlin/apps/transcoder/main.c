#include <config.h>
#include <time.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gui_gtk/gtkutils.h>

#include <pluginregistry.h>
#include <cmdline.h>

#include "transcoder_window.h"

static void opt_p(void * data, int * argc, char *** argv, int arg)
  {
  FILE * out = stderr;
  transcoder_window_t * win;
  win = (transcoder_window_t*)data;
  
  if(arg >= *argc)
    {
    fprintf(out, "Option -p requires an argument\n");
    exit(-1);
    }

  transcoder_window_load_profile(win, (*argv)[arg]);
  bg_cmdline_remove_arg(argc, argv, arg);
  }


bg_cmdline_arg_t args[] =
  {
    {
      arg: "-p",
      help_arg: "<file>",
      help_string: "Load a profile from the given file",
      callback:    opt_p,
    },
    { /* End of args */ }
  };

bg_cmdline_app_data_t app_data =
  {
    package:  PACKAGE,
    version:  VERSION,
    name:     "gmerlin_transcoder",
    synopsis: TRS("[options]\n"),
    help_before: TRS("GTK multimedia transcoder\n"),
    args: (bg_cmdline_arg_array_t[]) { { TRS("Options"), args },
                                       {  } },
  };


int main(int argc, char ** argv)
  {
  transcoder_window_t * win;

  bg_gtk_init(&argc, &argv, "transcoder_icon.png");

  /* We must initialize the random number generator if we want the
     Vorbis encoder to work */
  srand(time(NULL));
    
  win = transcoder_window_create();

  bg_cmdline_parse(args, &argc, &argv, win, &app_data);
  
  transcoder_window_run(win);

  transcoder_window_destroy(win);

  return 0;
  }
