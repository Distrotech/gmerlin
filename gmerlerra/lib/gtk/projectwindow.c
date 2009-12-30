#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>

#include <config.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/pluginregistry.h>


#include <track.h>
#include <project.h>
#include <editops.h>

#include <gui_gtk/timerange.h>
#include <gui_gtk/projectwindow.h>
#include <gui_gtk/timeline.h>
#include <gui_gtk/mediabrowser.h>
#include <gui_gtk/playerwidget.h>

#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/gui_gtk/fileselect.h>
#include <gmerlin/gui_gtk/display.h>
#include <gmerlin/gui_gtk/logwindow.h>

#include <gmerlin/utils.h>
#include <gmerlin/cfg_dialog.h>

#include <gdk/gdkkeysyms.h>

static char * project_path = (char*)0;

/* List of all open windows */
static GList * project_windows = NULL;

static bg_gtk_log_window_t * log_window = NULL;

static void configure_global(GtkWidget * w, bg_plugin_registry_t * plugin_reg);

/* Global configuration settings */

int bg_nle_snap_to_frames;
int bg_nle_play_every_frame;

static bg_cfg_registry_t * cfg_reg = NULL;

static bg_cfg_section_t  * log_window_section = NULL;
static bg_cfg_section_t  * display_section = NULL;
static bg_cfg_section_t  * oa_section = NULL;
static bg_cfg_section_t  * ov_section = NULL;

static bg_parameter_info_t * input_plugin_parameters = NULL;
static bg_parameter_info_t * image_reader_parameters = NULL;

static bg_parameter_info_t * oa_parameters = NULL;
static bg_parameter_info_t * ov_parameters = NULL;


typedef struct
  {
  GtkWidget * load;
  GtkWidget * load_media;
  GtkWidget * new;
  GtkWidget * save;
  GtkWidget * save_as;
  GtkWidget * set_default;
  GtkWidget * settings;
  GtkWidget * close;
  GtkWidget * quit;
  GtkWidget * menu;
  } project_menu_t;

typedef struct
  {
  GtkWidget * add_audio;
  GtkWidget * add_video;

  GtkWidget * menu;
  } track_menu_t;

typedef struct
  {
  GtkWidget * add_audio;
  GtkWidget * add_video;
  GtkWidget * menu;
  } outstream_menu_t;


typedef struct
  {
  GtkWidget * undo;
  GtkWidget * redo;
  
  GtkWidget * cut;
  GtkWidget * copy;
  GtkWidget * paste;
  GtkWidget * menu;
  } edit_menu_t;

typedef struct
  {
  GtkWidget * messages;
  guint messages_id;
  
  GtkWidget * menu;
  } view_menu_t;

typedef struct
  {
  GtkWidget * options;
  GtkWidget * snap_to_frames;
  guint snap_to_frames_id;
  
  GtkWidget * play_every_frame;
  guint play_every_frame_id;
  
  GtkWidget * menu;
  } options_menu_t;



struct bg_nle_project_window_s
  {
  GtkWidget * win;
  GtkWidget * menubar;
  project_menu_t project_menu;
  track_menu_t track_menu;
  outstream_menu_t outstream_menu;
  edit_menu_t edit_menu;
  view_menu_t view_menu;
  options_menu_t options_menu;
  
  bg_nle_project_t * p;

  bg_nle_timeline_t * timeline;
  bg_nle_media_browser_t * media_browser;
  
  char * filename;
  GtkWidget * notebook;

  bg_nle_player_widget_t * compositor;

  bg_gtk_time_display_t * time_display;
  GtkWidget * statusbar;
  GtkWidget * progressbar;

  GtkAccelGroup * accel_group;
  
  };

/* Configuration stuff */

static const bg_parameter_info_t display_parameters[] =
  {
    {
      .name = "view_audio_envelope",
      .long_name = "Audio envelope",
      .type = TRS(BG_PARAMETER_CHECKBUTTON),
      .val_default = { .val_i = 1 },
    },
    {
      .name = "view_video_pictures",
      .long_name =TRS("Video thumbnails"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name = "time_unit",
      .long_name =TRS("Time unit"),
      .type = BG_PARAMETER_STRINGLIST,
      .val_default = { .val_str = "milliseconds" },
      .multi_names = (const char*[]){ "milliseconds", "timecode", "framecount", (char*)0 },
      .multi_labels = (const char*[]){ TRS("Milliseconds"), TRS("Timecode"), TRS("Framecount"),
                                       (char*)0 },
    },
    {
      .name = "display_fg",
      .long_name =TRS("Display foreground"),
      .type = BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 0.0, 1.0, 0.0 } },
    },
    {
      .name = "display_bg",
      .long_name =TRS("Display background"),
      .type = BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 0.0, 0.0, 0.0 } },
    },
    {
      .name = "snap_to_frames",
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 1 },
    },
    {
      .name = "play_every_frame",
      .type = BG_PARAMETER_CHECKBUTTON,
      .flags = BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 0 },
    },
    { /* */ }
  };

