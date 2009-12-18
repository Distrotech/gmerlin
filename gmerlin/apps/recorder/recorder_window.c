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
#include <string.h>

#include <gtk/gtk.h>

#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#include <gmerlin/cfg_dialog.h>

#include <gui_gtk/gtkutils.h>
#include <gui_gtk/audio.h>
#include <gui_gtk/aboutwindow.h>
#include <gui_gtk/logwindow.h>
#include <gui_gtk/display.h>

#include <gmerlin/recorder.h>
#include "recorder_window.h"

#define DELAY_TIME 100

typedef struct
  {
  GtkWidget * win;
  GtkWidget * box;
  GtkWidget * socket;
  } window_t;

struct bg_recorder_window_s
  {
  bg_recorder_t * rec;
  bg_gtk_vumeter_t * vumeter;
  
  GtkWidget * statusbar;
  GtkWidget * toolbar;

  window_t normal_window;
  window_t fullscreen_window;
  window_t * current_window;
  
  guint framerate_context;
  int framerate_shown;
  
  guint noinput_context;
  int noinput_shown;

  guint record_id;
  
  GtkWidget * about_button;
  GtkWidget * log_button;
  GtkWidget * config_button;
  GtkWidget * restart_button;
  GtkWidget * record_button;
  GtkWidget * snapshot_button;

  GtkWidget * fullscreen_button;
  GtkWidget * nofullscreen_button;
  GtkWidget * hide_button;
  
  bg_dialog_t * cfg_dialog;
  
  bg_gtk_log_window_t * logwindow;

  bg_cfg_section_t * general_section;

  bg_cfg_section_t * audio_filter_section;
  bg_cfg_section_t * video_filter_section;
  bg_cfg_section_t * video_monitor_section;
  bg_cfg_section_t * video_snapshot_section;
  
  bg_cfg_section_t * audio_section;
  bg_cfg_section_t * video_section;
  
  bg_cfg_section_t * encoder_section;
  bg_cfg_section_t * output_section;
  bg_cfg_section_t * metadata_section;
  
  bg_cfg_section_t * log_section;
  bg_cfg_section_t * gui_section;
  
  bg_msg_queue_t * msg_queue;
  bg_gtk_time_display_t * display;

  bg_plugin_registry_t * plugin_reg;
  
  gavl_rectangle_i_t win_rect;

  int toolbar_hidden;
  
  };

static const bg_parameter_info_t gui_parameters[] =
  {
    {
      .name = "x",
      .type = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
    },
    {
      .name = "y",
      .type = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
    },
    {
      .name = "width",
      .type = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
    },
    {
      .name = "height",
      .type = BG_PARAMETER_INT,
      .flags = BG_PARAMETER_HIDE_DIALOG,
    },
    { /* End */ }
  };

static void set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val)
  {
  bg_recorder_window_t * win = data;
  if(!name)
    return;
  else if(!strcmp(name, "x"))
    {
    win->win_rect.x = val->val_i;
    }
  else if(!strcmp(name, "y"))
    {
    win->win_rect.y = val->val_i;
    }
  else if(!strcmp(name, "width"))
    {
    win->win_rect.w = val->val_i;
    }
  else if(!strcmp(name, "height"))
    {
    win->win_rect.h = val->val_i;
    }
  }

static int get_parameter(void * data, const char * name,
                         bg_parameter_value_t * val)
  {
  bg_recorder_window_t * win = data;
  if(!name)
    return 1;
  else if(!strcmp(name, "x"))
    {
    val->val_i = win->win_rect.x;
    return 1;
    }
  else if(!strcmp(name, "y"))
    {
    val->val_i = win->win_rect.y;
    return 1;
    }
  else if(!strcmp(name, "width"))
    {
    val->val_i = win->win_rect.w;
    return 1;
    }
  else if(!strcmp(name, "height"))
    {
    val->val_i = win->win_rect.h;
    return 1;
    }
  return 0;
  }


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

