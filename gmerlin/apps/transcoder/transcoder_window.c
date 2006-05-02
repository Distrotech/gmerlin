/*****************************************************************
 
  transcoder_window.c
 
  Copyright (c) 2003-2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include <config.h>
#include <pluginregistry.h>
#include <utils.h>

#include <cfg_dialog.h>
#include <transcoder_track.h>
#include <msgqueue.h>
#include <transcoder.h>
#include <transcodermsg.h>

#include <remote.h>

#include <gui_gtk/display.h>
#include <gui_gtk/scrolltext.h>
#include <gui_gtk/gtkutils.h>
#include <gui_gtk/logwindow.h>

#include "transcoder_window.h"
#include "transcoder_remote.h"

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

  bg_gtk_log_window_t * logwindow;
  int show_logwindow;
  
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

  /* Postprocessor */
  bg_transcoder_pp_t * pp;
  bg_plugin_handle_t * pp_plugin;
  

  float fg_color[3];
  float fg_color_e[3]; /* Foreground color for error messages */
  float bg_color[3];
  
  /* Load/Save stuff */
  
  char * task_path;
  char * profile_path;
  
  GtkWidget * filesel;
  char * filesel_file;
  char * filesel_path;
  
  
  GtkTooltips * tooltips;

  bg_remote_server_t * remote;
  
  GtkWidget * menubar;

  struct
    {
    GtkWidget * load_item;
    GtkWidget * save_item;
    GtkWidget * quit_item;
    GtkWidget * menu;
    } file_menu;

  struct
    {
    GtkWidget * config_item;
    GtkWidget * plugin_item;
    GtkWidget * load_item;
    GtkWidget * save_item;
    GtkWidget * menu;
    } options_menu;

  struct
    {
    GtkWidget * run_item;
    GtkWidget * stop_item;
    GtkWidget * menu;
    } actions_menu;

  struct
    {
    GtkWidget * log_item;
    GtkWidget * menu;
    } windows_menu;
    
  bg_msg_queue_t * msg_queue;
  
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
    {
      name:        "profile_path",
      long_name:   "Profile path",
      type:        BG_PARAMETER_DIRECTORY,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_str: "." },
    },
    {
      name:        "show_logwindow",
      long_name:   "Show log window",
      type:        BG_PARAMETER_CHECKBUTTON,
      flags:       BG_PARAMETER_HIDE_DIALOG,
      val_default: { val_i: 0 },
    },
    {
      name:        "gui",
      long_name:   "GUI",
      type:        BG_PARAMETER_SECTION,
    },
    {
      name:        "show_tooltips",
      long_name:   "Show Tooltips",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 1 },
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
  else if(!strcmp(name, "profile_path"))
    {
    win->profile_path = bg_strdup(win->profile_path, val->val_str);
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
  else if(!strcmp(name, "show_logwindow"))
    {
    win->show_logwindow = val->val_i;
    }
  else if(!strcmp(name, "show_tooltips"))
    {
    if(val->val_i)
      gtk_tooltips_enable(win->tooltips);
    else
      gtk_tooltips_disable(win->tooltips);

    track_list_set_tooltips(win->tracklist, val->val_i);
    plugin_window_set_tooltips(win->plugin_window, val->val_i);
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
  else if(!strcmp(name, "profile_path"))
    {
    val->val_str = bg_strdup(val->val_str, win->profile_path);
    return 1;
    }
  else if(!strcmp(name, "show_logwindow"))
    {
    val->val_i = win->show_logwindow;
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
  //  fprintf(stderr, "Finish transcoding\n");
  
  if(win->transcoder)
    {
    bg_transcoder_finish(win->transcoder);
    bg_transcoder_destroy(win->transcoder);
    win->transcoder = (bg_transcoder_t*)0;
    
    if(win->transcoder_track)
      {
      bg_transcoder_track_destroy(win->transcoder_track);
      win->transcoder_track = (bg_transcoder_track_t*)0;
      }
    if(win->pp)
      bg_transcoder_pp_update(win->pp);
    }
  else if(win->pp)
    {
    bg_transcoder_pp_finish(win->pp);
    bg_transcoder_pp_destroy(win->pp);
    win->pp = (bg_transcoder_pp_t*)0;
    }
  /* Flush message queue */
  while(bg_msg_queue_try_lock_read(win->msg_queue))
    bg_msg_queue_unlock_read(win->msg_queue);
  }

static gboolean idle_callback(gpointer data)
  {
  bg_msg_t * msg;
  float percentage_done;
  gavl_time_t remaining_time;
  char * arg_str;
  transcoder_window_t * win;
  win = (transcoder_window_t*)data;

  /* If the transcoder isn't there, it means that we were interrupted */

  //  fprintf(stderr, "idle_callback\n");

  while((msg = bg_msg_queue_try_lock_read(win->msg_queue)))
    {
    switch(bg_msg_get_id(msg))
      {
      case BG_TRANSCODER_MSG_START:
        //        fprintf(stderr, "BG_TRANSCODER_MSG_START\n");
        arg_str = bg_msg_get_arg_string(msg, 0);
        bg_gtk_scrolltext_set_text(win->scrolltext, arg_str, win->fg_color, win->bg_color);
        free(arg_str);
        break;
      case BG_TRANSCODER_MSG_NUM_AUDIO_STREAMS:
        fprintf(stderr, "BG_TRANSCODER_MSG_NUM_AUDIO_STREAMS\n");
        break;
      case BG_TRANSCODER_MSG_AUDIO_FORMAT:
        fprintf(stderr, "BG_TRANSCODER_MSG_AUDIO_FORMAT\n");
        break;
      case BG_TRANSCODER_MSG_AUDIO_FILE:
        fprintf(stderr, "BG_TRANSCODER_MSG_AUDIO_FILE\n");
        break;
      case BG_TRANSCODER_MSG_NUM_VIDEO_STREAMS:
        fprintf(stderr, "BG_TRANSCODER_MSG_NUM_VIDEO_STREAMS\n");
        break;
      case BG_TRANSCODER_MSG_VIDEO_FORMAT:
        fprintf(stderr, "BG_TRANSCODER_MSG_VIDEO_FORMAT\n");
        break;
      case BG_TRANSCODER_MSG_VIDEO_FILE:
        fprintf(stderr, "BG_TRANSCODER_MSG_VIDEO_FILE\n");
        break;
      case BG_TRANSCODER_MSG_FILE:
        fprintf(stderr, "BG_TRANSCODER_MSG_FILE\n");
        break;
      case BG_TRANSCODER_MSG_PROGRESS:
        //        fprintf(stderr, "BG_TRANSCODER_MSG_PROGRESS\n");
        percentage_done = bg_msg_get_arg_float(msg, 0);
        remaining_time = bg_msg_get_arg_time(msg, 1);

        bg_gtk_time_display_update(win->time_remaining,
                                   remaining_time);
        
        //  fprintf(stderr, "done\n");
        
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar),
                                      percentage_done);
        break;
      case BG_TRANSCODER_MSG_FINISHED:
        fprintf(stderr, "BG_TRANSCODER_MSG_FINISHED\n");

        finish_transcoding(win);
        
        bg_gtk_time_display_update(win->time_remaining, GAVL_TIME_UNDEFINED);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), 0.0);
        
        //    fprintf(stderr, "Transcoding done\n");
        
        if(!start_transcode(win))
          {
          gtk_widget_set_sensitive(win->run_button, 1);
          gtk_widget_set_sensitive(win->actions_menu.run_item, 1);
          
          gtk_widget_set_sensitive(win->stop_button, 0);
          gtk_widget_set_sensitive(win->actions_menu.stop_item, 0);
          bg_msg_queue_unlock_read(win->msg_queue);
          fprintf(stderr, "start_transcode failed\n");
          return TRUE;
          }
        else
          {
          bg_msg_queue_unlock_read(win->msg_queue);
          return TRUE;
          }
        break;
      }
    bg_msg_queue_unlock_read(win->msg_queue);
    }
  
  return TRUE;
  }

