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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <config.h>
#include <gmerlin/translation.h>

#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
//#include <msgqueue.h>
#include <gmerlin/player.h>
#include <gmerlin/playermsg.h>
#include <gui_gtk/infowindow.h>
//#include <gui_gtk/textview.h>
#include <gui_gtk/gtkutils.h>

#include <gmerlin/utils.h>

/* This is missing in the gtk headers */

#define FG_SENSITIVE   "#000000"
#define FG_INSENSITIVE "#808080"

extern void
gtk_decorated_window_move_resize_window(GtkWindow*, int, int, int, int);

enum
{
  COLUMN_1,
  COLUMN_2,
  COLUMN_FG_COLOR,
  NUM_COLUMNS
};

#define PATH_NAME             0
#define PATH_INPUT_PLUGIN     1
#define PATH_LOCATION         2
#define PATH_TRACK            3
#define PATH_FORMAT           4
#define PATH_METADATA         5
#define PATH_AUDIO            6
#define PATH_AUDIO_DESC       7
#define PATH_AUDIO_FORMAT_I   8
#define PATH_AUDIO_FORMAT_O   9

#define PATH_VIDEO           10
#define PATH_VIDEO_DESC      11
#define PATH_VIDEO_FORMAT_I  12
#define PATH_VIDEO_FORMAT_O  13

#define PATH_SUBTITLE          14
#define PATH_SUBTITLE_DESC     15
#define PATH_SUBTITLE_FORMAT_I 16
#define PATH_SUBTITLE_FORMAT_O 17

#define PATH_NUM             18

/* Delay between update calls in Milliseconds */

#define DELAY_TIME 50

struct bg_gtk_info_window_s
  {
  int x, y, width, height;

  /* We store everything interesting locally */
  
  gavl_video_format_t subtitle_format;
  

  int num_audio_streams;
  int num_video_streams;
  int num_subtitle_streams;

    
  GtkWidget * window;
  
  GtkWidget * treeview;
  
  bg_msg_queue_t * queue;

  void (*close_callback)(bg_gtk_info_window_t*, void*);
  void * close_callback_data;
  
  GtkTreePath * paths[PATH_NUM];
  int           expanded[PATH_NUM];

  guint expand_id;
  guint collapse_id;

  /* Clipboard */
  
  char * clipboard;
  int clipboard_len;

  struct
    {
    GtkWidget * copy_all;
    GtkWidget * copy_selected;
    GtkWidget * menu;
    } menu;

  guint idle_id;
  
  };

static void reset_tree(bg_gtk_info_window_t * w);

#define EXPANDED_PARAM(n) \
  { \
  .name = n, \
  .long_name = n,\
  .type = BG_PARAMETER_CHECKBUTTON, \
  .flags = BG_PARAMETER_HIDE_DIALOG, \
  }

static const bg_parameter_info_t parameters[] =
  {
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
    EXPANDED_PARAM("exp_0"),
    EXPANDED_PARAM("exp_1"),
    EXPANDED_PARAM("exp_2"),
    EXPANDED_PARAM("exp_3"),
    EXPANDED_PARAM("exp_4"),
    EXPANDED_PARAM("exp_5"),
    EXPANDED_PARAM("exp_6"),
    EXPANDED_PARAM("exp_7"),
    EXPANDED_PARAM("exp_8"),
    EXPANDED_PARAM("exp_9"),
    EXPANDED_PARAM("exp_10"),
    EXPANDED_PARAM("exp_11"),
    EXPANDED_PARAM("exp_12"),
    EXPANDED_PARAM("exp_13"),
    EXPANDED_PARAM("exp_14"),
    EXPANDED_PARAM("exp_15"),
    EXPANDED_PARAM("exp_16"),
    EXPANDED_PARAM("exp_17"),
    { /* End of parameters */ }
  };

const bg_parameter_info_t *
bg_gtk_info_window_get_parameters(bg_gtk_info_window_t * win)
  {
  return parameters;
  }
  
