/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include <config.h>

#include <gmerlin/translation.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>

#include <gmerlin/cfg_dialog.h>
#include <gmerlin/transcoder_track.h>
#include <gmerlin/msgqueue.h>
#include <gmerlin/transcoder.h>
#include <gmerlin/transcodermsg.h>

#include <gmerlin/remote.h>
#include <gmerlin/textrenderer.h>

#include <gmerlin/converters.h>
#include <gmerlin/filters.h>

#include <gui_gtk/display.h>
#include <gui_gtk/scrolltext.h>
#include <gui_gtk/gtkutils.h>
#include <gui_gtk/logwindow.h>
#include <gui_gtk/aboutwindow.h>
#include <gui_gtk/fileselect.h>

#include "transcoder_window.h"
#include "transcoder_remote.h"

#include "tracklist.h"

static const uint32_t stream_flags = BG_STREAM_AUDIO |
BG_STREAM_VIDEO |
BG_STREAM_SUBTITLE_TEXT |
BG_STREAM_SUBTITLE_OVERLAY;

static const uint32_t plugin_flags = BG_PLUGIN_FILE;

struct transcoder_window_s
  {
  GtkWidget * win;

  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;

  track_list_t * tracklist;

  GtkWidget * run_button;
  GtkWidget * stop_button;
  GtkWidget * properties_button;
  GtkWidget * quit_button;
  GtkWidget * load_button;
  GtkWidget * save_button;

  bg_gtk_log_window_t * logwindow;
  int show_logwindow;
  
  GtkWidget * progress_bar;
  bg_gtk_time_display_t * time_remaining;
  bg_gtk_scrolltext_t   * scrolltext;
  
  /* Configuration stuff */

  char * output_directory;
  int delete_incomplete;

  bg_cfg_section_t * track_defaults_section;

  bg_parameter_info_t * encoder_parameters;
  bg_cfg_section_t * encoder_section;
    
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

  struct
    {
    GtkWidget * about_item;
    GtkWidget * help_item;
    GtkWidget * menu;
    } help_menu;
  bg_msg_queue_t * msg_queue;
  };


static int start_transcode(transcoder_window_t * win);

static const bg_parameter_info_t transcoder_window_parameters[] =
  {
    {
      .name =        "display",
      .long_name =   TRS("Display"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name =        "display_foreground",
      .long_name =   TRS("Foreground"),
      .type =        BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 1.0, 1.0, 0.0, 1.0 } }
    },
    {
      .name =        "display_foreground_error",
      .long_name =   TRS("Error foreground"),
      .type =        BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 1.0, 0.0, 0.0, 1.0 } }
    },
    {
      .name =        "display_background",
      .long_name =   TRS("Background"),
      .type =        BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 0.0, 0.0, 0.0, 1.0 } }
    },
    {
      .name =        "display_font",
      .long_name =   TRS("Font"),
      .type =        BG_PARAMETER_FONT,
      .val_default = { .val_str = "Sans Bold 10" }
    },
    {
      .name =        "task_path",
      .long_name =   "Task path",
      .type =        BG_PARAMETER_DIRECTORY,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_str = "." },
    },
    {
      .name =        "profile_path",
      .long_name =   "Profile path",
      .type =        BG_PARAMETER_DIRECTORY,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_str = "." },
    },
    {
      .name =        "show_logwindow",
      .long_name =   "Show log window",
      .type =        BG_PARAMETER_CHECKBUTTON,
      .flags =       BG_PARAMETER_HIDE_DIALOG,
      .val_default = { .val_i = 0 },
    },
    {
      .name =        "gui",
      .long_name =   TRS("GUI"),
      .type =        BG_PARAMETER_SECTION,
    },
    {
      .name =        "show_tooltips",
      .long_name =   TRS("Show Tooltips"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    { /* End of parameters */ }
  };

static void
set_transcoder_window_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val)
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
    bg_gtk_set_tooltips(val->val_i);
    }
  }

static int
get_transcoder_window_parameter(void * data, const char * name,
                                bg_parameter_value_t * val)
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