typedef struct
  {
  const char * name;
  const bg_parameter_value_t * val;
  bg_set_parameter_func_t func;
  } set_parameteter_data_t;

static void
set_parameter_wrapper(void * data, void * user_data)
  {
  set_parameteter_data_t * d = user_data;
  d->func(data, d->name, d->val);
  }

static void
set_display_parameter(void * priv, const char * name,
                      const bg_parameter_value_t * val)
  {
  bg_nle_project_window_t * w = priv;

  if(name)
    {
    if(!strcmp(name, "snap_to_frames"))
      {
      g_signal_handler_block(G_OBJECT(w->options_menu.snap_to_frames),
                             w->options_menu.snap_to_frames_id);
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w->options_menu.snap_to_frames),
                                 !!val->val_i);
      g_signal_handler_unblock(G_OBJECT(w->options_menu.snap_to_frames),
                               w->options_menu.snap_to_frames_id);
      }
    else if(!strcmp(name, "play_every_frame"))
      {
      g_signal_handler_block(G_OBJECT(w->options_menu.play_every_frame),
                             w->options_menu.play_every_frame_id);
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w->options_menu.play_every_frame),
                                 !!val->val_i);
      g_signal_handler_unblock(G_OBJECT(w->options_menu.play_every_frame),
                               w->options_menu.play_every_frame_id);
      }
    }
  
  bg_nle_media_browser_set_display_parameter(w->media_browser, name, val);
  bg_nle_player_set_display_parameter(w->compositor,
                                      name, val);
  }

static void
set_display_parameter_global(void * priv, const char * name,
                             const bg_parameter_value_t * val)
  {
  set_parameteter_data_t d;

  if(name)
    {
    if(!strcmp(name, "snap_to_frames"))
      bg_nle_snap_to_frames = val->val_i;
    else if(!strcmp(name, "play_every_frame"))
      bg_nle_play_every_frame = val->val_i;
    }
  
  d.name = name;
  d.val = val;
  d.func = set_display_parameter;
  g_list_foreach(project_windows, set_parameter_wrapper, &d);
  
  }

static int
get_display_parameter_global(void * priv, const char * name,
                             bg_parameter_value_t * val)
  {

  if(name)
    {
    if(!strcmp(name, "snap_to_frames"))
      {
      val->val_i = bg_nle_snap_to_frames;
      fprintf(stderr, "snap_to_frames %d\n", val->val_i);
      return 1;
      }
    else if(!strcmp(name, "play_every_frame"))
      {
      val->val_i = bg_nle_play_every_frame;
      fprintf(stderr, "play_every_frame %d\n", val->val_i);
      return 1;
      }
    }
  return 0;
  }


static void
set_oa_parameter(void * priv, const char * name,
                 const bg_parameter_value_t * val)
  {
  bg_nle_project_window_t * w = priv;
  bg_nle_player_set_oa_parameter(w->compositor, name, val);
  bg_nle_media_browser_set_oa_parameter(w->media_browser, name, val);

  }

static void
set_oa_parameter_global(void * priv, const char * name,
                             const bg_parameter_value_t * val)
  {
  set_parameteter_data_t d;
  d.name = name;
  d.val = val;
  d.func = set_oa_parameter;
  g_list_foreach(project_windows, set_parameter_wrapper, &d);
  }

static void
set_ov_parameter(void * priv, const char * name,
                 const bg_parameter_value_t * val)
  {
  bg_nle_project_window_t * w = priv;
  bg_nle_player_set_ov_parameter(w->compositor, name, val);
  bg_nle_media_browser_set_ov_parameter(w->media_browser, name, val);
  }

static void
set_ov_parameter_global(void * priv, const char * name,
                        const bg_parameter_value_t * val)
  {
  set_parameteter_data_t d;
  d.name = name;
  d.val = val;
  d.func = set_ov_parameter;
  g_list_foreach(project_windows, set_parameter_wrapper, &d);
  }


/* Log window stuff */

static void disable_log_item(void * data, void * user_data)
  {
  bg_nle_project_window_t * win = data;

  g_signal_handler_block(G_OBJECT(win->view_menu.messages),
                         win->view_menu.messages_id);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(win->view_menu.messages),
                                 0);
  g_signal_handler_unblock(G_OBJECT(win->view_menu.messages),
                           win->view_menu.messages_id);
  }

static void log_close_callback(bg_gtk_log_window_t* w, void * data)
  {
  g_list_foreach(project_windows, disable_log_item, NULL);
  }

static void enable_log_item(void * data, void * user_data)
  {
  bg_nle_project_window_t * win = data;

  g_signal_handler_block(G_OBJECT(win->view_menu.messages),
                         win->view_menu.messages_id);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(win->view_menu.messages),
                                 1);
  g_signal_handler_unblock(G_OBJECT(win->view_menu.messages),
                           win->view_menu.messages_id);
  }

