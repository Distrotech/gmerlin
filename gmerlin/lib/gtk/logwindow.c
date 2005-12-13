/*****************************************************************
 
  logwindow.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <log.h>
#include <utils.h>

#include <gui_gtk/logwindow.h>

/* Delay between update calls in Milliseconds */
#define DELAY_TIME 50

struct bg_gtk_log_window_s
  {
  GtkWidget * window;
  GtkWidget * textview;
  GtkTextBuffer * buffer;

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
  
  w = (bg_gtk_log_window_t *)data;
  
  while((msg = bg_msg_queue_try_lock_read(w->queue)))
    {
    while(w->num_messages > w->max_messages - 1)
      delete_first_line(w);
    
    level = bg_msg_get_id(msg);
    level_name = bg_log_level_to_string(level);
    domain = bg_msg_get_arg_string(msg, 0);
    message = bg_msg_get_arg_string(msg, 1);
    
    //    fprintf(stderr, "LOG [%s] %s: %s\n", domain, level_name, message);

    switch(level)
      {
      case BG_LOG_DEBUG:
        tag = w->debug_tag;
        break;
      case BG_LOG_WARNING:
        tag = w->warning_tag;
        break;
      case BG_LOG_ERROR:
        tag = w->error_tag;
        break;
      case BG_LOG_INFO:
        tag = w->info_tag;
        break;
      }

    gtk_text_buffer_get_end_iter(w->buffer, &iter);
    str = bg_sprintf("[%s]: %s\n", domain, message);

    gtk_text_buffer_insert_with_tags(w->buffer,
                                     &iter,
                                     str, -1, tag, NULL);
    
    free(str);
    free(message);
    free(domain);
    
    
    bg_msg_queue_unlock_read(w->queue);
    w->num_messages++;
    
    }
  
  return TRUE;
  }

bg_gtk_log_window_t * bg_gtk_log_window_create(void (*close_callback)(bg_gtk_log_window_t*, void*),
                                               void * close_callback_data)
  {
  GtkWidget * scrolledwin;
  bg_gtk_log_window_t * ret;
  ret = calloc(1, sizeof(*ret));

  /* Create window */
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), "Log viewer");

  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  /* Create and connect message queue */
  ret->queue = bg_msg_queue_create();
  bg_set_log_dest(ret->queue);
  
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
  scrolledwin = gtk_scrolled_window_new((GtkAdjustment*)0, (GtkAdjustment*)0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
                                 GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
  
  gtk_container_add(GTK_CONTAINER(scrolledwin), ret->textview);
  gtk_widget_show(scrolledwin);
  gtk_container_add(GTK_CONTAINER(ret->window), scrolledwin);
  
  
  /* Add idle callback */
  g_timeout_add(DELAY_TIME, idle_callback, (gpointer)ret);
  
  return ret;
  }

void bg_gtk_log_window_destroy(bg_gtk_log_window_t * win)
  {
  gtk_widget_destroy(win->window);
  free(win);
  }

void
bg_gtk_log_window_show(bg_gtk_log_window_t * win)
  {
  gtk_widget_show(win->window);
  win->visible = 1;
  }

void bg_gtk_log_window_hide(bg_gtk_log_window_t * win)
  {
  gtk_widget_hide(win->window);
  win->visible = 0;
  }

/* Configuration stuff */

static bg_parameter_info_t parameters[] =
  {
    {
      name:        "max_messages",
      long_name:   "Number of messages",
      type:        BG_PARAMETER_INT,
      val_default: { val_i: 20 },
      help_string: "Maximum number of messages hold in the window"
    },
    {
      name:        "show_info",
      long_name:   "Show info massages",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "show_warning",
      long_name:   "Show warning massages",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "show_error",
      long_name:   "Show error massages",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 }
    },
    {
      name:        "show_debug",
      long_name:   "Show debug massages",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 }
    },
    {
      name:        "info_color",
      long_name:   "Info foreground",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 0.0, 0.0, 0.0 } },
      help_string: "Color for info messages",
    },
    {
      name:        "warning_color",
      long_name:   "Warning foreground",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 1.0, 0.5, 0.0 } },
      help_string: "Color for warning messages",
    },
    {
      name:        "error_color",
      long_name:   "Error foreground",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 1.0, 0.0, 0.0 } },
      help_string: "Color for error messages",
    },
    {
      name:        "debug_color",
      long_name:   "Debug foreground",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 0.0, 0.0, 1.0 } },
      help_string: "Color for debug messages",
    },
    { /* */ }
  };

bg_parameter_info_t * bg_gtk_log_window_get_parameters(bg_gtk_log_window_t * w)
  {
  return parameters;
  }

void bg_gtk_log_window_set_parameter(void * data, char * name,
                                     bg_parameter_value_t * v)
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

  
  }
