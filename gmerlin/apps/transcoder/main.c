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

static void opt_help(void * data, int * argc, char *** argv, int arg);

bg_cmdline_arg_t args[] =
  {
    {
      arg: "-p",
      help_arg: "<file>",
      help_string: "Load a profile from the given file",
      callback:    opt_p,
    },
    {
      arg: "-help",
      help_string: "Display this help and exit",
      callback:    opt_help,
    },
    { /* End of args */ }
  };

static void opt_help(void * data, int * argc, char *** argv, int arg)
  {
  FILE * out = stderr;
  fprintf(out, "Usage: %s [options]\n\n", (*argv)[0]);
  fprintf(out, "Options:\n\n");
  bg_cmdline_print_help(args);
  exit(0);
  }

int main(int argc, char ** argv)
  {
  transcoder_window_t * win;

  bg_gtk_init(&argc, &argv, "transcoder_icon.png");

  /* We must initialize the random number generator if we want the
     Vorbis encoder to work */
  srand(time(NULL));
    
  win = transcoder_window_create();

  bg_cmdline_parse(args, &argc, &argv, win);
  
  transcoder_window_run(win);

  transcoder_window_destroy(win);

  return 0;
  }
