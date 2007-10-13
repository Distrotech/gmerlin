#include "gmerlin.h"

#include <config.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>

struct plugin_window_s
  {
  GtkWidget * window;
  bg_gtk_plugin_widget_multi_t * inputs;
  //  bg_gtk_plugin_widget_single_t * video_output;
  //  bg_gtk_plugin_widget_single_t * audio_output;
  bg_gtk_plugin_widget_multi_t * image_readers;

  GtkWidget * close_button;
  void (*close_notify)(plugin_window_t*,void*);
  void * close_notify_data;

  gmerlin_t * g;
  
  };

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
    
  ret = calloc(1, sizeof(*ret));

  ret->g = g;
  
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->window), TR("Plugins"));

  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  
  ret->close_notify      = close_notify;
  ret->close_notify_data = close_notify_data;
  
  ret->inputs = 
    bg_gtk_plugin_widget_multi_create(g->plugin_reg,
                                      BG_PLUGIN_INPUT,
                                      BG_PLUGIN_FILE|
                                      BG_PLUGIN_URL|
                                      BG_PLUGIN_REMOVABLE);

  ret->image_readers = 
    bg_gtk_plugin_widget_multi_create(g->plugin_reg,
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
  
  label = gtk_label_new(TR("Inputs"));
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->inputs),
                           label);
  
  table = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);

 
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
  bg_gtk_plugin_widget_multi_destroy(w->image_readers);
  free(w);
  }

void plugin_window_show(plugin_window_t * w)
  {
  gtk_widget_show(w->window);
  gtk_window_present(GTK_WINDOW(w->window));
  
  }

