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
#include <gui_gtk/scrolltext.h>

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
  GtkWidget * load_button;
  GtkWidget * save_button;

  GtkWidget * progress_bar;
  bg_gtk_time_display_t * time_remaining;
  bg_gtk_scrolltext_t   * scrolltext;
  
  plugin_window_t * plugin_window;
  
  /* Configuration stuff */

  char * output_directory;
  int delete_incomplete;

  bg_cfg_section_t * track_defaults_section;

  /* The actual transcoder */

  bg_transcoder_t * transcoder;
  bg_transcoder_track_t * transcoder_track;
  const bg_transcoder_info_t * transcoder_info;
  
  guint idle_tag;

  float fg_color[3];
  float fg_color_e[3]; /* Foreground color for error messages */
  float bg_color[3];
  
  /* Load/Save stuff */
  
  char * task_path;
  char * task_file;
    
  GtkWidget * task_filesel;
  
  };


static int start_transcode(transcoder_window_t * win);

static bg_parameter_info_t transcoder_window_parameters[] =
  {
    {
      name:        "display",
      long_name:   "Display",
      type:        BG_PARAMETER_SECTION,
    },
    {
      name:        "display_foreground",
      long_name:   "Foreground",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 1.0, 1.0, 0.0, 1.0 } }
    },
    {
      name:        "display_foreground_error",
      long_name:   "Error foreground",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 1.0, 0.0, 0.0, 1.0 } }
    },
    {
      name:        "display_background",
      long_name:   "Background",
      type:        BG_PARAMETER_COLOR_RGB,
      val_default: { val_color: (float[]){ 0.0, 0.0, 0.0, 1.0 } }
    },
    {
      name:        "display_font",
      long_name:   "Font",
      type:        BG_PARAMETER_FONT,
      val_default: { val_str: "Sans Bold 10" }
    },
    {
      name:        "task_path",
      long_name:   "Task path",
      type:        BG_PARAMETER_DIRECTORY,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_str: "." },
    },
    { /* End of parameters */ }
  };

static void
set_transcoder_window_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;

  if(!name)
    {
    bg_gtk_scrolltext_set_colors(win->scrolltext, win->fg_color, win->bg_color);
    bg_gtk_time_display_set_colors(win->time_remaining, win->fg_color, win->bg_color);
    track_list_set_display_colors(win->tracklist, win->fg_color, win->bg_color);
    return;
    }

  if(!strcmp(name, "task_path"))
    {
    win->task_path = bg_strdup(win->task_path, val->val_str);
    }
  else if(!strcmp(name, "display_foreground"))
    {
    memcpy(win->fg_color, val->val_color, 3 * sizeof(float));
    }
  else if(!strcmp(name, "display_foreground_error"))
    {
    memcpy(win->fg_color_e, val->val_color, 3 * sizeof(float));
    }
  else if(!strcmp(name, "display_background"))
    {
    memcpy(win->bg_color, val->val_color, 3 * sizeof(float));
    }
  else if(!strcmp(name, "display_font"))
    {
    bg_gtk_scrolltext_set_font(win->scrolltext, val->val_str);
    }
  }

static int
get_transcoder_window_parameter(void * data, char * name, bg_parameter_value_t * val)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;

  if(!name)
    return 1;

  if(!strcmp(name, "task_path"))
    {
    val->val_str = bg_strdup(val->val_str, win->task_path);
    return 1;
    }
  return 0;
  }

  
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

  if(win->transcoder_track)
    {
    bg_transcoder_track_destroy(win->transcoder_track);
    win->transcoder_track = (bg_transcoder_track_t*)0;
    }
  
  g_source_remove(win->idle_tag);
  win->idle_tag = 0;

  }

static gboolean idle_callback(gpointer data)
  {
  transcoder_window_t * win;
  win = (transcoder_window_t*)data;

  /* If the transcoder isn't there, it means that we were interrupted */

  if(!win->transcoder)
    return FALSE;
  
  if(!bg_transcoder_iteration(win->transcoder))
    {
    /* Finish transcoding */
    finish_transcoding(win);

    bg_gtk_time_display_update(win->time_remaining, GAVL_TIME_UNDEFINED);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), 0.0);

    fprintf(stderr, "Transcoding done\n");

    if(!start_transcode(win))
      {
      gtk_widget_set_sensitive(win->run_button, 1);
      gtk_widget_set_sensitive(win->stop_button, 0);

      return FALSE;
      }
    else
      {
      win->idle_tag = g_idle_add(idle_callback, win);
      return FALSE;
      }
    }

  /* Update status */

  //  fprintf(stderr, "bg_gtk_time_display_update...");
  bg_gtk_time_display_update(win->time_remaining,
                             win->transcoder_info->remaining_time);
  
  //  fprintf(stderr, "done\n");
  
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar),
                                win->transcoder_info->percentage_done);
  return TRUE;
  }

