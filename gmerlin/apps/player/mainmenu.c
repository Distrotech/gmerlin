#include "gmerlin.h"

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
  GtkWidget * infowindow;
  GtkWidget * menu;
  };

struct options_menu_s
  {
  GtkWidget * preferences;
  GtkWidget * plugins;
  GtkWidget * menu;
  };

struct main_menu_s
  {
  struct windows_menu_s windows_menu;
  struct options_menu_s options_menu;
  
  GtkWidget * windows_item;
  GtkWidget * options_item;
    
  GtkWidget * audio_streams_item;
  GtkWidget * video_streams_item;
  GtkWidget * subpicture_streams_item;
  GtkWidget * programs_item;
      
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

static void infowindow_close_callback(bg_gtk_info_window_t * w, void * data)
  {
  gmerlin_t * g;
  main_menu_t * the_menu;
  g = (gmerlin_t*)data;
  the_menu = g->player_window->main_menu;

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.infowindow), FALSE);
  }

static void pluginwindow_close_callback(plugin_window_t * w, void * data)
  {
  gmerlin_t * g;
  main_menu_t * the_menu;
  g = (gmerlin_t*)data;
  the_menu = g->player_window->main_menu;
  gtk_widget_set_sensitive(the_menu->options_menu.plugins, 1);
  }

void gmerlin_tree_close_callback(bg_gtk_tree_window_t * win,
                                 void * data)
  {
  gmerlin_t * g;
  main_menu_t * the_menu;
  g = (gmerlin_t*)data;
  the_menu = g->player_window->main_menu;
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.mediatree), FALSE);
  
  }


static void menu_callback(GtkWidget * w, gpointer data)
  {
  gmerlin_t * g;
  main_menu_t * the_menu;
  g = (gmerlin_t*)data;
  the_menu = g->player_window->main_menu;
  
  
  if(w == the_menu->options_menu.preferences)
    {
    fprintf(stderr, "Launching config dialog\n");
    gmerlin_configure(g);
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
      }
    else
      bg_gtk_info_window_hide(g->info_window);
    }

  else if(w == the_menu->windows_menu.mediatree)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(the_menu->windows_menu.mediatree)))
      {
      bg_gtk_tree_window_show(g->tree_window);
      }
    else
      bg_gtk_tree_window_hide(g->tree_window);
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
                                      GtkWidget * menu)
  {
  GtkWidget * ret;
  ret = gtk_check_menu_item_new_with_label(label);
  g_signal_connect(G_OBJECT(ret), "toggled",
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
  
  ret->windows_menu.menu = create_menu();
  ret->windows_menu.mediatree =
    create_toggle_item("Media Tree", gmerlin, ret->windows_menu.menu);
  ret->windows_menu.infowindow =
    create_toggle_item("Info window", gmerlin, ret->windows_menu.menu);
  gtk_widget_show(ret->windows_menu.menu);
  
  ret->options_menu.menu = create_menu();
  ret->options_menu.preferences =
    create_item("Preferences...", gmerlin, ret->options_menu.menu);
  ret->options_menu.plugins =
    create_item("Plugins...", gmerlin, ret->options_menu.menu);

  ret->menu = create_menu();
  
  ret->windows_item = create_submenu_item("Windows...",
                                          ret->windows_menu.menu,
                                          ret->menu);
  ret->options_item = create_submenu_item("Options...",
                                          ret->options_menu.menu,
                                          ret->menu);
  gtk_widget_show(ret->menu);

  gmerlin->info_window = bg_gtk_info_window_create(gmerlin->player,
                                                   infowindow_close_callback, 
                                                   gmerlin);

  gmerlin->plugin_window = plugin_window_create(gmerlin,
                                                pluginwindow_close_callback, 
                                                gmerlin);
  
  
  return ret;
  }

GtkWidget * main_menu_get_widget(main_menu_t * m)
  {
  return m->menu;
  }
