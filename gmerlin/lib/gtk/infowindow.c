/*****************************************************************
 
  infowindow.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <parameter.h>
#include <streaminfo.h>
//#include <msgqueue.h>
#include <player.h>
#include <playermsg.h>
#include <gui_gtk/infowindow.h>
#include <gui_gtk/textview.h>
#include <gui_gtk/gtkutils.h>

#include <utils.h>

/* This is missing in the gtk headers */

extern void
gtk_decorated_window_move_resize_window(GtkWindow*, int, int, int, int);

/* Delay between update calls in Milliseconds */

#define DELAY_TIME 50

struct bg_gtk_info_window_s
  {
  int x, y, width, height;

  /* We store everything interesting locally */
  gavl_audio_format_t audio_format_i;
  gavl_audio_format_t audio_format_o;

  gavl_video_format_t video_format_i;
  gavl_video_format_t video_format_o;

  gavl_video_format_t subtitle_format;
  
  bg_metadata_t metadata;

  int num_audio_streams;
  int num_video_streams;
  int num_subtitle_streams;

  int current_audio_stream;
  int current_video_stream;
  int current_subtitle_stream;
  
  char * name;
  char * location;
  char * description;
    
  char * audio_description;
  char * video_description;
  char * subtitle_description;

  int subtitle_is_text;
    
  GtkWidget * window;

  /* The text views */

  bg_gtk_textview_t * w_audio_description;
  bg_gtk_textview_t * w_audio_format_i;
  bg_gtk_textview_t * w_audio_format_o;
  
  bg_gtk_textview_t * w_video_description;
  bg_gtk_textview_t * w_video_format_i;
  bg_gtk_textview_t * w_video_format_o;

  bg_gtk_textview_t * w_subtitle_description;
  bg_gtk_textview_t * w_subtitle_format;
  
  bg_gtk_textview_t * w_name;
  bg_gtk_textview_t * w_description;
  bg_gtk_textview_t * w_metadata;
  
  bg_msg_queue_t * queue;

  void (*close_callback)(bg_gtk_info_window_t*, void*);
  void * close_callback_data;
  
  };

