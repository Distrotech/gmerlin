#include "gmerlin.h"
#include <gui_gtk/plugin.h>

struct plugin_window_s
  {
  GtkWidget * window;
  bg_gtk_plugin_widget_multi_t * inputs;
  bg_gtk_plugin_widget_single_t * video_output;
  bg_gtk_plugin_widget_single_t * audio_output;
  bg_gtk_plugin_widget_multi_t * image_readers;
  GtkWidget * close_button;
  void (*close_notify)(plugin_window_t*,void*);
  void * close_notify_data;

  GtkTooltips * tooltips;
  };

static void set_audio_output(bg_plugin_handle_t * handle, void * data)
  {
  gmerlin_t * gmerlin = (gmerlin_t*)data;
  bg_player_set_oa_plugin(gmerlin->player, handle);
  }

static void set_video_output(bg_plugin_handle_t * handle, void * data)
  {
  gmerlin_t * gmerlin = (gmerlin_t*)data;
  bg_player_set_ov_plugin(gmerlin->player, handle);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  plugin_window_t * win = (plugin_window_t *)data;
  gtk_widget_hide(win->window);

  if(win->close_notify)
    win->close_notify(win, win->close_notify_data);
  }

static gboolean delete_callback(GtkWidget * w, GdkEventAny * evt, gpointer data)
  {
  button_callback(w, data);
  return TRUE;
  }

plugin_window_t * plugin_window_create(gmerlin_t * g,
                                       void (*close_notify)(plugin_window_t*,void*),
                                       void * close_notify_data)
  {
  plugin_window_t * ret;
  GtkWidget * label;
  GtkWidget * table;
  GtkWidget * notebook;
  int row = 0;
  int num_columns = 0;
    
  ret = calloc(1, sizeof(*ret));

  ret->tooltips = gtk_tooltips_new();

  g_object_ref (G_OBJECT (ret->tooltips));
  gtk_object_sink (GTK_OBJECT (ret->tooltips));

  
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), "Plugins");

  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  
  ret->close_notify      = close_notify;
  ret->close_notify_data = close_notify_data;
  
  ret->audio_output = 
    bg_gtk_plugin_widget_single_create("Audio",
                                       g->plugin_reg,
                                       BG_PLUGIN_OUTPUT_AUDIO,
                                       BG_PLUGIN_PLAYBACK,
                                       set_audio_output,
                                       g, ret->tooltips);
  ret->video_output = 
    bg_gtk_plugin_widget_single_create("Video",
                                       g->plugin_reg,
                                       BG_PLUGIN_OUTPUT_VIDEO,
                                       BG_PLUGIN_PLAYBACK,
                                       set_video_output,
                                       g, ret->tooltips);

  ret->inputs = 
    bg_gtk_plugin_widget_multi_create(g->plugin_reg,
                                      BG_PLUGIN_INPUT,
                                      BG_PLUGIN_FILE|
                                      BG_PLUGIN_URL|
                                      BG_PLUGIN_REMOVABLE,
                                      ret->tooltips);

  ret->image_readers = 
    bg_gtk_plugin_widget_multi_create(g->plugin_reg,
                                      BG_PLUGIN_IMAGE_READER,
                                      BG_PLUGIN_FILE, ret->tooltips);
  
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
  
  label = gtk_label_new("Inputs");
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->inputs),
                           label);
  
  table = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);

  bg_gtk_plugin_widget_single_attach(ret->audio_output,
                                     table,
                                     &row, &num_columns);

  bg_gtk_plugin_widget_single_attach(ret->video_output,
                                     table,
                                     &row, &num_columns);
  
  gtk_widget_show(table);

  label = gtk_label_new("Outputs");
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table,
                           label);

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
  bg_gtk_plugin_widget_single_destroy(w->audio_output);
  bg_gtk_plugin_widget_single_destroy(w->video_output);
  bg_gtk_plugin_widget_multi_destroy(w->inputs);
  bg_gtk_plugin_widget_multi_destroy(w->image_readers);
  g_object_unref(w->tooltips);
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

void plugin_window_set_tooltips(plugin_window_t * w, int enable)
  {
  if(enable)
    gtk_tooltips_enable(w->tooltips);
  else
    gtk_tooltips_disable(w->tooltips);
  }
