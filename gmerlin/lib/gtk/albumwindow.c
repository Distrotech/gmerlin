/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <gmerlin/tree.h>
#include <gmerlin/utils.h>

#include <gui_gtk/tree.h>
#include <gui_gtk/gtkutils.h>

/* This is missing in the gtk headers */

extern void
gtk_decorated_window_move_resize_window(GtkWindow*, int, int, int, int);

struct bg_gtk_album_window_s
  {
  bg_gtk_album_widget_t * widget;
  bg_gtk_tree_widget_t * tree_widget;
  GtkWidget * window;
  
  int x, y, width, height;

  bg_cfg_section_t * cfg_section;

  /* Notebook stuff */

  GtkWidget * tab_close_button;
  GtkWidget * tab_label;
  GtkWidget * tab_widget;

  GtkWidget * notebook;
  int name_len;
  GtkAccelGroup * accel_group;
  };

/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name = "x",
      .long_name = "X",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 100 }
    },
    {
      .name = "y",
      .long_name = "Y",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 100 }
    },
    {
      .name = "width",
      .long_name = "Width",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 200 }
    },
    {
      .name = "height",
      .long_name = "Height",
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 300 }
    },
    { /* End of parameters */ }
  };

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_gtk_album_window_t * win;
  win = (bg_gtk_album_window_t*)data;
  if(!name)
    return;
  else if(!strcmp(name, "x"))
    {
    win->x = val->val_i;
    }
  else if(!strcmp(name, "y"))
    {
    win->y = val->val_i;
    }
  else if(!strcmp(name, "width"))
    {
    win->width = val->val_i;
    }
  else if(!strcmp(name, "height"))
    {
    win->height = val->val_i;
    }
  }

static int get_parameter(void * data, const char * name,
                         bg_parameter_value_t * val)
  {
  bg_gtk_album_window_t * win;
  win = (bg_gtk_album_window_t*)data;
  if(!name)
    return 1;
  else if(!strcmp(name, "x"))
    {
    val->val_i = win->x;
    return 1;
    }
  else if(!strcmp(name, "y"))
    {
    val->val_i = win->y;
    return 1;
    }
  else if(!strcmp(name, "width"))
    {
    val->val_i = win->width;
    return 1;
    }
  else if(!strcmp(name, "height"))
    {
    val->val_i = win->height;
    return 1;
    }
  return 0;
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  bg_gtk_album_window_t * win;
  win = (bg_gtk_album_window_t*)data;
  bg_gtk_album_window_destroy(win, 1);
  return TRUE;
  }

static void close_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_album_window_t * win;
  win = (bg_gtk_album_window_t*)data;
  bg_gtk_album_window_destroy(win, 1);
  }

static void widget_close_callback(bg_gtk_album_widget_t * w, gpointer data)
  {
  bg_gtk_album_window_t * win;
  win = (bg_gtk_album_window_t*)data;
  bg_gtk_album_window_destroy(win, 1);
  }


static void name_change_callback(bg_album_t * a,
                                 const char * name,
                                 void * data)
  {
  bg_gtk_album_window_t * win = (bg_gtk_album_window_t *)data;
  win->name_len = strlen(name);
  if(win->notebook)
    {
    gtk_label_set_text(GTK_LABEL(win->tab_label), name);
    }
  else if(win->window)
    {
    gtk_window_set_title(GTK_WINDOW(win->window), name);
    }
  }

bg_gtk_album_window_t *
bg_gtk_album_window_create(bg_album_t * album,
                           bg_gtk_tree_widget_t * tree_widget, GtkAccelGroup * accel_group)
  {
  
  bg_gtk_album_window_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->tree_widget = tree_widget;

  ret->accel_group = accel_group;
  
  bg_album_set_name_change_callback(album, name_change_callback, ret);
  
  ret->widget = bg_gtk_album_widget_create(album, ret->window);
  bg_gtk_album_widget_set_close_callback(ret->widget,
                                         widget_close_callback,
                                         ret);
    

  g_object_ref(G_OBJECT(bg_gtk_album_widget_get_widget(ret->widget)));

  /* Set config stuff */

  ret->cfg_section =
    bg_cfg_section_find_subsection(bg_album_get_cfg_section(album), "gtk_albumwindow");

  bg_cfg_section_apply(ret->cfg_section, parameters, set_parameter, ret);
  
  return ret;
  }

void bg_gtk_album_window_destroy(bg_gtk_album_window_t * w, int notify)
  {
  int page_num;
  
  /* Get the window coordinates */
  
  if(w->window && w->window->window)
    {
    gdk_window_get_geometry(w->window->window,
                            NULL, NULL, &w->width, &w->height,
                            NULL);

    gdk_window_get_root_origin(w->window->window, &w->x, &w->y);

    bg_cfg_section_get(w->cfg_section, parameters, get_parameter, w);
    }

  bg_gtk_album_widget_put_config(w->widget);
  
  if(w->tree_widget && notify)
    {
    bg_gtk_tree_widget_close_album(w->tree_widget, w);
    }
  if(w->window)
    gtk_widget_destroy(w->window);

  if(w->notebook)
    {
    page_num = gtk_notebook_page_num(GTK_NOTEBOOK(w->notebook), bg_gtk_album_widget_get_widget(w->widget));
    gtk_notebook_remove_page(GTK_NOTEBOOK(w->notebook), page_num);
    }

  if(w->widget)
    {
    bg_album_set_name_change_callback(bg_gtk_album_widget_get_album(w->widget),
                                                                NULL, NULL);
    
    g_object_unref(G_OBJECT(bg_gtk_album_widget_get_widget(w->widget)));
    bg_gtk_album_widget_destroy(w->widget);
    }
  free(w);
  }

