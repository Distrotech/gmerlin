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
#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/cfg_dialog.h>

#include "webcam.h"
#include "webcam_window.h"

#include <gui_gtk/plugin.h>
#include <gui_gtk/fileentry.h>
#include <gui_gtk/message.h>
#include <gui_gtk/gtkutils.h>
#include <gui_gtk/aboutwindow.h>
#include <gui_gtk/logwindow.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "camelot"

#define DELAY_TIME 100

struct gmerlin_webcam_window_s 
  {
  bg_plugin_registry_t * plugin_reg;
  gmerlin_webcam_t * cam;
  GtkWidget * win;
  bg_gtk_plugin_widget_single_t * input_plugin;
  bg_gtk_plugin_widget_single_t * capture_plugin;
  bg_gtk_plugin_widget_single_t * monitor_plugin;

  GtkWidget * input_reopen;
  GtkWidget * capture_button;
  GtkWidget * auto_capture;
  GtkWidget * capture_interval;
  
  GtkWidget * monitor_button;
  GtkWidget * output_button;
  GtkWidget * output_framerate;
  GtkWidget * capture_framerate;
  
  GtkWidget * statusbar;
  GtkWidget * about_button;
  GtkWidget * log_button;
  GtkWidget * filter_button;

  bg_msg_queue_t * cmd_queue;
  bg_msg_queue_t * msg_queue;
      
  bg_gtk_file_entry_t * output_dir;
  GtkWidget * output_filename_base;
  GtkWidget * output_frame_counter;
  GtkWidget * output_set_frame_counter;

  guint framerate_context;

  int framerate_set;

  bg_cfg_section_t * general_section;
  bg_cfg_section_t * filter_section;
  
  bg_gtk_log_window_t * logwindow;
  bg_dialog_t * filter_dialog;

  GtkWidget * vloopback_device;
  GtkWidget * vloopback_button;
  
  };

static void set_input_plugin(const bg_plugin_info_t * info, void * data)
  {
  bg_msg_t * msg;
  gmerlin_webcam_window_t * w;
  bg_plugin_handle_t * h;
  w = (gmerlin_webcam_window_t *)data;
  h = bg_gtk_plugin_widget_single_load_plugin(w->input_plugin);
  bg_plugin_registry_set_default(w->plugin_reg, BG_PLUGIN_RECORDER_VIDEO, info->name);

  msg = bg_msg_queue_lock_write(w->cmd_queue);
  bg_msg_set_id(msg, CMD_SET_INPUT_PLUGIN);
  bg_msg_set_arg_ptr_nocopy(msg, 0, h);
  bg_msg_queue_unlock_write(w->cmd_queue);
  
  //  gmerlin_webcam_set_input_plugin(w->cam, h);
  }

static void set_capture_plugin(const bg_plugin_info_t * info, void *  data)
  {
  bg_msg_t * msg;
  gmerlin_webcam_window_t * w;
  bg_plugin_handle_t * h;
  w = (gmerlin_webcam_window_t *)data;
  h = bg_gtk_plugin_widget_single_load_plugin(w->capture_plugin);
  bg_plugin_registry_set_default(w->plugin_reg, BG_PLUGIN_IMAGE_WRITER, info->name);
  
  msg = bg_msg_queue_lock_write(w->cmd_queue);
  bg_msg_set_id(msg, CMD_SET_CAPTURE_PLUGIN);
  bg_msg_set_arg_ptr_nocopy(msg, 0, h);
  bg_msg_queue_unlock_write(w->cmd_queue);
  }

static void set_monitor_plugin(const bg_plugin_info_t * info, void *  data)
  {
  bg_msg_t * msg;
  gmerlin_webcam_window_t * w;
  bg_plugin_handle_t * h;
  w = (gmerlin_webcam_window_t *)data;
  h = bg_gtk_plugin_widget_single_load_plugin(w->monitor_plugin);
  bg_plugin_registry_set_default(w->plugin_reg, BG_PLUGIN_OUTPUT_VIDEO, info->name);
                                 
  msg = bg_msg_queue_lock_write(w->cmd_queue);
  bg_msg_set_id(msg, CMD_SET_MONITOR_PLUGIN);
  bg_msg_set_arg_ptr_nocopy(msg, 0, h);
  bg_msg_queue_unlock_write(w->cmd_queue);
  }