static void set_fullscreen(bg_recorder_window_t * v)
  {
  /* Reparent toolbar */
  gtk_container_remove(GTK_CONTAINER(v->normal_window.box),
                       v->toolbar);

  gtk_box_pack_start(GTK_BOX(v->fullscreen_window.box),
                     v->toolbar, FALSE, FALSE, 0);

  /* Remember coordinates */

  gdk_window_get_geometry(v->normal_window.win->window,
                          NULL, NULL, &v->win_rect.w, &v->win_rect.h,
                          NULL);
  
  gdk_window_get_root_origin(v->normal_window.win->window,
                             &v->win_rect.x, &v->win_rect.y);
  
  bg_cfg_section_get(v->gui_section,
                     gui_parameters,
                     get_parameter,
                     v);
  
  /* Hide normal window, show fullscreen window */
  gtk_widget_show(v->fullscreen_window.win);
  gtk_widget_hide(v->normal_window.win);
    
  gtk_window_fullscreen(GTK_WINDOW(v->fullscreen_window.win));
  /* Update toolbar */
  gtk_widget_show(v->nofullscreen_button);
  gtk_widget_hide(v->fullscreen_button);
  
  gtk_widget_hide(v->config_button);
  gtk_widget_hide(v->log_button);
  gtk_widget_hide(v->about_button);
  
  
  v->current_window = &v->fullscreen_window;
  
  }

static void set_nofullscreen(bg_recorder_window_t * v)
  {
  /* Reparent toolbar */
  gtk_container_remove(GTK_CONTAINER(v->fullscreen_window.box),
                       v->toolbar);
  gtk_box_pack_start(GTK_BOX(v->normal_window.box),
                     v->toolbar, FALSE, FALSE, 0);
  
  /* Hide normal window, show fullscreen window */
  gtk_widget_show(v->normal_window.win);
  gtk_widget_hide(v->fullscreen_window.win);

  gtk_decorated_window_move_resize_window(GTK_WINDOW(v->normal_window.win),
                                          v->win_rect.x, v->win_rect.y,
                                          v->win_rect.w, v->win_rect.h);

  
  /* Update toolbar */
  gtk_widget_show(v->fullscreen_button);
  gtk_widget_hide(v->nofullscreen_button);
  
  gtk_widget_show(v->config_button);
  gtk_widget_show(v->config_button);
  gtk_widget_show(v->log_button);
  gtk_widget_show(v->about_button);
  
  v->current_window = &v->normal_window;
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
    bg_dialog_show(win->cfg_dialog, win->current_window->win);
    }
  else if(w == win->restart_button)
    {
    bg_recorder_stop(win->rec);
    bg_recorder_run(win->rec);
    }
  else if(w == win->record_button)
    {
    int do_record =
      !!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->record_button));
    gtk_widget_set_sensitive(win->restart_button, !do_record);
    gtk_widget_set_sensitive(win->config_button, !do_record);
    bg_recorder_record(win->rec, do_record);
    }
  else if(w == win->snapshot_button)
    bg_recorder_snapshot(win->rec);
  else if(w == win->hide_button)
    {
    gtk_widget_hide(win->toolbar);
    win->toolbar_hidden = 1;
    }
  else if(w == win->about_button)
    {
    gtk_widget_set_sensitive(win->about_button, 0);
    bg_gtk_about_window_create("Gmerlin recorder", VERSION,
                               "recorder_icon.png",
                               about_window_close_callback,
                               win);
    }
  else if(w == win->fullscreen_button)
    set_fullscreen(win);
  else if(w == win->nofullscreen_button)
    set_nofullscreen(win);
  

  
  }

static gboolean button_press_callback(GtkWidget * w, GdkEventButton * evt,
                                      gpointer * data)
  {
  bg_recorder_window_t * win = (bg_recorder_window_t *)data;
  if(win->toolbar_hidden)
    {
    gtk_widget_show(win->toolbar);
    win->toolbar_hidden = 0;
    }
  return TRUE;
  }