static int start_transcode(transcoder_window_t * win)
  {
  bg_cfg_section_t * cfg_section;

  const char * error_msg;
  
  cfg_section = bg_cfg_registry_find_section(win->cfg_reg, "output");

  win->transcoder_track      = track_list_get_track(win->tracklist);
  if(!win->transcoder_track)
    {
    /* Check whether to postprocess */
    if(win->pp)
      {
      bg_transcoder_pp_add_message_queue(win->pp, win->msg_queue);
      bg_transcoder_pp_run(win->pp);
      return 1;
      }
    else
      {
      bg_gtk_scrolltext_set_text(win->scrolltext, "Gmerlin transcoder version "VERSION,
                                 win->fg_color, win->bg_color);
      return 0;
      }

    }
  win->transcoder            = bg_transcoder_create();
  bg_cfg_section_apply(cfg_section, bg_transcoder_get_parameters(),
                       bg_transcoder_set_parameter, win->transcoder);
  
  bg_transcoder_add_message_queue(win->transcoder,
                                  win->msg_queue);
  if(win->pp)
    {
    bg_transcoder_pp_connect(win->pp, win->transcoder);
    }
  

  if(!bg_transcoder_init(win->transcoder,
                         win->plugin_reg, win->transcoder_track))
    {
    error_msg = bg_transcoder_get_error(win->transcoder);
    
    if(error_msg)
      bg_gtk_scrolltext_set_text(win->scrolltext, error_msg, win->fg_color_e, win->bg_color);
    else
      bg_gtk_scrolltext_set_text(win->scrolltext, "Failed to initialize transcoder",
                                 win->fg_color_e, win->bg_color);
    
    if(win->transcoder_track)
      track_list_prepend_track(win->tracklist, win->transcoder_track);
    win->transcoder_track = (bg_transcoder_track_t*)0;

    bg_transcoder_destroy(win->transcoder);
    win->transcoder = (bg_transcoder_t*)0;
    return 0;
    }
  //  fprintf(stderr, "Initialized transcoder\n");
  
  gtk_widget_set_sensitive(win->run_button, 0);
  gtk_widget_set_sensitive(win->actions_menu.run_item, 0);

  gtk_widget_set_sensitive(win->stop_button, 1);
  gtk_widget_set_sensitive(win->actions_menu.stop_item, 1);

  bg_transcoder_run(win->transcoder);
  
  return 1;
  }

