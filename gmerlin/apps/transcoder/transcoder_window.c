#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>



#include <config.h>
#include <pluginregistry.h>
#include <utils.h>

#include <cfg_dialog.h>
#include <transcoder_track.h>
#include <transcoder.h>

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

  bg_cfg_section_t * track_defaults_section;

  /* The actual transcoder */

  bg_transcoder_t * transcoder;
  bg_transcoder_track_t * transcoder_track;
  const bg_transcoder_info_t * transcoder_info;
  };

static void
transcoder_window_preferences(transcoder_window_t * win);

static void plugin_window_close_notify(plugin_window_t * w,
                                       void * data)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;

  gtk_widget_set_sensitive(win->preferences_button, 1);
  }

static void finish_transcoding(transcoder_window_t * win)
  {
  bg_transcoder_destroy(win->transcoder);
  win->transcoder = (bg_transcoder_t*)0;

  bg_transcoder_track_destroy(win->transcoder_track);
  win->transcoder_track = (bg_transcoder_track_t*)0;
  }

static gboolean idle_callback(gpointer data)
  {
  transcoder_window_t * win;
  win = (transcoder_window_t*)data;
  
  if(!bg_transcoder_iteration(win->transcoder))
    {
    /* Finish transcoding */
    finish_transcoding(win);

    bg_gtk_time_display_update(win->time_remaining, GAVL_TIME_UNDEFINED);
    
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), 0.0);
    
    fprintf(stderr, "Transcoding done\n");
    return FALSE;
    }

  /* Update status */

  bg_gtk_time_display_update(win->time_remaining,
                             win->transcoder_info->remaining_time);
  
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), win->transcoder_info->percentage_done);
  return TRUE;
  }

static void start_transcode(transcoder_window_t * win)
  {
  bg_cfg_section_t * cfg_section;
  
  cfg_section = bg_cfg_registry_find_section(win->cfg_reg, "output");

  win->transcoder_track      = track_list_get_track(win->tracklist);
  if(!win->transcoder_track)
    return;
  
  win->transcoder            = bg_transcoder_create();

  win->transcoder_info = bg_transcoder_get_info(win->transcoder);
  
  bg_cfg_section_apply(cfg_section, bg_transcoder_get_parameters(),
                       bg_transcoder_set_parameter, win->transcoder);

  if(!bg_transcoder_init(win->transcoder,
                         win->plugin_reg, win->transcoder_track))
    {
    fprintf(stderr, "Failed to initialize transcoder\n");
    return;
    }
  fprintf(stderr, "Initialized transcoder\n");

  g_idle_add(idle_callback, win);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;

  if(w == win->run_button)
    {
    fprintf(stderr, "Run Button\n");
    start_transcode(win);
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

  ret->track_defaults_section = bg_cfg_registry_find_section(ret->cfg_reg, "track_defaults");
  ret->tracklist = track_list_create(ret->plugin_reg, ret->track_defaults_section);

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
  bg_gtk_time_display_update(ret->time_remaining, GAVL_TIME_UNDEFINED);

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

  track_list_destroy(w->tracklist);

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

static void transcoder_window_preferences(transcoder_window_t * w)
  {
  bg_dialog_t * dlg;
  bg_cfg_section_t * cfg_section;
  

  dlg = bg_dialog_create_multi("Transcoder configuration");

  cfg_section     = bg_cfg_registry_find_section(w->cfg_reg, "output");

  bg_dialog_add(dlg,
                "Output options",
                cfg_section,
                NULL,
                w,
                bg_transcoder_get_parameters());

  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section, "audio");
  
  bg_dialog_add(dlg,
                "Audio options",
                cfg_section,
                NULL,
                NULL,
                bg_transcoder_track_audio_get_general_parameters());

  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section, "video");
  
  bg_dialog_add(dlg,
                "Video options",
                cfg_section,
                NULL,
                NULL,
                bg_transcoder_track_video_get_general_parameters());

    
  
  bg_dialog_show(dlg);
  bg_dialog_destroy(dlg);
  
  }