static void about_window_close_callback(bg_gtk_about_window_t * w,
                                        void * data)
  {
  gmerlin_webcam_window_t * win;
  win = (gmerlin_webcam_window_t *)data;
  gtk_widget_set_sensitive(win->about_button, 1);
  }

static void log_window_close_callback(bg_gtk_log_window_t * w,
                                      void * data)
  {
  gmerlin_webcam_window_t * win;
  win = (gmerlin_webcam_window_t *)data;
  gtk_widget_set_sensitive(win->log_button, 1);
  }

static void button_callback(GtkWidget * w, gpointer data)
  {
  int i;
  bg_msg_t * msg;
  
  gmerlin_webcam_window_t * win;
  win = (gmerlin_webcam_window_t *)data;
  if(w == win->monitor_button)
    {
    msg = bg_msg_queue_lock_write(win->cmd_queue);
    bg_msg_set_id(msg, CMD_SET_MONITOR);
    bg_msg_set_arg_int(msg, 0, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->monitor_button)));
    bg_msg_queue_unlock_write(win->cmd_queue);
    }
  else if(w == win->auto_capture)
    {
    i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->auto_capture));
    if(i)
      {
      gtk_widget_set_sensitive(win->capture_button,   0);
      gtk_widget_set_sensitive(win->capture_interval, 1);
      }
    else
      {
      gtk_widget_set_sensitive(win->capture_button,   1);
      gtk_widget_set_sensitive(win->capture_interval, 0);
      }

    msg = bg_msg_queue_lock_write(win->cmd_queue);
    bg_msg_set_id(msg, CMD_SET_CAPTURE_AUTO);
    bg_msg_set_arg_int(msg, 0, i);
    bg_msg_queue_unlock_write(win->cmd_queue);
    }
  else if(w == win->capture_interval)
    {
    msg = bg_msg_queue_lock_write(win->cmd_queue);
    bg_msg_set_id(msg, CMD_SET_CAPTURE_INTERVAL);
    bg_msg_set_arg_float
      (msg, 0, gtk_spin_button_get_value(GTK_SPIN_BUTTON(win->capture_interval)));
    bg_msg_queue_unlock_write(win->cmd_queue);
    }
  else if(w == win->input_reopen)
    {
    msg = bg_msg_queue_lock_write(win->cmd_queue);
    bg_msg_set_id(msg, CMD_INPUT_REOPEN);
    bg_msg_queue_unlock_write(win->cmd_queue);
    }
  else if(w == win->capture_button)
    {
    msg = bg_msg_queue_lock_write(win->cmd_queue);
    bg_msg_set_id(msg, CMD_DO_CAPTURE);
    bg_msg_queue_unlock_write(win->cmd_queue);
    }
  else if(w == win->output_filename_base)
    {
    msg = bg_msg_queue_lock_write(win->cmd_queue);
    bg_msg_set_id(msg, CMD_SET_CAPTURE_NAMEBASE);
    bg_msg_set_arg_string
      (msg, 0, gtk_entry_get_text(GTK_ENTRY(win->output_filename_base)));
    bg_msg_queue_unlock_write(win->cmd_queue);
    }
  else if(w == win->output_set_frame_counter)
    {
    msg = bg_msg_queue_lock_write(win->cmd_queue);
    bg_msg_set_id(msg, CMD_SET_CAPTURE_FRAME_COUNTER);
    bg_msg_set_arg_int
      (msg, 0, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(win->output_frame_counter)));
    bg_msg_queue_unlock_write(win->cmd_queue);
    }
  else if(w == win->about_button)
    {
    gtk_widget_set_sensitive(win->about_button, 0);
    bg_gtk_about_window_create("Camelot", VERSION,
                               "camelot_icon.png", about_window_close_callback,
                               win);
    }
  else if(w == win->log_button)
    {
    gtk_widget_set_sensitive(win->log_button, 0);
    bg_gtk_log_window_show(win->logwindow);
    }
  else if(w == win->filter_button)
    {
    bg_dialog_show(win->filter_dialog, win->win);
    }