static void filesel_button_callback(GtkWidget * w, gpointer * data)
  {
  const char * end_pos;

  GtkFileSelection * filesel;
  transcoder_window_t * win = (transcoder_window_t *)data;
  
  filesel = GTK_FILE_SELECTION(win->filesel);

  if(w == filesel->ok_button)
    {
    win->filesel_file = bg_strdup((char*)0,
                                   gtk_file_selection_get_filename(filesel));
    gtk_widget_hide(win->filesel);

    end_pos = strrchr(win->filesel_file, '/');
    if(end_pos)
      {
      end_pos++;
      win->filesel_path = bg_strndup(win->filesel_path, win->filesel_file, end_pos);
      }
    gtk_main_quit();
    }
  else if((w == win->filesel) || (w == filesel->cancel_button))
    {
    gtk_widget_hide(win->filesel);
    gtk_main_quit();
    }
  }

static gboolean filesel_delete_callback(GtkWidget * w, GdkEventAny * event,
                                         gpointer * data)
  {
  filesel_button_callback(w, data);
  return TRUE;
  }

static GtkWidget * create_filesel(transcoder_window_t * win)
  {
  GtkWidget * ret;

  ret = gtk_file_selection_new("");

  gtk_window_set_modal(GTK_WINDOW(ret), 1);
  
  g_signal_connect(G_OBJECT(ret), "delete-event",
                   G_CALLBACK(filesel_delete_callback), win);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(ret)->ok_button),
                   "clicked", G_CALLBACK(filesel_button_callback), win);
  g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(ret)->cancel_button),
                   "clicked", G_CALLBACK(filesel_button_callback), win);
  
  return ret;
  }  