static void finish_transcoding(transcoder_window_t * win)
  {
  
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


  while((msg = bg_msg_queue_try_lock_read(win->msg_queue)))
    {
    switch(bg_msg_get_id(msg))
      {
      case BG_TRANSCODER_MSG_START:
        arg_str = bg_msg_get_arg_string(msg, 0);
        bg_gtk_scrolltext_set_text(win->scrolltext, arg_str, win->fg_color, win->bg_color);
        free(arg_str);
        break;
      case BG_TRANSCODER_MSG_NUM_AUDIO_STREAMS:
        break;
      case BG_TRANSCODER_MSG_AUDIO_FORMAT:
        break;
      case BG_TRANSCODER_MSG_AUDIO_FILE:
        break;
      case BG_TRANSCODER_MSG_NUM_VIDEO_STREAMS:
        break;
      case BG_TRANSCODER_MSG_VIDEO_FORMAT:
        break;
      case BG_TRANSCODER_MSG_VIDEO_FILE:
        break;
      case BG_TRANSCODER_MSG_FILE:
        break;
      case BG_TRANSCODER_MSG_PROGRESS:
        percentage_done = bg_msg_get_arg_float(msg, 0);
        remaining_time = bg_msg_get_arg_time(msg, 1);

        bg_gtk_time_display_update(win->time_remaining,
                                   remaining_time,
                                   BG_GTK_DISPLAY_MODE_HMS);
        
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar),
                                      percentage_done);
        break;
      case BG_TRANSCODER_MSG_FINISHED:

        finish_transcoding(win);
        
        bg_gtk_time_display_update(win->time_remaining,
                                   GAVL_TIME_UNDEFINED,
                                   BG_GTK_DISPLAY_MODE_HMS);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), 0.0);
        
        
        if(!start_transcode(win))
          {
          gtk_widget_set_sensitive(win->run_button, 1);
          gtk_widget_set_sensitive(win->actions_menu.run_item, 1);
          
          gtk_widget_set_sensitive(win->stop_button, 0);
          gtk_widget_set_sensitive(win->actions_menu.stop_item, 0);
          bg_msg_queue_unlock_read(win->msg_queue);
          return TRUE;
          }
        else
          {
          bg_msg_queue_unlock_read(win->msg_queue);
          return TRUE;
          }
        break;

      case BG_TRANSCODER_MSG_ERROR:
        arg_str = bg_msg_get_arg_string(msg, 0);
        bg_gtk_scrolltext_set_text(win->scrolltext,
                                   bg_gtk_log_window_last_error(win->logwindow),
                                   win->fg_color_e, win->bg_color);
        if(win->transcoder_track)
          {
          track_list_prepend_track(win->tracklist, win->transcoder_track);
          win->transcoder_track = (bg_transcoder_track_t*)0;
          }

        finish_transcoding(win);
        
        bg_gtk_time_display_update(win->time_remaining,
                                   GAVL_TIME_UNDEFINED,
                                   BG_GTK_DISPLAY_MODE_HMS);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), 0.0);
        
        win->transcoder = (bg_transcoder_t*)0;
        
        gtk_widget_set_sensitive(win->run_button, 1);
        gtk_widget_set_sensitive(win->actions_menu.run_item, 1);
        
        gtk_widget_set_sensitive(win->stop_button, 0);
        gtk_widget_set_sensitive(win->actions_menu.stop_item, 0);
        free(arg_str);

        bg_msg_queue_unlock_read(win->msg_queue);
        return TRUE;
      }
    bg_msg_queue_unlock_read(win->msg_queue);
    }
  
  return TRUE;
  }

static int start_transcode(transcoder_window_t * win)
  {
  bg_cfg_section_t * cfg_section;

  
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
    bg_gtk_log_window_flush(win->logwindow);
    
    bg_gtk_scrolltext_set_text(win->scrolltext, bg_gtk_log_window_last_error(win->logwindow),
                               win->fg_color_e, win->bg_color);
    
    if(win->transcoder_track)
      track_list_prepend_track(win->tracklist, win->transcoder_track);
    win->transcoder_track = (bg_transcoder_track_t*)0;

    bg_transcoder_destroy(win->transcoder);
    win->transcoder = (bg_transcoder_t*)0;
    return 0;
    }
  
  gtk_widget_set_sensitive(win->run_button, 0);
  gtk_widget_set_sensitive(win->actions_menu.run_item, 0);

  gtk_widget_set_sensitive(win->stop_button, 1);
  gtk_widget_set_sensitive(win->actions_menu.stop_item, 1);

  bg_transcoder_run(win->transcoder);
  
  return 1;
  }

#if 0
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
#endif

