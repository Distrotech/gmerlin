/*****************************************************************

  mainwindow.c

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

/* This file is part of gmerlin_visualizer */

#include <gtk/gtk.h>
#include <xmms/plugin.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "fft.h"

#include "config.h"
#include "mainwindow.h"
#include "input.h"

#include <sys/types.h>

#include <dirent.h>
#include <string.h>
#include <stdlib.h>

/* Scan the xmms plugin directory for visualization plugins */
#if 0
static GList * get_installed_plugins()
  {
  VisPlugin * plugin;
  GList * ret = (GList*)0;

  struct dirent * entry;

  char * filename;
  char * pos;
  xesd_plugin_info * plugin_info;
  DIR * dir = opendir(XMMS_VISUALIZATION_PLUGIN_DIR);
  filename = malloc(PATH_MAX + 1);
  
  while(1)
    {
    if(!(entry = readdir(dir)))
      break;

    /* Filter out filenames, which are no libraries */

    pos = strchr(entry->d_name, '.');

    if(!pos)
      continue;

    if(strcmp(pos, ".so"))
      continue;

    if(strncmp(entry->d_name, "lib", 3))
      continue;
    
    sprintf(filename, "%s/%s", XMMS_VISUALIZATION_PLUGIN_DIR, entry->d_name);
    
    plugin = load_vis_plugin(filename);
    if(!plugin)
      continue;
    else
      {
      plugin_info = calloc(1, sizeof(xesd_plugin_info));
      plugin_info->filename = g_strdup(filename);
      plugin_info->name = g_strdup(plugin->description);
      plugin_info->enabled = 0;
      plugin_info->plugin = plugin;
      ret = g_list_append(ret, (gpointer)plugin_info);
      unload_plugin(plugin_info);
      }
    }
  free(filename);
  closedir(dir);
  return ret;
  }
#endif

static void button_callback(GtkWidget * w, gpointer * data)
  {
  main_window_t * win;
  win = (main_window_t*)data;

  if(w == win->quit_button)
    gtk_main_quit();
  else if(w == win->configure_button)
    {
    if(win->current_plugin_info)
      {
      if(!win->current_plugin_handle)
        win->current_plugin_handle = vis_plugin_load(win->current_plugin_info);
      if(win->current_plugin_handle->configure)
        win->current_plugin_handle->configure(win->current_plugin_handle);
      }
    }
  else if(w == win->about_button)
    {
    if(!win->current_plugin_handle)
      win->current_plugin_handle = vis_plugin_load(win->current_plugin_info);
    if(win->current_plugin_handle->about)
      win->current_plugin_handle->about(win->current_plugin_handle);
    }
  else if((w == win->enable_button) && !(win->no_enable_callback) &&
          (win->current_plugin_info))
    {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->enable_button)))
      {
      win->current_plugin_info->enabled = 1;
#if 0
      fprintf(stderr, "Adding plugin: %s %s",
              win->current_plugin_info->name,
              win->current_plugin_info->filename);
#endif
      if(!win->current_plugin_handle)
        win->current_plugin_handle = vis_plugin_load(win->current_plugin_info);
      
      input_add_plugin(the_input,
                       win->current_plugin_handle);
      }
    else
      {
      win->current_plugin_info->enabled = 0;
      vis_plugins_save(win->plugins);
      
      input_remove_plugin(the_input,
                          win->current_plugin_info);
      }
    }
  }

static void delete_callback(GtkWidget * w, GdkEventAny * evt, gpointer * data)
  {
  gtk_main_quit();
  }

static void select_row_callback(GtkWidget * w, int row, int column,
                                GdkEvent * event,
                                gpointer * data)
  {
  int i;
  main_window_t * win;
  win = (main_window_t*)data;

  if(!win->current_plugin_info)
    gtk_widget_set_sensitive(win->enable_button, 1);

  /* Unload old plugins */
  
  if((win->current_plugin_handle) && !(win->current_plugin_info->enabled))
    {
    win->current_plugin_handle->unload(win->current_plugin_handle);
    win->current_plugin_handle = (vis_plugin_handle_t*)0;
    }
  
  win->current_plugin_info = win->plugins;
  for(i = 0; i < row; i++)
    win->current_plugin_info = win->current_plugin_info->next;

  /* Load the plugin if not already loaded */

  //  if(!win->current_plugin_info->plugin)
  //    load_plugin(win->current_plugin_info);
  
  win->no_enable_callback = 1;
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(win->enable_button),
                              win->current_plugin_info->enabled);
  win->no_enable_callback = 0;
      
  if(win->current_plugin_info->flags & VIS_PLGUIN_HAS_CONFIGURE)
    gtk_widget_set_sensitive(win->configure_button, 1);
  else
    gtk_widget_set_sensitive(win->configure_button, 0);

  if(win->current_plugin_info->flags & VIS_PLGUIN_HAS_ABOUT)
    gtk_widget_set_sensitive(win->about_button, 1);
  else
    gtk_widget_set_sensitive(win->about_button, 0);
  }

