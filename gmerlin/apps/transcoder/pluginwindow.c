#include <stdlib.h>
#include <gtk/gtk.h>

#include <pluginregistry.h>

#include <gui_gtk/plugin.h>

#include "transcoder_window.h"
#include "pluginwindow.h"

struct plugin_window_s
  {
  GtkWidget * window;
  bg_gtk_plugin_widget_multi_t * inputs;
  bg_gtk_plugin_widget_single_t * video_encoder;
  bg_gtk_plugin_widget_single_t * audio_encoder;
  GtkWidget * close_button;
  GtkWidget * audio_to_video;

  void (*close_notify)(plugin_window_t*,void*);
  void * close_notify_data;

  transcoder_window_t * tw;
  };

static void set_audio_encoder(bg_plugin_handle_t * handle, void * data)
  {
  plugin_window_t * win = (plugin_window_t *)data;
  transcoder_window_set_audio_encoder(win->tw, handle);
  
  }
 
static void set_video_encoder(bg_plugin_handle_t * handle, void * data)
  {
  plugin_window_t * win = (plugin_window_t *)data;
  if(handle->info->type == BG_PLUGIN_ENCODER_VIDEO)
    {
    gtk_widget_set_sensitive(win->audio_to_video, 0);

    gtk_widget_set_sensitive(bg_gtk_plugin_widget_single_get_widget(win->audio_encoder),
                             1);
    }
  else
    {
    gtk_widget_set_sensitive(win->audio_to_video, 1);
    gtk_widget_set_sensitive(bg_gtk_plugin_widget_single_get_widget(win->audio_encoder),
                             !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->audio_to_video)));
    }
  
  transcoder_window_set_video_encoder(win->tw, handle);
  }


static void button_callback(GtkWidget * w, gpointer data)
  {
  plugin_window_t * win = (plugin_window_t *)data;

  if((w == win->close_button) || (w == win->window))
    {
    gtk_widget_hide(win->window);
    
    if(win->close_notify)
      win->close_notify(win, win->close_notify_data);
    }
  else if(w == win->audio_to_video)
    {
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->audio_to_video)))
      {
      gtk_widget_set_sensitive(bg_gtk_plugin_widget_single_get_widget(win->audio_encoder),
                               0);
      transcoder_window_set_audio_encoder(win->tw, NULL);
      }
    else
      {
      gtk_widget_set_sensitive(bg_gtk_plugin_widget_single_get_widget(win->audio_encoder),
                               1);
      transcoder_window_set_audio_encoder(win->tw,
                                          bg_gtk_plugin_widget_single_get_plugin(win->audio_encoder));
      }
    
    }

  }
                                                                               
static gboolean delete_callback(GtkWidget * w, GdkEventAny * evt,
                                gpointer data)  {
  button_callback(w, data);
  return TRUE;
  }

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
  ret->tw = win;
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), "Plugins");
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  ret->close_notify      = close_notify;
  ret->close_notify_data = close_notify_data;

  ret->audio_to_video =
    gtk_check_button_new_with_label("Encode audio into video file");
  
  ret->audio_encoder =
    bg_gtk_plugin_widget_single_create(plugin_reg,
                                       BG_PLUGIN_ENCODER_AUDIO,
                                       BG_PLUGIN_FILE,
                                       set_audio_encoder,
                                       ret);
  ret->video_encoder =
    bg_gtk_plugin_widget_single_create(plugin_reg,
                                       BG_PLUGIN_ENCODER_VIDEO |
                                       BG_PLUGIN_ENCODER,
                                       BG_PLUGIN_FILE,
                                       set_video_encoder,
                                       ret);
  
  ret->inputs =
    bg_gtk_plugin_widget_multi_create(plugin_reg,
                                      BG_PLUGIN_INPUT,
                                      BG_PLUGIN_FILE|
                                      BG_PLUGIN_URL|
                                      BG_PLUGIN_REMOVABLE);

  
  ret->close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

  /* Set callbacks */
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback),
                   ret);
  g_signal_connect(G_OBJECT(ret->close_button), "clicked",
                   G_CALLBACK(button_callback),
                   ret);

  g_signal_connect(G_OBJECT(ret->audio_to_video), "clicked",
                   G_CALLBACK(button_callback),
                   ret);
  /* Show */
  gtk_widget_show(ret->close_button);
  gtk_widget_show(ret->audio_to_video);
  
  /* Pack */
  notebook = gtk_notebook_new();
  table = gtk_table_new(3, 2, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  label = gtk_label_new("Inputs");
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->inputs),
                           label);
  label = gtk_label_new("Audio");
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->audio_encoder),
                   1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  label = gtk_label_new("Video");
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->video_encoder),
                   1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table),
                   ret->audio_to_video,
                   0, 2, 2, 3, GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  
  gtk_widget_show(table);
  label = gtk_label_new("Encoders");
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table, label);

  
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
  bg_gtk_plugin_widget_single_destroy(w->audio_encoder);
  bg_gtk_plugin_widget_single_destroy(w->video_encoder);
  bg_gtk_plugin_widget_multi_destroy(w->inputs);
  free(w);
  }

void plugin_window_show(plugin_window_t * w)
  {
  gtk_widget_show(w->window);
  }

void plugin_window_hide(plugin_window_t * w)
  {
  gtk_widget_show(w->window);
  }