static void log_open_callback(void)
  {
  g_list_foreach(project_windows, enable_log_item, NULL);
  }

static void edit_callback(bg_nle_project_t * p,
                          bg_nle_edit_op_t op,
                          void * op_data,
                          void * user_data)
  {
  bg_nle_project_window_t * win = user_data;

  switch(op)
    {
    case BG_NLE_EDIT_ADD_TRACK:
      {
      bg_nle_op_track_t * d = op_data;
      bg_nle_timeline_add_track(win->timeline, d->track, d->index);
      }
      break;
    case BG_NLE_EDIT_DELETE_TRACK:
      {
      bg_nle_op_track_t * d = op_data;
      bg_nle_timeline_delete_track(win->timeline, d->index);
      }
      break;
    case BG_NLE_EDIT_MOVE_TRACK:
      {
      bg_nle_op_move_track_t * d = op_data;
      bg_nle_timeline_move_track(win->timeline, d->old_index, d->new_index);
      }
      break;
    case BG_NLE_EDIT_ADD_OUTSTREAM:
      {
      bg_nle_op_outstream_t * d = op_data;
      bg_nle_timeline_add_outstream(win->timeline, d->outstream, d->index);
      }
      break;
    case BG_NLE_EDIT_DELETE_OUTSTREAM:
      {
      bg_nle_op_outstream_t * d = op_data;
      bg_nle_timeline_delete_outstream(win->timeline, d->index);
      }
      break;
    case BG_NLE_EDIT_MOVE_OUTSTREAM:
      {
      bg_nle_op_move_outstream_t * d = op_data;
      bg_nle_timeline_move_outstream(win->timeline, d->old_index, d->new_index);
      }
      break;
    case BG_NLE_EDIT_CHANGE_SELECTION:
      {
      bg_nle_op_change_range_t * d = op_data;
      bg_nle_timeline_set_selection(win->timeline, &d->new_range, d->new_cursor_pos);
      }
      break;
    case BG_NLE_EDIT_CHANGE_IN_OUT:
      {
      bg_nle_op_change_range_t * d = op_data;
      bg_nle_timeline_set_in_out(win->timeline, &d->new_range);
      }
      break;
    case BG_NLE_EDIT_SET_CURSOR_POS:
      {
      bg_nle_op_cursor_pos_t * d = op_data;
      bg_nle_timeline_set_cursor_pos(win->timeline, d->new_pos);
      }
      break;
    case BG_NLE_EDIT_CHANGE_VISIBLE:
      {
      bg_nle_op_change_range_t * d = op_data;
      bg_nle_timeline_set_visible(win->timeline, &d->new_range);
      }
      break;
    case BG_NLE_EDIT_CHANGE_ZOOM:
      {
      bg_nle_op_change_range_t * d = op_data;
      bg_nle_timeline_set_zoom(win->timeline, &d->new_range);
      }
      break;
    case BG_NLE_EDIT_TRACK_FLAGS:
      {
      bg_nle_op_track_flags_t * d = op_data;
      bg_nle_timeline_set_track_flags(win->timeline, d->track, d->new_flags);
      }
      break;
    case BG_NLE_EDIT_OUTSTREAM_FLAGS:
      {
      bg_nle_op_outstream_flags_t * d = op_data;
      bg_nle_timeline_set_outstream_flags(win->timeline, d->outstream, d->new_flags);
      }
      break;
    case BG_NLE_EDIT_OUTSTREAM_ATTACH_TRACK:
    case BG_NLE_EDIT_OUTSTREAM_DETACH_TRACK:
      /* Nothing ATM because the menu is updated whenever it is popped up */
      break;
    case BG_NLE_EDIT_OUTSTREAM_MAKE_CURRENT:
      bg_nle_timeline_outstreams_make_current(win->timeline);
      break;
    case BG_NLE_EDIT_PROJECT_PARAMETERS:
      /* Nothing */
      break;
    case BG_NLE_EDIT_TRACK_PARAMETERS:
      {
      bg_nle_op_parameters_t * d = op_data;
      bg_nle_timeline_update_track_parameters(win->timeline, d->index, d->new_section);
      }
      break;
    case BG_NLE_EDIT_OUTSTREAM_PARAMETERS:
      {
      bg_nle_op_parameters_t * d = op_data;
      bg_nle_timeline_update_outstream_parameters(win->timeline, d->index, d->new_section);
      }
      break;
    case BG_NLE_EDIT_ADD_FILE:
      {
      bg_nle_op_file_t * d = op_data;
      bg_nle_media_browser_add_file(win->media_browser, d->index);
      }
      break;
    case BG_NLE_EDIT_DELETE_FILE:
      {
      bg_nle_op_file_t * d = op_data;
      bg_nle_media_browser_delete_file(win->media_browser, d->index);
      }
      break;
    }
  
  }