static char * _title = "XMMS Visualization Plugins";

main_window_t * main_window_create()
  {
  vis_plugin_info_t * info;
  GtkWidget * mainbox, * buttonbox, *scrolledwindow;
  
  main_window_t * ret = calloc(1, sizeof(*ret));

  ret->plugins = vis_plugins_find();
  
  
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), "Gmerlin visualizer "VERSION);
  
  gtk_signal_connect(GTK_OBJECT(ret->window),
                     "delete_event", GTK_SIGNAL_FUNC(delete_callback),
                     ret);
  
  ret->plugin_list = gtk_clist_new_with_titles(1, &_title);

  gtk_signal_connect(GTK_OBJECT(ret->plugin_list), "select_row",
                     GTK_SIGNAL_FUNC(select_row_callback), (gpointer)ret);

  info = ret->plugins;
  
  while(info)
    {
    gtk_clist_append(GTK_CLIST(ret->plugin_list), &(info->name));
    info = info->next;
    }
  scrolledwindow
    = gtk_scrolled_window_new(gtk_clist_get_hadjustment(GTK_CLIST(ret->plugin_list)),
                              gtk_clist_get_vadjustment(GTK_CLIST(ret->plugin_list)));

  gtk_widget_show(ret->plugin_list);
  gtk_container_add(GTK_CONTAINER(scrolledwindow), ret->plugin_list);
  gtk_widget_set_usize(scrolledwindow, 200, 100);
  gtk_widget_show(scrolledwindow);

  /* Set up the buttons */
  
  ret->quit_button =  gtk_button_new_with_label("Quit");
  ret->about_button = gtk_button_new_with_label("About");
  ret->configure_button = gtk_button_new_with_label("Configure");

  ret->enable_button = gtk_check_button_new_with_label("Enable Plugin");

  
  gtk_signal_connect(GTK_OBJECT(ret->quit_button), "clicked",
                     GTK_SIGNAL_FUNC(button_callback), (gpointer)ret);
  gtk_signal_connect(GTK_OBJECT(ret->configure_button), "clicked",
                     GTK_SIGNAL_FUNC(button_callback), (gpointer)ret);
  gtk_signal_connect(GTK_OBJECT(ret->about_button), "clicked",
                     GTK_SIGNAL_FUNC(button_callback), (gpointer)ret);
  gtk_signal_connect(GTK_OBJECT(ret->enable_button), "toggled",
                     GTK_SIGNAL_FUNC(button_callback), (gpointer)ret);

    
  gtk_widget_show(ret->configure_button);
  gtk_widget_show(ret->about_button);
  gtk_widget_show(ret->quit_button);
  gtk_widget_show(ret->enable_button);


  gtk_widget_set_sensitive(ret->configure_button, 0);
  gtk_widget_set_sensitive(ret->about_button, 0);
  gtk_widget_set_sensitive(ret->enable_button, 0);

  /* Pack everything into the mainbox */
  
  mainbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(mainbox), 5);

  gtk_box_pack_start_defaults(GTK_BOX(mainbox), scrolledwindow);

  gtk_box_pack_start(GTK_BOX(mainbox), ret->enable_button, FALSE, FALSE, 0);
  
  buttonbox = gtk_hbutton_box_new();
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->about_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->configure_button);
  gtk_container_add(GTK_CONTAINER(buttonbox), ret->quit_button);
  gtk_widget_show(buttonbox);
  
  gtk_box_pack_start(GTK_BOX(mainbox), buttonbox, FALSE, FALSE, 0);
  
  gtk_widget_show(mainbox);
  
  gtk_container_add(GTK_CONTAINER(ret->window), mainbox);

  info = ret->plugins;
  while(info)
    {
    if(info->enabled)
      {
      fprintf(stderr, "Enabling %s\n", info->name);
      ret->current_plugin_handle = vis_plugin_load(info);
      input_add_plugin(the_input, ret->current_plugin_handle);
      ret->current_plugin_handle = (vis_plugin_handle_t*)0;
      }
    info = info->next;
    }
  
  return ret;
  }

void main_window_destroy(main_window_t * main_win)
  {
  vis_plugins_save(main_win->plugins);
  free(main_win);
  }