#ifdef HAVE_V4L
  else if(w == win->vloopback_button)
    {
    msg = bg_msg_queue_lock_write(win->cmd_queue);
    bg_msg_set_id(msg, CMD_SET_VLOOPBACK);
    bg_msg_set_arg_int
      (msg, 0, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(win->vloopback_button)));
    bg_msg_set_arg_string
      (msg, 1, gtk_entry_get_text(GTK_ENTRY(win->vloopback_device)));
    bg_msg_queue_unlock_write(win->cmd_queue);
    }
#endif
  }

static gboolean timeout_func(gpointer data)
  {
  float arg_f;
  int arg_i;
  char * tmp_string;
  bg_msg_t * msg;
  
  gmerlin_webcam_window_t * w;
  w = (gmerlin_webcam_window_t *)data;
  
  msg = bg_msg_queue_try_lock_read(w->msg_queue);
  if(msg)
    {
    switch(bg_msg_get_id(msg))
      {
      case MSG_FRAMERATE:
        arg_f = bg_msg_get_arg_float(msg, 0);
        tmp_string = bg_sprintf(TR("Framerate: %.2f fps"), arg_f);

        if(w->framerate_set)
          gtk_statusbar_pop(GTK_STATUSBAR(w->statusbar),
                            w->framerate_context);
        else
          w->framerate_set = 1;
        
        gtk_statusbar_push(GTK_STATUSBAR(w->statusbar),
                           w->framerate_context,
                           tmp_string);
        free(tmp_string);
        break;
      case MSG_FRAME_COUNT:
        arg_i = bg_msg_get_arg_int(msg, 0);
        
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->output_frame_counter), (float)arg_i);
        break;
      case MSG_ERROR:
        break;
      }
    bg_msg_queue_unlock_read(w->msg_queue);
    }
  return TRUE;
  }

static void filename_changed_callback(bg_gtk_file_entry_t * entry, void * data)
  {
  bg_msg_t * msg;
  gmerlin_webcam_window_t * w;
  w = (gmerlin_webcam_window_t *)data;

  if(entry == w->output_dir)
    {
    msg = bg_msg_queue_lock_write(w->cmd_queue);
    bg_msg_set_id(msg, CMD_SET_CAPTURE_DIRECTORY);
    bg_msg_set_arg_string(msg, 0, bg_gtk_file_entry_get_filename(w->output_dir));
    bg_msg_queue_unlock_write(w->cmd_queue);
    }
  }

static void delete_callback(GtkWidget * w, GdkEvent * evt, gpointer data)
  {
  gmerlin_webcam_window_t * win = (gmerlin_webcam_window_t *)data;

  bg_cfg_section_get(win->general_section,
                     gmerlin_webcam_window_get_parameters(win),
                     gmerlin_webcam_window_get_parameter,
                     win);

  
  gtk_widget_hide(win->win);
  gtk_main_quit();
  }

