/*****************************************************************
 * gmerlin-mozilla - A gmerlin based mozilla plugin
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <string.h>
#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gdk/gdkkeysyms.h>

// #define DND_GMERLIN_TRACKS   1
#define DND_TEXT_URI_LIST    2
#define DND_TEXT_PLAIN       3

static const GtkTargetEntry copy_entries[] = 
  {
    //    {bg_gtk_atom_entries_name, 0, DND_GMERLIN_TRACKS },
    {"text/uri-list",          0, DND_TEXT_URI_LIST  },
    {"STRING",                 0, DND_TEXT_PLAIN  },
    {"text/plain",             0, DND_TEXT_URI_LIST  },
  };

static void clipboard_get_func(GtkClipboard *clipboard,
                               GtkSelectionData *selection_data,
                               guint info,
                               gpointer data)
  {
  bg_album_entry_t * e;
  GdkAtom type_atom;
  char * str;
  char * tmp_string_1;
  char * tmp_string_2;
  bg_mozilla_t * w = (bg_mozilla_t*)data;
  type_atom = gdk_atom_intern("STRING", FALSE);
  if(!type_atom)
    return;
  
  switch(info)
    {
#if 0
    case DND_GMERLIN_TRACKS:
      str = bg_album_entries_save_to_memory(w->clipboard);
      gtk_selection_data_set(selection_data, type_atom, 8, (uint8_t*)str,
                             strlen(str)+1);
      free(str);
      break;
#endif
    case DND_TEXT_URI_LIST:
      e = w->clipboard;
      str = (char*)0;
      while(e)
        {
        tmp_string_1 = bg_string_to_uri(e->location, -1);
        tmp_string_2 = bg_sprintf("%s\r\n", tmp_string_1);
        str = bg_strcat(str, tmp_string_2);
        free(tmp_string_1);
        free(tmp_string_2);
        e = e->next;
        }
      gtk_selection_data_set(selection_data, type_atom, 8, (uint8_t*)str,
                             strlen(str)+1);
      free(str);
      break;
    case DND_TEXT_PLAIN:
      e = w->clipboard;
      str = (char*)0;
      while(e)
        {
        tmp_string_2 = bg_sprintf("%s\n", (char*)e->location);
        str = bg_strcat(str, tmp_string_2);
        free(tmp_string_2);
        e = e->next;
        }
      gtk_selection_data_set(selection_data, type_atom, 8, (uint8_t*)str,
                             strlen(str)+1);
      free(str);
      break;
      
      //      break;
    }
  }

static void clipboard_clear_func(GtkClipboard *clipboard,
                                 gpointer data)
  {
  bg_mozilla_t * w = (bg_mozilla_t*)data;
  if(w->clipboard)
    {
    bg_album_entries_destroy(w->clipboard);
    w->clipboard = (bg_album_entry_t*)0;
    }
  }

static void do_copy(bg_mozilla_t * m)
  {
  GtkClipboard *clipboard;
  GdkAtom clipboard_atom;

  clipboard_atom = gdk_atom_intern ("CLIPBOARD", FALSE);   
  clipboard = gtk_clipboard_get(clipboard_atom);
  
  gtk_clipboard_set_with_data(clipboard,
                              copy_entries,
                              sizeof(copy_entries)/
                              sizeof(copy_entries[0]),
                              clipboard_get_func,
                              clipboard_clear_func,
                              (gpointer)m);
  
  if(m->clipboard)
    {
    bg_album_entries_destroy(m->clipboard);
    m->clipboard = (bg_album_entry_t*)0;
    }
  if(m->ti && m->current_url)
    {
    m->clipboard = bg_album_entry_create_from_track_info(m->ti,
                                                         m->current_url);
    }
  }


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
  else if(w == widget->menu.url_menu.copy)
    do_copy(widget->m);
  else if(w == widget->menu.fullscreen)
    bg_mozilla_widget_toggle_fullscreen(widget->m->widget);
  else if(w == widget->menu.windowed)
    bg_mozilla_widget_toggle_fullscreen(widget->m->widget);
  else if(w == widget->menu.save_item)
    bg_mozilla_set_download(widget->m,
                            gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
  
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

static GtkWidget * create_toggle_item(const char * label,
                                      bg_mozilla_widget_t * m,
                                      GtkWidget * menu)
  {
  guint32 handler_id;
  GtkWidget * ret;
  ret = gtk_check_menu_item_new_with_label(label);
  handler_id = g_signal_connect(G_OBJECT(ret), "toggled",
                   G_CALLBACK(menu_callback),
                   m);
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
  
  gtk_widget_add_accelerator(m->menu.windowed,
                             "activate",
                             m->accel_group,
                             GDK_Escape, 0, GTK_ACCEL_VISIBLE);

  m->menu.save_item = 
    create_toggle_item(TR("Download this stream"),
                       m, m->menu.menu);
    
  gtk_widget_hide(m->menu.windowed);
  gtk_widget_show(m->menu.menu);
  }
