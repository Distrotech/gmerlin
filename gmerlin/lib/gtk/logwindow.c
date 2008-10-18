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

#include <gmerlin/log.h>
#include <gmerlin/utils.h>

#include <gui_gtk/logwindow.h>
#include <gui_gtk/gtkutils.h>

/* This is missing in the gtk headers */

extern void
gtk_decorated_window_move_resize_window(GtkWindow*, int, int, int, int);

/* Delay between update calls in Milliseconds */
#define DELAY_TIME 50

struct bg_gtk_log_window_s
  {
  GtkWidget * window;
  GtkWidget * textview;
  GtkTextBuffer * buffer;
  GtkWidget * scrolledwindow;

  void (*close_callback)(bg_gtk_log_window_t*, void*);
  void * close_callback_data;

  bg_msg_queue_t * queue;

  GtkTextTagTable * tag_table;
  GtkTextTag      * info_tag;
  GtkTextTag      * debug_tag;
  GtkTextTag      * error_tag;
  GtkTextTag      * warning_tag;
  
  int visible;

  int num_messages;
  int max_messages;

  int show_info;
  int show_warning;
  int show_error;
  int show_debug;

  int x, y, width, height;

  char * last_error;
  };

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  bg_gtk_log_window_t * win;
  win = (bg_gtk_log_window_t*)data;
  if(win->close_callback)
    win->close_callback(win, win->close_callback_data);
  gtk_widget_hide(win->window);
  win->visible = 0;
  return TRUE;
  }

static void delete_first_line(bg_gtk_log_window_t * win)
  {
  GtkTextIter start_iter, end_iter;

  gtk_text_buffer_get_iter_at_line(win->buffer, &start_iter, 0);
  gtk_text_buffer_get_iter_at_line(win->buffer, &end_iter, 1);
  gtk_text_buffer_delete(win->buffer, &start_iter, &end_iter);
  win->num_messages--;
  }

static void changed_callback(gpointer       data)
  {
  bg_gtk_log_window_t * w;
  GtkTextIter iter;
  GtkTextMark * mark = (GtkTextMark *)0;
  w = (bg_gtk_log_window_t *)data;

  
  gtk_text_buffer_get_end_iter(w->buffer, &iter);

  mark = gtk_text_buffer_create_mark(w->buffer, NULL, &iter, FALSE);
  gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(w->textview), mark);
  gtk_text_buffer_delete_mark(w->buffer, mark);
  

  //  gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(w->textview),
  //                               &iter, 0.0, FALSE, 0.0, 0.0);
  
  }

static gboolean idle_callback(gpointer data)
  {
  bg_msg_t * msg;
  bg_gtk_log_window_t * w;
  bg_log_level_t level;
  const char * level_name;
  char * domain;
  char * message;
  GtkTextTag * tag = (GtkTextTag *)0;
  char * str;
  GtkTextIter iter;
  int i;
  char ** lines;
  int got_message = 0;
  int do_log = 0;
  
  w = (bg_gtk_log_window_t *)data;
  
  while((msg = bg_msg_queue_try_lock_read(w->queue)))
    {
    while(w->num_messages > w->max_messages - 1)
      delete_first_line(w);
    
    level = bg_msg_get_id(msg);
    level_name = bg_log_level_to_string(level);
    domain = bg_msg_get_arg_string(msg, 0);
    message = bg_msg_get_arg_string(msg, 1);
    
    do_log = 0;

    
    switch(level)
      {
      case BG_LOG_DEBUG:
        tag = w->debug_tag;
        if(w->show_debug) do_log = 1;
        break;
      case BG_LOG_WARNING:
        tag = w->warning_tag;
        if(w->show_warning) do_log = 1;
        break;
      case BG_LOG_ERROR:
        tag = w->error_tag;
        if(w->show_error) do_log = 1;
        w->last_error = bg_strdup(w->last_error, message);
        break;
      case BG_LOG_INFO:
        tag = w->info_tag;
        if(w->show_info) do_log = 1;
        break;
      }

    if(do_log)
      {
      
      gtk_text_buffer_get_end_iter(w->buffer, &iter);

      if(*message == '\0') /* Empty string */
        {
        str = bg_sprintf("[%s]\n", domain);
        gtk_text_buffer_insert_with_tags(w->buffer,
                                         &iter,
                                         str, -1, tag, NULL);
        }
      else
        {
        lines = bg_strbreak(message, '\n');
        i = 0;
        while(lines[i])
          {
          str = bg_sprintf("[%s]: %s\n", domain, lines[i]);
          gtk_text_buffer_insert_with_tags(w->buffer,
                                           &iter,
                                           str, -1, tag, NULL);
          free(str);
          i++;
          }
        bg_strbreak_free(lines);
        }
      w->num_messages++;
      got_message = 1;
      }

    free(message);
    free(domain);

    bg_msg_queue_unlock_read(w->queue);
    }
  if(got_message)
    changed_callback(w);


  return TRUE;
  }

