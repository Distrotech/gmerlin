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

#include <streaminfo.h>
//#include <msgqueue.h>
#include <player.h>
#include <playermsg.h>
#include <gui_gtk/infowindow.h>
#include <gui_gtk/textview.h>
#include <utils.h>

/* Delay between update calls in Milliseconds */

#define DELAY_TIME 10

struct bg_gtk_info_window_s
  {
  /* We store everything interesting locally */
  gavl_audio_format_t audio_format_i;
  gavl_audio_format_t audio_format_o;

  gavl_video_format_t video_format_i;
  gavl_video_format_t video_format_o;

  bg_metadata_t metadata;

  int num_audio_streams;
  int num_video_streams;
  int num_subpicture_streams;
  int num_programs;

  int current_audio_stream;
  int current_video_stream;
  int current_subpicture_stream;
  int current_program;
  
  char * name;
  char * location;
  char * description;
    
  char * audio_description;
  char * video_description;

  int do_audio_i;
  int do_video_i;

  int do_audio_o;
  int do_video_o;

  
  GtkWidget * window;

  /* The text views */

  bg_gtk_textview_t * w_audio_description;
  bg_gtk_textview_t * w_audio_format_i;
  bg_gtk_textview_t * w_audio_format_o;
  
  bg_gtk_textview_t * w_video_description;
  bg_gtk_textview_t * w_video_format_i;
  bg_gtk_textview_t * w_video_format_o;

  bg_gtk_textview_t * w_audio_stream;
  bg_gtk_textview_t * w_video_stream;

  bg_gtk_textview_t * w_name;
  bg_gtk_textview_t * w_description;
  bg_gtk_textview_t * w_metadata;
  
  bg_msg_queue_t * queue;

  void (*close_callback)(bg_gtk_info_window_t*, void*);
  void * close_callback_data;
  
  };

#define STRINGSET(index, str) \
arg_str = mg_msg_get_arg_string(msg, index); \
str = arg_str;

#define FREE(str) if(str) free(str);str=(char*)0;

static void clear_info(bg_gtk_info_window_t * w)
  {
  memset(&(w->audio_format_i), 0, sizeof(w->audio_format_i));
  memset(&(w->audio_format_o), 0, sizeof(w->audio_format_o));
  memset(&(w->video_format_i), 0, sizeof(w->video_format_i));
  memset(&(w->video_format_o), 0, sizeof(w->video_format_o));

  FREE(w->audio_description);
  FREE(w->video_description);
  FREE(w->description);

  FREE(w->metadata.artist);
  FREE(w->metadata.title);
  FREE(w->metadata.album);
  FREE(w->metadata.genre);
  FREE(w->metadata.comment);
  
  memset(&(w->metadata), 0, sizeof(w->metadata));

  bg_gtk_textview_update(w->w_audio_format_i, "");
  bg_gtk_textview_update(w->w_audio_format_o, "");
  bg_gtk_textview_update(w->w_video_format_i, "");
  bg_gtk_textview_update(w->w_video_format_o, "");
  bg_gtk_textview_update(w->w_metadata, "");
  
  bg_gtk_textview_update(w->w_description, "");
  bg_gtk_textview_update(w->w_audio_description, "");
  bg_gtk_textview_update(w->w_video_description, "");
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
    tmp_string = bg_sprintf("Audio stream %d: %s",
                            w->current_audio_stream +1,
                            w->audio_description);
    }
  else
    {
    tmp_string = bg_sprintf("Audio stream %d",
                            w->current_audio_stream +1);
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
    tmp_string = bg_sprintf("Video stream %d: %s",
                            w->current_video_stream +1,
                            w->video_description);
    }
  else
    {
    tmp_string = bg_sprintf("Video stream %d",
                            w->current_video_stream +1);
    }
  bg_gtk_textview_update(w->w_video_description, tmp_string);
  free(tmp_string);
  }

static void update_stream(bg_gtk_info_window_t * w)
  {
  if(w->description)
    bg_gtk_textview_update(w->w_description, w->description);
  }

#define META_STRCAT() \
    if(first)\
      { \
      /* Set window string to first line */\
      window_string = tmp_string_1;\
      first = 0; \
      } \
    else\
      {\
      /* Append line to window string */\
      tmp_string_2 = bg_sprintf("%s%s", window_string, tmp_string_1);\
      free(window_string);\
      free(tmp_string_1);\
      window_string = tmp_string_2;\
      }

