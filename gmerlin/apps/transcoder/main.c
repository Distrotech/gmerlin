/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <config.h>
#include <time.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gui_gtk/gtkutils.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/cmdline.h>

#include "transcoder_window.h"
#include "transcoder_remote.h"

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
      .arg = "-p",
      .help_arg = "<file>",
      .help_string = "Load a profile from the given file",
      .callback =    opt_p,
    },
    { /* End of args */ }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin_transcoder",
    .long_name = TRS("GTK multimedia transcoder"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("GTK multimedia transcoder\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Options"), args },
                                       {  } },
    .env = (bg_cmdline_ext_doc_t[])
    { { TRANSCODER_REMOTE_ENV,
        TRS("Default port for the remote control") },
      { /* End */ }
    },
  };


int main(int argc, char ** argv)
  {
  transcoder_window_t * win;

  bg_gtk_init(&argc, &argv, "transcoder_icon.png", NULL, NULL);

  /* We must initialize the random number generator if we want the
     Vorbis encoder to work */
  srand(time(NULL));
    
  win = transcoder_window_create();

  bg_cmdline_init(&app_data);

  bg_cmdline_parse(args, &argc, &argv, win);
  
  transcoder_window_run(win);

  transcoder_window_destroy(win);

  return 0;
  }