static void delete_callback(GtkWidget * w, GdkEvent * evt, gpointer data)
  {
  bg_recorder_window_t * win = (bg_recorder_window_t *)data;
#if 1
  if(win->current_window == &win->normal_window) /* Remember coordinates */
    {
    gdk_window_get_geometry(win->normal_window.win->window,
                            NULL, NULL, &win->win_rect.w, &win->win_rect.h,
                            NULL);
    
    gdk_window_get_root_origin(win->normal_window.win->window,
                               &win->win_rect.x, &win->win_rect.y);
    
    bg_cfg_section_get(win->gui_section,
                       gui_parameters,
                       get_parameter,
                       win);
    }
#endif
  //  gtk_widget_hide(win->win);
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

static GtkWidget * create_pixmap_toggle_button(bg_recorder_window_t * w,
                                               const char * filename,
                                               const char * tooltip, guint * callback_id)
  {
  GtkWidget * button;
  GtkWidget * image;
  char * path;
  guint id;
  
  path = bg_search_file_read("icons", filename);
  if(path)
    {
    image = gtk_image_new_from_file(path);
    free(path);
    }
  else
    image = gtk_image_new();

  gtk_widget_show(image);
  button = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(button), image);
  
  id= g_signal_connect(G_OBJECT(button), "toggled",
                       G_CALLBACK(button_callback), w);

  if(callback_id)
    *callback_id = id;
  
  gtk_widget_show(button);
  
  bg_gtk_tooltips_set_tip(button, tooltip, PACKAGE);
  
  return button;
  }

static gboolean timeout_func(void * data)
  {
  bg_msg_t * msg;
  bg_recorder_window_t * win = data;
  
  while((msg = bg_msg_queue_try_lock_read(win->msg_queue)))
    {
    switch(bg_msg_get_id(msg))
      {
      case BG_RECORDER_MSG_FRAMERATE:
        {
        char * tmp_string;
        float framerate;
        framerate = bg_msg_get_arg_float(msg, 0);
        
        tmp_string = bg_sprintf(TR("Framerate: %.2f fps"), framerate);
        
        if(win->framerate_shown)
          gtk_statusbar_pop(GTK_STATUSBAR(win->statusbar),
                            win->framerate_context);
        else
          win->framerate_shown = 1;
        
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar),
                           win->framerate_context,
                           tmp_string);
        free(tmp_string);
        }
        break; 
      case BG_RECORDER_MSG_AUDIOLEVEL:
        {
        double l[2];
        int samples;
        l[0] = bg_msg_get_arg_float(msg, 0);
        l[1] = bg_msg_get_arg_float(msg, 1);
        samples = bg_msg_get_arg_int(msg, 2);
        bg_gtk_vumeter_update_peak(win->vumeter,
                                   l, samples);
        }
        break; 
      case BG_RECORDER_MSG_TIME:
        {
        gavl_time_t t = bg_msg_get_arg_time(msg, 0);
        bg_gtk_time_display_update(win->display,
                                   t,
                                   BG_GTK_DISPLAY_MODE_HMS);
        }
        break;
      case BG_RECORDER_MSG_RUNNING:
        {
        int do_audio, do_video;
        
        do_audio = bg_msg_get_arg_int(msg, 0);
        if(do_audio)
          gtk_widget_show(bg_gtk_vumeter_get_widget(win->vumeter));
        else
          gtk_widget_hide(bg_gtk_vumeter_get_widget(win->vumeter));
        
        do_video = bg_msg_get_arg_int(msg, 1);
        if(do_video)
          {
          gtk_widget_show(win->current_window->socket);
          gtk_widget_set_sensitive(win->snapshot_button, 1);
          gtk_widget_show(win->hide_button);
          }
        else
          {
          gtk_widget_hide(win->current_window->socket);
          gtk_widget_hide(win->hide_button);
          
          gtk_widget_set_sensitive(win->snapshot_button, 0);
          }
        if(!do_audio && !do_video)
          {
          gtk_statusbar_push(GTK_STATUSBAR(win->statusbar),
                             win->noinput_context,
                             TR("Error, check messages and settings"));
          win->noinput_shown = 1;

          g_signal_handler_block(G_OBJECT(win->record_button), win->record_id);
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->record_button), 0);
          g_signal_handler_unblock(G_OBJECT(win->record_button), win->record_id);

          gtk_widget_set_sensitive(win->config_button, 1);
          gtk_widget_set_sensitive(win->restart_button, 1);
          gtk_widget_set_sensitive(win->snapshot_button, 1);
          }
        else if(win->noinput_shown)
          {
          gtk_statusbar_pop(GTK_STATUSBAR(win->statusbar),
                            win->noinput_context);
          }
        }
      }
    bg_msg_queue_unlock_read(win->msg_queue);
    }


  return TRUE;
  }