static void update_metadata(bg_gtk_info_window_t * w)
  {
  char * tmp_string_1;
  char * tmp_string_2;
  char * window_string = (char*)0;

  int first = 1;
  bg_gtk_textview_update(w->w_metadata, "");
  if(w->metadata.author)
    {
    tmp_string_1 = bg_sprintf("Author:\t %s\n", w->metadata.author);
    META_STRCAT();
    }
  if(w->metadata.artist)
    {
    tmp_string_1 = bg_sprintf("Artist:\t %s\n", w->metadata.artist);
    META_STRCAT();
    }
  if(w->metadata.title)
    {
    tmp_string_1 = bg_sprintf("Title:\t %s\n", w->metadata.title);
    META_STRCAT();
    }
  if(w->metadata.album)
    {
    tmp_string_1 = bg_sprintf("Album:\t %s\n", w->metadata.album);
    META_STRCAT();
    }
  if(w->metadata.copyright)
    {
    tmp_string_1 = bg_sprintf("Copyright:\t %s\n", w->metadata.copyright);
    META_STRCAT();
    }
  if(w->metadata.genre)
    {
    tmp_string_1 = bg_sprintf("Genre:\t %s\n", w->metadata.genre);
    META_STRCAT();
    }
  if(w->metadata.comment)
    {
    tmp_string_1 = bg_sprintf("Comment:\t %s\n", w->metadata.comment);
    META_STRCAT();
    }
  if(w->metadata.date)
    {
    tmp_string_1 = bg_sprintf("Date:\t %s\n", w->metadata.date);
    META_STRCAT();
    }
  if(w->metadata.track)
    {
    tmp_string_1 = bg_sprintf("Track:\t %d\n", w->metadata.track);
    META_STRCAT();
    }
  if(window_string)
    {
    /* Delete trailing newline */

    window_string[strlen(window_string) - 1] = '\0';
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
            //            fprintf(stderr, "Cleaning info\n");
            clear_info(w);
            break;
          case BG_PLAYER_STATE_PLAYING:
            /* All infos sent, update display */
            update_stream(w);
            break;
          case BG_PLAYER_STATE_CHANGING:
            /* Current track is over, let's clear all data */
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
        w->video_description = bg_msg_get_arg_string(msg, 0);
        break;
      case BG_PLAYER_MSG_AUDIO_STREAM:
        w->current_audio_stream = bg_msg_get_arg_int(msg, 0);
        bg_msg_get_arg_audio_format(msg, 1, &(w->audio_format_i));
        bg_msg_get_arg_audio_format(msg, 2, &(w->audio_format_o));
        update_audio(w);
        break;
      case BG_PLAYER_MSG_VIDEO_STREAM:
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

bg_gtk_info_window_t *
bg_gtk_info_window_create(bg_player_t * player,
                          void (*close_callback)(bg_gtk_info_window_t*, void*),
                          void * close_callback_data)
  {
  GtkWidget * notebook;
  GtkWidget * frame;
  GtkWidget * table;
  GtkWidget * tab_label;
  
  bg_gtk_info_window_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->close_callback      = close_callback;
  ret->close_callback_data = close_callback_data;
  
  
  /* Create objects */
  
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(ret->window), "Gmerlin Track Info");
  
  ret->queue = bg_msg_queue_create();

  /* Link message queue to the player */

  bg_player_add_message_queue(player, ret->queue);
  
  ret->w_audio_description = bg_gtk_textview_create();
  ret->w_video_description = bg_gtk_textview_create();

  ret->w_audio_format_i = bg_gtk_textview_create();
  ret->w_audio_format_o = bg_gtk_textview_create();
  
  ret->w_video_format_i = bg_gtk_textview_create();
  ret->w_video_format_o = bg_gtk_textview_create();

  ret->w_audio_stream = bg_gtk_textview_create();
  ret->w_video_stream = bg_gtk_textview_create();

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
  
  frame = gtk_frame_new("Name");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_name));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 0, 1);

  frame = gtk_frame_new("Stream type");
  gtk_container_add(GTK_CONTAINER(frame),
                    bg_gtk_textview_get_widget(ret->w_description));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 1, 2);

  frame = gtk_frame_new("Meta info");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_metadata));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 2, 3);

  gtk_widget_show(table);
  
  tab_label = gtk_label_new("Track");
  gtk_widget_show(tab_label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, tab_label);

  gtk_widget_show(notebook);

  /* Audio stream */

  table = gtk_table_new(2, 2, 0);

  frame = gtk_frame_new("Stream type");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_audio_description));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 0, 1);

  frame = gtk_frame_new("Input format");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_audio_format_i));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 1, 1, 2);

  frame = gtk_frame_new("Output format");
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

  frame = gtk_frame_new("Stream type");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_video_description));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 0, 1);

  frame = gtk_frame_new("Input format");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_video_format_i));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 1, 1, 2);

  frame = gtk_frame_new("Output format");
  gtk_container_add(GTK_CONTAINER(frame), bg_gtk_textview_get_widget(ret->w_video_format_o));
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 1, 2, 1, 2);
  
  gtk_widget_show(table);
  
  tab_label = gtk_label_new("Video");
  gtk_widget_show(tab_label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, tab_label);

  gtk_widget_show(notebook);
  gtk_container_add(GTK_CONTAINER(ret->window), notebook);
  
  return ret;
  }

void bg_gtk_info_window_destroy(bg_gtk_info_window_t * w)
  {
  gtk_widget_destroy(w->window);
  bg_msg_queue_destroy(w->queue);
  }

/*
 *  Return the queue. It must then be passed to the
 *  player with bg_player_add_message_queue()
 */

bg_msg_queue_t *
bg_gtk_info_window_get_queue(bg_gtk_info_window_t * w)
  {
  return w->queue;
  }

/* Show/hide the window */

void bg_gtk_info_window_show(bg_gtk_info_window_t * w)
  {
  gtk_widget_show(w->window);
  }

void bg_gtk_info_window_hide(bg_gtk_info_window_t * w)
  {
  gtk_widget_hide(w->window);
  }