void bg_gtk_info_window_set_parameter(void * data, const char * name,
                                      const bg_parameter_value_t * val)
  {
  int index;
  bg_gtk_info_window_t * win;
  GtkTreeModel * model;
  GtkTreeIter iter;
  
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
  else if(! strncmp(name, "exp_", 4))
    {
    index = atoi(name+4);
    win->expanded[index] = val->val_i;
    if(win->expanded[index] && win->paths[index])
      {
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(win->treeview));
      gtk_tree_model_get_iter(model, &iter, win->paths[index]);
      gtk_tree_view_expand_row(GTK_TREE_VIEW(win->treeview), win->paths[index], 0);
      }
    }
  }

int bg_gtk_info_window_get_parameter(void * data, const char * name,
                                     bg_parameter_value_t * val)
  {
  bg_gtk_info_window_t * win;
  int index;
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
  else if(! strncmp(name, "exp_", 4))
    {
    index = atoi(name+4);
    val->val_i = win->expanded[index];
    return 1;
    }
  return 0;
  }

static char * iter_to_string(bg_gtk_info_window_t * w, char * ret,
                             int depth,
                             GtkTreeIter * iter, int append_children)
  {
  int i;
  int num_children;
  GtkTreeModel * model;
  char * str;
  GtkTreeIter child;
  
  /* */
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));

  if(iter)
    {
    /* Check if the entry is present at all */
    gtk_tree_model_get(model, iter, COLUMN_FG_COLOR, &str, -1);
    
    if(!strcmp(str, FG_INSENSITIVE))
      {
      g_free(str);
      return ret;
      }
    
    g_free(str);

    /*
     *  Ugly, that's right, but the code isn't meant to be run
     *  for each video pixel :)
     */

    for(i = 0; i < depth; i++)
      ret = bg_strcat(ret, "  ");

    /* First column */
    gtk_tree_model_get(model, iter, COLUMN_1, &str, -1);

    if(*str)
      ret = bg_strcat(ret, str);
    else
      {
      g_free(str);
      return ret;
      }

    g_free(str);
    
    /* Second column */
    gtk_tree_model_get(model, iter, COLUMN_2, &str, -1);

    if(*str)
      {
      ret = bg_strcat(ret, "\t");
      ret = bg_strcat(ret, str);
      }
    
    g_free(str);

    ret = bg_strcat(ret, "\n");
    }

  if(!append_children)
    return ret;
  
  num_children = gtk_tree_model_iter_n_children(model, iter);

  if(!num_children)
    return ret;
  
  gtk_tree_model_iter_children(model, &child, iter);
  
  for(i = 0; i < num_children; i++)
    {
    ret = iter_to_string(w, ret, depth + !!(iter),
                         &child, append_children);
    gtk_tree_model_iter_next(model, &child);
    }
  
  return ret;
  
  }

static void set_line(bg_gtk_info_window_t * w,
                     GtkTreeIter * iter,
                     char * line, int sensitive)
  {
  char * pos;
  GtkTreeModel * model;
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));

  pos = strchr(line, '\t');
  if(pos)
    {
    *pos = '\0';
    pos++;
    }

  gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_1,
                     line, -1);
  
  if(pos)
    gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_2,
                       pos, -1);
  else
    gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_2,
                       "", -1);
  
  if(sensitive)
    gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_FG_COLOR,
                       FG_SENSITIVE, -1);
  else
    gtk_tree_store_set(GTK_TREE_STORE(model), iter, COLUMN_FG_COLOR,
                       FG_INSENSITIVE, -1);
  
  }

static
void set_line_index(bg_gtk_info_window_t * w, int iter_index, char * line,
                    int sensitive)
  {
  GtkTreeIter iter;
  GtkTreeModel * model;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  gtk_tree_model_get_iter(model, &iter, w->paths[iter_index]);
  set_line(w, &iter, line, sensitive);
  }