static bg_parameter_info_t parameters[] =
  {
    {
      name: "x",
      long_name: "X",
      flags: BG_PARAMETER_HIDE_DIALOG,
      type: BG_PARAMETER_INT,
      val_default: { val_i: 100 }
    },
    {
      name: "y",
      long_name: "Y",
      flags: BG_PARAMETER_HIDE_DIALOG,
      type: BG_PARAMETER_INT,
      val_default: { val_i: 100 }
    },
    {
      name: "width",
      long_name: "Width",
      flags: BG_PARAMETER_HIDE_DIALOG,
      type: BG_PARAMETER_INT,
      val_default: { val_i: 0 }
    },
    {
      name: "height",
      long_name: "Height",
      flags: BG_PARAMETER_HIDE_DIALOG,
      type: BG_PARAMETER_INT,
      val_default: { val_i: 0 }
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t *
bg_gtk_info_window_get_parameters(bg_gtk_info_window_t * win)
  {
  return parameters;
  }
  
void bg_gtk_info_window_set_parameter(void * data, char * name,
                                      bg_parameter_value_t * val)
  {
  bg_gtk_info_window_t * win;
  win = (bg_gtk_info_window_t*)data;
  if(!name)
    return;


  
  if(!strcmp(name, "x"))
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

int bg_gtk_info_window_get_parameter(void * data, char * name,
                                     bg_parameter_value_t * val)
  {
  bg_gtk_info_window_t * win;
  win = (bg_gtk_info_window_t*)data;
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

#define FREE(str) if(str) free(str);str=(char*)0;

static void clear_info(bg_gtk_info_window_t * w)
  {
  memset(&(w->audio_format_i), 0, sizeof(w->audio_format_i));
  memset(&(w->audio_format_o), 0, sizeof(w->audio_format_o));
  memset(&(w->video_format_i), 0, sizeof(w->video_format_i));
  memset(&(w->video_format_o), 0, sizeof(w->video_format_o));

  FREE(w->audio_description);
  FREE(w->video_description);
  FREE(w->subtitle_description);
  FREE(w->description);

  bg_metadata_free(&(w->metadata));
  memset(&(w->metadata), 0, sizeof(w->metadata));

  bg_gtk_textview_update(w->w_audio_format_i, "");
  bg_gtk_textview_update(w->w_audio_format_o, "");
  bg_gtk_textview_update(w->w_video_format_i, "");
  bg_gtk_textview_update(w->w_video_format_o, "");
  bg_gtk_textview_update(w->w_subtitle_format, "");
  bg_gtk_textview_update(w->w_metadata, "");
  
  bg_gtk_textview_update(w->w_description, "");
  bg_gtk_textview_update(w->w_audio_description, "");
  bg_gtk_textview_update(w->w_video_description, "");
  bg_gtk_textview_update(w->w_subtitle_description, "");
  bg_gtk_textview_update(w->w_name, "");
  
  }

static void update_audio(bg_gtk_info_window_t * w)
  {
  char * tmp_string;

  /* Audio formats */

  tmp_string = bg_audio_format_to_string(&(w->audio_format_i), 1);
  bg_gtk_textview_update(w->w_audio_format_i, tmp_string);
  
  free(tmp_string);
  
  tmp_string = bg_audio_format_to_string(&(w->audio_format_o), 1);
  bg_gtk_textview_update(w->w_audio_format_o, tmp_string);
  
  free(tmp_string);

  if(w->audio_description)
    {
    tmp_string = bg_sprintf("Audio stream %d/%d: %s",
                            w->current_audio_stream +1, w->num_audio_streams,
                            w->audio_description);
    }
  else
    {
    tmp_string = bg_sprintf("Audio stream %d/%d",
                            w->current_audio_stream +1, w->num_audio_streams);
    }
  bg_gtk_textview_update(w->w_audio_description, tmp_string);
  free(tmp_string);
  
  }

static void update_video(bg_gtk_info_window_t * w)
  {
  char * tmp_string;
  /* Video formats */

  tmp_string = bg_video_format_to_string(&(w->video_format_i), 1);
  bg_gtk_textview_update(w->w_video_format_i, tmp_string);
  
  free(tmp_string);
  
  tmp_string = bg_video_format_to_string(&(w->video_format_o), 1);
  bg_gtk_textview_update(w->w_video_format_o, tmp_string);
  
  free(tmp_string);

  if(w->video_description)
    {
    tmp_string = bg_sprintf("Video stream %d/%d: %s",
                            w->current_video_stream +1, w->num_video_streams, 
                            w->video_description);
    }
  else
    {
    tmp_string = bg_sprintf("Video stream %d/%d",
                            w->current_video_stream +1, w->num_video_streams);
    }
  bg_gtk_textview_update(w->w_video_description, tmp_string);
  free(tmp_string);
  }

static void update_subtitles(bg_gtk_info_window_t * w)
  {
  char * tmp_string;
  /* Subtitle format */

  tmp_string = bg_video_format_to_string(&(w->subtitle_format), 1);
  bg_gtk_textview_update(w->w_subtitle_format, tmp_string);
  free(tmp_string);
  
  if(w->subtitle_description)
    {
    tmp_string = bg_sprintf("Subtitle stream %d/%d: %s [%s]",
                            w->current_subtitle_stream +1, w->num_subtitle_streams,
                            w->subtitle_description, (w->subtitle_is_text ? "text" : "overlay"));
    }
  else
    {
    tmp_string = bg_sprintf("Subtitle stream %d/%d [%s]",
                            w->current_subtitle_stream +1, w->num_subtitle_streams,
                            (w->subtitle_is_text ? "text" : "overlay"));
    }
  bg_gtk_textview_update(w->w_subtitle_description, tmp_string);
  free(tmp_string);
  }


static void update_stream(bg_gtk_info_window_t * w)
  {
  if(w->description)
    bg_gtk_textview_update(w->w_description, w->description);
  }

static void update_metadata(bg_gtk_info_window_t * w)
  {
  char * window_string = (char*)0;

  bg_gtk_textview_update(w->w_metadata, "");

  window_string = bg_metadata_to_string(&w->metadata, 1);
  
  if(window_string)
    {
    /* Delete trailing newline */
    
    bg_gtk_textview_update(w->w_metadata, window_string);
    free(window_string);
    }
  }

static gboolean idle_callback(gpointer data)
  {
  int arg_i;
  char * arg_str;
  
  bg_gtk_info_window_t * w;
  bg_msg_t * msg;
  
  w = (bg_gtk_info_window_t*)data;

  while((msg = bg_msg_queue_try_lock_read(w->queue)))
    {
    
    switch(bg_msg_get_id(msg))
      {
      case BG_PLAYER_MSG_TIME_CHANGED:
        break;
      case BG_PLAYER_MSG_TRACK_CHANGED:
        break;
      case BG_PLAYER_MSG_STATE_CHANGED:
        arg_i = bg_msg_get_arg_int(msg, 0);
        
        switch(arg_i)
          {
          case BG_PLAYER_STATE_STARTING:
            /* New info on the way, clean up everything */
            clear_info(w);
            break;
          case BG_PLAYER_STATE_PLAYING:
            /* All infos sent, update display */
            update_stream(w);
            break;
          default:
            break;
          }
        break;
      case BG_PLAYER_MSG_TRACK_NAME:
        arg_str = bg_msg_get_arg_string(msg, 0);
        bg_gtk_textview_update(w->w_name, arg_str);
        free(arg_str);
        break;
      case BG_PLAYER_MSG_TRACK_NUM_STREAMS:
        w->num_audio_streams = bg_msg_get_arg_int(msg, 0);
        w->num_video_streams = bg_msg_get_arg_int(msg, 1);
        w->num_subtitle_streams = bg_msg_get_arg_int(msg, 2);
        break;
      case BG_PLAYER_MSG_TRACK_DURATION:
        break;
      case BG_PLAYER_MSG_STREAM_DESCRIPTION:
        w->description = bg_msg_get_arg_string(msg, 0);
        break;
      case BG_PLAYER_MSG_AUDIO_DESCRIPTION:
        w->audio_description = bg_msg_get_arg_string(msg, 0);
        break;
      case BG_PLAYER_MSG_VIDEO_DESCRIPTION:
      case BG_PLAYER_MSG_STILL_DESCRIPTION:
        w->video_description = bg_msg_get_arg_string(msg, 0);
        break;
      case BG_PLAYER_MSG_AUDIO_STREAM:
        w->current_audio_stream = bg_msg_get_arg_int(msg, 0);
        bg_msg_get_arg_audio_format(msg, 1, &(w->audio_format_i));
        bg_msg_get_arg_audio_format(msg, 2, &(w->audio_format_o));
        update_audio(w);
        break;
      case BG_PLAYER_MSG_SUBTITLE_STREAM:
        w->current_subtitle_stream = bg_msg_get_arg_int(msg, 0);
        w->subtitle_is_text = bg_msg_get_arg_int(msg, 1);
        bg_msg_get_arg_video_format(msg, 2, &(w->subtitle_format));
        update_subtitles(w);
        break;
      case BG_PLAYER_MSG_VIDEO_STREAM:
      case BG_PLAYER_MSG_STILL_STREAM:
        w->current_video_stream = bg_msg_get_arg_int(msg, 0);
        bg_msg_get_arg_video_format(msg, 1, &(w->video_format_i));
        bg_msg_get_arg_video_format(msg, 2, &(w->video_format_o));
        update_video(w);
        break;
      case BG_PLAYER_MSG_METADATA:
        bg_metadata_free(&(w->metadata));
        bg_msg_get_arg_metadata(msg, 0, &(w->metadata));
        update_metadata(w);
        break;
      }
    bg_msg_queue_unlock_read(w->queue);
    }
  return TRUE;
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  bg_gtk_info_window_t * win;
  win = (bg_gtk_info_window_t*)data;
  if(win->close_callback)
    win->close_callback(win, win->close_callback_data);
  gtk_widget_hide(win->window);
  return TRUE;
  }

static gboolean configure_callback(GtkWidget * w, GdkEventConfigure *event,
                                   gpointer data)
  {
  bg_gtk_info_window_t * win;

  win = (bg_gtk_info_window_t*)data;
  win->x = event->x;
  win->y = event->y;
  win->width = event->width;
  win->height = event->height;
  gdk_window_get_root_origin(win->window->window, &(win->x), &(win->y));
  return FALSE;
  }

static GtkWidget * create_frame(const char * label)
  {
  GtkWidget * ret = gtk_frame_new(label);
  gtk_container_set_border_width(GTK_CONTAINER(ret), 3);
  return ret;
  }

bg_gtk_info_window_t *
bg_gtk_info_window_create(bg_player_t * player,
                          void (*close_callback)(bg_gtk_info_window_t*, void*),
                          void * close_callback_data)
  {
  GtkWidget * notebook;
  GtkWidget * frame;
  GtkWidget * table;
  GtkWidget * tab_label;
  GtkWidget * scrolledwin;
  
  bg_gtk_info_window_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->close_callback      = close_callback;
  ret->close_callback_data = close_callback_data;
  
  
  /* Create objects */
  
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);


  g_signal_connect(G_OBJECT(ret->window), "configure-event",
                   G_CALLBACK(configure_callback), (gpointer)ret);
  
  gtk_window_set_title(GTK_WINDOW(ret->window), "Gmerlin Track Info");
  
  ret->queue = bg_msg_queue_create();

  /* Link message queue to the player */

  bg_player_add_message_queue(player, ret->queue);
  
  ret->w_audio_description = bg_gtk_textview_create();
  ret->w_video_description = bg_gtk_textview_create();
  ret->w_subtitle_description = bg_gtk_textview_create();

  ret->w_audio_format_i = bg_gtk_textview_create();
  ret->w_audio_format_o = bg_gtk_textview_create();
  
  ret->w_video_format_i = bg_gtk_textview_create();
  ret->w_video_format_o = bg_gtk_textview_create();

  ret->w_subtitle_format = bg_gtk_textview_create();
  
  ret->w_name        = bg_gtk_textview_create();
  ret->w_description = bg_gtk_textview_create();
  ret->w_metadata    = bg_gtk_textview_create();
  
  /* Set callbacks */
  
  g_timeout_add(DELAY_TIME, idle_callback, (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  /* pack objects */

  notebook = gtk_notebook_new();

  /* Overall info */
    
  table = gtk_table_new(3, 1, 0);
  
  frame = create_frame("Name");

  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_name));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 2, 0, 1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

  frame = create_frame("Stream type");
  gtk_container_add(GTK_CONTAINER(frame),
                    bg_gtk_textview_get_widget(ret->w_description));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 2, 1, 2,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
  
  frame = create_frame("Meta info");

  scrolledwin = gtk_scrolled_window_new((GtkAdjustment*)0, (GtkAdjustment*)0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  gtk_container_add(GTK_CONTAINER(scrolledwin),
                    bg_gtk_textview_get_widget(ret->w_metadata));
  gtk_widget_show(scrolledwin);
  
  gtk_container_add(GTK_CONTAINER(frame), scrolledwin);
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 2, 3);

  gtk_widget_show(table);
  
  tab_label = gtk_label_new("Track");
  gtk_widget_show(tab_label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, tab_label);

  gtk_widget_show(notebook);

  /* Audio stream */

  table = gtk_table_new(2, 2, 0);

  frame = create_frame("Stream type");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_audio_description));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 2, 0, 1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

  frame = create_frame("Input format");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_audio_format_i));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 1, 1, 2);

  frame = create_frame("Output format");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_audio_format_o));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 1, 2, 1, 2);
  
  gtk_widget_show(table);
  
  tab_label = gtk_label_new("Audio");
  gtk_widget_show(tab_label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, tab_label);

  gtk_widget_show(notebook);

  /* Video stream */

  table = gtk_table_new(2, 2, 0);

  frame = create_frame("Stream type");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_video_description));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 2, 0, 1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

  frame = create_frame("Input format");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_video_format_i));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 1, 1, 2);

  frame = create_frame("Output format");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_video_format_o));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 1, 2, 1, 2);
  
  gtk_widget_show(table);
  
  tab_label = gtk_label_new("Video");
  gtk_widget_show(tab_label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, tab_label);


  /* Subtitle stream */

  table = gtk_table_new(2, 2, 0);
  
  frame = create_frame("Stream type");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_subtitle_description));
  gtk_widget_show(frame);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 2, 0, 1,
                   GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);

  frame = create_frame("Format format");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_subtitle_format));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 1, 2);

  gtk_widget_show(table);
  
  tab_label = gtk_label_new("Subtitles");
  gtk_widget_show(tab_label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, tab_label);
  
  gtk_widget_show(notebook);
  gtk_container_add(GTK_CONTAINER(ret->window), notebook);
  
  return ret;
  }