static void show_settings_dialog(bg_nle_project_window_t * win)
  {
  bg_dialog_t * cfg_dialog;
  void * parent;
  void * child;
  int result;

  bg_cfg_section_t * s;

  s = bg_nle_project_set_parameters_start(win->p);
  
  cfg_dialog = bg_dialog_create_multi(TR("Project settings"));
  
  parent = bg_dialog_add_parent(cfg_dialog, NULL, TR("Audio"));
  
  child = bg_dialog_add_parent(cfg_dialog, parent, TR("Track format"));
  
  bg_dialog_add_child(cfg_dialog, child,
                      NULL,
                      bg_cfg_section_find_subsection(s, "audio_track"),
                      NULL,
                      NULL,
                      NULL,
                      bg_nle_track_audio_parameters);
  
  child = bg_dialog_add_parent(cfg_dialog, parent, TR("Compositing format"));
    
  bg_dialog_add_child(cfg_dialog, child,
                      NULL,
                      bg_cfg_section_find_subsection(s, "audio_outstream"),
                      NULL,
                      NULL,
                      NULL,
                      bg_nle_outstream_audio_parameters);

  parent = bg_dialog_add_parent(cfg_dialog, NULL, TR("Video"));

  child = bg_dialog_add_parent(cfg_dialog, parent, TR("Track format"));
  
  bg_dialog_add_child(cfg_dialog, child,
                      NULL,
                      bg_cfg_section_find_subsection(s, "video_track"),
                      NULL,
                      NULL,
                      NULL,
                      bg_nle_track_video_parameters);

  child = bg_dialog_add_parent(cfg_dialog, parent, TR("Compositing format"));

  bg_dialog_add_child(cfg_dialog, child,
                      NULL,
                      bg_cfg_section_find_subsection(s, "video_outstream"),
                      NULL,
                      NULL,
                      NULL,
                      bg_nle_outstream_video_parameters);

  bg_dialog_add(cfg_dialog,
                TR("Performance"),
                bg_cfg_section_find_subsection(s, "performance"),
                NULL,
                NULL,
                NULL,
                bg_nle_performance_parameters);

  bg_dialog_add(cfg_dialog,
                TR("Cache"),
                bg_cfg_section_find_subsection(s, "cache"),
                NULL,
                NULL,
                NULL,
                win->p->cache_parameters);
  
  result = bg_dialog_show(cfg_dialog, win->win);
  bg_dialog_destroy(cfg_dialog);
  
  bg_nle_project_set_parameters_end(win->p, s, result);
  }

static gboolean destroy_func(gpointer data)
  {
  bg_nle_project_window_t * win = data;

  bg_cfg_section_get(display_section, display_parameters, get_display_parameter_global, NULL);
  
  if(win->p->changed_flags)
    {
    /* Ask to save */
    }
  bg_nle_project_window_destroy(win);
  if(!project_windows)
    {
    gtk_main_quit();
    }
  return FALSE;
  }