static void remove_children(bg_gtk_info_window_t * w, GtkTreeIter * parent)
  {
  GtkTreeModel * model;
  GtkTreeIter child;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));

  while(gtk_tree_model_iter_children(GTK_TREE_MODEL(model),
                                     &child, parent))
    {
    gtk_tree_store_remove(GTK_TREE_STORE(model), &child);
    }
  
  }

static void set_line_multi(bg_gtk_info_window_t * w,
                           int parent_index,
                           char * line)
  {
  int i;
  GtkTreeIter parent;
  GtkTreeIter iter;
  GtkTreeModel * model;
  char ** tmp_strings;
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  
  gtk_tree_model_get_iter(model, &parent, w->paths[parent_index]);
  
  tmp_strings = bg_strbreak(line, '\n');
  i = 0;
  
  gtk_tree_store_set(GTK_TREE_STORE(model), &parent, COLUMN_FG_COLOR,
                     FG_SENSITIVE, -1);
  remove_children(w, &parent);
  
  while(tmp_strings[i])
    {
    gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &parent);
    set_line(w, &iter, tmp_strings[i], 1);
    i++;
    }
  if(w->expanded[parent_index])
    gtk_tree_view_expand_row(GTK_TREE_VIEW(w->treeview), w->paths[parent_index], 0);

  
  if(tmp_strings)
    bg_strbreak_free(tmp_strings);
  
  }
     
static void set_audio_format(bg_gtk_info_window_t * w,
                             int parent_index,
                             gavl_audio_format_t * format)
  {
  char * tmp_string;
  
  tmp_string = bg_audio_format_to_string(format, 1);
  set_line_multi(w, parent_index, tmp_string);
  free(tmp_string);
  }

static void set_video_format(bg_gtk_info_window_t * w,
                             int parent_index,
                             gavl_video_format_t * format)
  {
  char * tmp_string;
  
  tmp_string = bg_video_format_to_string(format, 1);
  set_line_multi(w, parent_index, tmp_string);
  free(tmp_string);
  }

