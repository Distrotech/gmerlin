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
  bg_gtk_plugin_widget_multi_t * image_readers;
  bg_gtk_plugin_widget_single_t * video_encoder;
  bg_gtk_plugin_widget_single_t * audio_encoder;
  GtkWidget * close_button;
  GtkWidget * audio_to_video;

  void (*close_notify)(plugin_window_t*,void*);
  void * close_notify_data;

  transcoder_window_t * tw;
  bg_plugin_registry_t * plugin_reg;
  };

 
static void set_video_encoder(bg_plugin_handle_t * handle, void * data)
  {
  plugin_window_t * win = (plugin_window_t *)data;
  if(handle->info->type == BG_PLUGIN_ENCODER_VIDEO)
    {
    gtk_widget_set_sensitive(win->audio_to_video, 0);

    bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder, 1);
    }
  else
    {
    gtk_widget_set_sensitive(win->audio_to_video, 1);
    bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder,
                                              !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->audio_to_video)));
    }
  
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
      bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder, 0);
      bg_plugin_registry_set_encode_audio_to_video(win->plugin_reg, 1);
      }
    else
      {
      bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder, 1);
      bg_plugin_registry_set_encode_audio_to_video(win->plugin_reg, 0);
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
  int row, num_columns;
  ret = calloc(1, sizeof(*ret));
  ret->tw = win;
  ret->plugin_reg = plugin_reg;
    
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), "Plugins");
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  ret->close_notify      = close_notify;
  ret->close_notify_data = close_notify_data;

  
  ret->audio_encoder =
    bg_gtk_plugin_widget_single_create("Audio", plugin_reg,
                                       BG_PLUGIN_ENCODER_AUDIO,
                                       BG_PLUGIN_FILE,
                                       NULL,
                                       ret);

  ret->audio_to_video =
    gtk_check_button_new_with_label("Encode audio into video file");

  g_signal_connect(G_OBJECT(ret->audio_to_video), "toggled",
                   G_CALLBACK(button_callback),
                   ret);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->audio_to_video),
                               bg_plugin_registry_get_encode_audio_to_video(plugin_reg));

  ret->video_encoder =
    bg_gtk_plugin_widget_single_create("Video", plugin_reg,
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
  gtk_widget_show(ret->audio_to_video);
  
  /* Pack */
  notebook = gtk_notebook_new();
  label = gtk_label_new("Inputs");
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->inputs),
                           label);

  table = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);

  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->audio_encoder,
                                     table,
                                     &row, &num_columns);
  bg_gtk_plugin_widget_single_attach(ret->video_encoder,
                                     table,
                                     &row, &num_columns);

  gtk_table_resize(GTK_TABLE(table), row+1, num_columns);
   
  gtk_table_attach(GTK_TABLE(table),
                   ret->audio_to_video,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  
  gtk_widget_show(table);
  label = gtk_label_new("Encoders");
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table, label);


  label = gtk_label_new("Image readers");
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
