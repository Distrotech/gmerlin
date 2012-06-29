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

#include <stdlib.h>


#include <gtk/gtk.h>
#include <gui_gtk/aboutwindow.h>
#include <gui_gtk/gtkutils.h>
#include <gui_gtk/scrolltext.h>
#include <gui_gtk/urllink.h>

#include <gmerlin/utils.h>

struct bg_gtk_about_window_s
  {
  GtkWidget * window;
  GtkWidget * close_button;
  GtkWidget * url_button;

  bg_gtk_urllink_t * link;
  
  char * url_opener;
  
  bg_gtk_scrolltext_t * scrolltext;
  
  void (*close_callback)(bg_gtk_about_window_t*, void*);
  void * close_callback_data;
  
  };

static void about_window_destroy(bg_gtk_about_window_t * win)
  {
  bg_gtk_scrolltext_destroy(win->scrolltext);
  bg_gtk_urllink_destroy(win->link);
  
  gtk_widget_destroy(win->window);
  free(win);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_gtk_about_window_t * win;
  win = (bg_gtk_about_window_t*)data;

  if((w == win->close_button) || (w == win->window))
    {
    if(win->close_callback)
      win->close_callback(win, win->close_callback_data);
    
    about_window_destroy(win);
    }
  else if(w == win->url_button)
    {
    
    }
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  bg_gtk_about_window_t * win;
  win = (bg_gtk_about_window_t*)data;
  button_callback(win->window, data);
  return TRUE;
  }

static const float scroll_fg[3] = { 0.0, 1.0, 1.0 };
static const float scroll_bg[3] = { 0.0, 0.0, 0.0 };

bg_gtk_about_window_t *
bg_gtk_about_window_create(const char * name, const char * version, const char * icon,
                           void (*close_callback)(bg_gtk_about_window_t*,
                                                  void*),
                           void * close_callback_data)
  {
  char * path;
  char * label_text;

  GtkWidget * box;
    
  GtkWidget * label1;
  GtkWidget * label2;
  GtkWidget * label3;
  
  GtkWidget * table;
  GtkWidget * image;
  bg_gtk_about_window_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->close_callback = close_callback;
  ret->close_callback_data = close_callback_data;
  
  ret->scrolltext = bg_gtk_scrolltext_create(300, 24);
  bg_gtk_scrolltext_set_font(ret->scrolltext,
                             "Sans-10:slant=0:weight=200:width=100");
  
  bg_gtk_scrolltext_set_text(ret->scrolltext, TR("Get the latest source version from http://gmerlin.sourceforge.net * * * If you installed gmerlin from a binary package, you might have limited features"), scroll_fg, scroll_bg);
  
  /* Create window */
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window),
                          GTK_WIN_POS_CENTER);
  
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("About"));

  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);
  
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);

  ret->link =
    bg_gtk_urllink_create(TR("Visit homepage"), "http://gmerlin.sourceforge.net");
  
  gtk_widget_show(ret->close_button);
  
  path = bg_search_file_read("icons", icon);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    gtk_widget_show(image);
    free(path);
    }
  else
    image = NULL;

  /* Label 1 */

  label_text = bg_sprintf("<span size=\"x-large\" weight=\"bold\">%s %s</span>",
                          name, version);
  
  label1 = gtk_label_new("");
  gtk_label_set_markup(GTK_LABEL(label1), label_text);
  free(label_text);
#if 0
  attr_list = pango_attr_list_new();
  //  attr = pango_attr_size_new_absolute(18);
  //  pango_attr_list_insert(attr_list,attr);
  attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
  pango_attr_list_insert(attr_list,attr);
  
  gtk_label_set_attributes(GTK_LABEL(label1), attr_list);
  pango_attr_list_unref(attr_list);
#endif
  gtk_widget_show(label1);

  label2 = gtk_label_new(TR("Copyright \302\251 2001-2009 Members of the gmerlin project"));
  gtk_widget_show(label2);
  
  label3 =
    gtk_label_new(TR("This is free software.  You may redistribute copies of it under the terms of\n\
the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n\
There is NO WARRANTY."));
  
  gtk_widget_show(label3);
  
  /* Pack */

  table = gtk_table_new(5, 2, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);
  gtk_container_set_border_width(GTK_CONTAINER(table), 10);
  
  if(image)
    gtk_table_attach_defaults(GTK_TABLE(table), image, 0, 1, 2, 3);

  gtk_table_attach_defaults(GTK_TABLE(table), label1, 0, 2, 0, 1);

  box = gtk_hbox_new(0, 5);
  bg_gtk_box_pack_start_defaults(GTK_BOX(box), label2);
  bg_gtk_box_pack_start_defaults(GTK_BOX(box),
                              bg_gtk_urllink_get_widget(ret->link));
  gtk_widget_show(box);
  gtk_table_attach_defaults(GTK_TABLE(table), box, 0, 2, 1, 2);

  gtk_table_attach_defaults(GTK_TABLE(table), label3, 1, 2, 2, 3);
  
  gtk_table_attach_defaults(GTK_TABLE(table),
                            bg_gtk_scrolltext_get_widget(ret->scrolltext), 0, 2, 3, 4);


  
  gtk_table_attach(GTK_TABLE(table), ret->close_button, 0, 2, 4, 5, GTK_SHRINK,
                   GTK_FILL, 0, 0);

  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(ret->window), table);
  
  gtk_widget_show(ret->window);
    
  return ret;
  }