static void about_window_close_callback(bg_gtk_about_window_t * w,
                                        void * data)
  {
  transcoder_window_t * win = (transcoder_window_t *)data;
  gtk_widget_set_sensitive(win->help_menu.about_item, 1);
  }

void transcoder_window_load_profile(transcoder_window_t * win,
                                    const char * file)
  {
  bg_cfg_registry_load(win->cfg_reg, file);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_cfg_section_t * cfg_section;
  transcoder_window_t * win = (transcoder_window_t *)data;
  char * tmp_string;
  
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
    
    start_transcode(win);
    }
  else if((w == win->stop_button) || (w == win->actions_menu.stop_item))
    {
    if(win->transcoder_track)
      track_list_prepend_track(win->tracklist, win->transcoder_track);
    win->transcoder_track = (bg_transcoder_track_t*)0;
    if(win->transcoder)
      bg_transcoder_stop(win->transcoder);
    else if(win->pp)
      bg_transcoder_pp_stop(win->pp);
    
    finish_transcoding(win);
    gtk_widget_set_sensitive(win->run_button, 1);
    gtk_widget_set_sensitive(win->actions_menu.run_item, 1);

    gtk_widget_set_sensitive(win->stop_button, 0);
    gtk_widget_set_sensitive(win->actions_menu.stop_item, 0);

    bg_gtk_time_display_update(win->time_remaining, GAVL_TIME_UNDEFINED,
                               BG_GTK_DISPLAY_MODE_HMS);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(win->progress_bar), 0.0);

    bg_gtk_scrolltext_set_text(win->scrolltext,
                               "Gmerlin transcoder version "VERSION,
                               win->fg_color, win->bg_color);
    
    }
  else if((w == win->load_button) || (w == win->file_menu.load_item))
    {
    tmp_string = bg_gtk_get_filename_read(TR("Load task list"),
                                          &win->task_path, win->load_button);
    if(tmp_string)
      {
      track_list_load(win->tracklist, tmp_string);
      free(tmp_string);
      }
    }
  else if((w == win->save_button) || (w == win->file_menu.save_item))
    {
    tmp_string = bg_gtk_get_filename_write(TR("Save task list"),
                                           &win->task_path, 1,
                                           win->save_button);
    if(tmp_string)
      {
      track_list_save(win->tracklist, tmp_string);
      free(tmp_string);
      }
    }
  else if(w == win->options_menu.load_item)
    {
    tmp_string = bg_gtk_get_filename_read(TR("Load profile"),
                                          &win->profile_path, win->win);
    if(tmp_string)
      {
      transcoder_window_load_profile(win, tmp_string);
      free(tmp_string);
      }
    }
  else if(w == win->options_menu.save_item)
    {
    tmp_string = bg_gtk_get_filename_write(TR("Save profile"),
                                           &win->profile_path, 1, win->win);
    if(tmp_string)
      {
      bg_cfg_registry_save(win->cfg_reg, tmp_string);
      free(tmp_string);
      }
    }
  else if((w == win->quit_button) || (w == win->file_menu.quit_item))
    {
    gtk_widget_hide(win->win);
    gtk_main_quit();
    }
  else if((w == win->properties_button) || (w == win->options_menu.config_item))
    {
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

  else if(w == win->help_menu.about_item)
    {
    bg_gtk_about_window_create("Gmerlin transcoder", VERSION,
                               "transcoder_icon.png", about_window_close_callback,
                               win);
    gtk_widget_set_sensitive(win->help_menu.about_item, 0);
    }
  else if(w == win->help_menu.help_item)
    {
    bg_display_html_help("userguide/GUI-Transcoder.html");
    }
  }

static GtkWidget * create_pixmap_button(transcoder_window_t * win,
                                        const char * pixmap,
                                        const char * tooltip)
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

  bg_gtk_tooltips_set_tip(ret, tooltip, PACKAGE);

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
    GtkWidget * load_item;
    GtkWidget * save_item;
    } options_menu;
#endif