static void filesel_set_path(GtkWidget * filesel, char * path)
  {
  if(path)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), path);
  }

                             
static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_cfg_section_t * cfg_section;
  transcoder_window_t * win = (transcoder_window_t *)data;

  if((w == win->run_button) || (w == win->actions_menu.run_item))
    {
    win->pp_plugin = track_list_get_pp_plugin(win->tracklist);
    if(win->pp_plugin)
      {
      win->pp = bg_transcoder_pp_create();

      cfg_section = bg_cfg_registry_find_section(win->cfg_reg, "output");
      
      bg_cfg_section_apply(cfg_section, bg_transcoder_get_parameters(),
                           bg_transcoder_pp_set_parameter, win->pp);

      if(!bg_transcoder_pp_init(win->pp, win->pp_plugin))
        {
        bg_transcoder_pp_destroy(win->pp);
        win->pp = (bg_transcoder_pp_t*)0;
        }
      }
    
    //    fprintf(stderr, "Run Button\n");
    start_transcode(win);
    }
  else if((w == win->stop_button) || (w == win->actions_menu.stop_item))
    {
    //    fprintf(stderr, "Stop Button\n");
    if(win->transcoder_track)
      track_list_prepend_track(win->tracklist, win->transcoder_track);
    win->transcoder_track = (bg_transcoder_track_t*)0;
    if(win->transcoder)
      bg_transcoder_stop(win->transcoder);
    else if(win->pp)
      bg_transcoder_pp_stop(win->pp);
    
    fprintf(stderr, "joining thread...");
    finish_transcoding(win);
    fprintf(stderr, "done\n");
    gtk_widget_set_sensitive(win->run_button, 1);
    gtk_widget_set_sensitive(win->actions_menu.run_item, 1);

    gtk_widget_set_sensitive(win->stop_button, 0);
    gtk_widget_set_sensitive(win->actions_menu.stop_item, 0);

    bg_gtk_time_display_update(win->time_remaining, GAVL_TIME_UNDEFINED);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), 0.0);

    bg_gtk_scrolltext_set_text(win->scrolltext,
                               "Gmerlin transcoder version "VERSION,
                               win->fg_color, win->bg_color);
    
    }
  else if((w == win->load_button) || (w == win->file_menu.load_item))
    {
    //    fprintf(stderr, "Load Button\n");
    
    if(!win->filesel)
      win->filesel = create_filesel(win);
    
    gtk_window_set_title(GTK_WINDOW(win->filesel), "Load task list");
    filesel_set_path(win->filesel, win->task_path);
    
    gtk_widget_show(win->filesel);
    gtk_main();

    if(win->filesel_file)
      {
      track_list_load(win->tracklist, win->filesel_file);
      free(win->filesel_file);
      win->filesel_file = (char*)0;
      }
    if(win->filesel_path)
      {
      if(win->task_path) free(win->task_path);
      win->task_path = win->filesel_path;
      win->filesel_path = (char*)0;
      }
    }
  else if((w == win->save_button) || (w == win->file_menu.save_item))
    {
    if(!win->filesel)
      win->filesel = create_filesel(win);
    
    gtk_window_set_title(GTK_WINDOW(win->filesel), "Save task list");
    filesel_set_path(win->filesel, win->task_path);
    
    gtk_widget_show(win->filesel);
    gtk_main();

    if(win->filesel_file)
      {
      track_list_save(win->tracklist, win->filesel_file);
      free(win->filesel_file);
      win->filesel_file = (char*)0;
      }
    if(win->filesel_path)
      {
      if(win->task_path) free(win->task_path);
      win->task_path = win->filesel_path;
      win->filesel_path = (char*)0;
      }
    }
  else if(w == win->options_menu.load_item)
    {
    fprintf(stderr, "Load profile\n");
    if(!win->filesel)
      win->filesel = create_filesel(win);
    
    gtk_window_set_title(GTK_WINDOW(win->filesel), "Load profile");
    filesel_set_path(win->filesel, win->profile_path);
    
    gtk_widget_show(win->filesel);
    gtk_main();

    if(win->filesel_file)
      {
      bg_cfg_registry_load(win->cfg_reg, win->filesel_file);
      free(win->filesel_file);
      win->filesel_file = (char*)0;
      }
    if(win->filesel_path)
      {
      if(win->profile_path) free(win->profile_path);
      win->profile_path = win->filesel_path;
      win->filesel_path = (char*)0;
      }
    
    }
  else if(w == win->options_menu.save_item)
    {
    if(!win->filesel)
      win->filesel = create_filesel(win);
    
    gtk_window_set_title(GTK_WINDOW(win->filesel), "Save profile");
    filesel_set_path(win->filesel, win->profile_path);
    
    gtk_widget_show(win->filesel);
    gtk_main();

    if(win->filesel_file)
      {
      bg_cfg_registry_save(win->cfg_reg, win->filesel_file);
      free(win->filesel_file);
      win->filesel_file = (char*)0;
      }
    if(win->filesel_path)
      {
      if(win->profile_path) free(win->profile_path);
      win->profile_path = win->filesel_path;
      win->filesel_path = (char*)0;
      }
    }
  else if((w == win->preferences_button) || (w == win->options_menu.plugin_item))
    {
    //    fprintf(stderr, "Preferences Button\n");
    gtk_widget_set_sensitive(win->preferences_button, 0);
    plugin_window_show(win->plugin_window);
    }
  else if((w == win->quit_button) || (w == win->file_menu.quit_item))
    {
    //    fprintf(stderr, "Quit Button\n");
    gtk_widget_hide(win->win);
    gtk_main_quit();
    }
  else if((w == win->properties_button) || (w == win->options_menu.config_item))
    {
    //    fprintf(stderr, "Properties Button\n");
    transcoder_window_preferences(win);
    }
  else if(w == win->windows_menu.log_item)
    {
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)))
      {
      bg_gtk_log_window_show(win->logwindow);
      win->show_logwindow = 1;
      }
    else
      {
      bg_gtk_log_window_hide(win->logwindow);
      win->show_logwindow = 0;
      }
    }

  }

