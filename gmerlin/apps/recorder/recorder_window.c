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

#include <config.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#include <gmerlin/cfg_dialog.h>

#include <gmerlin/gui_gtk/gtkutils.h>
#include <gmerlin/gui_gtk/audio.h>
#include <gmerlin/gui_gtk/aboutwindow.h>
#include <gmerlin/gui_gtk/logwindow.h>

#include <gmerlin/recorder.h>
#include "recorder_window.h"

struct bg_recorder_window_s
  {
  GtkWidget * win;
  bg_recorder_t * rec;
  GtkWidget * socket;
  bg_gtk_vumeter_t * vumeter;

  GtkWidget * statusbar;
  guint framerate_context;

  GtkWidget * about_button;
  GtkWidget * log_button;
  GtkWidget * config_button;
  
  bg_dialog_t * cfg_dialog;
  
  bg_gtk_log_window_t * logwindow;

  bg_cfg_section_t * general_section;

  bg_cfg_section_t * audio_filter_section;
  bg_cfg_section_t * video_filter_section;
  bg_cfg_section_t * video_monitor_section;
  
  bg_cfg_section_t * audio_section;
  bg_cfg_section_t * video_section;
  
  bg_cfg_section_t * log_section;
  };

static void about_window_close_callback(bg_gtk_about_window_t * w,
                                        void * data)
  {
  bg_recorder_window_t * win;
  win = (bg_recorder_window_t *)data;
  gtk_widget_set_sensitive(win->about_button, 1);
  }

static void log_window_close_callback(bg_gtk_log_window_t * w,
                                      void * data)
  {
  bg_recorder_window_t * win;
  win = (bg_recorder_window_t *)data;
  gtk_widget_set_sensitive(win->log_button, 1);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  bg_recorder_window_t * win;
  win = (bg_recorder_window_t *)data;
  if(w == win->log_button)
    {
    gtk_widget_set_sensitive(win->log_button, 0);
    bg_gtk_log_window_show(win->logwindow);
    }
  else if(w == win->config_button)
    {
    bg_dialog_show(win->cfg_dialog, win->win);
    }
  }

static void delete_callback(GtkWidget * w, GdkEvent * evt, gpointer data)
  {
  bg_recorder_window_t * win = (bg_recorder_window_t *)data;
  gtk_widget_hide(win->win);
  gtk_main_quit();
  }


static GtkWidget * create_pixmap_button(bg_recorder_window_t * w,
                                        const char * filename,
                                        const char * tooltip)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(button_callback), w);

  gtk_widget_show(button);

  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);
  
  return button;
  }

static void socket_realize(GtkWidget * w, gpointer data)
  {
  GdkDisplay * dpy;
  char * str;
  bg_recorder_window_t * win = data;

  dpy = gdk_display_get_default();
  
  str = bg_sprintf("%s:%08lx:", gdk_display_get_name(dpy),
                   (long unsigned int)gtk_socket_get_id(GTK_SOCKET(win->socket)));

  bg_recorder_set_display_string(win->rec, str);
  free(str);

  /* Now that we have the display string we can apply the config sections,
     which will load the plugins */
  bg_cfg_section_apply(win->audio_section,
                       bg_recorder_get_audio_parameters(win->rec),
                       bg_recorder_set_audio_parameter,
                       win->rec);
  bg_cfg_section_apply(win->audio_filter_section,
                       bg_recorder_get_audio_filter_parameters(win->rec),
                       bg_recorder_set_audio_filter_parameter,
                       win->rec);

  bg_cfg_section_apply(win->video_section,
                       bg_recorder_get_video_parameters(win->rec),
                       bg_recorder_set_video_parameter,
                       win->rec);
  bg_cfg_section_apply(win->video_filter_section,
                       bg_recorder_get_video_filter_parameters(win->rec),
                       bg_recorder_set_video_filter_parameter,
                       win->rec);

  bg_cfg_section_apply(win->video_monitor_section,
                       bg_recorder_get_video_monitor_parameters(win->rec),
                       bg_recorder_set_video_monitor_parameter,
                       win->rec);
  }