static void init_menus(transcoder_window_t * w)
  {
  w->file_menu.menu = gtk_menu_new();
  w->file_menu.load_item = create_item(w, w->file_menu.menu, TR("Load tasklist..."), "folder_open_16.png");
  w->file_menu.save_item = create_item(w, w->file_menu.menu, TR("Save tasklist..."), "save_16.png");
  w->file_menu.quit_item = create_item(w, w->file_menu.menu, TR("Quit"), "quit_16.png");
  gtk_widget_show(w->file_menu.menu);

  w->options_menu.menu = gtk_menu_new();
  w->options_menu.config_item = create_item(w, w->options_menu.menu, TR("Preferences..."), "config_16.png");
  w->options_menu.load_item = create_item(w, w->options_menu.menu, TR("Load profile..."), "folder_open_16.png");
  w->options_menu.save_item = create_item(w, w->options_menu.menu, TR("Save profile..."), "save_16.png");
  gtk_widget_show(w->options_menu.menu);

  w->actions_menu.menu = gtk_menu_new();
  w->actions_menu.run_item = create_item(w, w->actions_menu.menu, TR("Start transcoding"), "run_16.png");
  w->actions_menu.stop_item = create_item(w, w->actions_menu.menu, TR("Stop transcoding"), "stop_16.png");
  gtk_widget_set_sensitive(w->actions_menu.stop_item, 0);

  w->windows_menu.menu = gtk_menu_new();
  w->windows_menu.log_item = create_toggle_item(w, w->windows_menu.menu, TR("Log messages"));
  gtk_widget_show(w->windows_menu.menu);

  w->help_menu.menu = gtk_menu_new();
  w->help_menu.about_item = create_item(w, w->help_menu.menu, TR("About..."), "about_16.png");
  w->help_menu.help_item = create_item(w, w->help_menu.menu, TR("Userguide"), "help_16.png");
  gtk_widget_show(w->help_menu.menu);
  
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

  /* Create encoding parameters */

  ret->encoder_parameters =
    bg_plugin_registry_create_encoder_parameters(ret->plugin_reg,
                                                 stream_flags, plugin_flags);
  ret->encoder_section =
    bg_encoder_section_get_from_registry(ret->plugin_reg,
                                         ret->encoder_parameters,
                                         stream_flags, plugin_flags);
  
  /* Create track list */

  ret->track_defaults_section = bg_cfg_registry_find_section(ret->cfg_reg, "track_defaults");
  ret->tracklist = track_list_create(ret->plugin_reg,
                                     ret->track_defaults_section,
                                     ret->encoder_parameters, ret->encoder_section);
  
  gtk_window_add_accel_group(GTK_WINDOW(ret->win), track_list_get_accel_group(ret->tracklist));
  
  cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "track_list");
  bg_cfg_section_apply(cfg_section, track_list_get_parameters(ret->tracklist),
                       track_list_set_parameter, ret->tracklist);


  /* Create log window */

  ret->logwindow = bg_gtk_log_window_create(logwindow_close_callback, ret,
                                            TR("Gmerlin transcoder"));
  cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "logwindow");
  bg_cfg_section_apply(cfg_section, bg_gtk_log_window_get_parameters(ret->logwindow),
                       bg_gtk_log_window_set_parameter, ret->logwindow);
  
  /* Create buttons */

  ret->run_button  = create_pixmap_button(ret, "run_16.png",
                                          TRS("Start transcoding"));
  ret->stop_button = create_pixmap_button(ret,
                                          "stop_16.png",
                                          TRS("Stop transcoding"));

  ret->properties_button = create_pixmap_button(ret,
                                               "config_16.png", TRS("Set global options and track defaults"));
  ret->quit_button = create_pixmap_button(ret,
                                         "quit_16.png", TRS("Quit program"));
  ret->load_button  = create_pixmap_button(ret,
                                          "folder_open_16.png", TRS("Load task list"));
  ret->save_button  = create_pixmap_button(ret,
                                          "save_16.png", TRS("Save task list"));

  gtk_widget_set_sensitive(ret->stop_button, 0);
  
  /* Progress bar */
  ret->progress_bar = gtk_progress_bar_new();
  gtk_widget_show(ret->progress_bar);

  /* Time display */
  ret->time_remaining =
    bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL, 4,
                               BG_GTK_DISPLAY_MODE_HMS);

  bg_gtk_tooltips_set_tip(bg_gtk_time_display_get_widget(ret->time_remaining),
                          "Estimated remaining transcoding time",
                          PACKAGE);


  bg_gtk_time_display_update(ret->time_remaining, GAVL_TIME_UNDEFINED,
                             BG_GTK_DISPLAY_MODE_HMS);

  /* Scrolltext */

  ret->scrolltext = bg_gtk_scrolltext_create(100, 24);

  /* Menubar */

  init_menus(ret);
  
  ret->menubar = gtk_menu_bar_new();

  menuitem = gtk_menu_item_new_with_label(TR("File"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->file_menu.menu);
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label(TR("Options"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->options_menu.menu);
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label(TR("Actions"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->actions_menu.menu);
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label(TR("Tasklist"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), track_list_get_menu(ret->tracklist));
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label(TR("Windows"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->windows_menu.menu);
  gtk_widget_show(menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(ret->menubar), menuitem);

  menuitem = gtk_menu_item_new_with_label(TR("Help"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), ret->help_menu.menu);
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
  bg_gtk_box_pack_start_defaults(GTK_BOX(box),
                              bg_gtk_scrolltext_get_widget(ret->scrolltext));

  gtk_widget_show(box);
  
  gtk_table_attach(GTK_TABLE(main_table),
                   box,
                   0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(main_table),
                   ret->progress_bar,
                   0, 1, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  
  frame = gtk_frame_new(TR("Tasklist"));
  gtk_container_add(GTK_CONTAINER(frame),
                    track_list_get_widget(ret->tracklist));

  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(main_table),
                            frame,
                            0, 1, 4, 5);
  
  gtk_widget_show(main_table);
  gtk_container_add(GTK_CONTAINER(ret->win), main_table);
  
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
  
  
  return ret;
  }

void transcoder_window_destroy(transcoder_window_t* w)
  {
  char * tmp_path;
  bg_cfg_section_t * cfg_section;

  bg_encoder_section_store_in_registry(w->plugin_reg,
                                       w->encoder_section,
                                       w->encoder_parameters,
                                       stream_flags, plugin_flags);

  bg_cfg_section_destroy(w->encoder_section);
  bg_parameter_info_destroy_array(w->encoder_parameters);
  
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
  if(w->profile_path)
    free(w->profile_path);
  
  bg_msg_queue_destroy(w->msg_queue);
  
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
        track_list_add_albumentries_xml(win->tracklist, arg_str);
        free(arg_str);
        break;
      case TRANSCODER_REMOTE_ADD_FILE:
        arg_str = bg_msg_get_arg_string(msg, 0);
        track_list_add_url(win->tracklist, arg_str);
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

static const bg_parameter_info_t input_plugin_parameters[] =
  {
    {
      .name = "input_plugins",
      .long_name = "Input plugins",
    },
    { /* */ },
  };

static const bg_parameter_info_t image_reader_parameters[] =
  {
    {
      .name = "image_readers",
      .long_name = "Image readers",
    },
    { /* */ },
  };

/* Configuration stuff */

static void transcoder_window_preferences(transcoder_window_t * w)
  {
  bg_dialog_t * dlg;
  bg_cfg_section_t * cfg_section;
  void * parent;

  bg_audio_filter_chain_t * ac;
  bg_video_filter_chain_t * vc;

  bg_gavl_audio_options_t ao;
  bg_gavl_video_options_t vo;

  bg_parameter_info_t * params_i;
  bg_parameter_info_t * params_ir;

  params_i = bg_parameter_info_copy_array(input_plugin_parameters);
  params_ir = bg_parameter_info_copy_array(image_reader_parameters);

  bg_plugin_registry_set_parameter_info_input(w->plugin_reg,
                                              BG_PLUGIN_INPUT,
                                              BG_PLUGIN_FILE|
                                              BG_PLUGIN_URL|
                                              BG_PLUGIN_REMOVABLE|
                                              BG_PLUGIN_TUNER,
                                              params_i);
  bg_plugin_registry_set_parameter_info_input(w->plugin_reg,
                                              BG_PLUGIN_IMAGE_READER,
                                              BG_PLUGIN_FILE,
                                              params_ir);
  
  memset(&ao, 0, sizeof(ao));
  memset(&vo, 0, sizeof(vo));

  bg_gavl_audio_options_init(&ao);
  bg_gavl_video_options_init(&vo);
  
  ac = bg_audio_filter_chain_create(&ao, w->plugin_reg);
  vc = bg_video_filter_chain_create(&vo, w->plugin_reg);
  
  dlg = bg_dialog_create_multi(TR("Transcoder configuration"));

  cfg_section     = bg_cfg_registry_find_section(w->cfg_reg, "output");

  bg_dialog_add(dlg,
                TR("Output options"),
                cfg_section,
                NULL,
                NULL,
                NULL,
                bg_transcoder_get_parameters());

  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "general");
  
  bg_dialog_add(dlg,
                TR("Track defaults"),
                cfg_section,
                NULL,
                NULL,
                NULL,
                bg_transcoder_track_get_general_parameters());

  
  parent = bg_dialog_add_parent(dlg, NULL,
                                TR("Audio defaults"));

  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "audio");
  bg_dialog_add_child(dlg, parent,
                TR("General"),
                cfg_section,
                NULL,
                NULL,
                NULL,
                bg_transcoder_track_audio_get_general_parameters());


  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "audiofilters");
  bg_dialog_add_child(dlg, parent,
                TR("Filters"),
                cfg_section,
                NULL,
                NULL,
                NULL,
                bg_audio_filter_chain_get_parameters(ac));

  
  
  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "video");

  parent = bg_dialog_add_parent(dlg, NULL,
                                TR("Video defaults"));

  
  bg_dialog_add_child(dlg, parent, TR("General"),
                      cfg_section,
                      NULL,
                      NULL,
                      NULL,
                      bg_transcoder_track_video_get_general_parameters());

  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "videofilters");
  bg_dialog_add_child(dlg, parent,
                      TR("Filters"),
                      cfg_section,
                      NULL,
                      NULL,
                      NULL,
                      bg_video_filter_chain_get_parameters(vc));
  
  parent = bg_dialog_add_parent(dlg, NULL,
                                TR("Text subtitle defaults"));
    
  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "subtitle_text");

  bg_dialog_add_child(dlg, parent, TR("General"),
                      cfg_section,
                      NULL,
                      NULL,
                      NULL,
                      bg_transcoder_track_subtitle_text_get_general_parameters());

  cfg_section = bg_cfg_section_find_subsection(w->track_defaults_section,
                                               "textrenderer");

  bg_dialog_add_child(dlg, parent, TR("Textrenderer"),
                      cfg_section,
                      NULL,
                      NULL,
                      NULL,
                      bg_text_renderer_get_parameters());
  
  cfg_section = bg_cfg_registry_find_section(w->cfg_reg,
                                             "subtitle_overlay");

  bg_dialog_add(dlg,
                TR("Overlay subtitle defaults"),
                cfg_section,
                NULL,
                NULL,
                NULL,
                bg_transcoder_track_subtitle_overlay_get_general_parameters());

  bg_dialog_add(dlg,
                TR("Encoders"),
                w->encoder_section,
                NULL,
                NULL,
                NULL,
                w->encoder_parameters);
  
  bg_dialog_add(dlg,
                TR("Input plugins"),
                NULL,
                bg_plugin_registry_set_parameter_input,
                bg_plugin_registry_get_parameter_input,
                w->plugin_reg,
                params_i);

  bg_dialog_add(dlg,
                TR("Image readers"),
                NULL,
                bg_plugin_registry_set_parameter_input,
                bg_plugin_registry_get_parameter_input,
                w->plugin_reg,
                params_ir);
  
  cfg_section = bg_cfg_registry_find_section(w->cfg_reg,
                                             "transcoder_window");

  bg_dialog_add(dlg,
                TR("Window"),
                cfg_section,
                set_transcoder_window_parameter,
                NULL,
                w,
                transcoder_window_parameters);

  cfg_section = bg_cfg_registry_find_section(w->cfg_reg,
                                             "remote");

  bg_dialog_add(dlg,
                TR("Remote"),
                cfg_section,
                bg_remote_server_set_parameter,
                NULL,
                w->remote,
                bg_remote_server_get_parameters(w->remote));

  cfg_section = bg_cfg_registry_find_section(w->cfg_reg,
                                             "logwindow");

  bg_dialog_add(dlg,
                TR("Log window"),
                cfg_section,
                bg_gtk_log_window_set_parameter,
                NULL,
                w->logwindow,
                bg_gtk_log_window_get_parameters(w->logwindow));

  
  bg_dialog_show(dlg, w->win);
  bg_dialog_destroy(dlg);

  bg_audio_filter_chain_destroy(ac);
  bg_video_filter_chain_destroy(vc);

  bg_gavl_audio_options_free(&ao);
  bg_gavl_video_options_free(&vo);
    
  }