static GtkWidget * create_pixmap_button(transcoder_window_t * win,
                                        const char * pixmap,
                                        const char * tooltip, const char * tooltip_private)
  {
  GtkWidget * ret;
  GtkWidget * image;
  char * path;
  
  path = bg_search_file_read("icons", pixmap);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();
  
  ret = gtk_button_new();
  gtk_widget_show(image);
  gtk_container_add(GTK_CONTAINER(ret), image);

  g_signal_connect(G_OBJECT(ret), "clicked", G_CALLBACK(button_callback),
                   win);
  gtk_widget_show(ret);

  gtk_tooltips_set_tip(win->tooltips, ret, tooltip, tooltip_private);

  return ret;
  }

static gboolean delete_callback(GtkWidget * w, GdkEvent * evt,
                                gpointer data)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;
  fprintf(stderr, "Transcoder delete\n");
  gtk_widget_hide(win->win);
  gtk_main_quit();
  return TRUE;
  }

static GtkWidget *
create_item(transcoder_window_t * w, GtkWidget * parent,
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
  
  g_signal_connect(G_OBJECT(ret), "activate", G_CALLBACK(button_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }

static GtkWidget *
create_toggle_item(transcoder_window_t * w, GtkWidget * parent,
                   const char * label)
  {
  GtkWidget * ret;
  ret = gtk_check_menu_item_new_with_label(label);
  
  g_signal_connect(G_OBJECT(ret), "toggled", G_CALLBACK(button_callback),
                   (gpointer)w);
  gtk_widget_show(ret);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), ret);
  return ret;
  }

