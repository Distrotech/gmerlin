/*****************************************************************

  main.c

  Copyright (c) 2001 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

/* This file is part of gmerlin_vizualizer */

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkrgb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <xmms/plugin.h>
#include "fft.h"

#include "config.h"
#include "mainwindow.h"
#include "input.h"

#if 0

#include <gmerlin/cmdline.h>
int iconify = 0;

static void cmd_iconify(void * data, int * argc, char *** _argv, int arg)
  {
  iconify = 1;
  }

bg_cmdline_arg_t options[] =
  {
    {
      arg:         "-iconify",
      help_string: "Iconify after startup",
      callback:    cmd_iconify,
    },
    { /* End of options */ }
  };
#endif
typedef struct
  {
  GtkWidget * window;
  } info_window_t;

static void button_callback(GtkWidget * w, gpointer data)
  {
  info_window_t * win;
  win = (info_window_t*)data;
  gtk_widget_hide(win->window);
  }

static gboolean delete_callback(GtkWidget * w, GdkEvent * evt, gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

static void message(const char * text)
  {
  GtkWidget * label;
  GtkWidget * button;
  info_window_t win;
  GtkWidget * table;
  win.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win.window), "Error");
  button = gtk_button_new_with_label("Ok");

  gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(button_callback),
                     (gpointer)(&win));
  gtk_signal_connect(GTK_OBJECT(win.window), "delete-event",
                     GTK_SIGNAL_FUNC(delete_callback),
                     (gpointer)(&win));

  gtk_widget_show(button);
  
  label = gtk_label_new(text);
  
  gtk_widget_show(label);
  
  table = gtk_table_new(2, 1, 0);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(table), button, 0, 1, 1, 2,
                   GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL, 0, 0);

  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(win.window), table);

  gtk_widget_show(win.window);
  gtk_main();
  
  }


int main(int argc, char ** argv)
  {
  main_window_t * main_window;
    
  gtk_init(&argc, &argv);
#if 0
  bg_cmdline_parse(options, &argc, &argv, NULL);
#endif
  gdk_rgb_init();

  if(!input_create())
    {
    message("Could not open recording plugin!\nMake sure the soundcard is not\nused by other applications\nor use gmerlin_plugincfg to set things up");
    return -1;
    }
  
  main_window =  main_window_create();

  gtk_widget_show(main_window->window);
#if 0
  if(iconify)
    gtk_window_iconify(GTK_WINDOW(main_window->window));
#endif
  //  gtk_timeout_add(10, input_iteration, the_input);
  gtk_idle_add(input_iteration, the_input);
  gtk_main();

  main_window_destroy(main_window);
  return 0;
  }