static void menu_callback(GtkWidget * w, gpointer data)
  {
  bg_nle_project_window_t * win = data;
  bg_nle_project_window_t * new_win;
  
  if(w == win->project_menu.new)
    {
    new_win = bg_nle_project_window_create((char*)0, win->p->plugin_reg);
    bg_nle_project_window_show(new_win);
    }
  else if(w == win->project_menu.load)
    {
    char * filename;
    filename = bg_gtk_get_filename_read("Load project",
                                        &project_path, win->win);
    if(filename)
      {
      new_win = bg_nle_project_window_create(filename, win->p->plugin_reg);
      bg_nle_project_window_show(new_win);
      free(filename);

      /* If our project has no filename and is not changed,
         we can close ourselves */
      if(!win->filename && !win->p->changed_flags)
        {
        g_idle_add(destroy_func, win);
        }
      }
    }
  else if(w == win->project_menu.save)
    {
    if(win->filename)
      bg_nle_project_save(win->p, win->filename);
    else
      {
      char * filename;
      filename = bg_gtk_get_filename_write("Save project",
                                           &project_path, 1, win->win);
      if(filename)
        {
        bg_nle_project_save(win->p, filename);
        win->filename = bg_strdup(win->filename, filename);
        free(filename);
        }
      }
    }
  else if(w == win->project_menu.save_as)
    {
    char * filename;
    filename = bg_gtk_get_filename_write("Save project",
                                         &project_path, 1, win->win);
    if(filename)
      {
      bg_nle_project_save(win->p, filename);
      win->filename = bg_strdup(win->filename, filename);
      free(filename);
      }
    }
  else if(w == win->project_menu.close)
    {
    g_idle_add(destroy_func, win);
    }
  else if(w == win->project_menu.quit)
    {
    bg_cfg_section_get(display_section, display_parameters, get_display_parameter_global, NULL);
    gtk_main_quit();
    }
  else if(w == win->project_menu.settings)
    {
    show_settings_dialog(win);
    }
  else if(w == win->project_menu.load_media)
    {
    bg_nle_media_browser_load_files(win->media_browser);
    }
  else if(w == win->project_menu.set_default)
    {
    char * filename;
    filename = bg_search_file_write("gmerlerra", "default_project.xml");
    bg_nle_project_save(win->p, filename);
    free(filename);
    }
  else if(w == win->track_menu.add_audio)
    {
    bg_nle_project_add_audio_track(win->p);
    }
  else if(w == win->track_menu.add_video)
    {
    bg_nle_project_add_video_track(win->p);
    }
  else if(w == win->outstream_menu.add_audio)
    {
    bg_nle_project_add_audio_outstream(win->p);
    }
  else if(w == win->outstream_menu.add_video)
    {
    bg_nle_project_add_video_outstream(win->p);
    }
  else if(w == win->edit_menu.cut)
    {
    
    }
  else if(w == win->edit_menu.copy)
    {

    }
  else if(w == win->edit_menu.paste)
    {
    
    }
  else if(w == win->edit_menu.undo)
    {
    bg_nle_project_undo(win->p);
    }
  else if(w == win->edit_menu.redo)
    {
    bg_nle_project_redo(win->p);
    }
  else if(w == win->view_menu.messages)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
      {
      bg_gtk_log_window_show(log_window);
      log_open_callback();
      }
    else
      {
      bg_gtk_log_window_hide(log_window);
      log_close_callback(NULL, NULL);
      }
    }
  else if(w == win->options_menu.options)
    {
    configure_global(win->win, win->p->plugin_reg);
    }
  else if(w == win->options_menu.snap_to_frames)
    {
    bg_parameter_value_t val;
    val.val_i = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
    set_display_parameter_global(NULL, "snap_to_frames", &val);
    }
  else if(w == win->options_menu.play_every_frame)
    {
    bg_parameter_value_t val;
    val.val_i = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w));
    set_display_parameter_global(NULL, "play_every_frame", &val);
    }
  }