static GtkWidget * create_pixmap_button(gmerlin_webcam_window_t * w,
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

gmerlin_webcam_window_t *
gmerlin_webcam_window_create(gmerlin_webcam_t * w,
                             bg_plugin_registry_t * reg,
                             bg_cfg_registry_t * cfg_reg)
  {
  GtkWidget * notebook;
  GtkWidget * mainbox;
  GtkWidget * hbox;
  GtkWidget * label;
  GtkWidget * table;
  GtkWidget * box;
  GtkWidget * frame;
  int row, num_columns;
  
  gmerlin_webcam_window_t * ret;
  
  ret = calloc(1, sizeof(*ret));
  
  ret->cam = w;
  
  ret->logwindow =
    bg_gtk_log_window_create(log_window_close_callback,
                             ret, "Camelot");
  
  gmerlin_webcam_get_message_queues(ret->cam,
                                    &(ret->cmd_queue),
                                    &(ret->msg_queue));
  
  ret->plugin_reg = reg;
    
  ret->win = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(ret->win), "Camelot-"VERSION);
  
  g_signal_connect(G_OBJECT(ret->win), "delete-event", G_CALLBACK(delete_callback),
                   ret);
    
  /* Create input stuff */
    
  ret->input_plugin =
    bg_gtk_plugin_widget_single_create(TR("Plugin"), ret->plugin_reg,
                                       BG_PLUGIN_RECORDER_VIDEO,
                                       BG_PLUGIN_RECORDER);
  bg_gtk_plugin_widget_single_set_change_callback(ret->input_plugin, set_input_plugin, ret);
  
  set_input_plugin(bg_gtk_plugin_widget_single_get_plugin(ret->input_plugin),
                                                          ret);
  
  ret->input_reopen = gtk_button_new_with_label(TR("Reopen"));
  g_signal_connect(G_OBJECT(ret->input_reopen), "clicked",
                   G_CALLBACK(button_callback), ret);
  gtk_widget_show(ret->input_reopen);

  /* Create filter stuff */
  ret->filter_button = gtk_button_new_with_label(TR("Setup filters"));
  g_signal_connect(G_OBJECT(ret->filter_button), "clicked",
                   G_CALLBACK(button_callback), ret);
  gtk_widget_show(ret->filter_button);
  
  /* Create capture stuff */
  
  ret->capture_plugin =
    bg_gtk_plugin_widget_single_create(TR("Plugin"), ret->plugin_reg,
                                       BG_PLUGIN_IMAGE_WRITER,
                                       BG_PLUGIN_FILE);
  bg_gtk_plugin_widget_single_set_change_callback(ret->capture_plugin, set_capture_plugin, ret);

  set_capture_plugin(bg_gtk_plugin_widget_single_get_plugin(ret->capture_plugin),
                                                          ret);

  
  ret->capture_button = gtk_button_new_with_label(TR("Take picture"));
  g_signal_connect(G_OBJECT(ret->capture_button), "clicked", G_CALLBACK(button_callback),
                   ret);
  gtk_widget_show(ret->capture_button);
    
  ret->auto_capture = gtk_check_button_new_with_label(TR("Automatic capture"));
  g_signal_connect(G_OBJECT(ret->auto_capture), "toggled", G_CALLBACK(button_callback),
                   ret);
  gtk_widget_show(ret->auto_capture);

  ret->capture_interval = gtk_spin_button_new_with_range(0.5, 1200.0, 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ret->capture_interval), 1);
  g_signal_connect(G_OBJECT(ret->capture_interval), "value-changed",
                   G_CALLBACK(button_callback), ret);
  gtk_widget_show(ret->capture_interval);

  /* Create vloopback stuff */
#ifdef HAVE_V4L
  ret->vloopback_device = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(ret->vloopback_device), "/dev/video1");
  gtk_widget_show(ret->vloopback_device);

  ret->vloopback_button = gtk_check_button_new_with_label(TR("Enable vloopback"));
  g_signal_connect(G_OBJECT(ret->vloopback_button), "toggled",
                   G_CALLBACK(button_callback), ret);
  
  gtk_widget_show(ret->vloopback_button);
  
