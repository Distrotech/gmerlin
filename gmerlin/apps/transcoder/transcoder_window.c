#include <stdlib.h>
#include <gtk/gtk.h>

#include <config.h>
#include <pluginregistry.h>
#include <utils.h>

#include "transcoder_window.h"
#include "tracklist.h"

struct transcoder_window_s
  {
  GtkWidget * win;

  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;

  track_list_t * tracklist;
  };

static gboolean delete_callback(GtkWidget * w, GdkEvent * evt,
                                gpointer data)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;
  gtk_widget_hide(win->win);
  gtk_main_quit();
  return TRUE;
  }

transcoder_window_t * transcoder_window_create()
  {
  GtkWidget * main_table;
  GtkWidget * frame;
  char * tmp_path;
  transcoder_window_t * ret;
  bg_cfg_section_t * cfg_section;
  
  ret = calloc(1, sizeof(*ret));
  
  /* Create window */
  
  ret->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(ret->win),
                       "Gmerlin transcoder "VERSION);
  g_signal_connect(G_OBJECT(ret->win), "delete_event",
                   G_CALLBACK(delete_callback),
                   ret);

  /* Create config registry */

  ret->cfg_reg = bg_cfg_registry_create();
  tmp_path = bg_search_file_read("transcoder", "config.xml");
  bg_cfg_registry_load(ret->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  /* Create plugin registry */
  
  cfg_section     = bg_cfg_registry_find_section(ret->cfg_reg, "plugins");
  ret->plugin_reg = bg_plugin_registry_create(cfg_section);

  /* Create track list */

  ret->tracklist = track_list_create(ret->plugin_reg);

  /* Pack everything */

  main_table = gtk_table_new(1, 1, 0);
  gtk_container_set_border_width(GTK_CONTAINER(main_table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(main_table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(main_table), 5);

  frame = gtk_frame_new("Track queue");
  gtk_container_add(GTK_CONTAINER(frame),
                    track_list_get_widget(ret->tracklist));

  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(main_table),
                            frame,
                            0, 1, 0, 1);
  gtk_widget_show(main_table);
  gtk_container_add(GTK_CONTAINER(ret->win), main_table);
  
  return ret;
  }

void transcoder_window_destroy(transcoder_window_t* w)
  {
  free(w);
  }

void transcoder_window_run(transcoder_window_t * w)
  {
  gtk_widget_show(w->win);
  gtk_main();
  }

