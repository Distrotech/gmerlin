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

#include <gtk/gtk.h>
#include <stdlib.h>

#include <gmerlin/utils.h>
#include <gmerlin/subprocess.h>

#include <gui_gtk/urllink.h>

struct bg_gtk_urllink_s
  {
  GtkWidget * widget;
  GdkCursor * cursor;
  char * url;
  char * browser_command;
  };

static void realize_callback(GtkWidget * w,
                             gpointer data)
  {
  bg_gtk_urllink_t * u;
  u = (bg_gtk_urllink_t *)data;
  gdk_window_set_cursor(u->widget->window, u->cursor);
  }

static void button_callback(GtkWidget * w, GdkEventButton * evt,
                            gpointer data)
  {
  char * command;
  bg_gtk_urllink_t * u;
  u = (bg_gtk_urllink_t *)data;
  command = bg_sprintf(u->browser_command, u->url);
  command = bg_strcat(command, " &");
  bg_system(command);
  }

bg_gtk_urllink_t * bg_gtk_urllink_create(const char * text, const char * url)
  {
  bg_gtk_urllink_t * ret;
  
  GtkWidget * label;
  char * tmp_string;
  ret = calloc(1, sizeof(*ret));
  ret->browser_command = bg_find_url_launcher();
  if(!ret->browser_command)
    {
    tmp_string = bg_sprintf("%s [%s]", text, url);
    ret->widget = gtk_label_new(tmp_string);
    gtk_widget_show(ret->widget);
    free(tmp_string);
    }
  else
    {
    ret->url = bg_strdup(ret->url, url);
    label = gtk_label_new("");
    tmp_string =
      bg_sprintf("<span foreground=\"blue\" underline=\"single\">%s</span>",
                 text);
    gtk_label_set_markup(GTK_LABEL(label), tmp_string);
    gtk_widget_show(label);
    
    ret->widget = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(ret->widget), label);
    ret->cursor = gdk_cursor_new(GDK_HAND2);

    gtk_widget_set_events(ret->widget, GDK_BUTTON_PRESS_MASK);
    
    g_signal_connect(ret->widget, "realize",
                     G_CALLBACK(realize_callback), ret);
    g_signal_connect(ret->widget, "button-press-event",
                     G_CALLBACK(button_callback), ret);
    
    gtk_widget_show(ret->widget);
    }
  if(tmp_string) free(tmp_string);
  return ret;
  }

GtkWidget * bg_gtk_urllink_get_widget(bg_gtk_urllink_t * u)
  {
  return u->widget;
  }

void bg_gtk_urllink_destroy(bg_gtk_urllink_t * u)
  {
  gdk_cursor_unref(u->cursor);
  if(u->url) free(u->url);
  if(u->browser_command) free(u->browser_command);
  free(u);
  }