#if 0
  struct
    {
    GtkWidget * load_item;
    GtkWidget * save_item;
    GtkWidget * quit_item;
    GtkWidget * menu;
    } file_menu;

  struct
    {
    GtkWidget * config_item;
    GtkWidget * plugin_item;
    GtkWidget * load_item;
    GtkWidget * save_item;
    } options_menu;
#endif

static void init_menus(transcoder_window_t * w)
  {
  w->file_menu.menu = gtk_menu_new();
  w->file_menu.load_item = create_item(w, w->file_menu.menu, "Load tasklist...", "folder_open_16.png");
  w->file_menu.save_item = create_item(w, w->file_menu.menu, "Save tasklist...", "save_16.png");
  w->file_menu.quit_item = create_item(w, w->file_menu.menu, "Quit", "quit_16.png");
  gtk_widget_show(w->file_menu.menu);

  w->options_menu.menu = gtk_menu_new();
  w->options_menu.config_item = create_item(w, w->options_menu.menu, "Preferences...", "config_16.png");
  w->options_menu.plugin_item = create_item(w, w->options_menu.menu, "Plugins...", "plugin_16.png");
  w->options_menu.load_item = create_item(w, w->options_menu.menu, "Load profile...", "folder_open_16.png");
  w->options_menu.save_item = create_item(w, w->options_menu.menu, "Save profile...", "save_16.png");
  gtk_widget_show(w->options_menu.menu);

  w->actions_menu.menu = gtk_menu_new();
  w->actions_menu.run_item = create_item(w, w->actions_menu.menu, "Start transcoding", "run_16.png");
  w->actions_menu.stop_item = create_item(w, w->actions_menu.menu, "Stop transcoding", "stop_16.png");
  gtk_widget_set_sensitive(w->actions_menu.stop_item, 0);

  w->windows_menu.menu = gtk_menu_new();
  w->windows_menu.log_item = create_toggle_item(w, w->windows_menu.menu, "Log messages");
  gtk_widget_show(w->windows_menu.menu);

  
  }

static void logwindow_close_callback(bg_gtk_log_window_t*w, void*data)
  {
  transcoder_window_t * win = (transcoder_window_t*)data;
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(win->windows_menu.log_item), 0);
  win->show_logwindow = 0;

  }

