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
#include <gtk/gtk.h>

#include <config.h>

#include <pluginregistry.h>
#include <transcoder_track.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>

#include "transcoder_window.h"
#include "pluginwindow.h"
#include "encoderwidget.h"
#include "ppwidget.h"

struct plugin_window_s
  {
  GtkWidget * window;
  bg_gtk_plugin_widget_multi_t * inputs;
  bg_gtk_plugin_widget_multi_t * image_readers;
  GtkWidget * close_button;

  void (*close_notify)(plugin_window_t*,void*);
  void * close_notify_data;

  transcoder_window_t * tw;
  bg_plugin_registry_t * plugin_reg;


  encoder_widget_t encoders;
  };

static void button_callback(GtkWidget * w, gpointer data)
  {
  plugin_window_t * win = (plugin_window_t *)data;

  if((w == win->close_button) || (w == win->window))
    {
    gtk_widget_hide(win->window);
    
    if(win->close_notify)
      win->close_notify(win, win->close_notify_data);
    }

  }
                                                                               
static gboolean delete_callback(GtkWidget * w, GdkEventAny * evt,
                                gpointer data)  {
  button_callback(w, data);
  return TRUE;
  }

static void set_audio_encoder(const bg_plugin_info_t * info, void * data)
  {
  plugin_window_t * w = (plugin_window_t*)data;
  bg_plugin_registry_set_default(w->plugin_reg,
                                 BG_PLUGIN_ENCODER_AUDIO, info->name);
  
  }

static void set_video_encoder(const bg_plugin_info_t * info, void * data)
  {
  plugin_window_t * w = (plugin_window_t*)data;
  bg_plugin_registry_set_default(w->plugin_reg,
                                 BG_PLUGIN_ENCODER_VIDEO | BG_PLUGIN_ENCODER, info->name);
  encoder_widget_update_sensitive(&w->encoders);
  }

static void set_subtitle_text_encoder(const bg_plugin_info_t * info, void * data)
  {
  plugin_window_t * w = (plugin_window_t*)data;
  bg_plugin_registry_set_default(w->plugin_reg,
                                 BG_PLUGIN_ENCODER_SUBTITLE_TEXT, info->name);
  
  }

#if 0
static void set_subtitle_overlay_encoder(const bg_plugin_info_t * info,
                                         void * data)
  {
  plugin_window_t * w = (plugin_window_t*)data;
  bg_plugin_registry_set_default(w->plugin_reg,
                                 BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY, info->name);
  
  }
#endif


plugin_window_t *
plugin_window_create(bg_plugin_registry_t * plugin_reg,
                     transcoder_window_t * win,
                     void (*close_notify)(plugin_window_t*,void*),
                     void * close_notify_data)
  {
  plugin_window_t * ret;
  GtkWidget * label;
  GtkWidget * table;
  GtkWidget * notebook;
  ret = calloc(1, sizeof(*ret));

  encoder_widget_init(&(ret->encoders), plugin_reg);

  bg_gtk_plugin_widget_single_set_change_callback(ret->encoders.audio_encoder,
                                                  set_audio_encoder, ret);
  bg_gtk_plugin_widget_single_set_change_callback(ret->encoders.video_encoder,
                                                  set_video_encoder, ret);
  bg_gtk_plugin_widget_single_set_change_callback(ret->encoders.subtitle_text_encoder,
                                                  set_subtitle_text_encoder, ret);
  
    
  ret->tw = win;
  ret->plugin_reg = plugin_reg;
    
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Plugins"));
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  ret->close_notify      = close_notify;
  ret->close_notify_data = close_notify_data;
 
  
  ret->inputs =
    bg_gtk_plugin_widget_multi_create(plugin_reg,
                                      BG_PLUGIN_INPUT,
                                      BG_PLUGIN_FILE|
                                      BG_PLUGIN_URL|
                                      BG_PLUGIN_REMOVABLE);

  ret->image_readers =
    bg_gtk_plugin_widget_multi_create(plugin_reg,
                                      BG_PLUGIN_IMAGE_READER,
                                      BG_PLUGIN_FILE);
    
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

  /* Set callbacks */
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   ret);
  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(button_callback),
                   ret);

  /* Show */
  gtk_widget_show(ret->close_button);
  
  /* Pack */
  notebook = gtk_notebook_new();

  label = gtk_label_new(TR("Encoders"));
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           ret->encoders.widget, label);

  

  label = gtk_label_new(TR("Inputs"));
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->inputs),
                           label);

  label = gtk_label_new(TR("Image readers"));
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->image_readers),
                           label);
  
  gtk_widget_show(notebook);
  table = gtk_table_new(2, 1, 0);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_attach_defaults(GTK_TABLE(table),
                            notebook,
                            0, 1, 0, 1);
  gtk_table_attach(GTK_TABLE(table),
                   ret->close_button,
                   0, 1, 1, 2, GTK_EXPAND, GTK_SHRINK, 0, 0);
  gtk_widget_show(table);
  gtk_container_add(GTK_CONTAINER(ret->window), table);
  return ret;
  }

void plugin_window_destroy(plugin_window_t * w)
  {
  bg_gtk_plugin_widget_multi_destroy(w->inputs);


  free(w);
  }

void plugin_window_show(plugin_window_t * w)
  {
  /* Get values from registry (they might have changed by loading a profile) */
  encoder_widget_set_from_registry(&w->encoders,
                                   w->plugin_reg);
  gtk_widget_show(w->window);
  }


