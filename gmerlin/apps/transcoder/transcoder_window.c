#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>



#include <config.h>
#include <pluginregistry.h>
#include <utils.h>

#include <cfg_dialog.h>

#include <gui_gtk/display.h>


#include "transcoder_window.h"
#include "tracklist.h"

#include "pluginwindow.h"

struct transcoder_window_s
  {
  GtkWidget * win;

  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;

  track_list_t * tracklist;

  GtkWidget * run_button;
  GtkWidget * stop_button;
  GtkWidget * preferences_button;
  GtkWidget * properties_button;
  GtkWidget * quit_button;

  GtkWidget * progress_bar;
  GtkWidget * status_bar;
  bg_gtk_time_display_t * time_remaining;

  plugin_window_t * plugin_window;
  
  /* Configuration stuff */

  char * output_directory;
  int delete_incomplete;
  
  };


void transcoder_window_set_audio_encoder(transcoder_window_t * t,
                                         bg_plugin_handle_t * h)
  {
  track_list_set_audio_encoder(t->tracklist, h);
  }

void transcoder_window_set_video_encoder(transcoder_window_t * t,
                                         bg_plugin_handle_t * h)
  {
  track_list_set_video_encoder(t->tracklist, h);
  }

static void
transcoder_window_preferences(transcoder_window_t * win);

static void plugin_window_close_notify(plugin_window_t * w,
                                       void * data)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;

  gtk_widget_set_sensitive(win->preferences_button, 1);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;

  if(w == win->run_button)
    {
    fprintf(stderr, "Run Button\n");
    }
  else if(w == win->stop_button)
    {
    fprintf(stderr, "Stop Button\n");
    
    }
  else if(w == win->preferences_button)
    {
    fprintf(stderr, "Preferences Button\n");
    gtk_widget_set_sensitive(win->preferences_button, 0);
    plugin_window_show(win->plugin_window);
    }
  else if(w == win->quit_button)
    {
    fprintf(stderr, "Quit Button\n");
    gtk_widget_hide(win->win);
    gtk_main_quit();
    }
  else if(w == win->properties_button)
    {
    fprintf(stderr, "Properties Button\n");
    transcoder_window_preferences(win);
    }
  }

static GtkWidget * create_stock_button(transcoder_window_t * win,
                                       const char * stock_id)
  {
  GtkWidget * ret;
  GtkWidget * image;
  image = gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_LARGE_TOOLBAR);
  ret = gtk_button_new();
  gtk_widget_show(image);
  gtk_container_add(GTK_CONTAINER(ret), image);

  g_signal_connect(G_OBJECT(ret), "clicked", G_CALLBACK(button_callback),
                   win);
  gtk_widget_show(ret);
  return ret;
  }

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
  GtkWidget * box;
  GtkWidget * label;
  char * tmp_path;
  transcoder_window_t * ret;
  bg_cfg_section_t * cfg_section;
  
  
  ret = calloc(1, sizeof(*ret));
  
  /* Create window */
  
  ret->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(ret->win), GTK_WIN_POS_CENTER);
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

  /* Create buttons */

  ret->run_button  = create_stock_button(ret,
                                         GTK_STOCK_EXECUTE);
  ret->stop_button = create_stock_button(ret,
                                         GTK_STOCK_STOP);
  ret->preferences_button = create_stock_button(ret,
                                                GTK_STOCK_PREFERENCES);
  ret->properties_button = create_stock_button(ret,
                                               GTK_STOCK_PROPERTIES);
  ret->quit_button = create_stock_button(ret,
                                         GTK_STOCK_QUIT);
  
  /* Progress bar */
  ret->progress_bar = gtk_progress_bar_new();
  gtk_widget_show(ret->progress_bar);

  /* Time display */
  ret->time_remaining = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL);

  /* Status bar */

  ret->status_bar = gtk_statusbar_new();
  gtk_widget_show(ret->status_bar);
  
  /* Pack everything */
  
  main_table = gtk_table_new(4, 1, 0);
  gtk_container_set_border_width(GTK_CONTAINER(main_table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(main_table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(main_table), 5);

  box = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->run_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->stop_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->preferences_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->properties_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->quit_button, FALSE, FALSE, 0);
  gtk_widget_show(box);
  gtk_table_attach(GTK_TABLE(main_table),
                   box,
                   0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  
  frame = gtk_frame_new("Track queue");
  gtk_container_add(GTK_CONTAINER(frame),
                    track_list_get_widget(ret->tracklist));

  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(main_table),
                            frame,
                            0, 1, 1, 2);

  box = gtk_hbox_new(0, 0);

  gtk_box_pack_start_defaults(GTK_BOX(box), ret->progress_bar);

  label = gtk_label_new("Remaining time");
  gtk_widget_show(label);
  
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box),
                     bg_gtk_time_display_get_widget(ret->time_remaining),
                     FALSE, FALSE, 0);

  gtk_widget_show(box);
  gtk_table_attach(GTK_TABLE(main_table),
                   box,
                   0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(main_table),
                   ret->status_bar,
                   0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);

  
  gtk_widget_show(main_table);
  gtk_container_add(GTK_CONTAINER(ret->win), main_table);

  ret->plugin_window =
    plugin_window_create(ret->plugin_reg,
                         ret,
                         plugin_window_close_notify,
                         ret);
  
  return ret;
  }

void transcoder_window_destroy(transcoder_window_t* w)
  {
  char * tmp_path;
  tmp_path =  bg_search_file_write("transcoder", "config.xml");
 
  bg_cfg_registry_save(w->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
   
  bg_cfg_registry_destroy(w->cfg_reg);

  free(w);
  }

void transcoder_window_run(transcoder_window_t * w)
  {
  gtk_widget_show(w->win);
  gtk_main();
  }

/* Configuration stuff */


static bg_parameter_info_t parameters[] =
  {
    {
      name:      "output_directory",
      long_name: "Output Directory",
      type:      BG_PARAMETER_DIRECTORY,
      val_default: { val_str: "." },
    },
    {
      name:        "delete_incomplete",
      long_name:   "Delete incomplete output files",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
    },
    { /* End of parameters */ }
  };

static void set_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  transcoder_window_t * w = (transcoder_window_t*)data;

  if(!name)
    return;

  if(!strcmp(name, "output_directory"))
    {
    w->output_directory = bg_strdup(w->output_directory, val->val_str);
    }
  else if(!strcmp(name, "delete_incomplete"))
    {
    w->delete_incomplete = val->val_i;
    }
  }

static void transcoder_window_preferences(transcoder_window_t * w)
  {
  bg_dialog_t * dlg;
  bg_cfg_section_t * cfg_section;
  
  cfg_section     = bg_cfg_registry_find_section(w->cfg_reg, "general");


  dlg = bg_dialog_create(cfg_section,
                         set_parameter,
                         w,
                         parameters,
                         "Transcoder configuration");

  bg_dialog_show(dlg);
  bg_dialog_destroy(dlg);
  
  }


