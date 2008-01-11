/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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


#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <config.h>

#include <tree.h>
#include <gui_gtk/tree.h>
#include <gui_gtk/gtkutils.h>

extern void
gtk_decorated_window_move_resize_window(GtkWindow*,int, int, int, int);


struct bg_gtk_tree_window_s
  {
  bg_gtk_tree_widget_t * widget;
  GtkWidget * window;
  void (*close_callback)(bg_gtk_tree_window_t*,void*);
  void * close_callback_data;

  /* Configuration stuff */

  bg_cfg_section_t * cfg_section;
  
  int x, y, width, height;
  };

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
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
  bg_gtk_tree_window_t * win;
  win = (bg_gtk_tree_window_t*)data;
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
  bg_gtk_tree_window_t * win;
  win = (bg_gtk_tree_window_t*)data;
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
  bg_gtk_tree_window_t * win;
  win = (bg_gtk_tree_window_t*)data;

  if(win->close_callback)
    win->close_callback(win, win->close_callback_data);
  
  bg_gtk_tree_window_hide(win);
  return TRUE;
  }

bg_gtk_tree_window_t *
bg_gtk_tree_window_create(bg_media_tree_t * tree,
                          void (*close_callback)(bg_gtk_tree_window_t*,void*),
                          void * close_callback_data, GtkAccelGroup * accel_group)
  {
  bg_gtk_tree_window_t * ret = calloc(1, sizeof(*ret));
    
  ret->cfg_section =
    bg_cfg_section_find_subsection(bg_media_tree_get_cfg_section(tree), "gtk_treewindow");
  

  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_add_accel_group(GTK_WINDOW(ret->window), accel_group);

  ret->widget = bg_gtk_tree_widget_create(tree, accel_group, ret->window);
  
  ret->close_callback = close_callback;
  ret->close_callback_data = close_callback_data;
    
  
  g_signal_connect(G_OBJECT(ret->window), "delete-event",
                   G_CALLBACK(delete_callback),
                   ret);
  
  gtk_container_add(GTK_CONTAINER(ret->window),
                    bg_gtk_tree_widget_get_widget(ret->widget));

  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Gmerlin Media Tree"));
  
  return ret;
  }

void bg_gtk_tree_window_destroy(bg_gtk_tree_window_t * w)
  {
  bg_gtk_tree_widget_destroy(w->widget);
  gtk_widget_hide(w->window);
  gtk_widget_destroy(w->window);
  free(w);
  }

void bg_gtk_tree_window_show(bg_gtk_tree_window_t* w)
  {
  gtk_widget_show(w->window);

  //  gtk_window_present(GTK_WINDOW(w->window));

  
  bg_cfg_section_apply(w->cfg_section,
                       parameters,
                       set_parameter,
                       w);
  
  if((w->width > 0) && (w->height > 0))
    {
    gtk_decorated_window_move_resize_window(GTK_WINDOW(w->window),
                                            w->x, w->y, w->width, w->height);
    }
  }

void bg_gtk_tree_window_hide(bg_gtk_tree_window_t* w)
  {
  if(w->window->window)
    {
    gdk_window_get_geometry(w->window->window,
                            (gint *)0, (gint *)0, &(w->width), &(w->height),
                            (gint *)0);

    gdk_window_get_root_origin(w->window->window, &(w->x), &(w->y));

    bg_cfg_section_get(w->cfg_section,
                       parameters,
                       get_parameter,
                       w);
    }
  gtk_widget_hide(w->window);
  }


void bg_gtk_tree_window_open_incoming(bg_gtk_tree_window_t * win)
  {
  bg_gtk_tree_widget_open_incoming(win->widget);
  }

void bg_gtk_tree_window_goto_current(bg_gtk_tree_window_t * win)
  {
  bg_gtk_tree_widget_goto_current(win->widget);
  }

void bg_gtk_tree_window_update(bg_gtk_tree_window_t * w,
                               int open_albums)
  {
  bg_gtk_tree_widget_update(w->widget, open_albums);
  }