static gboolean idle_callback(gpointer data)
  {
  int arg_i;
  int arg_i2;
  char * arg_str;

  char * tmp_string;
  
  gavl_audio_format_t arg_af;
  gavl_video_format_t arg_vf;
  bg_metadata_t     arg_m;
  
  bg_gtk_info_window_t * w;
  bg_msg_t * msg;
  
  w = (bg_gtk_info_window_t*)data;

  while((msg = bg_msg_queue_try_lock_read(w->queue)))
    {
    
    switch(bg_msg_get_id(msg))
      {
      case BG_PLAYER_MSG_STATE_CHANGED:
        arg_i = bg_msg_get_arg_int(msg, 0);
        
        switch(arg_i)
          {
          case BG_PLAYER_STATE_STARTING:
            /* New info on the way, clean up everything */
            reset_tree(w);
            break;
          case BG_PLAYER_STATE_PLAYING:
            /* All infos sent, update display */
            //            update_format(w);
            break;
          default:
            break;
          }
        break;
      case BG_PLAYER_MSG_TRACK_NAME:

        arg_str = bg_msg_get_arg_string(msg, 0);
        
        if(arg_str)
          {
          tmp_string = bg_sprintf(TR("Name:\t%s"), arg_str);
          set_line_index(w, PATH_NAME, tmp_string, 1);
          free(tmp_string);
          free(arg_str);
          }
        break;
      case BG_PLAYER_MSG_INPUT:
        arg_str = bg_msg_get_arg_string(msg, 0);
        if(arg_str)
          {
          tmp_string = bg_sprintf(TR("Input plugin:\t%s"), arg_str);
          set_line_index(w, PATH_INPUT_PLUGIN, tmp_string, 1);
          free(tmp_string);
          free(arg_str);
          }

        arg_str = bg_msg_get_arg_string(msg, 1);
        if(arg_str)
          {
          tmp_string = bg_sprintf(TR("Location:\t%s"), arg_str);
          set_line_index(w, PATH_LOCATION, tmp_string, 1);
          free(tmp_string);
          free(arg_str);
          }

        arg_i = bg_msg_get_arg_int(msg, 2);

        tmp_string = bg_sprintf(TR("Track:\t%d"), arg_i+1);
        set_line_index(w, PATH_TRACK, tmp_string, 1);
        free(tmp_string);
        
        break;
      case BG_PLAYER_MSG_TRACK_NUM_STREAMS:
        w->num_audio_streams = bg_msg_get_arg_int(msg, 0);
        w->num_video_streams = bg_msg_get_arg_int(msg, 1);
        w->num_subtitle_streams = bg_msg_get_arg_int(msg, 2);
        break;
      case BG_PLAYER_MSG_TRACK_DURATION:
        break;
      case BG_PLAYER_MSG_STREAM_DESCRIPTION:
        
        arg_str = bg_msg_get_arg_string(msg, 0);
        
        if(arg_str)
          {
          tmp_string = bg_sprintf(TR("Format:\t%s"), arg_str);
          set_line_index(w, PATH_FORMAT, tmp_string, 1);
          free(tmp_string);
          free(arg_str);
          }
        
        break;
      case BG_PLAYER_MSG_AUDIO_DESCRIPTION:
        arg_str = bg_msg_get_arg_string(msg, 0);
        if(arg_str)
          {
          tmp_string = bg_sprintf(TR("Stream type:\t%s"), arg_str);
          set_line_index(w, PATH_AUDIO_DESC, tmp_string, 1);
          free(tmp_string);
          free(arg_str);
          }
        break;
      case BG_PLAYER_MSG_VIDEO_DESCRIPTION:
        arg_str = bg_msg_get_arg_string(msg, 0);
        if(arg_str)
          {
          tmp_string = bg_sprintf(TR("Stream type:\t%s"), arg_str);
          set_line_index(w, PATH_VIDEO_DESC, tmp_string, 1);
          free(tmp_string);
          free(arg_str);
          }
        break;
      case BG_PLAYER_MSG_AUDIO_STREAM:
        arg_i = bg_msg_get_arg_int(msg, 0);
        tmp_string = bg_sprintf(TR("Audio stream %d/%d"),
                                arg_i + 1, w->num_audio_streams);
        set_line_index(w, PATH_AUDIO, tmp_string, 1);
        free(tmp_string);
        
        bg_msg_get_arg_audio_format(msg, 1, &arg_af, NULL);
        set_audio_format(w, PATH_AUDIO_FORMAT_I, &arg_af);

        bg_msg_get_arg_audio_format(msg, 2, &arg_af, NULL);
        set_audio_format(w, PATH_AUDIO_FORMAT_O, &arg_af);
        
        break;
      case BG_PLAYER_MSG_SUBTITLE_STREAM:
        arg_i = bg_msg_get_arg_int(msg, 0);
        arg_i2 = bg_msg_get_arg_int(msg, 1);
        tmp_string = bg_sprintf(TR("Subtitle stream %d/%d [%s]"),
                                arg_i + 1, w->num_subtitle_streams,
                                (arg_i2 ? "Text" : "Overlay"));
        set_line_index(w, PATH_SUBTITLE, tmp_string, 1);
        free(tmp_string);
        
        bg_msg_get_arg_video_format(msg, 2, &arg_vf, NULL);
        set_video_format(w, PATH_SUBTITLE_FORMAT_I, &arg_vf);

        bg_msg_get_arg_video_format(msg, 3, &arg_vf, NULL);
        set_video_format(w, PATH_SUBTITLE_FORMAT_O, &arg_vf);
        
        break;
      case BG_PLAYER_MSG_VIDEO_STREAM:
        arg_i = bg_msg_get_arg_int(msg, 0);
        tmp_string = bg_sprintf(TR("Video stream %d/%d"),
                                arg_i + 1, w->num_video_streams);
        set_line_index(w, PATH_VIDEO, tmp_string, 1);
        free(tmp_string);
        
        bg_msg_get_arg_video_format(msg, 1, &arg_vf, NULL);
        set_video_format(w, PATH_VIDEO_FORMAT_I, &arg_vf);

        bg_msg_get_arg_video_format(msg, 2, &arg_vf, NULL);
        set_video_format(w, PATH_VIDEO_FORMAT_O, &arg_vf);
        
        break;
      case BG_PLAYER_MSG_METADATA:
        memset(&arg_m, 0, sizeof(arg_m));
        bg_msg_get_arg_metadata(msg, 0, &arg_m);
        
        tmp_string = bg_metadata_to_string(&arg_m, 1);
        
        if(tmp_string)
          {
          set_line_multi(w, PATH_METADATA, tmp_string);
          free(tmp_string);
          }
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
  gdk_window_get_root_origin(win->window->window, &win->x, &win->y);
  return FALSE;
  }



#define FREE(str) if(str) free(str);str=(char*)0;


static void reset_tree(bg_gtk_info_window_t * w)
  {
  GtkTreeModel * model;
  GtkTreeIter iter;

  g_signal_handler_block(w->treeview, w->collapse_id);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));
  
  /* Format */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_FORMAT]);
  set_line(w, &iter, TR("Format"), 0);
  
  /* Metadata */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_METADATA]);
  set_line(w, &iter, TR("Metadata"), 0);
  remove_children(w, &iter);
  
  /* Audio */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_AUDIO]);
  set_line(w, &iter, TR("Audio"), 0);

  /* Audio -> desc */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_AUDIO_DESC]);
  set_line(w, &iter, TR("Stream type"), 0);
  
  /* Audio -> format_i */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_AUDIO_FORMAT_I]);
  set_line(w, &iter, TR("Input format"), 0);
  remove_children(w, &iter);
  
  /* Audio -> format_o */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_AUDIO_FORMAT_O]);
  set_line(w, &iter, TR("Output format"), 0);
  remove_children(w, &iter);
    
  /* Video */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_VIDEO]);
  set_line(w, &iter, TR("Video"), 0);

  /* Video -> desc */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_VIDEO_DESC]);
  set_line(w, &iter, TR("Stream type"), 0);

  /* Video -> format_i */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_VIDEO_FORMAT_I]);
  set_line(w, &iter, TR("Input format"), 0);
  remove_children(w, &iter);
  
  /* Video -> format_o */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_VIDEO_FORMAT_O]);
  set_line(w, &iter, TR("Output format"), 0);
  remove_children(w, &iter);
  
  /* Subtitle */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_SUBTITLE]);
  set_line(w, &iter, TR("Subtitles"), 0);

  /* Subtitle -> desc */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_SUBTITLE_DESC]);
  set_line(w, &iter, TR("Stream type"), 0);
  
  /* Subtitle -> format_i */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_SUBTITLE_FORMAT_I]);
  set_line(w, &iter, TR("Input format"), 0);
  remove_children(w, &iter);

  /* Subtitle -> format_o */
  gtk_tree_model_get_iter(model, &iter, w->paths[PATH_SUBTITLE_FORMAT_O]);
  set_line(w, &iter, TR("Output format"), 0);
  remove_children(w, &iter);
  
  g_signal_handler_unblock(w->treeview, w->collapse_id);

  }

