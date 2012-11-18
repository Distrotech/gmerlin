/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <stdio.h>

#include <gtk/gtk.h>

#include <config.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>
#include <gmerlin/cfg_dialog.h>

#include <gui_gtk/plugin.h>
#include <gui_gtk/gtkutils.h>

typedef struct
  {
  bg_gtk_plugin_widget_multi_t  * input_plugins;
  bg_gtk_plugin_widget_single_t * audio_output_plugins;
  bg_gtk_plugin_widget_single_t * video_output_plugins;
  bg_gtk_plugin_widget_single_t * audio_recorder_plugins;
  bg_gtk_plugin_widget_single_t * video_recorder_plugins;

  bg_gtk_plugin_widget_multi_t  * decompressor_plugins;

  bg_gtk_plugin_widget_single_t * audio_compressor_plugins;
  bg_gtk_plugin_widget_single_t * video_compressor_plugins;
  
  bg_gtk_plugin_widget_single_t * audio_encoder_plugins;
  bg_gtk_plugin_widget_single_t * video_encoder_plugins;
  bg_gtk_plugin_widget_single_t * subtitle_text_encoder_plugins;
  bg_gtk_plugin_widget_single_t * subtitle_overlay_encoder_plugins;

  bg_gtk_plugin_widget_multi_t  * audio_filters;
  bg_gtk_plugin_widget_multi_t  * video_filters;
  bg_gtk_plugin_widget_multi_t  * visualizations;
  
  bg_gtk_plugin_widget_single_t * encoder_pp_plugins;
  
  bg_gtk_plugin_widget_multi_t  * image_readers;
  bg_gtk_plugin_widget_single_t  * image_writers;

  GtkWidget * audio_to_video;
  GtkWidget * subtitle_text_to_video;
  GtkWidget * subtitle_overlay_to_video;
  GtkWidget * use_pp;
    
  GtkWidget * window;
  bg_plugin_registry_t * plugin_reg;

  } app_window;

static void encode_audio_to_video_callback(GtkWidget * w, gpointer data)
  {
  app_window * win;
  int audio_to_video;
  
  win = (app_window*)data;

  audio_to_video = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

  bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder_plugins,
                                            !audio_to_video);
  
  bg_plugin_registry_set_encode_audio_to_video(win->plugin_reg, audio_to_video);
  }

static void encode_subtitle_text_to_video_callback(GtkWidget * w, gpointer data)
  {
  app_window * win;
  int subtitle_text_to_video;
  
  win = (app_window*)data;

  subtitle_text_to_video = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

  bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_text_encoder_plugins,
                                            !subtitle_text_to_video);
  
  bg_plugin_registry_set_encode_subtitle_text_to_video(win->plugin_reg, subtitle_text_to_video);
  }

static void encode_subtitle_overlay_to_video_callback(GtkWidget * w, gpointer data)
  {
  app_window * win;
  int subtitle_overlay_to_video;
  
  win = (app_window*)data;

  subtitle_overlay_to_video = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

  bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_overlay_encoder_plugins,
                                            !subtitle_overlay_to_video);
  
  bg_plugin_registry_set_encode_subtitle_overlay_to_video(win->plugin_reg, subtitle_overlay_to_video);
  }


static void use_pp_callback(GtkWidget * w, gpointer data)
  {
  app_window * win;
  int use_pp;
  
  win = (app_window*)data;

  use_pp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

  bg_gtk_plugin_widget_single_set_sensitive(win->encoder_pp_plugins,
                                            use_pp);
  
  bg_plugin_registry_set_encode_pp(win->plugin_reg, use_pp);
  }


static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  app_window * win = (app_window *)data;
  gtk_widget_hide(win->window);
  gtk_main_quit();
  return TRUE;
  }

