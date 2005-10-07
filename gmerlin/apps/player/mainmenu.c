/*****************************************************************
 
  mainmenu.h
 
  Copyright (c) 2003-2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include "gmerlin.h"
#include <utils.h>

struct stream_menu_s
  {
  GtkWidget ** stream_items;

  int num_streams;
  int streams_alloc;
  
  GtkWidget * menu;
  };

struct stream_menu_plugin_s
  {
  GtkWidget ** stream_items;

  int num_streams;
  int streams_alloc;

  GtkWidget * plugin_item;
  GtkWidget * menu;
  };

struct windows_menu_s
  {
  GtkWidget * mediatree;
  guint       mediatree_id;
  GtkWidget * infowindow;
  guint       infowindow_id;
  GtkWidget * logwindow;
  guint       logwindow_id;
  GtkWidget * menu;
  };

struct options_menu_s
  {
  GtkWidget * preferences;
  GtkWidget * plugins;
  GtkWidget * skins;
  GtkWidget * menu;
  };

struct accessories_menu_s
  {
  GtkWidget * transcoder;
  GtkWidget * visualizer;
  GtkWidget * mixer;

  GtkWidget * menu;
  };

struct main_menu_s
  {
  struct windows_menu_s     windows_menu;
  struct options_menu_s     options_menu;
  struct accessories_menu_s accessories_menu;
  
  GtkWidget * windows_item;
  GtkWidget * options_item;
  GtkWidget * accessories_item;
  
  GtkWidget * audio_streams_item;
  GtkWidget * video_streams_item;
  GtkWidget * subpicture_streams_item;
      
  GtkWidget * menu;
  };

static GtkWidget * create_menu()
  {
  GtkWidget * ret;
  GtkWidget * tearoff_item;

  ret = gtk_menu_new();
  tearoff_item = gtk_tearoff_menu_item_new();
  gtk_widget_show(tearoff_item);

  gtk_menu_shell_append(GTK_MENU_SHELL(ret), tearoff_item);
  return ret;
  }


static void menu_callback(GtkWidget * w, gpointer data)
  {
  gmerlin_t * g;
  main_menu_t * the_menu;
  g = (gmerlin_t*)data;
  the_menu = g->player_window->main_menu;
  
  
  if(w == the_menu->options_menu.preferences)
    {
    //    fprintf(stderr, "Launching config dialog\n");
    gmerlin_configure(g);
    }
  else if(w == the_menu->options_menu.skins)
    {
    //    fprintf(stderr, "Launching config dialog\n");
    if(!g->skin_browser)
      g->skin_browser = gmerlin_skin_browser_create(g);
    gmerlin_skin_browser_show(g->skin_browser);
    }
  else if(w == the_menu->options_menu.plugins)
    {
    plugin_window_show(g->plugin_window);
    gtk_widget_set_sensitive(the_menu->options_menu.plugins, 0);
    }

  else if(w == the_menu->windows_menu.infowindow)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.infowindow)))
      {
      bg_gtk_info_window_show(g->info_window);
      g->show_info_window = 1;
      }
    else
      {
      bg_gtk_info_window_hide(g->info_window);
      g->show_info_window = 0;
      }
    }
  else if(w == the_menu->windows_menu.logwindow)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.logwindow)))
      {
      bg_gtk_log_window_show(g->log_window);
      g->show_log_window = 1;
      }
    else
      {
      bg_gtk_log_window_hide(g->log_window);
      g->show_log_window = 0;
      }
    }
  

  else if(w == the_menu->windows_menu.mediatree)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.mediatree)))
      {
      bg_gtk_tree_window_show(g->tree_window);
      g->show_tree_window = 1;
      }
    else
      {
      bg_gtk_tree_window_hide(g->tree_window);
      g->show_tree_window = 0;
      }
    }
  else if(w == the_menu->accessories_menu.visualizer)
    {
    system("gmerlin_visualizer_launcher");
    }
  else if(w == the_menu->accessories_menu.mixer)
    {
    system("gmerlin_alsamixer &");
    }
  else if(w == the_menu->accessories_menu.transcoder)
    {
    system("gmerlin_transcoder_remote -launch");
    }
  }

static GtkWidget * create_item(const char * label,
                               gmerlin_t * gmerlin,
                               GtkWidget * menu)
  {
  GtkWidget * ret;
  ret = gtk_menu_item_new_with_label(label);
  g_signal_connect(G_OBJECT(ret), "activate",
                   G_CALLBACK(menu_callback),
                   gmerlin);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), ret);
  return ret;
  }

static GtkWidget * create_toggle_item(const char * label,
                                      gmerlin_t * gmerlin,
                                      GtkWidget * menu, guint * id)
  {
  GtkWidget * ret;
  ret = gtk_check_menu_item_new_with_label(label);
  *id = g_signal_connect(G_OBJECT(ret), "toggled",
                   G_CALLBACK(menu_callback),
                   gmerlin);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), ret);
  return ret;
  }

static GtkWidget * create_submenu_item(const char * label,
                                       GtkWidget * child_menu,
                                       GtkWidget * parent_menu)
  {
  GtkWidget * ret;
  ret = gtk_menu_item_new_with_label(label);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(ret), child_menu);
  gtk_widget_show(ret);

  gtk_menu_shell_append(GTK_MENU_SHELL(parent_menu), ret);
  return ret;
  }

main_menu_t * main_menu_create(gmerlin_t * gmerlin)
  {
  main_menu_t * ret;
  ret = calloc(1, sizeof(*ret));

  /* Windows */
    
  ret->windows_menu.menu = create_menu();
  ret->windows_menu.mediatree =
    create_toggle_item("Media Tree", gmerlin, ret->windows_menu.menu, &ret->windows_menu.mediatree_id);
  ret->windows_menu.infowindow =
    create_toggle_item("Info window", gmerlin, ret->windows_menu.menu, &ret->windows_menu.infowindow_id);
  ret->windows_menu.logwindow =
    create_toggle_item("Log window", gmerlin, ret->windows_menu.menu, &ret->windows_menu.logwindow_id);
  gtk_widget_show(ret->windows_menu.menu);

  /* Options */
  
  ret->options_menu.menu = create_menu();
  ret->options_menu.preferences =
    create_item("Preferences...", gmerlin, ret->options_menu.menu);
  ret->options_menu.plugins =
    create_item("Plugins...", gmerlin, ret->options_menu.menu);

  ret->options_menu.skins =
    create_item("Skins...", gmerlin, ret->options_menu.menu);

  /* Accessories */

  ret->accessories_menu.menu = create_menu();

  if(bg_search_file_exec("gmerlin_transcoder", (char**)0))
    ret->accessories_menu.transcoder =
      create_item("Transcoder", gmerlin, ret->accessories_menu.menu);
  else
    fprintf(stderr, "gmerlin_transcoder not found\n");

  if(bg_search_file_exec("gmerlin_visualizer", (char**)0))
    ret->accessories_menu.visualizer =
      create_item("Visualizer", gmerlin, ret->accessories_menu.menu);
  else
    fprintf(stderr, "gmerlin_visualizer not found\n");

  if(bg_search_file_exec("gmerlin_alsamixer", (char**)0))
    ret->accessories_menu.mixer =
      create_item("Mixer", gmerlin, ret->accessories_menu.menu);
  else
    fprintf(stderr, "gmerlin_alsamixer not found\n");

  
  /* Main menu */
    
  ret->menu = create_menu();
  
  ret->windows_item = create_submenu_item("Windows...",
                                          ret->windows_menu.menu,
                                          ret->menu);
  ret->options_item = create_submenu_item("Options...",
                                          ret->options_menu.menu,
                                          ret->menu);

  ret->accessories_item = create_submenu_item("Accessories...",
                                              ret->accessories_menu.menu,
                                              ret->menu);
  gtk_widget_show(ret->menu);

  
  
  return ret;
  }