static int start_transcode(transcoder_window_t * win)
  {
  char * name, * message;
  bg_cfg_section_t * cfg_section;

  const char * error_msg;
  
  cfg_section = bg_cfg_registry_find_section(win->cfg_reg, "output");

  win->transcoder_track      = track_list_get_track(win->tracklist);
  if(!win->transcoder_track)
    {
    bg_gtk_scrolltext_set_text(win->scrolltext, "Gmerlin transcoder version "VERSION,
                               win->fg_color, win->bg_color);
    return 0;
    }
  win->transcoder            = bg_transcoder_create();

  win->transcoder_info = bg_transcoder_get_info(win->transcoder);
  
  bg_cfg_section_apply(cfg_section, bg_transcoder_get_parameters(),
                       bg_transcoder_set_parameter, win->transcoder);

  if(!bg_transcoder_init(win->transcoder,
                         win->plugin_reg, win->transcoder_track))
    {
    error_msg = bg_transcoder_get_error(win->transcoder);
    
    if(error_msg)
      bg_gtk_scrolltext_set_text(win->scrolltext, error_msg, win->fg_color_e, win->bg_color);
    else
      bg_gtk_scrolltext_set_text(win->scrolltext, "Failed to initialize transcoder",
                                 win->fg_color_e, win->bg_color);
    
    track_list_prepend_track(win->tracklist, win->transcoder_track);
    win->transcoder_track = (bg_transcoder_track_t*)0;

    bg_transcoder_destroy(win->transcoder);
    win->transcoder = (bg_transcoder_t*)0;
    return 0;
    }
  fprintf(stderr, "Initialized transcoder\n");

  name = bg_transcoder_track_get_name(win->transcoder_track);
  message = bg_sprintf("Transcoding %s", name);

  bg_gtk_scrolltext_set_text(win->scrolltext, message, win->fg_color, win->bg_color);
  free(message);
  free(name);
    
  gtk_widget_set_sensitive(win->run_button, 0);
  gtk_widget_set_sensitive(win->stop_button, 1);
  
  return 1;
  }

static void task_filesel_button_callback(GtkWidget * w, gpointer * data)
  {
  const char * end_pos;

  GtkFileSelection * filesel;
  transcoder_window_t * win = (transcoder_window_t *)data;
  
  filesel = GTK_FILE_SELECTION(win->task_filesel);

  if(w == filesel->ok_button)
    {
    win->task_file = bg_strdup((char*)0,
                                   gtk_file_selection_get_filename(filesel));
    gtk_widget_hide(win->task_filesel);

    end_pos = strrchr(win->task_file, '/');
    if(end_pos)
      {
      end_pos++;
      win->task_path = bg_strndup(win->task_path, win->task_file, end_pos);
      }
    gtk_main_quit();
    }
  else if((w == win->task_filesel) || (w == filesel->cancel_button))
    {
    gtk_widget_hide(win->task_filesel);
    gtk_main_quit();
    }
  }

static gboolean task_filesel_delete_callback(GtkWidget * w, GdkEventAny * event,
                                         gpointer * data)
  {
  task_filesel_button_callback(w, data);
  return TRUE;
  }



static GtkWidget * create_task_filesel(transcoder_window_t * win)
  {
  GtkWidget * ret;

  ret = gtk_file_selection_new("");

  gtk_window_set_modal(GTK_WINDOW(ret), 1);
  
  g_signal_connect(G_OBJECT(ret), "delete-event",
                   G_CALLBACK(task_filesel_delete_callback), win);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(ret)->ok_button),
                   "clicked", G_CALLBACK(task_filesel_button_callback), win);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(ret)->cancel_button),
                   "clicked", G_CALLBACK(task_filesel_button_callback), win);

  if(win->task_path)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(ret), win->task_path);
  
  return ret;
  }  

