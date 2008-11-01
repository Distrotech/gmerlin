#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>

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
  bg_mozilla_widget_t * widget = data;
  if(w == widget->menu.config_menu.plugins)
    bg_mozilla_plugin_window_show(widget->m->plugin_window);
  else if(w == widget->menu.config_menu.preferences)
    bg_dialog_show(widget->m->cfg_dialog, NULL);
  else if(w == widget->menu.url_menu.info)
    bg_gtk_info_window_show(widget->m->info_window);
  else if(w == widget->menu.fullscreen)
    {
    bg_mozilla_widget_toggle_fullscreen(widget->m->widget);
    }
  else if(w == widget->menu.windowed)
    {
    bg_mozilla_widget_toggle_fullscreen(widget->m->widget);
    }
  }

static GtkWidget *
create_pixmap_item(const char * label, const char * pixmap,
                   bg_mozilla_widget_t * m,
                   GtkWidget * menu)
  {
  GtkWidget * ret, *image;
  char * path;
  
  
  if(pixmap)
    {
    path = bg_search_file_read("icons", pixmap);
    if(path)
      {
      image = gtk_image_new_from_file(path);
      free(path);
      }
    else
      image = gtk_image_new();
    gtk_widget_show(image);
    ret = gtk_image_menu_item_new_with_label(label);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(ret), image);
    }
  else
    {
    ret = gtk_menu_item_new_with_label(label);
    }
  
  g_signal_connect(G_OBJECT(ret), "activate",
                   G_CALLBACK(menu_callback),
                   (gpointer)m);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), ret);
  return ret;
  }

static GtkWidget * create_item(const char * label,
                               bg_mozilla_widget_t * m,
                               GtkWidget * menu)
  {
  GtkWidget * ret;
  ret = gtk_menu_item_new_with_label(label);
  g_signal_connect(G_OBJECT(ret), "activate",
                   G_CALLBACK(menu_callback),
                   m);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), ret);
  return ret;
  }

#if 0
static GtkWidget * create_toggle_item(const char * label,
                                      bg_mozilla_widget_t * m,
                                      GtkWidget * menu, guint * id)
  {
  guint32 handler_id;
  GtkWidget * ret;
  ret = gtk_check_menu_item_new_with_label(label);
  handler_id = g_signal_connect(G_OBJECT(ret), "toggled",
                   G_CALLBACK(menu_callback),
                   m);
  if(id)
    *id = handler_id;
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), ret);
  return ret;
  }
#endif

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

void bg_mozilla_widget_init_menu(bg_mozilla_widget_t * m)
  {
  /* Location... */
  m->menu.url_menu.menu = create_menu();
  m->menu.url_menu.copy = create_pixmap_item(TR("Copy"), "copy_16.png",
                                             m, m->menu.url_menu.menu);
  m->menu.url_menu.info = create_pixmap_item(TR("Info"), "info_16.png",
                                             m, m->menu.url_menu.menu);
  m->menu.url_menu.launch = create_item(TR("Launch"),
                                        m, m->menu.url_menu.menu);

  gtk_widget_show(m->menu.url_menu.menu);
  
  /* Options */
  m->menu.config_menu.menu = create_menu();
  m->menu.config_menu.preferences =
    create_pixmap_item(TR("Preferences..."), "config_16.png",
                       m, m->menu.config_menu.menu);
  m->menu.config_menu.plugins =
    create_pixmap_item(TR("Plugins..."), "plugin_16.png",
                       m, m->menu.config_menu.menu);
  
  gtk_widget_show(m->menu.config_menu.menu);

  
  m->menu.menu = create_menu();

  m->menu.url_item = create_submenu_item(TR("Location..."),
                                         m->menu.url_menu.menu,
                                         m->menu.menu);
  m->menu.url_item = create_submenu_item(TR("Options..."),
                                         m->menu.config_menu.menu,
                                         m->menu.menu);

  m->menu.fullscreen = create_pixmap_item(TR("Fullscreen"), "fullscreen_16.png",
                                          m, m->menu.menu);
  m->menu.windowed = create_pixmap_item(TR("Windowed"), "windowed_16.png",
                                        m, m->menu.menu);

  gtk_widget_hide(m->menu.windowed);
  gtk_widget_show(m->menu.menu);
  }
