#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <pluginregistry.h>
#include <utils.h>
#include <cfg_dialog.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>

typedef struct
  {
  bg_gtk_plugin_widget_multi_t  * input_plugins;
  bg_gtk_plugin_widget_single_t * audio_output_plugins;
  bg_gtk_plugin_widget_single_t * video_output_plugins;
  bg_gtk_plugin_widget_single_t * audio_recorder_plugins;
  bg_gtk_plugin_widget_single_t * video_recorder_plugins;

  bg_gtk_plugin_widget_single_t * audio_encoder_plugins;
  bg_gtk_plugin_widget_single_t * video_encoder_plugins;

  bg_gtk_plugin_widget_multi_t  * image_readers;
  bg_gtk_plugin_widget_single_t  * image_writers;
  
  GtkWidget * window;
  } app_window;

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  app_window * win = (app_window *)data;
  gtk_widget_hide(win->window);
  gtk_main_quit();
  return TRUE;
  }

static app_window * create_window(bg_plugin_registry_t * reg)
  {
  app_window * ret;
  GtkWidget * notebook;
  GtkWidget * label;
  GtkWidget * table;
  
  ret = calloc(1, sizeof(*ret));

  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  gtk_window_set_title(GTK_WINDOW(ret->window),
                       "Gmerlin Plugin Configurator");
  
  notebook = gtk_notebook_new();
    
  ret->input_plugins =
    bg_gtk_plugin_widget_multi_create(reg,
                                      BG_PLUGIN_INPUT,
                                      BG_PLUGIN_FILE|
                                      BG_PLUGIN_URL|
                                      BG_PLUGIN_REMOVABLE);
  
  label = gtk_label_new("Input plugins");
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->input_plugins),
                           label);

  ret->audio_output_plugins =
    bg_gtk_plugin_widget_single_create(reg,
                                       BG_PLUGIN_OUTPUT_AUDIO,
                                       BG_PLUGIN_PLAYBACK, NULL, NULL);
  ret->video_output_plugins =
    bg_gtk_plugin_widget_single_create(reg,
                                       BG_PLUGIN_OUTPUT_VIDEO,
                                       BG_PLUGIN_PLAYBACK, NULL, NULL);

  ret->audio_recorder_plugins =
    bg_gtk_plugin_widget_single_create(reg,
                                       BG_PLUGIN_RECORDER_AUDIO,
                                       BG_PLUGIN_RECORDER, NULL, NULL);

  ret->video_recorder_plugins =
    bg_gtk_plugin_widget_single_create(reg,
                                       BG_PLUGIN_RECORDER_VIDEO,
                                       BG_PLUGIN_RECORDER, NULL, NULL);

  ret->audio_encoder_plugins =
    bg_gtk_plugin_widget_single_create(reg,
                                       BG_PLUGIN_ENCODER_AUDIO,
                                       BG_PLUGIN_FILE, NULL, NULL);

  ret->video_encoder_plugins =
    bg_gtk_plugin_widget_single_create(reg,
                                       BG_PLUGIN_ENCODER_VIDEO |
                                       BG_PLUGIN_ENCODER,
                                       BG_PLUGIN_FILE, NULL, NULL);
  
  table = gtk_table_new(4, 2, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    
  label = gtk_label_new("Audio");
  gtk_widget_show(label);
  
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->audio_output_plugins),
                   1, 2, 0, 1, GTK_EXPAND, GTK_SHRINK, 0, 0);

  label = gtk_label_new("Video");
  gtk_widget_show(label);
  
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->video_output_plugins),
                   1, 2, 1, 2,
                   GTK_EXPAND, GTK_SHRINK, 0, 0);

  label = gtk_label_new("Audio Encoder");
  gtk_widget_show(label);
  
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->audio_encoder_plugins),
                   1, 2, 2, 3,
                   GTK_EXPAND, GTK_SHRINK, 0, 0);

  label = gtk_label_new("Video Encoder");
  gtk_widget_show(label);
  
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->video_encoder_plugins),
                   1, 2, 3, 4,
                   GTK_EXPAND, GTK_SHRINK, 0, 0);

  
  gtk_widget_show(table);
  
  label = gtk_label_new("Output");
  gtk_widget_show(label);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table, label);

  table = gtk_table_new(2, 2, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    
  label = gtk_label_new("Audio");
  gtk_widget_show(label);
  
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->audio_recorder_plugins),
                   1, 2, 0, 1, GTK_EXPAND, GTK_SHRINK, 0, 0);

  label = gtk_label_new("Video");
  gtk_widget_show(label);
  
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->video_recorder_plugins),
                   1, 2, 1, 2, GTK_EXPAND, GTK_SHRINK, 0, 0);
  
  gtk_widget_show(table);
  
  label = gtk_label_new("Recorder");
  gtk_widget_show(label);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table, label);

  /* Image readers */

  ret->image_readers =
    bg_gtk_plugin_widget_multi_create(reg,
                                      BG_PLUGIN_IMAGE_READER,
                                      BG_PLUGIN_FILE|
                                      BG_PLUGIN_URL|
                                      BG_PLUGIN_REMOVABLE);
  
  label = gtk_label_new("Image readers");
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->image_readers),
                           label);

  /* Image writers */

  ret->image_writers =
    bg_gtk_plugin_widget_single_create(reg,
                                       BG_PLUGIN_IMAGE_WRITER,
                                       BG_PLUGIN_FILE|
                                       BG_PLUGIN_URL|
                                       BG_PLUGIN_REMOVABLE,
                                       NULL, NULL);
  

  table = gtk_table_new(1, 2, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    
  label = gtk_label_new("Image writer");
  gtk_widget_show(label);
  
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_plugin_widget_single_get_widget(ret->image_writers),
                   1, 2, 0, 1, GTK_EXPAND, GTK_SHRINK, 0, 0);

  gtk_widget_show(table);
  
  label = gtk_label_new("Image writers");
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table,
                           label);
  
  gtk_widget_show(notebook);
  gtk_container_add(GTK_CONTAINER(ret->window), notebook);

  return ret;
  }

static void destroy_window(app_window * win)
  {
  
  //  bg_gtk_plugin_widget_multi_destroy(win->input_plugins);
  bg_gtk_plugin_widget_single_destroy(win->audio_output_plugins);
  bg_gtk_plugin_widget_single_destroy(win->video_output_plugins);
  bg_gtk_plugin_widget_single_destroy(win->audio_recorder_plugins);

                                      
  free(win);
  }


int main(int argc, char ** argv)
  {
  bg_plugin_registry_t * reg;
  app_window * window;
  char * config_path;

  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;

  bg_gtk_init(&argc, &argv);
  cfg_reg = bg_cfg_registry_create();
    
  config_path = bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, config_path);
  if(config_path)
    free(config_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  reg = bg_plugin_registry_create(cfg_section);

  window = create_window(reg);

  gtk_widget_show(window->window);
  gtk_main();
  
  destroy_window(window);
    
  config_path = bg_search_file_write("generic", "config.xml");
  bg_cfg_registry_save(cfg_reg, config_path);
  if(config_path)
    free(config_path);

  
  return 0;
  }