static GtkWidget *
create_menu_item(bg_nle_project_window_t * w, GtkWidget * parent,
                 const char * label, const char * pixmap)
  {
  GtkWidget * ret, *image;
  char * path;
  
  if(pixmap)
    {
    path = bg_search_file_read("icons", pixmap);
    if(path)
      {
      image = gtk_image_new_from_file(path);
      free(path);
      }
    else
      image = gtk_image_new();
    gtk_widget_show(image);
    ret = gtk_image_menu_item_new_with_label(label);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(ret), image);
    }
  else
    {
    ret = gtk_menu_item_new_with_label(label);
    }
  
  g_signal_connect(G_OBJECT(ret), "activate", G_CALLBACK(menu_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }

static GtkWidget *
create_toggle_item(bg_nle_project_window_t * w, GtkWidget * parent,
                   const char * label, guint * id)
  {
  GtkWidget * ret;
  ret = gtk_check_menu_item_new_with_label(label);

  *id = g_signal_connect(G_OBJECT(ret), "toggled", G_CALLBACK(menu_callback),
                         (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }

static void init_menu_bar(bg_nle_project_window_t * w)
  {
  GtkWidget * item;
  /* Project */
  w->project_menu.menu = gtk_menu_new();
  w->project_menu.new =
    create_menu_item(w, w->project_menu.menu, TR("New"), "gmerlerra/new_16.png");

  gtk_widget_add_accelerator(w->project_menu.new,
                             "activate",
                             w->accel_group,
                             GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  w->project_menu.load =
    create_menu_item(w, w->project_menu.menu, TR("Open..."), "folder_open_16.png");
  gtk_widget_add_accelerator(w->project_menu.load,
                             "activate",
                             w->accel_group,
                             GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  w->project_menu.save =
    create_menu_item(w, w->project_menu.menu, TR("Save"), "save_16.png");
  gtk_widget_add_accelerator(w->project_menu.save,
                             "activate",
                             w->accel_group,
                             GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  w->project_menu.save_as =
    create_menu_item(w, w->project_menu.menu, TR("Save as..."), "gmerlerra/save_as_16.png");
  gtk_widget_add_accelerator(w->project_menu.save_as,
                             "activate",
                             w->accel_group,
                             GDK_s, GDK_CONTROL_MASK|GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);
  w->project_menu.set_default =
    create_menu_item(w, w->project_menu.menu, TR("Set as default"), NULL);
  w->project_menu.load_media =
    create_menu_item(w, w->project_menu.menu, TR("Load media..."), "video_16.png");
  w->project_menu.settings =
    create_menu_item(w, w->project_menu.menu, TR("Settings..."), "config_16.png");
  w->project_menu.close =
    create_menu_item(w, w->project_menu.menu, TR("Close"), "close_16.png");
  w->project_menu.quit =
    create_menu_item(w, w->project_menu.menu, TR("Quit..."), "quit_16.png");
  gtk_widget_add_accelerator(w->project_menu.close,
                             "activate",
                             w->accel_group,
                             GDK_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator(w->project_menu.quit,
                             "activate",
                             w->accel_group,
                             GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  gtk_widget_show(w->project_menu.menu);

  /* Track */
  w->track_menu.menu = gtk_menu_new();
  w->track_menu.add_audio =
    create_menu_item(w, w->track_menu.menu, TR("Add audio track"),
                     "audio_16.png");
  w->track_menu.add_video =
    create_menu_item(w, w->track_menu.menu, TR("Add video track"),
                     "video_16.png");

  /* Output stream */
  w->outstream_menu.menu = gtk_menu_new();
  w->outstream_menu.add_audio =
    create_menu_item(w, w->outstream_menu.menu, TR("Add audio output stream"),
                     "audio_16.png");
  w->outstream_menu.add_video =
    create_menu_item(w, w->outstream_menu.menu, TR("Add video output stream"),
                     "video_16.png");
  
  /* Edit */
  w->edit_menu.menu = gtk_menu_new();
  w->edit_menu.undo =
    create_menu_item(w, w->edit_menu.menu, TR("Undo"), "gmerlerra/undo_16.png");

  gtk_widget_add_accelerator(w->edit_menu.undo,
                             "activate",
                             w->accel_group,
                             GDK_z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  
  w->edit_menu.redo =
    create_menu_item(w, w->edit_menu.menu, TR("Redo"), "gmerlerra/redo_16.png");

  gtk_widget_add_accelerator(w->edit_menu.redo,
                             "activate",
                             w->accel_group,
                             GDK_y, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  w->edit_menu.cut =
    create_menu_item(w, w->edit_menu.menu, TR("Cut"), "cut_16.png");

  gtk_widget_add_accelerator(w->edit_menu.cut,
                             "activate",
                             w->accel_group,
                             GDK_x, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  w->edit_menu.copy =
    create_menu_item(w, w->edit_menu.menu, TR("Copy"), "copy_16.png");
  gtk_widget_add_accelerator(w->edit_menu.copy,
                             "activate",
                             w->accel_group,
                             GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  w->edit_menu.paste =
    create_menu_item(w, w->edit_menu.menu, TR("Paste"), "paste_16.png");
  gtk_widget_add_accelerator(w->edit_menu.paste,
                             "activate",
                             w->accel_group,
                             GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  /* View */
  w->view_menu.menu = gtk_menu_new();
  w->view_menu.messages =
    create_toggle_item(w, w->view_menu.menu,
                       TR("Messages"), &w->view_menu.messages_id);

  /* Options */
  w->options_menu.menu = gtk_menu_new();
  w->options_menu.options =
    create_menu_item(w, w->options_menu.menu,
                     TR("Program settings..."), "config_16.png");

  w->options_menu.snap_to_frames =
    create_toggle_item(w, w->options_menu.menu,
                       TR("Snap cursor to frames"),
                       &w->options_menu.snap_to_frames_id);

  w->options_menu.play_every_frame =
    create_toggle_item(w, w->options_menu.menu,
                       TR("Play every frame"),
                       &w->options_menu.play_every_frame_id);
  
  /* Menubar */
  w->menubar = gtk_menu_bar_new();

  item = gtk_menu_item_new_with_label(TR("Project"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), w->project_menu.menu);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menubar), item);

  item = gtk_menu_item_new_with_label(TR("Track"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), w->track_menu.menu);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menubar), item);

  item = gtk_menu_item_new_with_label(TR("Output stream"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), w->outstream_menu.menu);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menubar), item);
  
  item = gtk_menu_item_new_with_label(TR("Edit"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), w->edit_menu.menu);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menubar), item);

  item = gtk_menu_item_new_with_label(TR("View"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), w->view_menu.menu);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menubar), item);

  item = gtk_menu_item_new_with_label(TR("Options"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), w->options_menu.menu);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(w->menubar), item);
  
  gtk_widget_show(w->menubar);
  }

static void motion_callback(gavl_time_t time, void * data)
  {
  bg_nle_project_window_t * p = data;
  bg_gtk_time_display_update(p->time_display, time, BG_GTK_DISPLAY_MODE_HMSMS);  
  }



bg_nle_project_window_t *
bg_nle_project_window_create(const char * project_file,
                             bg_plugin_registry_t * plugin_reg)
  {
  GtkWidget * mainbox;
  GtkWidget * paned;
  GtkWidget * label;
  GtkWidget * box;
  
  bg_nle_project_window_t * ret;
  
  ret = calloc(1, sizeof(*ret));
  
  ret->accel_group = gtk_accel_group_new();
  
  if(project_file)
    {
    ret->p = bg_nle_project_load(project_file, plugin_reg);
    ret->filename = bg_strdup(ret->filename, project_file);
    }
  else
    {
    char * filename = bg_search_file_read("gmerlerra", "default_project.xml");

    if(filename)
      {
      ret->p = bg_nle_project_load(filename, plugin_reg);
      free(filename);
      }
    else
      ret->p = bg_nle_project_create(plugin_reg);
    }

  bg_nle_project_set_edit_callback(ret->p, edit_callback, ret);
  
  project_windows = g_list_append(project_windows, ret);
  
  ret->win = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(ret->win), 1024, 768);

  gtk_window_add_accel_group(GTK_WINDOW(ret->win), ret->accel_group);
  
  ret->timeline = bg_nle_timeline_create(ret->p);

  bg_nle_timeline_set_motion_callback(ret->timeline,
                                      motion_callback, ret);

  ret->media_browser = bg_nle_media_browser_create(ret->p);
  
  ret->compositor = bg_nle_player_widget_create(plugin_reg,
                                                bg_nle_timeline_get_ruler(ret->timeline));
  
  /* menubar */
  init_menu_bar(ret);

  /* Statusbar */

  ret->time_display = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL,
                                                 4,
                                                 BG_GTK_DISPLAY_MODE_TIMECODE |
                                                 BG_GTK_DISPLAY_MODE_HMSMS);
  
  ret->progressbar = gtk_progress_bar_new();
  gtk_widget_show(ret->progressbar);

  ret->statusbar = gtk_statusbar_new();

  gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(ret->statusbar), FALSE);
  
  gtk_widget_show(ret->statusbar);
  
  
  /* Pack everything */
  mainbox = gtk_vbox_new(0, 0);
  paned = gtk_vpaned_new();
  
  gtk_box_pack_start(GTK_BOX(mainbox), ret->menubar, FALSE, FALSE, 0);
  
  ret->notebook = gtk_notebook_new();
  
  gtk_widget_show(paned);

  label = gtk_label_new(TR("Media browser"));
  gtk_widget_show(label);

  gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook),
                           bg_nle_media_browser_get_widget(ret->media_browser),
                           label); 

  label = gtk_label_new(TR("Compositor"));
  gtk_widget_show(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(ret->notebook),
                           bg_nle_player_widget_get_widget(ret->compositor),
                           label); 
  
  gtk_widget_show(ret->notebook);

  gtk_paned_add1(GTK_PANED(paned),
                 ret->notebook);
  gtk_paned_add2(GTK_PANED(paned),
                 bg_nle_timeline_get_widget(ret->timeline));

  
  gtk_box_pack_start(GTK_BOX(mainbox), paned, TRUE, TRUE, 0);

  box = gtk_hbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(box),
                     bg_gtk_time_display_get_widget(ret->time_display),
                     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->statusbar, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->progressbar, FALSE, FALSE, 0);

  gtk_widget_show(box);
  
  gtk_box_pack_start(GTK_BOX(mainbox), box, FALSE, FALSE, 0);
  
  gtk_widget_show(mainbox);
  
  gtk_container_add(GTK_CONTAINER(ret->win), mainbox);
  
  return ret;
  }