transcoder_window_t * transcoder_window_create()
  {
  int port;
  char * env;
  GtkWidget * menuitem;
  
  GtkWidget * main_table;
  GtkWidget * frame;
  GtkWidget * box;
  char * tmp_path;
  transcoder_window_t * ret;
  bg_cfg_section_t * cfg_section;
    
  ret = calloc(1, sizeof(*ret));

  ret->msg_queue = bg_msg_queue_create();

  g_timeout_add(200, idle_callback, ret);

  ret->tooltips = gtk_tooltips_new();
  
  /* Create window */
  
  ret->win = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
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

  cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "track_list");
  bg_cfg_section_apply(cfg_section, track_list_get_parameters(ret->tracklist),
                       track_list_set_parameter, ret->tracklist);


  /* Create log window */

  ret->logwindow = bg_gtk_log_window_create(logwindow_close_callback, ret);
  cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "logwindow");
  bg_cfg_section_apply(cfg_section, bg_gtk_log_window_get_parameters(ret->logwindow),
                       bg_gtk_log_window_set_parameter, ret->logwindow);
  
  /* Create buttons */

  ret->run_button  = create_pixmap_button(ret, "run_16.png", "Start transcoding", "Start transcoding");
  ret->stop_button = create_pixmap_button(ret,
                                          "stop_16.png", "Stop transcoding", "Stop transcoding");

  ret->preferences_button = create_pixmap_button(ret,
                                                "plugin_16.png", "Change and configure plugins\nfor newly added tracks",
                                                "Change and configure plugins\nfor newly added tracks");
  ret->properties_button = create_pixmap_button(ret,
                                               "config_16.png", "Set global options and track defaults",
                                               "Set global options and track defaults");
  ret->quit_button = create_pixmap_button(ret,
                                         "quit_16.png", "Quit program", "Quit program");
  ret->load_button  = create_pixmap_button(ret,
                                          "folder_open_16.png", "Load track list", "Load track list");
  ret->save_button  = create_pixmap_button(ret,
                                          "save_16.png", "Save track list", "Save track list");

  gtk_widget_set_sensitive(ret->stop_button, 0);
  
  /* Progress bar */
  ret->progress_bar = gtk_progress_bar_new();
  gtk_widget_show(ret->progress_bar);

  /* Time display */
  ret->time_remaining = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL, 4);

  gtk_tooltips_set_tip(ret->tooltips,
                       bg_gtk_time_display_get_widget(ret->time_remaining),
                       "Estimated remaining transcoding time",
                       "Estimated remaining transcoding time");


  bg_gtk_time_display_update(ret->time_remaining, GAVL_TIME_UNDEFINED);

  /* Scrolltext */

  ret->scrolltext = bg_gtk_scrolltext_create(100, 24);

  /* Menubar */

  init_menus(ret);
  
  ret->menubar = gtk_menu_bar_new();

  menuitem = gtk_menu_item_new_with_label("File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->file_menu.menu);
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label("Options");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->options_menu.menu);
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label("Actions");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->actions_menu.menu);
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label("Tasklist");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), track_list_get_menu(ret->tracklist));
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label("Windows");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->windows_menu.menu);
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);
  
  gtk_widget_show(ret->menubar);
    
  /* Pack everything */
  
  main_table = gtk_table_new(5, 1, 0);
  gtk_container_set_border_width(GTK_CONTAINER(main_table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(main_table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(main_table), 5);

  gtk_table_attach(GTK_TABLE(main_table),
                   ret->menubar,
                   0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  
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
                   0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

  box = gtk_hbox_new(0, 0);

  gtk_box_pack_end(GTK_BOX(box),
                   bg_gtk_time_display_get_widget(ret->time_remaining),
                   FALSE, FALSE, 0);
  gtk_box_pack_start_defaults(GTK_BOX(box),
                              bg_gtk_scrolltext_get_widget(ret->scrolltext));

  gtk_widget_show(box);
  
  gtk_table_attach(GTK_TABLE(main_table),
                   box,
                   0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(main_table),
                   ret->progress_bar,
                   0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  
  frame = gtk_frame_new("Tasklist");
  gtk_container_add(GTK_CONTAINER(frame),
                    track_list_get_widget(ret->tracklist));

  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(main_table),
                            frame,
                            0, 1, 4, 5);
  
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

  port = TRANSCODER_REMOTE_PORT;
  env = getenv(TRANSCODER_REMOTE_ENV);
  if(env)
    port = atoi(env);
  
  ret->remote = bg_remote_server_create(port, TRANSCODER_REMOTE_ID);

  cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "remote");
  bg_cfg_section_apply(cfg_section, bg_remote_server_get_parameters(ret->remote),
                       bg_remote_server_set_parameter, ret->remote);
  
  //  if(!bg_remote_server_init(ret->remote))
  //    fprintf(stderr, "Cannot open remote server (Port busy?)\n");
  
  return ret;
  }

void transcoder_window_destroy(transcoder_window_t* w)
  {
  char * tmp_path;
  bg_cfg_section_t * cfg_section;
  
  cfg_section = bg_cfg_registry_find_section(w->cfg_reg, "transcoder_window");
  bg_cfg_section_get(cfg_section, transcoder_window_parameters,
                       get_transcoder_window_parameter, w);
  

  cfg_section = bg_cfg_registry_find_section(w->cfg_reg, "track_list");
  bg_cfg_section_get(cfg_section, track_list_get_parameters(w->tracklist),
                     track_list_get_parameter, w->tracklist);

  track_list_destroy(w->tracklist);

  cfg_section = bg_cfg_registry_find_section(w->cfg_reg, "logwindow");
  bg_cfg_section_get(cfg_section, bg_gtk_log_window_get_parameters(w->logwindow),
                     bg_gtk_log_window_get_parameter, w->logwindow);
  
  bg_gtk_log_window_destroy(w->logwindow);

  tmp_path =  bg_search_file_write("transcoder", "config.xml");
 
  bg_cfg_registry_save(w->cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  bg_cfg_registry_destroy(w->cfg_reg);

  if(w->task_path)
    free(w->task_path);

  bg_remote_server_destroy(w->remote);
  
  //  g_object_unref(w->tooltips);
  
  free(w);
  }

static gboolean remote_callback(gpointer data)
  {
  int id;
  char * arg_str;
  bg_msg_t * msg;
  transcoder_window_t * win;
  win = (transcoder_window_t*)data;

  while((msg = bg_remote_server_get_msg(win->remote)))
    {
    id = bg_msg_get_id(msg);

    switch(id)
      {
      case TRANSCODER_REMOTE_ADD_ALBUM:
        arg_str = bg_msg_get_arg_string(msg, 0);
        track_list_add_xml(win->tracklist, arg_str, strlen(arg_str));
        free(arg_str);
        break;
      }
    }
  return TRUE;
  }


void transcoder_window_run(transcoder_window_t * w)
  {
  gtk_widget_show(w->win);

  if(w->show_logwindow)
    {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w->windows_menu.log_item), 1);
    }
  //    bg_gtk_log_window_show(w->logwindow);

  remote_callback(w);

  g_timeout_add(50, remote_callback, w);
  
  gtk_main();
  }