static float display_fg[] = { 0.0, 1.0, 0.0 };
static float display_bg[] = { 0.0, 0.0, 0.0 };

static void window_init(bg_recorder_window_t * r, window_t * w, int fullscreen)
  {

  /* Window */
  w->win = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(w->win), "Gmerlin-recorder-"VERSION);
  
  if(fullscreen)
    gtk_window_fullscreen(GTK_WINDOW(w->win));

  /* Socket */
  w->socket = gtk_socket_new();
  gtk_widget_set_events(w->socket,
                        GDK_BUTTON_PRESS_MASK);

  g_signal_connect(G_OBJECT(w->socket), "button-press-event",
                   G_CALLBACK(button_press_callback),
                   r);
  
  gtk_widget_show(w->socket);
  
  w->box = gtk_vbox_new(0, 0);

  gtk_box_pack_start(GTK_BOX(w->box), w->socket, TRUE, TRUE, 0);
  
  gtk_widget_show(w->box);
  
  gtk_container_add(GTK_CONTAINER(w->win), w->box);
  
  g_signal_connect(G_OBJECT(w->win), "delete_event",
                   G_CALLBACK(delete_callback),
                   r);
  
  g_signal_connect(G_OBJECT(w->socket), "button-press-event",
                   G_CALLBACK(button_press_callback),
                   r);
  gtk_widget_realize(w->socket);
  }