static void init_tree(bg_gtk_info_window_t * w)
  {
  GtkTreeIter iter;
  GtkTreeIter child;
  GtkTreeModel * model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w->treeview));

  /* Name */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_NAME] = gtk_tree_model_get_path(model, &iter);
  set_line(w, &iter, TR("Name"), 0);

  /* Plugin */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_INPUT_PLUGIN] = gtk_tree_model_get_path(model, &iter);
  set_line(w, &iter, TR("Input plugin"), 0);
    
  /* Location */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_LOCATION] = gtk_tree_model_get_path(model, &iter);
  set_line(w, &iter, TR("Location"), 0);
  
  /* Track */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_TRACK] = gtk_tree_model_get_path(model, &iter);
  set_line(w, &iter, TR("Track"), 0);
  
  /* Format */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_FORMAT] = gtk_tree_model_get_path(model, &iter);
  
  /* Metadata */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_METADATA] = gtk_tree_model_get_path(model, &iter);
  
  /* Audio */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_AUDIO] = gtk_tree_model_get_path(model, &iter);

  /* Audio -> desc */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_AUDIO_DESC] = gtk_tree_model_get_path(model, &child);
  
  /* Audio -> format_i */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_AUDIO_FORMAT_I] = gtk_tree_model_get_path(model, &child);
  
  /* Audio -> format_o */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_AUDIO_FORMAT_O] = gtk_tree_model_get_path(model, &child);
    
  /* Video */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_VIDEO] = gtk_tree_model_get_path(model, &iter);

  /* Video -> desc */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_VIDEO_DESC] = gtk_tree_model_get_path(model, &child);

  /* Video -> format_i */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_VIDEO_FORMAT_I] = gtk_tree_model_get_path(model, &child);
  
  /* Video -> format_o */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_VIDEO_FORMAT_O] = gtk_tree_model_get_path(model, &child);
  
  
  /* Subtitle */
  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, (GtkTreeIter*)0);
  w->paths[PATH_SUBTITLE] = gtk_tree_model_get_path(model, &iter);

  /* Subtitle -> desc */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_SUBTITLE_DESC] = gtk_tree_model_get_path(model, &child);
  
  /* Subtitle -> format_i */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_SUBTITLE_FORMAT_I] = gtk_tree_model_get_path(model, &child);

  /* Subtitle -> format_o */
  gtk_tree_store_append(GTK_TREE_STORE(model), &child, &iter);
  w->paths[PATH_SUBTITLE_FORMAT_O] = gtk_tree_model_get_path(model, &child);

  reset_tree(w);
  }