/* Configuration stuff */

static void transcoder_window_preferences(transcoder_window_t * w)
  {
  bg_dialog_t * dlg;
  bg_cfg_section_t * cfg_section;
  void * parent;

  dlg = bg_dialog_create_multi("Transcoder configuration");

  cfg_section     = bg_cfg_registry_find_section(w->cfg_reg, "output");

  bg_dialog_add(dlg,
                "Output options",
                cfg_section,
                NULL,
                NULL,
                bg_transcoder_get_parameters());

  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "general");
  
  bg_dialog_add(dlg,
                "Track defaults",
                cfg_section,
                NULL,
                NULL,
                bg_transcoder_track_get_general_parameters());

  
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

  parent = bg_dialog_add_parent(dlg, NULL,
                                "Video defaults");

  
  bg_dialog_add_child(dlg, parent, "Video",
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

  cfg_section = bg_cfg_registry_find_section(w->cfg_reg,
                                             "remote");

  bg_dialog_add(dlg,
                "Remote",
                cfg_section,
                bg_remote_server_set_parameter,
                w->remote,
                bg_remote_server_get_parameters(w->remote));

  cfg_section = bg_cfg_registry_find_section(w->cfg_reg,
                                             "logwindow");

  bg_dialog_add(dlg,
                "Log window",
                cfg_section,
                bg_gtk_log_window_set_parameter,
                w->logwindow,
                bg_gtk_log_window_get_parameters(w->logwindow));

  
  bg_dialog_show(dlg);
  bg_dialog_destroy(dlg);
  
  }