static gboolean configure_callback(GtkWidget * w, GdkEventConfigure *event,
                                   gpointer data)
  {
  bg_gtk_log_window_t * win;

  win = (bg_gtk_log_window_t*)data;
  win->x = event->x;
  win->y = event->y;
  win->width = event->width;
  win->height = event->height;
  gdk_window_get_root_origin(win->window->window, &(win->x), &(win->y));
  return FALSE;
  }


bg_gtk_log_window_t * bg_gtk_log_window_create(void (*close_callback)(bg_gtk_log_window_t*, void*),
                                               void * close_callback_data,
                                               const char * app_name)
  {
  char * tmp_string;
  bg_gtk_log_window_t * ret;
  ret = calloc(1, sizeof(*ret));

  /* Create window */
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);

  tmp_string = bg_sprintf(TR("%s messages"), app_name);
  gtk_window_set_title(GTK_WINDOW(ret->window), tmp_string);
  free(tmp_string);
  
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->window), "configure-event",
                   G_CALLBACK(configure_callback), (gpointer)ret);
  
  /* Create and connect message queue */
  ret->queue = bg_msg_queue_create();
  bg_log_set_dest(ret->queue);
  
  ret->close_callback      = close_callback;
  ret->close_callback_data = close_callback_data;

  /* Create tag table */
  ret->tag_table   = gtk_text_tag_table_new();
  ret->info_tag    = gtk_text_tag_new((char*)0);
  ret->debug_tag   = gtk_text_tag_new((char*)0);
  ret->error_tag   = gtk_text_tag_new((char*)0);
  ret->warning_tag = gtk_text_tag_new((char*)0);

  gtk_text_tag_table_add(ret->tag_table, ret->info_tag);
  gtk_text_tag_table_add(ret->tag_table, ret->debug_tag);
  gtk_text_tag_table_add(ret->tag_table, ret->error_tag);
  gtk_text_tag_table_add(ret->tag_table, ret->warning_tag);

  /* Create textbuffer */

  ret->buffer = gtk_text_buffer_new(ret->tag_table);
  
  /* Create textview */  
  
  ret->textview = gtk_text_view_new_with_buffer(ret->buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(ret->textview), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(ret->textview), FALSE);
  gtk_widget_set_size_request(ret->textview, 300, 100);
  
  gtk_widget_show(ret->textview);

  /* Create scrolledwindow */
  ret->scrolledwindow = gtk_scrolled_window_new((GtkAdjustment*)0, (GtkAdjustment*)0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ret->scrolledwindow),
                                 GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
  
  gtk_container_add(GTK_CONTAINER(ret->scrolledwindow), ret->textview);
  gtk_widget_show(ret->scrolledwindow);
  gtk_container_add(GTK_CONTAINER(ret->window), ret->scrolledwindow);
  
  
  /* Add idle callback */
  g_timeout_add(DELAY_TIME, idle_callback, (gpointer)ret);
  
  return ret;
  }

void bg_gtk_log_window_destroy(bg_gtk_log_window_t * win)
  {
  if(win->last_error)
    free(win->last_error);
  gtk_widget_destroy(win->window);
  free(win);
  }