static int find_node(bg_gtk_info_window_t * w, GtkTreePath * path)
  {
  int i;
  for(i = 0; i < PATH_NUM; i++)
    {
    if(!gtk_tree_path_compare(path, w->paths[i]))
      return i;
    }
  return -1;
  }

static void row_expanded_callback(GtkTreeView *treeview,
                                  GtkTreeIter *arg1,
                                  GtkTreePath *arg2,
                                  gpointer user_data)
  {
  int i;
  bg_gtk_info_window_t * w;
  
  w = (bg_gtk_info_window_t*)user_data;

  i = find_node(w, arg2);
  if(i < 0)
    return;
  w->expanded[i] = 1;
  }

static void row_collapsed_callback(GtkTreeView *treeview,
                                   GtkTreeIter *arg1,
                                   GtkTreePath *arg2,
                                   gpointer user_data)
  {
  int i;
  bg_gtk_info_window_t * w;
  
  w = (bg_gtk_info_window_t*)user_data;

  i = find_node(w, arg2);
  if(i < 0)
    return;
  w->expanded[i] = 0;
  }

/* Clipboard */

#define TARGET_TEXT_PLAIN 1

static const GtkTargetEntry copy_paste_entries[] =
  {
    { "STRING", 0, TARGET_TEXT_PLAIN },
  };

/* Callback functions for the clipboard */

static void clipboard_get_func(GtkClipboard *clipboard,
                               GtkSelectionData *selection_data,
                               guint info,
                               gpointer data)
  {
  GdkAtom type_atom;
  bg_gtk_info_window_t * w = (bg_gtk_info_window_t*)data;

  type_atom = gdk_atom_intern("STRING", FALSE);
  if(!type_atom)
    return;
  
  gtk_selection_data_set(selection_data, type_atom, 8, (uint8_t*)w->clipboard,
                         w->clipboard_len);
  }

static void clipboard_clear_func(GtkClipboard *clipboard,
                                 gpointer data)
  {
  bg_gtk_info_window_t * w = (bg_gtk_info_window_t*)data;
  if(w->clipboard)
    {
    free(w->clipboard);
    w->clipboard_len = 0;
    w->clipboard = (char*)0;
    }
  }

