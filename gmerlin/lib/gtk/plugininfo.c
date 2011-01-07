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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>

#include <gmerlin/utils.h>
#include <gmerlin/pluginregistry.h>
#include <gui_gtk/plugin.h>
#include <gui_gtk/textview.h>
#include <gui_gtk/gtkutils.h>

typedef struct 
  {
  GtkWidget * window;
  GtkWidget * close_button;
  bg_gtk_textview_t * textview1;
  bg_gtk_textview_t * textview2;
  }pluginwindow_t;

static void button_callback(GtkWidget * w, gpointer data)
  {
  pluginwindow_t * win;
  win = (pluginwindow_t*)data;
  bg_gtk_textview_destroy(win->textview1);
  bg_gtk_textview_destroy(win->textview2);
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

static pluginwindow_t *
pluginwindow_create(const char * title, const char * properties, const char * description)
  {
  GtkWidget * table;
  GtkWidget * frame;

  pluginwindow_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint(GTK_WINDOW(ret->window),
                           GDK_WINDOW_TYPE_HINT_DIALOG);
  
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER_ON_PARENT);
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  gtk_window_set_title(GTK_WINDOW(ret->window), title);

  /* Create close button */

  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  bg_gtk_widget_set_can_default(ret->close_button, TRUE);

  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(button_callback), (gpointer)ret);
  gtk_widget_show(ret->close_button);
  
  /* Create texts */
  
  ret->textview1 = bg_gtk_textview_create();
  bg_gtk_textview_update(ret->textview1, properties);
  
  ret->textview2 = bg_gtk_textview_create();
  bg_gtk_textview_update(ret->textview2, description);

  table = gtk_table_new(3, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);


  frame = gtk_frame_new("Properties");
  gtk_container_add(GTK_CONTAINER(frame),
                    bg_gtk_textview_get_widget(ret->textview1));
  gtk_widget_show(frame);
  
  gtk_table_attach_defaults(GTK_TABLE(table),
                            frame, 0, 1, 0, 1);

  frame = gtk_frame_new("Description");
  gtk_container_add(GTK_CONTAINER(frame),
                    bg_gtk_textview_get_widget(ret->textview2));
  gtk_widget_show(frame);
  
  gtk_table_attach_defaults(GTK_TABLE(table),
                            frame, 0, 1, 1, 2);
  

  gtk_table_attach(GTK_TABLE(table), ret->close_button, 0, 1, 2, 3,
                   GTK_SHRINK, GTK_SHRINK, 0, 0);
  
  
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(ret->window), table);
    
  return ret;
  }

static void pluginwindow_show(pluginwindow_t * w, int modal,
                              GtkWidget * parent)
  {
  parent = bg_gtk_get_toplevel(parent);
  if(parent)
    gtk_window_set_transient_for(GTK_WINDOW(w->window),
                                 GTK_WINDOW(parent));
  
  gtk_window_set_modal(GTK_WINDOW(w->window), modal);

  gtk_widget_grab_default(w->close_button);
  gtk_widget_show(w->window);
  }





static const struct
  {
  char * name;
  bg_plugin_type_t type;
  }
type_names[] =
  {
    { TRS("Input"),          BG_PLUGIN_INPUT },
    { TRS("Audio output"),   BG_PLUGIN_OUTPUT_AUDIO },
    { TRS("Video output"),   BG_PLUGIN_OUTPUT_VIDEO },
    { TRS("Audio recorder"), BG_PLUGIN_RECORDER_AUDIO },
    { TRS("Video recorder"), BG_PLUGIN_RECORDER_VIDEO },
    { TRS("Audio encoder"),  BG_PLUGIN_ENCODER_AUDIO },
    { TRS("Video encoder"),  BG_PLUGIN_ENCODER_VIDEO },
    { TRS("Text subtitle exporter"),  BG_PLUGIN_ENCODER_SUBTITLE_TEXT },
    { TRS("Overlay subtitle exporter"),  BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY },
    { TRS("Audio/Video encoder"),  BG_PLUGIN_ENCODER },
    { TRS("Image reader"),   BG_PLUGIN_IMAGE_READER  },
    { TRS("Image writer"),   BG_PLUGIN_IMAGE_WRITER  },
    { TRS("Encoding postprocessor"),   BG_PLUGIN_ENCODER_PP  },
    { TRS("Audio filter"),   BG_PLUGIN_FILTER_AUDIO  },
    { TRS("Video filter"),   BG_PLUGIN_FILTER_VIDEO  },
    { TRS("Visualization"),   BG_PLUGIN_VISUALIZATION  },
    { (char*)0,         BG_PLUGIN_NONE }
  };

static const struct
  {
  char * name;
  uint32_t flag;
  }
flag_names[] =
  {
    { TRS("Removable Device"), BG_PLUGIN_REMOVABLE }, /* Removable media (CD, DVD etc.) */
    { TRS("Recorder"),    BG_PLUGIN_RECORDER       }, /* Plugin can record              */
    { TRS("File"),        BG_PLUGIN_FILE           }, /* Plugin reads/writes files      */
    { TRS("URL"),         BG_PLUGIN_URL            }, /* Plugin reads URLs or streams   */
    { TRS("Playback"),    BG_PLUGIN_PLAYBACK       }, /* Output plugins for playback    */
    { TRS("Tuner"),       BG_PLUGIN_TUNER           }, /* Plugin has tuner */
    { TRS("Filter with 1 input"),  BG_PLUGIN_FILTER_1 },
    { TRS("Renders via gmerlin"),  BG_PLUGIN_VISUALIZE_FRAME },
    { TRS("Renders via OpenGL"),  BG_PLUGIN_VISUALIZE_GL },
    { (char*)0,      0                        },
  };

static char * get_flag_string(uint32_t flags)
  {
  char * ret;
  int i, j, index, num_flags;
  uint32_t flag;

  ret = malloc(1024);
  *ret = '\0';
  
  /* Count the flags */
    
  num_flags = 0;
  for(i = 0; i < 32; i++)
    {
    flag = (1<<i);
    if(flags & flag)
      num_flags++;
    }

  /* Create the string */
  
  index = 0;
  
  for(i = 0; i < 32; i++)
    {
    flag = (1<<i);
    if(flags & flag)
      {
      j = 0;
      while(flag_names[j].name)
        {
        if(flag_names[j].flag == flag)
          {
          strcat(ret, TR(flag_names[j].name));
          if(index < num_flags - 1)
            strcat(ret, ", ");
          index++;
          break;
          }
        j++;
        }
      }
    }
  return ret;
  }

static const char * get_type_string(bg_plugin_type_t type)
  {
  int i = 0;
  while(type_names[i].name)
    {
    if(type_names[i].type == type)
      return TR(type_names[i].name);
    i++;
    }
  return (char*)0;
  }

void bg_gtk_plugin_info_show(const bg_plugin_info_t * info, GtkWidget * parent)
  {
  char * text;
  char * flag_string;
  
  pluginwindow_t * win;
  
  flag_string = get_flag_string(info->flags);
  text = bg_sprintf(TR("Name:\t %s\nLong name:\t %s\nType:\t %s\nFlags:\t %s\nPriority:\t %d\nDLL Filename:\t %s"),
                    info->name, info->long_name, get_type_string(info->type),
                    flag_string, info->priority, info->module_filename);
  win = pluginwindow_create(TRD(info->long_name, info->gettext_domain), text,
                            TRD(info->description, info->gettext_domain));
  
  free(text);
  free(flag_string);
  
  pluginwindow_show(win, 1, parent);
  }