void bg_nle_project_window_destroy(bg_nle_project_window_t * w)
  {
  project_windows = g_list_remove(project_windows, w);
  
  gtk_widget_destroy(w->win);
  
  bg_nle_project_destroy(w->p);

  bg_nle_timeline_destroy(w->timeline);
  bg_nle_media_browser_destroy(w->media_browser);

  bg_nle_player_widget_destroy(w->compositor);

  bg_gtk_time_display_destroy(w->time_display);
  
  if(w->filename)
    free(w->filename);

  g_object_unref(w->accel_group);
  
  free(w);
  }

int bg_nle_set_time_unit(const char * name, const
                         bg_parameter_value_t * val, int * mode)
  {
  int new_mode;
  if(!name || strcmp(name, "time_unit"))
    return 0;

  if(!strcmp(val->val_str, "timecode"))
    new_mode = BG_GTK_DISPLAY_MODE_TIMECODE;
  else if(!strcmp(val->val_str, "framecount"))
    new_mode = BG_GTK_DISPLAY_MODE_FRAMECOUNT;
  else
    new_mode = BG_GTK_DISPLAY_MODE_HMSMS;
  
  if(new_mode != *mode)
    {
    *mode =new_mode;
    return 1;
    }
  return 0;
  }

void bg_nle_convert_time(gavl_time_t t,
                         int64_t * ret,
                         bg_nle_time_info_t * info)
  {
  int64_t time_scaled;

  switch(info->mode)
    {
    case BG_GTK_DISPLAY_MODE_TIMECODE:
      time_scaled = gavl_time_scale(info->scale, t+10);
      if(info->tab)
        *ret = gavl_frame_table_time_to_timecode(info->tab, time_scaled, NULL, &info->fmt);
      else
        *ret = GAVL_TIMECODE_UNDEFINED;
      break;
    case BG_GTK_DISPLAY_MODE_FRAMECOUNT:
      time_scaled = gavl_time_scale(info->scale, t+10);
      if(info->tab)
        *ret = gavl_frame_table_time_to_frame(info->tab, time_scaled, NULL);
      else
        *ret = 0;
      break;
    case BG_GTK_DISPLAY_MODE_HMSMS:
      *ret = t;
      break;
    }
  }