static void copy_selected(bg_gtk_info_window_t * w)
  {
  GtkTreeIter iter;
  GtkTreeSelection * selection;
  GtkClipboard *clipboard;
  GdkAtom clipboard_atom;
  
  clipboard_atom = gdk_atom_intern ("CLIPBOARD", FALSE);   
  clipboard = gtk_clipboard_get(clipboard_atom);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w->treeview));
  
  gtk_clipboard_set_with_data(clipboard,
                              copy_paste_entries,
                              sizeof(copy_paste_entries)/
                              sizeof(copy_paste_entries[0]),
                              clipboard_get_func,
                              clipboard_clear_func,
                              (gpointer)w);

  
  
  if(w->clipboard)
    free(w->clipboard);

  gtk_tree_selection_get_selected(selection,
                                  (GtkTreeModel**)0,
                                  &iter);
  
  w->clipboard = iter_to_string(w, (char*)0, 0, &iter, 1);

  if(w->clipboard)
    {
    /* Remove trailing '\n' */
    w->clipboard[strlen(w->clipboard)-1] = '\0';
    w->clipboard_len = strlen(w->clipboard) + 1;
    }
  else
    w->clipboard_len = 0;
  }

static void copy_all(bg_gtk_info_window_t * w)
  {
  GtkClipboard *clipboard;
  GdkAtom clipboard_atom;
  clipboard_atom = gdk_atom_intern ("CLIPBOARD", FALSE);   
  clipboard = gtk_clipboard_get(clipboard_atom);
  
  gtk_clipboard_set_with_data(clipboard,
                              copy_paste_entries,
                              sizeof(copy_paste_entries)/
                              sizeof(copy_paste_entries[0]),
                              clipboard_get_func,
                              clipboard_clear_func,
                              (gpointer)w);
  
  if(w->clipboard)
    free(w->clipboard);
  w->clipboard = iter_to_string(w, (char*)0, 0, (GtkTreeIter*)0, 1);

  if(w->clipboard)
    {
    w->clipboard_len = strlen(w->clipboard) + 1;
    }
  else
    w->clipboard_len = 0;
  }

static void menu_callback(GtkWidget * wid, gpointer data)
  {
  bg_gtk_info_window_t * w = (bg_gtk_info_window_t*)data;
  
  /* Add files */
  
  if(wid == w->menu.copy_all)
    copy_all(w);
  else if(wid == w->menu.copy_selected)
    copy_selected(w);
  }