#endif
  
  /* Create monitor stuff */
  
  ret->monitor_plugin =
    bg_gtk_plugin_widget_single_create(TR("Plugin"), ret->plugin_reg,
                                       BG_PLUGIN_OUTPUT_VIDEO,
                                       BG_PLUGIN_PLAYBACK);
  bg_gtk_plugin_widget_single_set_change_callback(ret->monitor_plugin,
                                                  set_monitor_plugin, ret);

  set_monitor_plugin(bg_gtk_plugin_widget_single_get_plugin(ret->monitor_plugin),
                                                            ret);
                     
  
  ret->monitor_button = gtk_check_button_new_with_label(TR("Enable Monitor"));
  g_signal_connect(G_OBJECT(ret->monitor_button), "toggled",
                   G_CALLBACK(button_callback), ret);
  
  gtk_widget_show(ret->monitor_button);

  /* Create output file stuff */

  ret->output_dir = bg_gtk_file_entry_create(1, filename_changed_callback, ret,
                                             (char*)0, (char*)0);
  ret->output_filename_base = gtk_entry_new();
  g_signal_connect(ret->output_filename_base, "changed", G_CALLBACK(button_callback),
                   ret);
  
  gtk_widget_show(ret->output_filename_base);
    
  ret->output_frame_counter = gtk_spin_button_new_with_range(0.0, 1000000.0, 1.0);
  //  g_signal_connect(G_OBJECT(ret->output_frame_counter), "value-changed",
  //                   G_CALLBACK(button_callback), ret);
  gtk_widget_show(ret->output_frame_counter);
  
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ret->output_frame_counter), 0);
  
  ret->output_set_frame_counter = gtk_button_new_with_label(TR("Set"));
  g_signal_connect(G_OBJECT(ret->output_set_frame_counter), "clicked",
                   G_CALLBACK(button_callback), ret);
  gtk_widget_show(ret->output_set_frame_counter);
  
  notebook = gtk_notebook_new();

  /* Pack input stuff */

  frame = gtk_frame_new(TR("Input"));

  box = gtk_vbox_new(0, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
    
  table = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);

  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->input_plugin,
                                     table,
                                     &row, &num_columns);

  gtk_table_resize(GTK_TABLE(table), row+1, num_columns);

  gtk_table_attach(GTK_TABLE(table),
                   ret->input_reopen, 
                   0, num_columns, row, row+1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(table);
  
  gtk_widget_show(label);

  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(frame);
  gtk_box_pack_start_defaults(GTK_BOX(box), frame);

  /* Pack filter stuff */

  frame = gtk_frame_new(TR("Filters"));

  table = gtk_table_new(1, 1, 0);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);

  gtk_table_attach(GTK_TABLE(table),
                   ret->filter_button, 
                   0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  gtk_widget_show(table);
  
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(frame);
  gtk_box_pack_start_defaults(GTK_BOX(box), frame);
  
  /* Pack monitor stuff */

  frame = gtk_frame_new(TR("Monitor"));
  
  table = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->monitor_plugin,
                                     table,
                                     &row, &num_columns);
  gtk_table_resize(GTK_TABLE(table), row+1, num_columns);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->monitor_button, 
                   0, num_columns, row, row+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  
  gtk_widget_show(table);

  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(frame);
  gtk_box_pack_start_defaults(GTK_BOX(box), frame);
  
  gtk_widget_show(box);
  
  label = gtk_label_new(TR("Cam"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);

  /* Pack vloopback stuff */

#ifdef HAVE_V4L
  table = gtk_table_new(2, 1, 0);

  label = gtk_label_new(TR("Device"));
  gtk_widget_show(label);

  gtk_table_attach(GTK_TABLE(table),
                   ret->vloopback_button, 
                   0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);


  gtk_table_attach(GTK_TABLE(table),
                   label, 
                   0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   ret->vloopback_device, 
                   1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(table);
  
  label = gtk_label_new(TR("vloopback"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);

  
#endif

  
  /* Pack Capture stuff */

  
  table = gtk_table_new(1, 1, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  row = 0;
  num_columns = 0;

  bg_gtk_plugin_widget_single_attach(ret->capture_plugin,
                                     table,
                                     &row, &num_columns);

  gtk_table_resize(GTK_TABLE(table), row+3, num_columns);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->auto_capture, 
                   0, num_columns, row, row+1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  
  label = gtk_label_new(TR("Interval (sec)"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(table),
                   label, 
                   0, 1, row+1, row+2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table),
                   ret->capture_interval, 
                   1, num_columns, row+1, row+2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(table),
                   ret->capture_button, 
                   0, num_columns, row+2, row+3, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  
  gtk_widget_show(table);
  
  label = gtk_label_new(TR("Capture"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);

  /* Pack output file stuff */

  table = gtk_table_new(3, 3, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);

  label = gtk_label_new(TR("Directory"));
  gtk_widget_show(label);
  gtk_table_attach(GTK_TABLE(table),
                   label, 
                   0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  gtk_table_attach_defaults(GTK_TABLE(table),
                            bg_gtk_file_entry_get_entry(ret->output_dir), 
                            1, 2, 0, 1);

  gtk_table_attach(GTK_TABLE(table),
                   bg_gtk_file_entry_get_button(ret->output_dir), 
                   2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

  box = gtk_vbox_new(0, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  
  label = gtk_label_new(TR("Extension is appended by the plugin\n\
%t    Inserts time\n\
%d    Inserts date\n\
%<i>n Inserts Frame number with <i> digits"));
  gtk_widget_show(label);
  
  gtk_box_pack_start_defaults(GTK_BOX(box), label);
  gtk_box_pack_start_defaults(GTK_BOX(box), ret->output_filename_base);

  gtk_widget_show(box);
  
  frame = gtk_frame_new(TR("Filename base"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
  
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(frame);
  gtk_table_attach_defaults(GTK_TABLE(table),
                            frame, 
                            0, 3, 1, 2);

  label = gtk_label_new(TR("Frame counter"));
  gtk_widget_show(label);

  gtk_table_attach(GTK_TABLE(table),
                   label, 
                   0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->output_frame_counter, 
                   1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach(GTK_TABLE(table),
                   ret->output_set_frame_counter, 
                   2, 3, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_widget_show(table);
  
  label = gtk_label_new(TR("Output files"));
  gtk_widget_show(label);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);
  
  /* Pack rest */
    
  gtk_widget_show(notebook);

  ret->statusbar = gtk_statusbar_new();
  ret->framerate_context =
    gtk_statusbar_get_context_id(GTK_STATUSBAR(ret->statusbar),
                                 "framerate");
  
  gtk_widget_show(ret->statusbar);

  ret->about_button = create_pixmap_button(ret, "info_16.png", TRS("About"));
  ret->log_button = create_pixmap_button(ret, "log_16.png", TRS("Log messages"));
  
  mainbox = gtk_vbox_new(0, 5);
  gtk_box_pack_start_defaults(GTK_BOX(mainbox), notebook);
  
  hbox = gtk_hbox_new(0, 0);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), ret->statusbar);
  gtk_box_pack_start(GTK_BOX(hbox), ret->about_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), ret->log_button, FALSE, FALSE, 0);
  
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(mainbox), hbox, FALSE, FALSE, 0);
  
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(ret->win), mainbox);
  
  /* Add timeout */

  g_timeout_add(DELAY_TIME, timeout_func, ret);

  ret->general_section = bg_cfg_registry_find_section(cfg_reg, "general");
  
  bg_cfg_section_apply(ret->general_section,
                       gmerlin_webcam_window_get_parameters(ret),
                       gmerlin_webcam_window_set_parameter,
                       ret);

  ret->filter_section = bg_cfg_registry_find_section(cfg_reg, "filters");

  /* Create config dialog */
  ret->filter_dialog = bg_dialog_create(ret->filter_section,
                                        gmerlin_webcam_set_filter_parameter,
                                        ret->cam,
                                        gmerlin_webcam_get_filter_parameters(ret->cam),
                                        "Filters");
  
  return ret;
  }

void
gmerlin_webcam_window_destroy(gmerlin_webcam_window_t * w)
  {
  bg_gtk_plugin_widget_single_destroy(w->input_plugin);
  bg_gtk_plugin_widget_single_destroy(w->monitor_plugin);
  bg_gtk_plugin_widget_single_destroy(w->capture_plugin);
  bg_gtk_file_entry_destroy(w->output_dir);
  
  free(w);
  }

void
gmerlin_webcam_window_show(gmerlin_webcam_window_t * w)
  {
  gtk_widget_show(w->win);
  
  }


bg_parameter_info_t parameters[] =
  {
    {
      .name =        "do_monitor",
      .long_name =   TRS("Do monitor"),
      .type =      BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
      .help_string = TRS("Enable monitoring of the webcam image")
    },
    {
      .name =        "auto_capture",
      .long_name =   TRS("Automatic capture"),
      .type =      BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
      .help_string = TRS("Automatically capture in the specified capture interval")
    },
    {
      .name =        "capture_interval",
      .long_name =   TRS("Capture interval"),
      .type =        BG_PARAMETER_FLOAT,
      .val_default = { .val_f = 10.0 },
      .val_min =     { .val_f = 0.5 },
      .val_max =     { .val_f = 1200.0 },
      .help_string = TRS("Capture interval (in seconds)"),
    },
    {
      .name =        "output_directory",
      .long_name =   TRS("Output directory"),
      .type =        BG_PARAMETER_DIRECTORY,
      .val_default = { .val_str = "/tmp/" },
    },
    {
      .name =        "output_namebase",
      .long_name =   TRS("Output namebase"),
      .type =        BG_PARAMETER_STRING,
      .val_default = { .val_str = "webcam-shot-%6n" },
      .help_string = TRS("Template for the output files. A mask \"%<d>n\" inserts the frame number with <n> digits (e.g. webcam-shot-%6n)")
    },
    {
      .name =        "output_frame_counter",
      .long_name =   TRS("Output framecounter"),
      .type =        BG_PARAMETER_INT,
      .val_default = { .val_i = 0 },
    },
    { /* End of parameters */  }
  };

bg_parameter_info_t * gmerlin_webcam_window_get_parameters(gmerlin_webcam_window_t*w)
  {
  return parameters;
  }

void
gmerlin_webcam_window_set_parameter(void * priv,
                                    const char * name,
                                    const bg_parameter_value_t * val)
  {
  bg_msg_t * msg;
  gmerlin_webcam_window_t * w;
  w = (gmerlin_webcam_window_t *)priv;

  if(!name)
    return;

  if(!strcmp(name, "do_monitor"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->monitor_button), val->val_i);
    }
  else if(!strcmp(name, "auto_capture"))
    {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->auto_capture), val->val_i);
    if(!val->val_i)
      button_callback(w->auto_capture, w);
    }
  else if(!strcmp(name, "capture_interval"))
    {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->capture_interval), val->val_f);
    }
  else if(!strcmp(name, "output_directory"))
    {
    bg_gtk_file_entry_set_filename(w->output_dir, val->val_str);
    }
  else if(!strcmp(name, "output_namebase"))
    {
    gtk_entry_set_text(GTK_ENTRY(w->output_filename_base), val->val_str);
    }
  else if(!strcmp(name, "output_frame_counter"))
    {
    msg = bg_msg_queue_lock_write(w->cmd_queue);
    bg_msg_set_id(msg, CMD_SET_CAPTURE_FRAME_COUNTER);
    bg_msg_set_arg_int(msg, 0, val->val_i);
    bg_msg_queue_unlock_write(w->cmd_queue);
    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->output_frame_counter),
                              val->val_i);
    }
  }

int
gmerlin_webcam_window_get_parameter(void * priv,
                                    const char * name, bg_parameter_value_t * val)
  {
  gmerlin_webcam_window_t * w;
  w = (gmerlin_webcam_window_t *)priv;

  if(!name)
    return 0;

  if(!strcmp(name, "do_monitor"))
    {
    val->val_i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->monitor_button));
    }
  else if(!strcmp(name, "auto_capture"))
    {
    val->val_i = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->auto_capture));
    }
  else if(!strcmp(name, "capture_interval"))
    {
    val->val_f = gtk_spin_button_get_value(GTK_SPIN_BUTTON(w->capture_interval));
    }
  else if(!strcmp(name, "output_directory"))
    {
    val->val_str = bg_strdup(val->val_str,
                             bg_gtk_file_entry_get_filename(w->output_dir));
    }
  else if(!strcmp(name, "output_namebase"))
    {
    val->val_str = bg_strdup(val->val_str,
                             gtk_entry_get_text(GTK_ENTRY(w->output_filename_base)));
    }
  else if(!strcmp(name, "output_frame_counter"))
    {
    val->val_i =
      gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w->output_frame_counter));
    }
  return 1;
  }