void
bg_gtk_log_window_show(bg_gtk_log_window_t * w)
  {
  if(!w->width || !w->height)
    gtk_window_set_position(GTK_WINDOW(w->window), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_widget_show(w->window);

  if(w->width && w->height)
    gtk_decorated_window_move_resize_window(GTK_WINDOW(w->window),
                                            w->x, w->y, w->width, w->height);
  w->visible = 1;
  }

void bg_gtk_log_window_hide(bg_gtk_log_window_t * win)
  {
  gtk_widget_hide(win->window);
  win->visible = 0;
  }

/* Configuration stuff */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name =        "max_messages",
      .long_name =   TRS("Number of messages"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 20 },
      .help_string = TRS("Maximum number of messages hold in the window")
    },
    {
      .name =        "show_info",
      .long_name =   TRS("Show info messages"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 }
    },
    {
      .name =        "show_warning",
      .long_name =   TRS("Show warning messages"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 }
    },
    {
      .name =        "show_error",
      .long_name =   TRS("Show error messages"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 }
    },
    {
      .name =        "show_debug",
      .long_name =   TRS("Show debug messages"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 }
    },
    {
      .name =        "info_color",
      .long_name =   TRS("Info foreground"),
      .type =        BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 0.0, 0.0, 0.0 } },
      .help_string = TRS("Color for info messages"),
    },
    {
      .name =        "warning_color",
      .long_name =   TRS("Warning foreground"),
      .type =        BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 1.0, 0.5, 0.0 } },
      .help_string = TRS("Color for warning messages"),
    },
    {
      .name =        "error_color",
      .long_name =   TRS("Error foreground"),
      .type =        BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 1.0, 0.0, 0.0 } },
      .help_string = TRS("Color for error messages"),
    },
    {
      .name =        "debug_color",
      .long_name =   TRS("Debug foreground"),
      .type =        BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 0.0, 0.0, 1.0 } },
      .help_string = TRS("Color for debug messages"),
    },
    {
      .name = "x",
      .long_name = "X",
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 100 }
    },
    {
      .name = "y",
      .long_name = "Y",
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 100 }
    },
    {
      .name = "width",
      .long_name = "Width",
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 0 }
    },
    {
      .name = "height",
      .long_name = "Height",
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .type = BG_PARAMETER_INT,
      .val_default = { .val_i = 0 }
    },
    { /* */ }
  };

const bg_parameter_info_t *
bg_gtk_log_window_get_parameters(bg_gtk_log_window_t * w)
  {
  return parameters;
  }

void bg_gtk_log_window_set_parameter(void * data, const char * name,
                                     const bg_parameter_value_t * v)
  {
  GdkColor color;
  bg_gtk_log_window_t * win;
  if(!name)
    return;

  win = (bg_gtk_log_window_t *)data;

  if(!strcmp(name, "max_messages"))
    {
    win->max_messages = v->val_i;
    while(win->num_messages > win->max_messages)
      delete_first_line(win);
    }
  else if(!strcmp(name, "show_info"))
    win->show_info = v->val_i;
  else if(!strcmp(name, "show_warning"))
    win->show_warning = v->val_i;
  else if(!strcmp(name, "show_error"))
    win->show_error = v->val_i;
  else if(!strcmp(name, "show_debug"))
    win->show_debug = v->val_i;

  else if(!strcmp(name, "info_color"))
    {
    color.red   = (guint16)(v->val_color[0] * 65535.0);
    color.green = (guint16)(v->val_color[1] * 65535.0);
    color.blue  = (guint16)(v->val_color[2] * 65535.0);
    g_object_set(win->info_tag, "foreground-gdk", &color, NULL);
    }
  else if(!strcmp(name, "warning_color"))
    {
    color.red   = (guint16)(v->val_color[0] * 65535.0);
    color.green = (guint16)(v->val_color[1] * 65535.0);
    color.blue  = (guint16)(v->val_color[2] * 65535.0);
    g_object_set(win->warning_tag, "foreground-gdk", &color, NULL);
    }
  else if(!strcmp(name, "error_color"))
    {
    color.red   = (guint16)(v->val_color[0] * 65535.0);
    color.green = (guint16)(v->val_color[1] * 65535.0);
    color.blue  = (guint16)(v->val_color[2] * 65535.0);
    g_object_set(win->error_tag, "foreground-gdk", &color, NULL);
    }
  else if(!strcmp(name, "debug_color"))
    {
    color.red   = (guint16)(v->val_color[0] * 65535.0);
    color.green = (guint16)(v->val_color[1] * 65535.0);
    color.blue  = (guint16)(v->val_color[2] * 65535.0);
    g_object_set(win->debug_tag, "foreground-gdk", &color, NULL);
    }
  if(!strcmp(name, "x"))
    {
    win->x = v->val_i;
    }
  else if(!strcmp(name, "y"))
    {
    win->y = v->val_i;
    }
  else if(!strcmp(name, "width"))
    {
    win->width = v->val_i;
    }
  else if(!strcmp(name, "height"))
    {
    win->height = v->val_i;
    }
  
  }

int bg_gtk_log_window_get_parameter(void * data, const char * name,
                                     bg_parameter_value_t * val)
  {
  bg_gtk_log_window_t * win;
  win = (bg_gtk_log_window_t*)data;
  if(!name)
    return 1;

  if(!strcmp(name, "x"))
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

void bg_gtk_log_window_flush(bg_gtk_log_window_t * win)
  {
  idle_callback(win);
  }

const char * bg_gtk_log_window_last_error(bg_gtk_log_window_t * win)
  {
  return win->last_error;
  }