static GtkWidget *
create_item(bg_gtk_info_window_t * w, GtkWidget * parent,
            const char * label, const char * pixmap)
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
  
  g_signal_connect(G_OBJECT(ret), "activate", G_CALLBACK(menu_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }


static void init_menu(bg_gtk_info_window_t * w)
  {
  w->menu.menu = gtk_menu_new();
  w->menu.copy_selected =
    create_item(w, w->menu.menu, TR("Copy selected"), "copy_16.png");
  w->menu.copy_all =
    create_item(w, w->menu.menu, TR("Copy all"), "copy_16.png");
  
  }

static gboolean button_press_callback(GtkWidget * wid, GdkEventButton * evt,
                                      gpointer data)
  {
  bg_gtk_info_window_t * w = (bg_gtk_info_window_t *)data;

  if(evt->button == 3)
    {
    gtk_menu_popup(GTK_MENU(w->menu.menu),
                   (GtkWidget *)0,
                   (GtkWidget *)0,
                   (GtkMenuPositionFunc)0,
                   (gpointer)0,
                   3, evt->time);
    return  FALSE;
    }
  return FALSE;
  }

bg_gtk_info_window_t *
bg_gtk_info_window_create(bg_player_t * player,
                          void (*close_callback)(bg_gtk_info_window_t*, void*),
                          void * close_callback_data)
  {
  GtkTreeStore * store;
  GtkWidget * scrolledwin;
  GtkCellRenderer * text_renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection  *selection;

  bg_gtk_info_window_t * ret;

  ret = calloc(1, sizeof(*ret));

  ret->close_callback      = close_callback;
  ret->close_callback_data = close_callback_data;
  
  /* Create objects */
  
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);


  g_signal_connect(G_OBJECT(ret->window), "configure-event",
                   G_CALLBACK(configure_callback), (gpointer)ret);
  
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Gmerlin Track Info"));
  
  ret->queue = bg_msg_queue_create();

  /* Link message queue to the player */

  bg_player_add_message_queue(player, ret->queue);

  /* Create treeview */
  
  store = gtk_tree_store_new(NUM_COLUMNS, G_TYPE_STRING,
                             G_TYPE_STRING, G_TYPE_STRING);
  
  ret->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ret->treeview), 0);
  
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(ret->treeview), TRUE);

  gtk_widget_set_events(ret->treeview, GDK_BUTTON_PRESS_MASK);
  
  ret->collapse_id =
    g_signal_connect(G_OBJECT(ret->treeview), "row-collapsed",
                     G_CALLBACK(row_collapsed_callback), (gpointer)ret);
  
  ret->expand_id =
    g_signal_connect(G_OBJECT(ret->treeview), "row-expanded",
                     G_CALLBACK(row_expanded_callback), (gpointer)ret);

  g_signal_connect(G_OBJECT(ret->treeview), "button-press-event",
                   G_CALLBACK(button_press_callback), (gpointer)ret);
  
  /* Column 1 */
  text_renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
  gtk_tree_view_column_add_attribute(column, text_renderer, "text", COLUMN_1);
  gtk_tree_view_column_add_attribute(column, text_renderer,
                                     "foreground", COLUMN_FG_COLOR);

  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview), column);

  /* Column 2 */
  text_renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start(column, text_renderer, TRUE);
  gtk_tree_view_column_add_attribute(column, text_renderer, "text", COLUMN_2);
  gtk_tree_view_append_column (GTK_TREE_VIEW(ret->treeview), column);
  gtk_tree_view_column_add_attribute(column, text_renderer,
                                     "foreground", COLUMN_FG_COLOR);
  
  gtk_widget_show(ret->treeview);
  
  /* Selection mode */
  
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ret->treeview));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  
  /* Set callbacks */
  
  ret->idle_id = g_timeout_add(DELAY_TIME, idle_callback, (gpointer)ret);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  /* pack objects */
  
  scrolledwin = gtk_scrolled_window_new((GtkAdjustment*)0, (GtkAdjustment*)0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  gtk_container_add(GTK_CONTAINER(scrolledwin), ret->treeview);
  gtk_widget_show(scrolledwin);

  /* */
  gtk_container_add(GTK_CONTAINER(ret->window), scrolledwin);

  init_tree(ret);
  init_menu(ret);
  
  return ret;
  }

void bg_gtk_info_window_destroy(bg_gtk_info_window_t * w)
  {
  int i;
  bg_msg_queue_destroy(w->queue);
  if(w->clipboard)
    free(w->clipboard);

  g_source_remove(w->idle_id);
  
  for(i = 0; i < PATH_NUM; i++)
    {
    if(w->paths[i])
      gtk_tree_path_free(w->paths[i]);
    }
  gtk_widget_destroy(w->window);
  free(w);
  }


/* Show/hide the window */

void bg_gtk_info_window_show(bg_gtk_info_window_t * w)
  {
  if(!w->width || !w->height)
    {
    gtk_window_set_position(GTK_WINDOW(w->window), GTK_WIN_POS_CENTER);
    }
  gtk_widget_show(w->window);
  if(w->width && w->height)
    gtk_decorated_window_move_resize_window(GTK_WINDOW(w->window),
                                            w->x, w->y, w->width, w->height);
  else
    gtk_window_resize(GTK_WINDOW(w->window), 400, 400);
  }

void bg_gtk_info_window_hide(bg_gtk_info_window_t * w)
  {
  gtk_widget_hide(w->window);
  }