// static bg_parameter_info_t * output_plugin_parameters = NULL;

void
bg_nle_project_window_init_global(bg_cfg_registry_t * cfg_reg1,
                                  bg_plugin_registry_t * plugin_reg)
  {
  if(cfg_reg)
    return;
  
  cfg_reg = cfg_reg1;

  log_window =
    bg_gtk_log_window_create(log_close_callback, NULL, "Gmerlerra");
  
  /* Get sections */
  log_window_section =
    bg_cfg_registry_find_section(cfg_reg, "log_window");
  display_section =
    bg_cfg_registry_find_section(cfg_reg, "display");
  oa_section =
    bg_cfg_registry_find_section(cfg_reg, "audio_output");
  ov_section =
    bg_cfg_registry_find_section(cfg_reg, "video_output");
  
  /* Apply sections */
  bg_cfg_section_apply(log_window_section,
                       bg_gtk_log_window_get_parameters(log_window),
                       bg_gtk_log_window_set_parameter,
                       log_window);

  bg_cfg_section_apply(display_section,
                       display_parameters,
                       set_display_parameter_global,
                       NULL);
  
  if(!input_plugin_parameters)
    {
    input_plugin_parameters = calloc(2, sizeof(*input_plugin_parameters));
    input_plugin_parameters->name = bg_strdup(NULL, "input_plugins");
    input_plugin_parameters->long_name = bg_strdup(NULL, TR("Input plugins"));
    bg_plugin_registry_set_parameter_info_input(plugin_reg,
                                                BG_PLUGIN_INPUT,
                                                BG_PLUGIN_FILE,
                                                input_plugin_parameters);
    }
  
  if(!image_reader_parameters)
    {
    image_reader_parameters = calloc(2, sizeof(*image_reader_parameters));
    image_reader_parameters->name = bg_strdup(NULL, "image_reader_plugins");
    image_reader_parameters->long_name =
      bg_strdup(NULL, TR("Image reader plugins"));
    bg_plugin_registry_set_parameter_info_input(plugin_reg,
                                                BG_PLUGIN_IMAGE_READER,
                                                BG_PLUGIN_FILE,
                                                image_reader_parameters);
    }

  if(!oa_parameters)
    oa_parameters = bg_nle_player_get_oa_parameters(plugin_reg);
  if(!ov_parameters)
    ov_parameters = bg_nle_player_get_ov_parameters(plugin_reg);
  
  }

static void
configure_global(GtkWidget * parent, bg_plugin_registry_t * plugin_reg)
  {
  bg_dialog_t * dialog;

  /* Initialize plugins */
  
  dialog = bg_dialog_create_multi(TR("Program settings"));  
  
  bg_dialog_add(dialog,
                TR("Display"),
                display_section,
                set_display_parameter_global,
                get_display_parameter_global,
                NULL,
                display_parameters);

  bg_dialog_add(dialog,
                TR("Input plugins"),
                NULL,
                bg_plugin_registry_set_parameter_input,
                bg_plugin_registry_get_parameter_input,
                plugin_reg,
                input_plugin_parameters);

  bg_dialog_add(dialog,
                TR("Image readers"),
                NULL,
                bg_plugin_registry_set_parameter_input,
                bg_plugin_registry_get_parameter_input,
                plugin_reg,
                image_reader_parameters);

  bg_dialog_add(dialog,
                TR("Audio output"),
                oa_section,
                set_oa_parameter_global,
                NULL,
                NULL,
                oa_parameters);

  bg_dialog_add(dialog,
                TR("Video output"),
                ov_section,
                set_ov_parameter_global,
                NULL,
                NULL,
                ov_parameters);
  
  bg_dialog_add(dialog,
                TR("Log window"),
                log_window_section,
                bg_gtk_log_window_set_parameter,
                NULL,
                log_window,
                bg_gtk_log_window_get_parameters(log_window));
  
  bg_dialog_show(dialog, parent);
  bg_dialog_destroy(dialog);
  
  
  }

void bg_nle_project_window_show(bg_nle_project_window_t * w)
  {
  gtk_widget_show(w->win);

  /* Now we can assume everything is realized, so lets set up the plugins */

  bg_cfg_section_apply(oa_section, oa_parameters, set_oa_parameter, w);
  bg_cfg_section_apply(ov_section, ov_parameters, set_ov_parameter, w);
  bg_cfg_section_apply(display_section, display_parameters, set_display_parameter, w);
  
  
  }
