#include <stdlib.h>
#include <gtk/gtk.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/pluginregistry.h>

#include <config.h>

#include <track.h>
#include <project.h>

#include <gui_gtk/projectwindow.h>
#include <gui_gtk/timeline.h>
#include <gui_gtk/mediabrowser.h>
#include <gui_gtk/playerwidget.h>

#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/gui_gtk/fileselect.h>
#include <gmerlin/gui_gtk/display.h>

#include <gmerlin/utils.h>
#include <gmerlin/cfg_dialog.h>

static char * project_path = (char*)0;

/* List of all open windows */
static GList * project_windows = NULL;

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
  GtkWidget * move_up;
  GtkWidget * move_down;
  GtkWidget * delete;
  GtkWidget * menu;
  } track_menu_t;

typedef struct
  {
  GtkWidget * add_audio;
  GtkWidget * add_video;
  GtkWidget * move_up;
  GtkWidget * move_down;
  GtkWidget * delete;
  GtkWidget * menu;
  } outstream_menu_t;


typedef struct
  {
  GtkWidget * cut;
  GtkWidget * copy;
  GtkWidget * paste;
  GtkWidget * menu;
  } edit_menu_t;

struct bg_nle_project_window_s
  {
  GtkWidget * win;
  GtkWidget * menubar;
  project_menu_t project_menu;
  track_menu_t track_menu;
  outstream_menu_t outstream_menu;
  edit_menu_t edit_menu;
  
  bg_nle_project_t * p;

  bg_nle_timeline_t * timeline;
  bg_nle_media_browser_t * media_browser;
  
  char * filename;
  GtkWidget * notebook;

  bg_nle_player_widget_t * compositor;

  bg_gtk_time_display_t * time_display;
  GtkWidget * statusbar;
  GtkWidget * progressbar;
  
  };

static void show_settings_dialog(bg_nle_project_window_t * win)
  {
  bg_dialog_t * cfg_dialog;
  void * parent;
  
  cfg_dialog = bg_dialog_create_multi(TR("Project settings"));
  
  parent = bg_dialog_add_parent(cfg_dialog, NULL, TR("Audio"));
  bg_dialog_add_child(cfg_dialog, parent,
                      TR("Track format"),
                      win->p->audio_track_section,
                      NULL,
                      NULL,
                      NULL,
                      bg_nle_track_audio_parameters);
  bg_dialog_add_child(cfg_dialog, parent,
                      TR("Compositing format"),
                      win->p->audio_outstream_section,
                      NULL,
                      NULL,
                      NULL,
                      bg_nle_outstream_audio_parameters);

  parent = bg_dialog_add_parent(cfg_dialog, NULL, TR("Video"));
  bg_dialog_add_child(cfg_dialog, parent,
                      TR("Track format"),
                      win->p->video_track_section,
                      NULL,
                      NULL,
                      NULL,
                      bg_nle_track_video_parameters);
  bg_dialog_add_child(cfg_dialog, parent,
                      TR("Compositing format"),
                      win->p->video_outstream_section,
                      NULL,
                      NULL,
                      NULL,
                      bg_nle_outstream_video_parameters);

  
    

  bg_dialog_show(cfg_dialog, win->win);
  }