bg_album_t * bg_gtk_album_window_get_album(bg_gtk_album_window_t*w)
  {
  return bg_gtk_album_widget_get_album(w->widget);
  }

void bg_gtk_album_window_raise(bg_gtk_album_window_t* w)
  {
  int page_num;
  
  if(w->window && w->window->window)
    gtk_window_present(GTK_WINDOW(w->window));

  else if(w->notebook)
    {
    page_num = gtk_notebook_page_num(GTK_NOTEBOOK(w->notebook), bg_gtk_album_widget_get_widget(w->widget));
    gtk_notebook_set_current_page(GTK_NOTEBOOK(w->notebook), page_num);
    
    }
  }



static GtkWidget * create_close_button(bg_gtk_album_window_t * w)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", "tab_close.png");
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(close_callback), w);

  gtk_widget_show(button);
  return button;
  }


void bg_gtk_album_window_attach(bg_gtk_album_window_t * w, GtkWidget * notebook)
  {
  const char * name;
  int page_num;
  
  bg_album_t * album;
  
  /* Remove widget from container and delete window */

  if(w->window)
    {
    gtk_container_remove(GTK_CONTAINER(w->window), bg_gtk_album_widget_get_widget(w->widget));
    gtk_widget_destroy(w->window);
    w->window = NULL;
    }
  /* Attach stuff to notebook */

  album = bg_gtk_album_widget_get_album(w->widget);

  name = bg_album_get_label(album);
  
  w->tab_label = gtk_label_new(name);
  w->name_len = strlen(name);
  
  gtk_widget_show(w->tab_label);
  
  w->tab_close_button = create_close_button(w);
  w->tab_widget = gtk_hbox_new(0, 2);
  bg_gtk_box_pack_start_defaults(GTK_BOX(w->tab_widget), w->tab_label);
  gtk_box_pack_start(GTK_BOX(w->tab_widget), w->tab_close_button, FALSE, FALSE, 0);
  gtk_widget_show(w->tab_widget);

  page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), bg_gtk_album_widget_get_widget(w->widget),
                           w->tab_widget);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);

#if GTK_CHECK_VERSION(2,20,0) 
  g_object_set(notebook, "tab-fill", FALSE, NULL);
#else 
  gtk_notebook_set_tab_label_packing(GTK_NOTEBOOK(notebook), bg_gtk_album_widget_get_widget(w->widget),
                                     FALSE, FALSE, GTK_PACK_START);
#endif
  
  gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(notebook),
                                   bg_gtk_album_widget_get_widget(w->widget),
                                   bg_album_get_label(album));

  
  w->notebook = notebook;
  if(bg_album_is_current(album))
    bg_gtk_album_window_set_current(w, 1);
  }

void bg_gtk_album_window_detach(bg_gtk_album_window_t * w)
  {
  int page_num;
  bg_album_t * album;

  album = bg_gtk_album_widget_get_album(w->widget);

  if(w->notebook)
    {
    page_num = gtk_notebook_page_num(GTK_NOTEBOOK(w->notebook), bg_gtk_album_widget_get_widget(w->widget));
    gtk_notebook_remove_page(GTK_NOTEBOOK(w->notebook), page_num);
    w->notebook = NULL;
    }
  
  w->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_add_accel_group(GTK_WINDOW (w->window), w->accel_group);
  gtk_window_add_accel_group(GTK_WINDOW (w->window), bg_gtk_album_widget_get_accel_group(w->widget));
    
  g_signal_connect(G_OBJECT(w->window), "delete-event",
                   G_CALLBACK(delete_callback),
                   w);
  gtk_window_set_title(GTK_WINDOW(w->window), bg_album_get_label(album));
  
  gtk_container_add(GTK_CONTAINER(w->window),
                    bg_gtk_album_widget_get_widget(w->widget));

  gtk_widget_show(w->window);

  gtk_decorated_window_move_resize_window(GTK_WINDOW(w->window),
                                          w->x, w->y, w->width, w->height);
  
  }

void bg_gtk_album_window_set_current(bg_gtk_album_window_t * w, int current)
  {
  PangoAttribute *attr;
  PangoAttrList *attr_list;


  if(!w->notebook)
    return;
  attr_list = pango_attr_list_new();
  attr = pango_attr_weight_new(current ? PANGO_WEIGHT_BOLD: PANGO_WEIGHT_NORMAL);
  attr->start_index = 0;
  attr->end_index = w->name_len;

  pango_attr_list_insert(attr_list,attr);
  gtk_label_set_attributes(GTK_LABEL(w->tab_label), attr_list);
  pango_attr_list_unref(attr_list);
  }

void bg_gtk_album_window_goto_current(bg_gtk_album_window_t * w)
  {
  bg_gtk_album_window_raise(w);
  bg_gtk_album_widget_goto_current(w->widget);
  }

GtkAccelGroup * bg_gtk_album_window_get_accel_group(bg_gtk_album_window_t * w)
  {
  return bg_gtk_album_widget_get_accel_group(w->widget);
  }
