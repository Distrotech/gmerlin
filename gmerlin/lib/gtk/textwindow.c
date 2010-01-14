/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <gtk/gtk.h>
#include <gui_gtk/textview.h>
#include <gui_gtk/gtkutils.h>

/* Box for displaying static text */

struct bg_gtk_textwindow_s
  {
  GtkWidget * window;
  GtkWidget * close_button;
  bg_gtk_textview_t * textview;
  };

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_textwindow_t * win;
  win = (bg_gtk_textwindow_t*)data;
  bg_gtk_textview_destroy(win->textview);
  gtk_widget_hide(win->window);
  gtk_widget_destroy(win->window);
  free(win);
  } 

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

bg_gtk_textwindow_t *
bg_gtk_textwindow_create(const char * text, const char * title)
  {
  GtkWidget * table;

  bg_gtk_textwindow_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER_ON_PARENT);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  gtk_window_set_title(GTK_WINDOW(ret->window), title);

  /* Create close button */

  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  GTK_WIDGET_SET_FLAGS(ret->close_button, GTK_CAN_DEFAULT);

  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);
  gtk_widget_show(ret->close_button);
    
  table = gtk_table_new(2, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  
  /* Create text */
  
  ret->textview = bg_gtk_textview_create();
  bg_gtk_textview_update(ret->textview, text);
  
  gtk_table_attach_defaults(GTK_TABLE(table),
                            bg_gtk_textview_get_widget(ret->textview), 0, 1, 0, 1);
  gtk_table_attach(GTK_TABLE(table), ret->close_button, 0, 1, 1, 2,
                   GTK_SHRINK, GTK_SHRINK, 0, 0);
  
  
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(ret->window), table);
    
  return ret;
  }

void bg_gtk_textwindow_show(bg_gtk_textwindow_t * w, int modal, GtkWidget * parent)
  {
  parent = bg_gtk_get_toplevel(parent);
  if(parent)
    gtk_window_set_transient_for(GTK_WINDOW(w->window),
                                 GTK_WINDOW(parent));
  
  gtk_window_set_modal(GTK_WINDOW(w->window), modal);

  gtk_widget_grab_default(w->close_button);
  gtk_widget_show(w->window);
  }