bg_recorder_window_t *
bg_recorder_window_create(bg_cfg_registry_t * cfg_reg,
                          bg_plugin_registry_t * plugin_reg)
  {
  void * parent;

  GtkWidget * box;
  GtkWidget * mainbox;
  bg_recorder_window_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  /* Create recorder */  
  ret->rec = bg_recorder_create(plugin_reg);

  /* Create widgets */  
  ret->win = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(G_OBJECT(ret->win), "delete-event", G_CALLBACK(delete_callback),
                   ret);
  
  ret->socket = gtk_socket_new();

  g_signal_connect(G_OBJECT(ret->socket), "realize",
                   G_CALLBACK(socket_realize), ret);
  
  gtk_widget_show(ret->socket);

  ret->vumeter = bg_gtk_vumeter_create(2, 1);

  ret->about_button =
    create_pixmap_button(ret, "info_16.png", TRS("About"));
  ret->log_button =
    create_pixmap_button(ret, "log_16.png", TRS("Log messages"));
  ret->config_button =
    create_pixmap_button(ret, "config_16.png", TRS("Preferences"));

  /* Create other windows */
  ret->logwindow =
    bg_gtk_log_window_create(log_window_close_callback,
                             ret, "Recorder");

  
  /* Pack everything */

  mainbox = gtk_vbox_new(0, 0);

  /* Monitor */
  box = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->socket, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(box), bg_gtk_vumeter_get_widget(ret->vumeter),
                     FALSE, FALSE, 0);
  gtk_widget_show(box);
  
  gtk_box_pack_start(GTK_BOX(mainbox),
                     box,
                     TRUE, TRUE, 0);
  
  /* Display and buttons */

  box = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->config_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->about_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->log_button, FALSE, FALSE, 0);
  gtk_widget_show(box);

  gtk_box_pack_start(GTK_BOX(mainbox),
                     box,
                     FALSE, FALSE, 0);
  
  /* Statusbar */
  
  /* */
    
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(ret->win), mainbox);

  /* Config stuff */

  ret->log_section = bg_cfg_registry_find_section(cfg_reg, "log");
  ret->audio_section = bg_cfg_registry_find_section(cfg_reg, "audio");
  ret->video_section = bg_cfg_registry_find_section(cfg_reg, "video");

  ret->audio_filter_section =
    bg_cfg_registry_find_section(cfg_reg, "audiofilter");
  ret->video_filter_section =
    bg_cfg_registry_find_section(cfg_reg, "videofilter");
  ret->video_monitor_section =
    bg_cfg_registry_find_section(cfg_reg, "video_monitor");
  
  ret->cfg_dialog = bg_dialog_create_multi(TR("Recorder confiuration"));

  /* Audio */
  parent = bg_dialog_add_parent(ret->cfg_dialog, NULL, TRS("Audio"));
  bg_dialog_add_child(ret->cfg_dialog, parent,
                      TRS("General"),
                      ret->audio_section,
                      bg_recorder_set_audio_parameter,
                      NULL,
                      ret->rec,
                      bg_recorder_get_audio_parameters(ret->rec));

  bg_dialog_add_child(ret->cfg_dialog, parent,
                      TRS("Filters"),
                      ret->audio_filter_section,
                      bg_recorder_set_audio_filter_parameter,
                      NULL,
                      ret->rec,
                      bg_recorder_get_audio_filter_parameters(ret->rec));

  /* Video */
  parent = bg_dialog_add_parent(ret->cfg_dialog, NULL, TRS("Video"));
  
  bg_dialog_add_child(ret->cfg_dialog, parent,
                      TRS("General"),
                      ret->video_section,
                      bg_recorder_set_video_parameter,
                      NULL,
                      ret->rec,
                      bg_recorder_get_video_parameters(ret->rec));

  bg_dialog_add_child(ret->cfg_dialog, parent,
                      TRS("Filters"),
                      ret->video_filter_section,
                      bg_recorder_set_video_filter_parameter,
                      NULL,
                      ret->rec,
                      bg_recorder_get_video_filter_parameters(ret->rec));

  bg_dialog_add_child(ret->cfg_dialog, parent,
                      TRS("Monitor"),
                      ret->video_monitor_section,
                      bg_recorder_set_video_monitor_parameter,
                      NULL,
                      ret->rec,
                      bg_recorder_get_video_monitor_parameters(ret->rec));

  bg_dialog_add(ret->cfg_dialog,
                TR("Log window"),
                ret->log_section,
                bg_gtk_log_window_set_parameter,
                NULL,
                (void*)(ret->logwindow),
                bg_gtk_log_window_get_parameters(ret->logwindow));

  
  /* Apply config sections */


  bg_cfg_section_apply(ret->log_section,
                       bg_gtk_log_window_get_parameters(ret->logwindow),
                       bg_gtk_log_window_set_parameter,
                       ret->logwindow);
  return ret;
  }

void bg_recorder_window_destroy(bg_recorder_window_t * win)
  {
  bg_recorder_destroy(win->rec);
  
  free(win);
  }

void bg_recorder_window_run(bg_recorder_window_t * win)
  {
  gtk_widget_show(win->win);
  bg_recorder_run(win->rec);
  gtk_main();
  }
