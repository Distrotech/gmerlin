/*****************************************************************
 * gmerlin-mozilla - A gmerlin based mozilla plugin
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

#include <gmerlin_mozilla.h>
#include <gmerlin/gui_gtk/plugin.h>
#include <gmerlin/gui_gtk/gtkutils.h>

struct plugin_window_s
  {
  bg_gtk_plugin_widget_multi_t  * input_plugins;
  bg_gtk_plugin_widget_single_t * audio_output_plugins;
  bg_gtk_plugin_widget_single_t * video_output_plugins;
  bg_gtk_plugin_widget_single_t * vis_plugins;

  GtkWidget * window;
  } ;

static gboolean delete_callback(GtkWidget * w, GdkEventAny * event,
                                gpointer data)
  {
  plugin_window_t * win = (plugin_window_t*)data;
  gtk_widget_hide(win->window);
  return TRUE;
  }

static void set_audio_output(const bg_plugin_info_t * info, void * data)
  {
  bg_mozilla_t * m = data;
  bg_plugin_registry_set_default(m->plugin_reg,
                                 BG_PLUGIN_OUTPUT_AUDIO,
                                 BG_PLUGIN_PLAYBACK,
                                 info->name);
  gmerlin_mozilla_set_oa_plugin(m, info);
  }

static void set_video_output(const bg_plugin_info_t * info, void * data)
  {
  bg_mozilla_t * m = data;
  bg_plugin_registry_set_default(m->plugin_reg,
                                 BG_PLUGIN_OUTPUT_VIDEO,
                                 BG_PLUGIN_PLAYBACK,
                                 info->name);
  gmerlin_mozilla_set_ov_plugin(m, info);
  }

static void set_visualization(const bg_plugin_info_t * info, void * data)
  {
  bg_mozilla_t * m = data;
  bg_plugin_registry_set_default(m->plugin_reg,
                                 BG_PLUGIN_VISUALIZATION,
                                 BG_PLUGIN_VISUALIZE_FRAME | BG_PLUGIN_VISUALIZE_GL,
                                 info->name);
  gmerlin_mozilla_set_vis_plugin(m, info);
  }

plugin_window_t * bg_mozilla_plugin_window_create(bg_mozilla_t * m)
  {
  plugin_window_t * ret;
  GtkWidget * notebook;
  GtkWidget * label;
  GtkWidget * table;
  int row, num_columns;
  bg_plugin_registry_t * reg = m->plugin_reg;
  
  ret = calloc(1, sizeof(*ret));
  
  //  ret->plugin_reg = reg;
  
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->window), GTK_WIN_POS_CENTER);
  
  g_signal_connect(G_OBJECT(ret->window), "delete_event",
                   G_CALLBACK(delete_callback), (gpointer)ret);

  gtk_window_set_title(GTK_WINDOW(ret->window),
                       TR("Plugins"));
  
  notebook = gtk_notebook_new();
  
  ret->input_plugins =
    bg_gtk_plugin_widget_multi_create(reg,
                                      BG_PLUGIN_INPUT,
                                      BG_PLUGIN_FILE|
                                      BG_PLUGIN_URL|
                                      BG_PLUGIN_REMOVABLE |
                                      BG_PLUGIN_TUNER);
  
  label = gtk_label_new(TR("Input"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           bg_gtk_plugin_widget_multi_get_widget(ret->input_plugins),
                           label);

  ret->audio_output_plugins =
    bg_gtk_plugin_widget_single_create(TR("Audio"), reg,
                                       BG_PLUGIN_OUTPUT_AUDIO,
                                       BG_PLUGIN_PLAYBACK);
  bg_gtk_plugin_widget_single_set_change_callback(ret->audio_output_plugins, set_audio_output, m);
  
  ret->video_output_plugins =
    bg_gtk_plugin_widget_single_create("Video", reg,
                                       BG_PLUGIN_OUTPUT_VIDEO,
                                       BG_PLUGIN_PLAYBACK);
  bg_gtk_plugin_widget_single_set_change_callback(ret->video_output_plugins,
                                                  set_video_output, m);

  ret->vis_plugins =
    bg_gtk_plugin_widget_single_create("Visualization", reg,
                                       BG_PLUGIN_VISUALIZATION,
                                       BG_PLUGIN_VISUALIZE_FRAME |
                                       BG_PLUGIN_VISUALIZE_GL);
  
  bg_gtk_plugin_widget_single_set_change_callback(ret->vis_plugins,
                                                  set_visualization, m);
  
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
  bg_gtk_plugin_widget_single_attach(ret->vis_plugins,
                                     table,
                                     &row, &num_columns);
  
  gtk_widget_show(table);
  
  label = gtk_label_new(TR("Output"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           table, label);
  gtk_widget_show(notebook);
  gtk_container_add(GTK_CONTAINER(ret->window),
                    notebook);

  gmerlin_mozilla_set_oa_plugin(m,
                                bg_gtk_plugin_widget_single_get_plugin(ret->audio_output_plugins));
  gmerlin_mozilla_set_ov_plugin(m,
                                bg_gtk_plugin_widget_single_get_plugin(ret->video_output_plugins));

  gmerlin_mozilla_set_vis_plugin(m,
                                 bg_gtk_plugin_widget_single_get_plugin(ret->vis_plugins));
  

  
  return ret;
  }

void bg_mozilla_plugin_window_show(plugin_window_t * w)
  {
  gtk_widget_show(w->window);
  }