static void menu_callback(GtkWidget * w, gpointer data)
  {
  bg_nle_project_window_t * win = data;
  bg_nle_track_t * track;
  bg_nle_project_window_t * new_win;
  bg_nle_outstream_t * outstream;
  
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
      }
    }
  else if(w == win->project_menu.save)
    {
    if(win->filename)
      bg_nle_project_save(win->p, win->filename);
    else
      {
      char * filename;
      filename = bg_gtk_get_filename_write("Load project",
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
  else if(w == win->project_menu.set_default)
    {
    bg_nle_project_save(win->p, (char*)0);
    }
  else if(w == win->project_menu.close)
    {
    
    }
  else if(w == win->project_menu.quit)
    {
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
  else if(w == win->track_menu.add_audio)
    {
    track = bg_nle_project_add_audio_track(win->p);
    bg_nle_timeline_add_track(win->timeline, track);
    }
  else if(w == win->track_menu.add_video)
    {
    track = bg_nle_project_add_video_track(win->p);
    bg_nle_timeline_add_track(win->timeline, track);
    }
  else if(w == win->track_menu.delete)
    {
    /* */
    }
  else if(w == win->outstream_menu.add_audio)
    {
    outstream = bg_nle_project_add_audio_outstream(win->p);
    bg_nle_timeline_add_outstream(win->timeline, outstream);
    }
  else if(w == win->outstream_menu.add_video)
    {
    outstream = bg_nle_project_add_video_outstream(win->p);
    bg_nle_timeline_add_outstream(win->timeline, outstream);
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

static void init_menu_bar(bg_nle_project_window_t * w)
  {
  GtkWidget * item;
  /* Project */
  w->project_menu.menu = gtk_menu_new();
  w->project_menu.new =
    create_menu_item(w, w->project_menu.menu, TR("New"), "new_16.png");
  w->project_menu.load =
    create_menu_item(w, w->project_menu.menu, TR("Open..."), "folder_open_16.png");
  w->project_menu.save =
    create_menu_item(w, w->project_menu.menu, TR("Save"), "save_16.png");
  w->project_menu.save_as =
    create_menu_item(w, w->project_menu.menu, TR("Save as..."), "save_as_16.png");
  w->project_menu.set_default =
    create_menu_item(w, w->project_menu.menu, TR("Set as default"), NULL);
  w->project_menu.load_media =
    create_menu_item(w, w->project_menu.menu, TR("Load media..."), "video_16.png");
  w->project_menu.settings =
    create_menu_item(w, w->project_menu.menu, TR("Settings..."), "config_16.png");
  w->project_menu.close =
    create_menu_item(w, w->project_menu.menu, TR("close"), "close_16.png");
  w->project_menu.quit =
    create_menu_item(w, w->project_menu.menu, TR("Quit..."), "quit_16.png");
  
  gtk_widget_show(w->project_menu.menu);

  /* Track */
  w->track_menu.menu = gtk_menu_new();
  w->track_menu.add_audio =
    create_menu_item(w, w->track_menu.menu, TR("Add audio track"), "audio_16.png");
  w->track_menu.add_video =
    create_menu_item(w, w->track_menu.menu, TR("Add video track"), "video_16.png");
  w->track_menu.move_up =
    create_menu_item(w, w->track_menu.menu, TR("Move up"), "up_16.png");
  w->track_menu.move_down =
    create_menu_item(w, w->track_menu.menu, TR("Move down"), "down_16.png");
  w->track_menu.delete =
    create_menu_item(w, w->track_menu.menu, TR("Delete selected"), "trash_16.png");

  /* Output stream */
  w->outstream_menu.menu = gtk_menu_new();
  w->outstream_menu.add_audio =
    create_menu_item(w, w->outstream_menu.menu, TR("Add audio output stream"), "audio_16.png");
  w->outstream_menu.add_video =
    create_menu_item(w, w->outstream_menu.menu, TR("Add video output stream"), "video_16.png");
  w->outstream_menu.move_up =
    create_menu_item(w, w->outstream_menu.menu, TR("Move up"), "up_16.png");
  w->outstream_menu.move_down =
    create_menu_item(w, w->outstream_menu.menu, TR("Move down"), "down_16.png");
  w->outstream_menu.delete =
    create_menu_item(w, w->outstream_menu.menu, TR("Delete selected"), "trash_16.png");
  
  /* Edit */
  w->edit_menu.menu = gtk_menu_new();
  w->edit_menu.cut =
    create_menu_item(w, w->edit_menu.menu, TR("Cut"), "cut_16.png");
  w->edit_menu.copy =
    create_menu_item(w, w->edit_menu.menu, TR("Copy"), "copy_16.png");
  w->edit_menu.paste =
    create_menu_item(w, w->edit_menu.menu, TR("Paste"), "paste_16.png");
  
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

  if(project_file)
    {
    ret->p = bg_nle_project_load(project_file, plugin_reg);
    ret->filename = bg_strdup(ret->filename, project_file);
    }
  else
    ret->p = bg_nle_project_create(project_file, plugin_reg);
  
  project_windows = g_list_append(project_windows, ret);
  
  ret->win = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(ret->win), 1024, 768);
  
  ret->timeline = bg_nle_timeline_create(ret->p);

  bg_nle_timeline_set_motion_callback(ret->timeline,
                                      motion_callback, ret);

  ret->media_browser = bg_nle_media_browser_create(ret->p->media_list);
  
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

void bg_nle_project_window_show(bg_nle_project_window_t * w)
  {
  gtk_widget_show(w->win);
  }

void bg_nle_project_window_destroy(bg_nle_project_window_t * w)
  {
  project_windows = g_list_remove(project_windows, w);
  free(w);
  }