bg_recorder_window_t *
bg_recorder_window_create(bg_cfg_registry_t * cfg_reg,
                          bg_plugin_registry_t * plugin_reg)
  {
  void * parent;
  GdkDisplay * dpy;
  char * tmp_string;
  
  GtkWidget * box;
  GtkWidget * mainbox;
  bg_recorder_window_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  /* Create recorder */  
  ret->plugin_reg = plugin_reg;

  ret->rec = bg_recorder_create(plugin_reg);

  ret->msg_queue = bg_msg_queue_create();
  bg_recorder_add_message_queue(ret->rec, ret->msg_queue);
    
  /* Create widgets */  
  
  ret->display = bg_gtk_time_display_create(BG_GTK_DISPLAY_SIZE_SMALL,
                                            4, BG_GTK_DISPLAY_MODE_HMS);

  bg_gtk_time_display_update(ret->display,
                             GAVL_TIME_UNDEFINED, BG_GTK_DISPLAY_MODE_HMS);

  bg_gtk_time_display_set_colors(ret->display, display_fg, display_bg );
    
  ret->vumeter = bg_gtk_vumeter_create(2, 0);
  
  ret->about_button =
    create_pixmap_button(ret, "info_16.png", TRS("About"));
  ret->log_button =
    create_pixmap_button(ret, "log_16.png", TRS("Log messages"));
  ret->config_button =
    create_pixmap_button(ret, "config_16.png", TRS("Preferences"));
  ret->restart_button =
    create_pixmap_button(ret, "refresh_16.png", TRS("Reopen inputs"));
  ret->record_button =
    create_pixmap_toggle_button(ret, "record_16.png", TRS("Record"), &ret->record_id);
  ret->snapshot_button =
    create_pixmap_button(ret, "snapshot_16.png", TRS("Make snapshot"));

  ret->fullscreen_button =
    create_pixmap_button(ret, "fullscreen_16.png", TRS("Fullscreen mode"));
  ret->nofullscreen_button =
    create_pixmap_button(ret, "windowed_16.png", TRS("Leave fullscreen mode"));
  ret->hide_button =
    create_pixmap_button(ret, "close_16.png", TRS("Hide controls"));
  
  ret->statusbar = gtk_statusbar_new();
  ret->framerate_context =
    gtk_statusbar_get_context_id(GTK_STATUSBAR(ret->statusbar),
                                 "framerate");
  ret->noinput_context =
    gtk_statusbar_get_context_id(GTK_STATUSBAR(ret->statusbar),
                                 "noinput");
  
  gtk_widget_show(ret->statusbar);
  
  /* Create other windows */
  ret->logwindow =
    bg_gtk_log_window_create(log_window_close_callback,
                             ret, "Recorder");
  
  /* Pack everything */

  mainbox = gtk_vbox_new(0, 0);
  ret->toolbar = gtk_vbox_new(0, 0);
  
  /* Display and buttons */

  gtk_box_pack_start(GTK_BOX(ret->toolbar), bg_gtk_vumeter_get_widget(ret->vumeter),
                     FALSE, FALSE, 0);

  
  box = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->hide_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->config_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->about_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->log_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->restart_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->record_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->snapshot_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->fullscreen_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), ret->nofullscreen_button, FALSE, FALSE, 0);
  
  gtk_box_pack_end(GTK_BOX(box), bg_gtk_time_display_get_widget(ret->display),
                   FALSE, FALSE, 0);
  
  gtk_widget_show(box);

  gtk_box_pack_start(GTK_BOX(ret->toolbar), box, FALSE, FALSE, 0);
    
  /* Statusbar */

  gtk_box_pack_start(GTK_BOX(ret->toolbar),
                     ret->statusbar,
                     FALSE, FALSE, 0);

  g_object_ref(ret->toolbar);
  gtk_widget_show(ret->toolbar);
  
  /* Config stuff */

  ret->log_section = bg_cfg_registry_find_section(cfg_reg, "log");
  ret->gui_section = bg_cfg_registry_find_section(cfg_reg, "gui");
  
  ret->audio_section = bg_cfg_registry_find_section(cfg_reg, "audio");
  ret->video_section = bg_cfg_registry_find_section(cfg_reg, "video");

  ret->audio_filter_section =
    bg_cfg_registry_find_section(cfg_reg, "audiofilter");
  ret->video_filter_section =
    bg_cfg_registry_find_section(cfg_reg, "videofilter");
  ret->video_monitor_section =
    bg_cfg_registry_find_section(cfg_reg, "video_monitor");
  ret->video_snapshot_section =
    bg_cfg_registry_find_section(cfg_reg, "video_snapshot");
  ret->output_section =
    bg_cfg_registry_find_section(cfg_reg, "output");
  ret->metadata_section =
    bg_cfg_registry_find_section(cfg_reg, "metadata");
  
  /* Encoders */

  ret->encoder_section =
    bg_encoder_section_get_from_registry(plugin_reg,
                                         bg_recorder_get_encoder_parameters(ret->rec),
                                         bg_recorder_stream_mask,
                                         bg_recorder_plugin_mask);
  
  bg_recorder_set_encoder_section(ret->rec, ret->encoder_section);
  
  ret->cfg_dialog = bg_dialog_create_multi(TR("Recorder configuration"));
  bg_dialog_set_plugin_registry(ret->cfg_dialog, plugin_reg);

  
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

  bg_dialog_add_child(ret->cfg_dialog, parent,
                      TRS("Snapshots"),
                      ret->video_snapshot_section,
                      bg_recorder_set_video_snapshot_parameter,
                      NULL,
                      ret->rec,
                      bg_recorder_get_video_snapshot_parameters(ret->rec));
  
  /* Other stuff */
  
  bg_dialog_add(ret->cfg_dialog,
                TR("Encoders"),
                ret->encoder_section,
                NULL,
                NULL,
                NULL,
                bg_recorder_get_encoder_parameters(ret->rec));

  /* Output */
  
  bg_dialog_add(ret->cfg_dialog,
                TR("Output files"),
                ret->output_section,
                NULL,
                NULL,
                NULL,
                bg_recorder_get_output_parameters(ret->rec));

  bg_dialog_add(ret->cfg_dialog,
                TR("Metadata"),
                ret->metadata_section,
                NULL,
                NULL,
                NULL,
                bg_recorder_get_metadata_parameters(ret->rec));
  
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

  bg_cfg_section_apply(ret->gui_section,
                       gui_parameters,
                       set_parameter,
                       ret);
  
  bg_cfg_section_apply(ret->output_section,
                       bg_recorder_get_output_parameters(ret->rec),
                       bg_recorder_set_output_parameter,
                       ret->rec);

  /* Initialize windows */

  dpy = gdk_display_get_default();
  
  window_init(ret, &ret->normal_window, 0);
  window_init(ret, &ret->fullscreen_window, 1);
  
  tmp_string =
    bg_sprintf("%s:%08lx:%08lx", gdk_display_get_name(dpy),
               (long unsigned int)gtk_socket_get_id(GTK_SOCKET(ret->normal_window.socket)),
               (long unsigned int)gtk_socket_get_id(GTK_SOCKET(ret->fullscreen_window.socket)));
  bg_recorder_set_display_string(ret->rec, tmp_string);
  free(tmp_string);

  /* Now that we have the display string we can apply the config sections,
     which will load the plugins */
  bg_cfg_section_apply(ret->audio_section,
                       bg_recorder_get_audio_parameters(ret->rec),
                       bg_recorder_set_audio_parameter,
                       ret->rec);
  bg_cfg_section_apply(ret->audio_filter_section,
                       bg_recorder_get_audio_filter_parameters(ret->rec),
                       bg_recorder_set_audio_filter_parameter,
                       ret->rec);

  bg_cfg_section_apply(ret->video_section,
                       bg_recorder_get_video_parameters(ret->rec),
                       bg_recorder_set_video_parameter,
                       ret->rec);
  bg_cfg_section_apply(ret->video_filter_section,
                       bg_recorder_get_video_filter_parameters(ret->rec),
                       bg_recorder_set_video_filter_parameter,
                       ret->rec);

  bg_cfg_section_apply(ret->video_monitor_section,
                       bg_recorder_get_video_monitor_parameters(ret->rec),
                       bg_recorder_set_video_monitor_parameter,
                       ret->rec);

  bg_cfg_section_apply(ret->video_snapshot_section,
                       bg_recorder_get_video_snapshot_parameters(ret->rec),
                       bg_recorder_set_video_snapshot_parameter,
                       ret->rec);

  /* Intialize windowed mode */
  ret->current_window = &ret->normal_window;
  gtk_box_pack_start(GTK_BOX(ret->current_window->box),
                     ret->toolbar, FALSE, FALSE, 0);
  gtk_widget_hide(ret->nofullscreen_button);
  
  /* Timeout */
  g_timeout_add(DELAY_TIME, timeout_func, ret);
  
  return ret;
  }

void bg_recorder_window_destroy(bg_recorder_window_t * win)
  {
  bg_encoder_section_store_in_registry(win->plugin_reg,
                                       win->encoder_section,
                                       bg_recorder_get_encoder_parameters(win->rec),
                                       bg_recorder_stream_mask,
                                       bg_recorder_plugin_mask);

  
  bg_recorder_destroy(win->rec);
  bg_cfg_section_destroy(win->encoder_section);
  free(win);
  }

void bg_recorder_window_run(bg_recorder_window_t * win)
  {
  gtk_widget_show(win->current_window->win);
  
  if((win->win_rect.w > 0) && (win->win_rect.h > 0))
    {
    gtk_decorated_window_move_resize_window(GTK_WINDOW(win->current_window->win),
                                            win->win_rect.x, win->win_rect.y,
                                            win->win_rect.w, win->win_rect.h);
    }
  else
    {
    gtk_window_set_default_size(GTK_WINDOW(win->current_window->win),
                                320, 300);
    }
  
  bg_recorder_run(win->rec);
  
  gtk_main();
  }