void main_menu_destroy(main_menu_t * m)
  {
  free(m);
  }

GtkWidget * main_menu_get_widget(main_menu_t * m)
  {
  return m->menu;
  }

void main_menu_set_tree_window_item(main_menu_t * m, int state)
  {
  g_signal_handler_block(G_OBJECT(m->windows_menu.mediatree), m->windows_menu.mediatree_id);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m->windows_menu.mediatree), state);
  g_signal_handler_unblock(G_OBJECT(m->windows_menu.mediatree), m->windows_menu.mediatree_id);
  
  }

void main_menu_set_info_window_item(main_menu_t * m, int state)
  {
  g_signal_handler_block(G_OBJECT(m->windows_menu.infowindow), m->windows_menu.infowindow_id);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m->windows_menu.infowindow), state);
  g_signal_handler_unblock(G_OBJECT(m->windows_menu.infowindow), m->windows_menu.infowindow_id);
  }

void main_menu_set_log_window_item(main_menu_t * m, int state)
  {
  g_signal_handler_block(G_OBJECT(m->windows_menu.logwindow), m->windows_menu.logwindow_id);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m->windows_menu.logwindow), state);
  g_signal_handler_unblock(G_OBJECT(m->windows_menu.logwindow), m->windows_menu.logwindow_id);
  }

void main_menu_set_plugin_window_item(main_menu_t * m, int state)
  {
  gtk_widget_set_sensitive(m->options_menu.plugins, !state);
  }