static void set_video_encoder(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;

  bg_plugin_registry_set_default(win->plugin_reg,
                                 BG_PLUGIN_ENCODER_VIDEO|BG_PLUGIN_ENCODER, BG_PLUGIN_FILE | BG_PLUGIN_BROADCAST, info->name);

  if(!info->max_audio_streams)
    {
    gtk_widget_set_sensitive(win->audio_to_video, 0);
    
    bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder_plugins, 1);
    }
  else
    {
    gtk_widget_set_sensitive(win->audio_to_video, 1);
    bg_gtk_plugin_widget_single_set_sensitive(win->audio_encoder_plugins,
                                              !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->audio_to_video)));
    }

  if(!info->max_subtitle_text_streams)
    {
    gtk_widget_set_sensitive(win->subtitle_text_to_video, 0);
    
    bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_text_encoder_plugins, 1);
    }
  else
    {
    gtk_widget_set_sensitive(win->subtitle_text_to_video, 1);
    bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_text_encoder_plugins,
                                              !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->subtitle_text_to_video)));
    }

  if(!info->max_subtitle_overlay_streams)
    {
    gtk_widget_set_sensitive(win->subtitle_overlay_to_video, 0);
    
    bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_overlay_encoder_plugins, 1);
    }
  else
    {
    gtk_widget_set_sensitive(win->subtitle_overlay_to_video, 1);
    bg_gtk_plugin_widget_single_set_sensitive(win->subtitle_overlay_encoder_plugins,
                                              !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->subtitle_overlay_to_video)));
    }

  }

static void set_audio_encoder(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_ENCODER_AUDIO, BG_PLUGIN_FILE, info->name);
  }

static void set_subtitle_text_encoder(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_ENCODER_SUBTITLE_TEXT, BG_PLUGIN_FILE, info->name);
  }

static void set_subtitle_overlay_encoder(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY, BG_PLUGIN_FILE, info->name);
  }

static void set_audio_recorder(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_RECORDER_AUDIO, BG_PLUGIN_RECORDER, info->name);
  }

static void set_video_recorder(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_RECORDER_VIDEO, BG_PLUGIN_RECORDER, info->name);
  }

static void set_audio_output(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_OUTPUT_AUDIO, BG_PLUGIN_PLAYBACK, info->name);
  }

static void set_video_output(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_OUTPUT_VIDEO, BG_PLUGIN_PLAYBACK, info->name);
  }

static void set_encoder_pp(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_ENCODER_PP, BG_PLUGIN_PP, info->name);
  }

static void set_image_writer(const bg_plugin_info_t * info, void * data)
  {
  app_window * win = (app_window *)data;
  bg_plugin_registry_set_default(win->plugin_reg, BG_PLUGIN_IMAGE_WRITER, BG_PLUGIN_FILE, info->name);
  }