void bg_gtk_info_window_destroy(bg_gtk_info_window_t * w)
  {
  FREE(w->name);
  FREE(w->location);
  FREE(w->description);
  FREE(w->audio_description);
  FREE(w->video_description);

  bg_gtk_textview_destroy(w->w_audio_format_i);
  bg_gtk_textview_destroy(w->w_audio_format_o);
  bg_gtk_textview_destroy(w->w_video_format_i);
  bg_gtk_textview_destroy(w->w_video_format_o);
  bg_gtk_textview_destroy(w->w_metadata);
  
  bg_gtk_textview_destroy(w->w_description);
  bg_gtk_textview_destroy(w->w_audio_description);
  bg_gtk_textview_destroy(w->w_video_description);
  bg_gtk_textview_destroy(w->w_name);

    
  bg_msg_queue_destroy(w->queue);
  
  free(w);
  }


/* Show/hide the window */

void bg_gtk_info_window_show(bg_gtk_info_window_t * w)
  {
  if(!w->width || !w->height)
    gtk_window_set_position(GTK_WINDOW(w->window), GTK_WIN_POS_CENTER);
  
  gtk_widget_show(w->window);
  if(w->width && w->height)
    gtk_decorated_window_move_resize_window(GTK_WINDOW(w->window),
                                            w->x, w->y, w->width, w->height);
  }

void bg_gtk_info_window_hide(bg_gtk_info_window_t * w)
  {
  gtk_widget_hide(w->window);
  }