static void button_callback(GtkWidget * w, gpointer data)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;

  if(w == win->run_button)
    {
    //    fprintf(stderr, "Run Button\n");
    if(start_transcode(win))
      win->idle_tag = g_idle_add(idle_callback, win);
    }
  else if(w == win->stop_button)
    {
    //    fprintf(stderr, "Stop Button\n");
    
    track_list_prepend_track(win->tracklist, win->transcoder_track);
    win->transcoder_track = (bg_transcoder_track_t*)0;
    
    finish_transcoding(win);
    gtk_widget_set_sensitive(win->run_button, 1);
    gtk_widget_set_sensitive(win->stop_button, 0);

    bg_gtk_time_display_update(win->time_remaining, GAVL_TIME_UNDEFINED);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), 0.0);

    bg_gtk_scrolltext_set_text(win->scrolltext,
                               "Gmerlin transcoder version "VERSION,
                               win->fg_color, win->bg_color);
    
    }
  else if(w == win->load_button)
    {
    //    fprintf(stderr, "Load Button\n");
    
    if(!win->task_filesel)
      win->task_filesel = create_task_filesel(win);
    
    gtk_window_set_title(GTK_WINDOW(win->task_filesel), "Load task list");
    
    gtk_widget_show(win->task_filesel);
    gtk_main();

    if(win->task_file)
      {
      track_list_load(win->tracklist, win->task_file);
      free(win->task_file);
      win->task_file = (char*)0;
      }
    }
  else if(w == win->save_button)
    {
    //    fprintf(stderr, "Save Button\n");

    if(!win->task_filesel)
      win->task_filesel = create_task_filesel(win);
    
    gtk_window_set_title(GTK_WINDOW(win->task_filesel), "Save task list");

    gtk_widget_show(win->task_filesel);
    gtk_main();

    if(win->task_file)
      {
      track_list_save(win->tracklist, win->task_file);
      free(win->task_file);
      win->task_file = (char*)0;
      }
    }
  else if(w == win->preferences_button)
    {
    //    fprintf(stderr, "Preferences Button\n");
    gtk_widget_set_sensitive(win->preferences_button, 0);
    plugin_window_show(win->plugin_window);
    }
  else if(w == win->quit_button)
    {
    //    fprintf(stderr, "Quit Button\n");
    gtk_widget_hide(win->win);
    gtk_main_quit();
    }
  else if(w == win->properties_button)
    {
    //    fprintf(stderr, "Properties Button\n");
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
  ret->load_button  = create_stock_button(ret,
                                          GTK_STOCK_OPEN);
  ret->save_button  = create_stock_button(ret,
                                          GTK_STOCK_SAVE);

  gtk_widget_set_sensitive(ret->stop_button, 0);
  
  /* Progress bar */
  ret->progress_bar = gtk_progress_bar_new();
  gtk_widget_show(ret->progress_bar);

  /* Time display */
  ret->time_remaining = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL, 4);
  bg_gtk_time_display_update(ret->time_remaining, GAVL_TIME_UNDEFINED);

  /* Scrolltext */

  ret->scrolltext = bg_gtk_scrolltext_create(100, 24);
  
  /* Pack everything */
  
  main_table = gtk_table_new(4, 1, 0);
  gtk_container_set_border_width(GTK_CONTAINER(main_table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(main_table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(main_table), 5);

  box = gtk_hbox_new(0, 0);
  
  gtk_box_pack_start(GTK_BOX(box), ret->load_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->save_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->run_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->stop_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->preferences_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->properties_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->quit_button, FALSE, FALSE, 0);
  gtk_widget_show(box);
  gtk_table_attach(GTK_TABLE(main_table),
                   box,
                   0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  box = gtk_hbox_new(0, 0);

  gtk_box_pack_end(GTK_BOX(box),
                   bg_gtk_time_display_get_widget(ret->time_remaining),
                   FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(box),
                              bg_gtk_scrolltext_get_widget(ret->scrolltext));

  gtk_widget_show(box);
  
  gtk_table_attach(GTK_TABLE(main_table),
                   box,
                   0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(main_table),
                   ret->progress_bar,
                   0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  frame = gtk_frame_new("Track queue");
  gtk_container_add(GTK_CONTAINER(frame),
                    track_list_get_widget(ret->tracklist));

  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(main_table),
                            frame,
                            0, 1, 3, 4);
  
  gtk_widget_show(main_table);
  gtk_container_add(GTK_CONTAINER(ret->win), main_table);

  ret->plugin_window =
    plugin_window_create(ret->plugin_reg,
                         ret,
                         plugin_window_close_notify,
                         ret);

  bg_gtk_scrolltext_set_text(ret->scrolltext, "Gmerlin transcoder version "VERSION,
                             ret->fg_color, ret->bg_color);

  /* Apply config stuff */


  cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "transcoder_window");
  bg_cfg_section_apply(cfg_section, transcoder_window_parameters,
                       set_transcoder_window_parameter, ret);
  

  
  return ret;
  }

void transcoder_window_destroy(transcoder_window_t* w)
  {
  char * tmp_path;
  bg_cfg_section_t * cfg_section;
  
  cfg_section = bg_cfg_registry_find_section(w->cfg_reg, "transcoder_window");
  bg_cfg_section_get(cfg_section, transcoder_window_parameters,
                       get_transcoder_window_parameter, w);
  

  
  track_list_destroy(w->tracklist);

  tmp_path =  bg_search_file_write("transcoder", "config.xml");
 
  bg_cfg_registry_save(w->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);



  
  bg_cfg_registry_destroy(w->cfg_reg);

  if(w->task_path)
    free(w->task_path);
  
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
  
  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "audio");
  
  bg_dialog_add(dlg,
                "Audio defaults",
                cfg_section,
                NULL,
                NULL,
                bg_transcoder_track_audio_get_general_parameters());

  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "video");
  
  bg_dialog_add(dlg,
                "Video defaults",
                cfg_section,
                NULL,
                NULL,
                bg_transcoder_track_video_get_general_parameters());

  cfg_section = bg_cfg_registry_find_section(w->cfg_reg,
                                             "transcoder_window");

  bg_dialog_add(dlg,
                "Window",
                cfg_section,
                set_transcoder_window_parameter,
                w,
                transcoder_window_parameters);

  bg_dialog_show(dlg);
  bg_dialog_destroy(dlg);
  
  }