static app_window * create_window(bg_plugin_registry_t * reg)
  {
  app_window * ret;
  GtkWidget * notebook;
  GtkWidget * label;
  GtkWidget * table;
  int row, num_columns;
  
  ret = calloc(1, sizeof(*ret));
  
  ret->plugin_reg = reg;
  
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
    
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  gtk_window_set_title(GTK_WINDOW(ret->window),
                       TR("Gmerlin Plugin Configurator"));
  
  notebook = gtk_notebook_new();

  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);

  
  ret->input_plugins =
    bg_gtk_plugin_widget_multi_create(reg,
                                      BG_PLUGIN_INPUT,
                                      BG_PLUGIN_FILE|
                                      BG_PLUGIN_URL|
                                      BG_PLUGIN_REMOVABLE |
                                      BG_PLUGIN_TUNER);
  
  label = gtk_label_new(TR("Input plugins"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->input_plugins),
                           label);

  ret->audio_output_plugins =
    bg_gtk_plugin_widget_single_create(TR("Audio"), reg,
                                       BG_PLUGIN_OUTPUT_AUDIO,
                                       BG_PLUGIN_PLAYBACK);
  bg_gtk_plugin_widget_single_set_change_callback(ret->audio_output_plugins, set_audio_output, ret);
  
  ret->video_output_plugins =
    bg_gtk_plugin_widget_single_create("Video", reg,
                                       BG_PLUGIN_OUTPUT_VIDEO,
                                       BG_PLUGIN_PLAYBACK);
  bg_gtk_plugin_widget_single_set_change_callback(ret->video_output_plugins, set_video_output, ret);

  ret->audio_compressor_plugins =
    bg_gtk_plugin_widget_single_create(TR("Audio"), reg,
                                       BG_PLUGIN_CODEC,
                                       BG_PLUGIN_AUDIO_COMPRESSOR);
  
  ret->video_compressor_plugins =
    bg_gtk_plugin_widget_single_create("Video", reg,
                                       BG_PLUGIN_CODEC,
                                       BG_PLUGIN_VIDEO_COMPRESSOR);
  
  
  ret->audio_recorder_plugins =
    bg_gtk_plugin_widget_single_create("Audio", reg,
                                       BG_PLUGIN_RECORDER_AUDIO,
                                       BG_PLUGIN_RECORDER);
  bg_gtk_plugin_widget_single_set_change_callback(ret->audio_recorder_plugins, set_audio_recorder, ret);

  ret->video_recorder_plugins =
    bg_gtk_plugin_widget_single_create("Video", reg,
                                       BG_PLUGIN_RECORDER_VIDEO,
                                       BG_PLUGIN_RECORDER);
  bg_gtk_plugin_widget_single_set_change_callback(ret->video_recorder_plugins, set_video_recorder, ret);

  /* Audio Encoders */

  ret->audio_encoder_plugins =
    bg_gtk_plugin_widget_single_create("Audio", reg,
                                       BG_PLUGIN_ENCODER_AUDIO,
                                       BG_PLUGIN_FILE);
  bg_gtk_plugin_widget_single_set_change_callback(ret->audio_encoder_plugins, set_audio_encoder, ret);

  ret->audio_to_video = gtk_check_button_new_with_label(TR("Encode audio into video file"));
  
  g_signal_connect(G_OBJECT(ret->audio_to_video), "toggled",
                   G_CALLBACK(encode_audio_to_video_callback), ret);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->audio_to_video),
                              bg_plugin_registry_get_encode_audio_to_video(reg));
                              
  gtk_widget_show(ret->audio_to_video);

  /* Text subtitle encoders */
  
  ret->subtitle_text_encoder_plugins =
    bg_gtk_plugin_widget_single_create("Text subtitles", reg,
                                       BG_PLUGIN_ENCODER_SUBTITLE_TEXT,
                                       BG_PLUGIN_FILE);
  bg_gtk_plugin_widget_single_set_change_callback(ret->subtitle_text_encoder_plugins, set_subtitle_text_encoder, ret);

  ret->subtitle_text_to_video = gtk_check_button_new_with_label(TR("Encode text subtitles into video file"));
  
  g_signal_connect(G_OBJECT(ret->subtitle_text_to_video), "toggled",
                   G_CALLBACK(encode_subtitle_text_to_video_callback), ret);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->subtitle_text_to_video),
                              bg_plugin_registry_get_encode_subtitle_text_to_video(reg));
  
  gtk_widget_show(ret->subtitle_text_to_video);
  
  /* Overlay subtitle encoders */
  
  ret->subtitle_overlay_encoder_plugins =
    bg_gtk_plugin_widget_single_create("Overlay subtitles", reg,
                                       BG_PLUGIN_ENCODER_SUBTITLE_OVERLAY,
                                       BG_PLUGIN_FILE);
  bg_gtk_plugin_widget_single_set_change_callback(ret->subtitle_overlay_encoder_plugins, set_subtitle_overlay_encoder,
                                                  ret);

  ret->subtitle_overlay_to_video =
    gtk_check_button_new_with_label(TR("Encode overlay subtitles into video file"));
  
  g_signal_connect(G_OBJECT(ret->subtitle_overlay_to_video), "toggled",
                   G_CALLBACK(encode_subtitle_overlay_to_video_callback), ret);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->subtitle_overlay_to_video),
                              bg_plugin_registry_get_encode_subtitle_overlay_to_video(reg));
  
  gtk_widget_show(ret->subtitle_overlay_to_video);
  
  /* Video encoders */
    
  ret->video_encoder_plugins =
    bg_gtk_plugin_widget_single_create("Video", reg,
                                       BG_PLUGIN_ENCODER_VIDEO |
                                       BG_PLUGIN_ENCODER,
                                       BG_PLUGIN_FILE | BG_PLUGIN_BROADCAST);
  bg_gtk_plugin_widget_single_set_change_callback(ret->video_encoder_plugins, set_video_encoder, ret);
  
  /* Postprocessors */
  
  ret->encoder_pp_plugins =
    bg_gtk_plugin_widget_single_create("Postprocessor", reg,
                                       BG_PLUGIN_ENCODER_PP,
                                       BG_PLUGIN_PP);
  bg_gtk_plugin_widget_single_set_change_callback(ret->encoder_pp_plugins, set_encoder_pp, ret);

  bg_gtk_plugin_widget_single_set_sensitive(ret->encoder_pp_plugins, 0);

  
  /* Postprocess */
  ret->use_pp = gtk_check_button_new_with_label(TR("Enable postprocessing"));
  
  g_signal_connect(G_OBJECT(ret->use_pp), "toggled",
                   G_CALLBACK(use_pp_callback), ret);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret->use_pp),
                               bg_plugin_registry_get_encode_pp(reg));
  
  gtk_widget_show(ret->use_pp);

  /* Audio filters */

  ret->audio_filters =
    bg_gtk_plugin_widget_multi_create(reg,
                                      BG_PLUGIN_FILTER_AUDIO,
                                      BG_PLUGIN_FILTER_1);
  
  label = gtk_label_new(TR("Audio filters"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->audio_filters),
                           label);
  
  /* Video filters */

  ret->video_filters =
    bg_gtk_plugin_widget_multi_create(reg,
                                      BG_PLUGIN_FILTER_VIDEO,
                                      BG_PLUGIN_FILTER_1);
  
  label = gtk_label_new(TR("Video filters"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->video_filters),
                           label);

  /* Visualizations */

  ret->visualizations =
    bg_gtk_plugin_widget_multi_create(reg,
                                      BG_PLUGIN_VISUALIZATION,
                                      BG_PLUGIN_VISUALIZE_FRAME|BG_PLUGIN_VISUALIZE_GL);
  
  label = gtk_label_new(TR("Visualizations"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->visualizations),
                           label);

  /* Compressors */
  
  table = gtk_table_new(1, 1, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->audio_compressor_plugins,
                                     table,
                                     &row, &num_columns);
  bg_gtk_plugin_widget_single_attach(ret->video_compressor_plugins,
                                     table,
                                     &row, &num_columns);

  gtk_widget_show(table);
  
  label = gtk_label_new(TR("Compressors"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table, label);
  
  /* Decompressors */

  ret->decompressor_plugins =
    bg_gtk_plugin_widget_multi_create(reg,
                                      BG_PLUGIN_CODEC,
                                      BG_PLUGIN_AUDIO_DECOMPRESSOR|
                                      BG_PLUGIN_VIDEO_DECOMPRESSOR);
  
  label = gtk_label_new(TR("Decompressors"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->decompressor_plugins),
                           label);

  
  /* Pack */
  
  table = gtk_table_new(1, 1, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->audio_output_plugins,
                                     table,
                                     &row, &num_columns);
  bg_gtk_plugin_widget_single_attach(ret->video_output_plugins,
                                     table,
                                     &row, &num_columns);

  gtk_widget_show(table);
  
  label = gtk_label_new(TR("Output"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table, label);

  /* Encoders */
  
  table = gtk_table_new(1, 1, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  row = 0;
  num_columns = 6;

  gtk_table_resize(GTK_TABLE(table), row+1, num_columns);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->audio_to_video,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  row += 1;

  
  bg_gtk_plugin_widget_single_attach(ret->audio_encoder_plugins,
                                     table,
                                     &row, &num_columns);

  /* Text subtitles */
  
  gtk_table_resize(GTK_TABLE(table), row+1, num_columns);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->subtitle_text_to_video,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  row += 1;

  bg_gtk_plugin_widget_single_attach(ret->subtitle_text_encoder_plugins,
                                     table,
                                     &row, &num_columns);
  
  /* Ovetrlay subtitles */
  
  gtk_table_resize(GTK_TABLE(table), row+1, num_columns);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->subtitle_overlay_to_video,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  row += 1;

  bg_gtk_plugin_widget_single_attach(ret->subtitle_overlay_encoder_plugins,
                                     table,
                                     &row, &num_columns);
  
  /* Video */
  bg_gtk_plugin_widget_single_attach(ret->video_encoder_plugins,
                                     table,
                                     &row, &num_columns);

  
  /* Postprocessing */
  
  
  gtk_table_resize(GTK_TABLE(table), row+1, num_columns);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->use_pp,
                   0, num_columns, row, row+1,
                   GTK_FILL | GTK_EXPAND, GTK_SHRINK, 0, 0);
  row += 1;

  
  
  bg_gtk_plugin_widget_single_attach(ret->encoder_pp_plugins,
                                     table,
                                     &row, &num_columns);

  
  gtk_widget_show(table);

  
  label = gtk_label_new(TR("Encoders"));
  gtk_widget_show(label);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table, label);
  

  table = gtk_table_new(1, 1, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->audio_recorder_plugins,
                                     table,
                                     &row, &num_columns);
  bg_gtk_plugin_widget_single_attach(ret->video_recorder_plugins,
                                     table,
                                     &row, &num_columns);
  
  gtk_widget_show(table);
  
  label = gtk_label_new(TR("Recorder"));
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
  
  label = gtk_label_new(TR("Image readers"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->image_readers),
                           label);

  /* Image writers */

  ret->image_writers =
    bg_gtk_plugin_widget_single_create("Image writer", reg,
                                       BG_PLUGIN_IMAGE_WRITER,
                                       BG_PLUGIN_FILE|
                                       BG_PLUGIN_URL|
                                       BG_PLUGIN_REMOVABLE);

  bg_gtk_plugin_widget_single_set_change_callback(ret->image_writers, set_image_writer, ret);

  

  table = gtk_table_new(1, 1, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->image_writers,
                                     table,
                                     &row, &num_columns);
  
  gtk_widget_show(table);
  
  label = gtk_label_new(TR("Image writers"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table,
                           label);
  
  gtk_widget_show(notebook);
  gtk_container_add(GTK_CONTAINER(ret->window), notebook);

  set_video_encoder(bg_gtk_plugin_widget_single_get_plugin(ret->video_encoder_plugins),
                    ret);
  
  return ret;
  }

static void destroy_window(app_window * win)
  {
  
  //  bg_gtk_plugin_widget_multi_destroy(win->input_plugins);
  bg_gtk_plugin_widget_single_destroy(win->audio_output_plugins);
  bg_gtk_plugin_widget_single_destroy(win->video_output_plugins);
  bg_gtk_plugin_widget_single_destroy(win->audio_recorder_plugins);
  bg_gtk_plugin_widget_single_destroy(win->video_recorder_plugins);

  bg_gtk_plugin_widget_single_destroy(win->audio_encoder_plugins);
  bg_gtk_plugin_widget_single_destroy(win->video_encoder_plugins);

  bg_gtk_plugin_widget_single_destroy(win->subtitle_text_encoder_plugins);
  bg_gtk_plugin_widget_single_destroy(win->subtitle_overlay_encoder_plugins);
  bg_gtk_plugin_widget_single_destroy(win->encoder_pp_plugins);

  
                                      
  free(win);
  }


int main(int argc, char ** argv)
  {
  bg_plugin_registry_t * reg;
  app_window * window;
  char * config_path;

  bg_cfg_registry_t * cfg_reg;
  bg_cfg_section_t * cfg_section;

  bg_gtk_init(&argc, &argv, "plugincfg_icon.png", NULL, NULL);
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

  bg_cfg_registry_destroy(cfg_reg);
  bg_plugin_registry_destroy(reg);
  
  
  
  return 0;
  }
