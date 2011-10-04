/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <config.h>

#include "gmerlin.h"
#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>
#include <gmerlin/keycodes.h>

#include <gui_gtk/gtkutils.h>

#define DELAY_TIME 10

#define BACKGROUND_WINDOW GTK_LAYOUT(win->layout)->bin_window
// #define MASK_WINDOW       win->window->window

#define BACKGROUND_WIDGET win->layout
#define MASK_WIDGET       win->window

static void set_background(player_window_t * win)
  {
  GdkBitmap * mask;
  GdkPixmap * pixmap;
  int width, height;
  GdkPixbuf * pixbuf;

  
  if(win->mouse_inside)
    pixbuf = win->background_pixbuf_highlight;
  else
    pixbuf = win->background_pixbuf;

  if(!pixbuf)
    return;
  
  width = gdk_pixbuf_get_width(pixbuf);
  height = gdk_pixbuf_get_height(pixbuf);

  gtk_widget_set_size_request(win->window, width, height);
  //  gtk_window_resize(GTK_WINDOW(win->window), width, height);
  
  bg_gdk_pixbuf_render_pixmap_and_mask(pixbuf,
                                       &pixmap, &mask);

  if(pixmap)
    bg_gtk_set_widget_bg_pixmap(win->layout, pixmap);
  gtk_widget_shape_combine_mask(win->window, mask, 0, 0);

  if(mask)
    {
    g_object_unref(G_OBJECT(mask));
    }
  
  if(BACKGROUND_WINDOW)
    gdk_window_clear_area_e(BACKGROUND_WINDOW, 0, 0, width, height);

  gdk_display_sync(gdk_display_get_default());
  
  while(gdk_events_pending() || gtk_events_pending())
    gtk_main_iteration();
  
  }

void player_window_set_skin(player_window_t * win,
                            player_window_skin_t * s,
                            const char * directory)
  {
  int x, y;
  char * tmp_path;
  
  if(win->background_pixbuf)
    g_object_unref(G_OBJECT(win->background_pixbuf));
  
  tmp_path = bg_sprintf("%s/%s", directory, s->background);
  win->background_pixbuf = gdk_pixbuf_new_from_file(tmp_path, NULL);
  free(tmp_path);

  if(s->background_highlight)
    {
    tmp_path = bg_sprintf("%s/%s", directory, s->background_highlight);
    win->background_pixbuf_highlight = gdk_pixbuf_new_from_file(tmp_path, NULL);
    free(tmp_path);
    }

  set_background(win);

  /* Apply the button skins */

  bg_gtk_button_set_skin(win->play_button, &s->play_button, directory);
  bg_gtk_button_get_coords(win->play_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                  bg_gtk_button_get_widget(win->play_button),
                  x, y);

  bg_gtk_button_set_skin(win->stop_button, &s->stop_button, directory);
  bg_gtk_button_get_coords(win->stop_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->stop_button),
                 x, y);

  bg_gtk_button_set_skin(win->pause_button, &s->pause_button, directory);
  bg_gtk_button_get_coords(win->pause_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->pause_button),
                 x, y);
  
  bg_gtk_button_set_skin(win->next_button, &s->next_button, directory);
  bg_gtk_button_get_coords(win->next_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->next_button),
                 x, y);
  
  bg_gtk_button_set_skin(win->prev_button, &s->prev_button, directory);
  bg_gtk_button_get_coords(win->prev_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->prev_button),
                 x, y);

  bg_gtk_button_set_skin(win->close_button, &s->close_button, directory);
  bg_gtk_button_get_coords(win->close_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->close_button),
                 x, y);

  bg_gtk_button_set_skin(win->menu_button, &s->menu_button, directory);
  bg_gtk_button_get_coords(win->menu_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->menu_button),
                 x, y);

  /* Apply slider skins */
  
  bg_gtk_slider_set_skin(win->seek_slider, &s->seek_slider, directory);
  bg_gtk_slider_get_coords(win->seek_slider, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_slider_get_widget(win->seek_slider),
                 x, y);

  bg_gtk_slider_set_skin(win->volume_slider, &s->volume_slider, directory);
  bg_gtk_slider_get_coords(win->volume_slider, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                  bg_gtk_slider_get_widget(win->volume_slider),
                  x, y);

  /* Apply display skin */
  
  display_set_skin(win->display, &s->display);
  display_get_coords(win->display, &x, &y);
  
  gtk_layout_move(GTK_LAYOUT(win->layout),
                  display_get_widget(win->display),
                  x, y);

  /* Update slider positions */

  bg_gtk_slider_set_pos(win->volume_slider,
                        (win->volume - BG_PLAYER_VOLUME_MIN)/
                        (-BG_PLAYER_VOLUME_MIN));
  
  }

/* Gtk Callbacks */

static void realize_callback(GtkWidget * w, gpointer data)
  {
  
  player_window_t * win;
  
  win = (player_window_t*)data;
  
  set_background(win);
  }

static gboolean button_press_callback(GtkWidget * w, GdkEventButton * evt,
                                      gpointer data)
  {
  player_window_t * win;
  
  win = (player_window_t*)data;
  
  win->mouse_x = (int)(evt->x);
  win->mouse_y = (int)(evt->y);
  
  return TRUE;
  }


static gboolean motion_callback(GtkWidget * w, GdkEventMotion * evt,
                                gpointer data)
  {
  player_window_t * win;

  /* Buggy (newer) gtk versions send motion events even if no button
     is pressed */
  if(!(evt->state & (GDK_BUTTON1_MASK|GDK_BUTTON2_MASK|GDK_BUTTON3_MASK)))
    return TRUE;

  win = (player_window_t*)data;
  
  gtk_window_move(GTK_WINDOW(win->window),
                  (int)(evt->x_root) - win->mouse_x,
                  (int)(evt->y_root) - win->mouse_y);

  win->window_x = (int)(evt->x_root - evt->x);
  win->window_y = (int)(evt->y_root - evt->y);
  
  return TRUE;
  }

/* Gmerlin callbacks */

static void seek_change_callback(bg_gtk_slider_t * slider, float perc,
                                 void * data)
  {
  gavl_time_t time;
  player_window_t * win = (player_window_t *)data;
  
  time = (gavl_time_t)(perc * (double)win->duration);

  if(!win->seek_active)
    {
    if(win->gmerlin->player_state == BG_PLAYER_STATE_PAUSED)
      win->seek_active = 2;
    else
      {
      win->seek_active = 1;
      bg_player_pause(win->gmerlin->player);
      }
    }
  
  //  player_window_t * win = (player_window_t *)data;

  bg_player_seek(win->gmerlin->player, time, GAVL_TIME_SCALE);
  display_set_time(win->display, time);
  }

static void seek_release_callback(bg_gtk_slider_t * slider, float perc,
                                  void * data)
  {
  gavl_time_t time;
  player_window_t * win = (player_window_t *)data;

  time = (gavl_time_t)(perc * (double)win->duration);
  
  //  player_window_t * win = (player_window_t *)data;
  bg_player_seek(win->gmerlin->player, time, GAVL_TIME_SCALE);

  if(win->seek_active == 1)
    bg_player_pause(win->gmerlin->player);
  }

static void
slider_scroll_callback(bg_gtk_slider_t * slider, int up, void * data)
  {
  player_window_t * win = (player_window_t *)data;

  if(slider == win->volume_slider)
    {
    if(up)
      bg_player_set_volume_rel(win->gmerlin->player, 1.0);
    else
      bg_player_set_volume_rel(win->gmerlin->player, -1.0);
    }
  else if(slider == win->seek_slider)
    {
    if(up)
      bg_player_seek_rel(win->gmerlin->player, 2 * GAVL_TIME_SCALE);
    else
      bg_player_seek_rel(win->gmerlin->player, -2 * GAVL_TIME_SCALE);
    }
  
  }

static void volume_change_callback(bg_gtk_slider_t * slider, float perc,
                                   void * data)
  {
  float volume;
  player_window_t * win = (player_window_t *)data;
  
  volume = BG_PLAYER_VOLUME_MIN - BG_PLAYER_VOLUME_MIN * perc;
  if(volume > 0.0)
    volume = 0.0;
  
  bg_player_set_volume(win->gmerlin->player, volume);
  win->volume = volume;
  }

static void gmerlin_button_callback_2(bg_gtk_button_t * b, void * data)
  {
  player_window_t * win = (player_window_t *)data;

  if(b == win->next_button)
    {
    bg_player_next_chapter(win->gmerlin->player);
    }
  else if(b == win->prev_button)
    {
    bg_player_prev_chapter(win->gmerlin->player);
    }
  }

static void gmerlin_button_callback(bg_gtk_button_t * b, void * data)
  {
  player_window_t * win = (player_window_t *)data;
  if(b == win->play_button)
    {

    gmerlin_play(win->gmerlin, BG_PLAY_FLAG_IGNORE_IF_PLAYING | BG_PLAY_FLAG_RESUME);
    }
  else if(b == win->pause_button)
    {
    gmerlin_pause(win->gmerlin);
    }
  else if(b == win->stop_button)
    {
    bg_player_stop(win->gmerlin->player);
    }
  else if(b == win->next_button)
    {
    bg_media_tree_next(win->gmerlin->tree, 1, win->gmerlin->shuffle_mode);

    gmerlin_play(win->gmerlin, BG_PLAY_FLAG_IGNORE_IF_STOPPED);
    }
  else if(b == win->prev_button)
    {
    bg_media_tree_previous(win->gmerlin->tree, 1, win->gmerlin->shuffle_mode);

    gmerlin_play(win->gmerlin, BG_PLAY_FLAG_IGNORE_IF_STOPPED);
    }
  else if(b == win->close_button)
    {
    gtk_main_quit();
    }
  }

static gboolean do_configure(gpointer data)
  {
  gmerlin_t * gmerlin;
  gmerlin = (gmerlin_t*)data;
  gmerlin_configure(gmerlin);
  return FALSE;
  }

static void handle_message(player_window_t * win,
                           bg_msg_t * msg)
  {
  int id;
  int arg_i_1;
  int arg_i_2;
  int arg_i_3;
  float arg_f_1;
  char * arg_str_1;
  char * arg_str_2;
  gavl_time_t time;

  id = bg_msg_get_id(msg);
  
  switch(id)
    {
    case BG_PLAYER_MSG_TIME_CHANGED:
      if(!win->seek_active)
        {
        time = bg_msg_get_arg_time(msg, 0);
        display_set_time(win->display, time);
        if(win->duration != GAVL_TIME_UNDEFINED)
          bg_gtk_slider_set_pos(win->seek_slider,
                                (float)(time) / (float)(win->duration));
        }
      break;
    case BG_PLAYER_MSG_VOLUME_CHANGED:
      arg_f_1 = bg_msg_get_arg_float(msg, 0);
      bg_gtk_slider_set_pos(win->volume_slider,
                            (arg_f_1 - BG_PLAYER_VOLUME_MIN) /
                            (- BG_PLAYER_VOLUME_MIN));
      break;
    case BG_PLAYER_MSG_TRACK_CHANGED:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      gmerlin_check_next_track(win->gmerlin, arg_i_1);
      

      break;
    case BG_PLAYER_MSG_MUTE:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      display_set_mute(win->display, arg_i_1);
      break;
    case BG_PLAYER_MSG_STATE_CHANGED:
      win->gmerlin->player_state = bg_msg_get_arg_int(msg, 0);
      
      switch(win->gmerlin->player_state)
        {
        case BG_PLAYER_STATE_PAUSED:
          display_set_state(win->display, win->gmerlin->player_state, NULL);
          break;
        case BG_PLAYER_STATE_SEEKING:
          display_set_state(win->display, win->gmerlin->player_state, NULL);
          break;
        case BG_PLAYER_STATE_ERROR:
          bg_gtk_log_window_flush(win->gmerlin->log_window);
          display_set_state(win->display, win->gmerlin->player_state,
                            bg_gtk_log_window_last_error(win->gmerlin->log_window));
          break;
        case BG_PLAYER_STATE_BUFFERING:
          arg_f_1 = bg_msg_get_arg_float(msg, 1);
          display_set_state(win->display, win->gmerlin->player_state, &arg_f_1);
          break;
        case BG_PLAYER_STATE_PLAYING:
          display_set_state(win->display, win->gmerlin->player_state, NULL);
          bg_media_tree_mark_error(win->gmerlin->tree, 0);
          win->seek_active = 0;
          break;
        case BG_PLAYER_STATE_STOPPED:
          bg_gtk_slider_set_state(win->seek_slider,
                                  BG_GTK_SLIDER_INACTIVE);
          display_set_state(win->display, win->gmerlin->player_state, NULL);
          break;
        case BG_PLAYER_STATE_CHANGING:
          arg_i_2 = bg_msg_get_arg_int(msg, 1);
          display_set_state(win->display, win->gmerlin->player_state, NULL);
          if(arg_i_2)
            gmerlin_next_track(win->gmerlin);
          break;
        case BG_PLAYER_STATE_EOF:
          display_set_state(win->display, win->gmerlin->player_state, NULL);
          gmerlin_next_track(win->gmerlin);
          break;
        }
      break;
    case BG_PLAYER_MSG_TRACK_NAME:
      arg_str_1 = bg_msg_get_arg_string(msg, 0);
      display_set_track_name(win->display, arg_str_1);
      free(arg_str_1);
      break;
    case BG_PLAYER_MSG_TRACK_NUM_STREAMS:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      arg_i_2 = bg_msg_get_arg_int(msg, 1);
      arg_i_3 = bg_msg_get_arg_int(msg, 2);
      main_menu_set_num_streams(win->main_menu, arg_i_1, arg_i_2, arg_i_3);
      break;
    case BG_PLAYER_MSG_NUM_CHAPTERS:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      arg_i_2 = bg_msg_get_arg_int(msg, 1);
      main_menu_set_num_chapters(win->main_menu, arg_i_1, arg_i_2);
      break;
    case BG_PLAYER_MSG_CHAPTER_INFO:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      arg_str_1 = bg_msg_get_arg_string(msg, 1);
      time = bg_msg_get_arg_time(msg, 2);
      main_menu_set_chapter_info(win->main_menu, arg_i_1, arg_str_1, time);
      break;
    case BG_PLAYER_MSG_CHAPTER_CHANGED:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      main_menu_chapter_changed(win->main_menu, arg_i_1);
      break;
    case BG_PLAYER_MSG_ACCEL:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      switch(arg_i_1)
        {
        case ACCEL_VOLUME_DOWN:
          bg_player_set_volume_rel(win->gmerlin->player,  -1.0);
          break;
        case ACCEL_VOLUME_UP:
          bg_player_set_volume_rel(win->gmerlin->player,  1.0);
          break;
        case ACCEL_SEEK_BACKWARD:
          bg_player_seek_rel(win->gmerlin->player,   -2 * GAVL_TIME_SCALE );
          break;
        case ACCEL_SEEK_FORWARD:
          bg_player_seek_rel(win->gmerlin->player,   2 * GAVL_TIME_SCALE );
          break;
        case ACCEL_SEEK_START:
          bg_player_seek(win->gmerlin->player,0, GAVL_TIME_SCALE );
          break;
        case ACCEL_PAUSE:
          bg_player_pause(win->gmerlin->player);
          break;
        case ACCEL_MUTE:
          bg_player_toggle_mute(win->gmerlin->player);
          break;
        case ACCEL_NEXT_CHAPTER:
          bg_player_next_chapter(win->gmerlin->player);
          break;
        case ACCEL_PREV_CHAPTER:
          bg_player_prev_chapter(win->gmerlin->player);
          break;
        case ACCEL_PREV:
          bg_media_tree_previous(win->gmerlin->tree, 1, win->gmerlin->shuffle_mode);
          gmerlin_play(win->gmerlin, BG_PLAY_FLAG_IGNORE_IF_STOPPED);
          break;
        case ACCEL_NEXT:
          bg_media_tree_next(win->gmerlin->tree, 1, win->gmerlin->shuffle_mode);
          gmerlin_play(win->gmerlin, BG_PLAY_FLAG_IGNORE_IF_STOPPED);
          break;
        case ACCEL_QUIT:
          gtk_main_quit();
          return;
          break;
        case ACCEL_CURRENT_TO_FAVOURITES:
          bg_media_tree_copy_current_to_favourites(win->gmerlin->tree);
          return;
          break;
        case ACCEL_OPTIONS:
          g_idle_add(do_configure, win->gmerlin);
          break;
        case ACCEL_GOTO_CURRENT:
          bg_gtk_tree_window_goto_current(win->gmerlin->tree_window);
          break;
        }
      break;
    case BG_PLAYER_MSG_TRACK_DURATION:
      win->duration = bg_msg_get_arg_time(msg, 0);

      arg_i_2 = bg_msg_get_arg_int(msg, 1);

      if(arg_i_2)
        bg_gtk_slider_set_state(win->seek_slider,
                                BG_GTK_SLIDER_ACTIVE);
      else if(win->duration != GAVL_TIME_UNDEFINED)
        bg_gtk_slider_set_state(win->seek_slider,
                                BG_GTK_SLIDER_INACTIVE);
      else
        bg_gtk_slider_set_state(win->seek_slider,
                                BG_GTK_SLIDER_HIDDEN);
      break;
    case BG_PLAYER_MSG_AUDIO_STREAM:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      main_menu_set_audio_index(win->main_menu, arg_i_1);
      break;
    case BG_PLAYER_MSG_VIDEO_STREAM:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      main_menu_set_video_index(win->main_menu, arg_i_1);
      break;
    case BG_PLAYER_MSG_SUBTITLE_STREAM:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      main_menu_set_subtitle_index(win->main_menu, arg_i_1);
      break;
    case BG_PLAYER_MSG_AUDIO_DESCRIPTION:
      break;
    case BG_PLAYER_MSG_VIDEO_DESCRIPTION:
      break;
    case BG_PLAYER_MSG_SUBTITLE_DESCRIPTION:
      break;
    case BG_PLAYER_MSG_STREAM_DESCRIPTION:
      break;
    case BG_PLAYER_MSG_AUDIO_STREAM_INFO:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      arg_str_1 = bg_msg_get_arg_string(msg, 1);
      arg_str_2 = bg_msg_get_arg_string(msg, 2);
      main_menu_set_audio_info(win->main_menu, arg_i_1, arg_str_1, arg_str_2);
      if(arg_str_1) free(arg_str_1);
      if(arg_str_2) free(arg_str_2);
      break;
    case BG_PLAYER_MSG_VIDEO_STREAM_INFO:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      arg_str_1 = bg_msg_get_arg_string(msg, 1);
      arg_str_2 = bg_msg_get_arg_string(msg, 2);
      main_menu_set_video_info(win->main_menu, arg_i_1, arg_str_1, arg_str_2);
      if(arg_str_1) free(arg_str_1);
      if(arg_str_2) free(arg_str_2);
      break;
    case BG_PLAYER_MSG_SUBTITLE_STREAM_INFO:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      arg_str_1 = bg_msg_get_arg_string(msg, 1);
      arg_str_2 = bg_msg_get_arg_string(msg, 2);
      main_menu_set_subtitle_info(win->main_menu, arg_i_1, arg_str_1,
                                  arg_str_2);
      if(arg_str_1) free(arg_str_1);
      if(arg_str_2) free(arg_str_2);
      break;
    }
  
  }

void player_window_push_accel(player_window_t * w, int accel)
  {
  bg_msg_t * msg;
  msg = bg_msg_queue_lock_write(w->msg_queue);
  bg_msg_set_id(msg, BG_PLAYER_MSG_ACCEL);
  bg_msg_set_arg_int(msg, 0, accel);
  bg_msg_queue_unlock_write(w->msg_queue);
  }

static gboolean idle_callback(gpointer data)
  {
  bg_msg_t * msg;
  
  player_window_t * w = (player_window_t *)data;

  if(!w->msg_queue_locked)
    {
    while((msg = bg_msg_queue_try_lock_read(w->msg_queue)))
      {
      w->msg_queue_locked = 1;
      handle_message(w, msg);
      if(w->msg_queue_locked)
        {
        bg_msg_queue_unlock_read(w->msg_queue);
        w->msg_queue_locked = 0;
        }
      }
    }
  
  /* Handle remote control */

  while((msg = bg_remote_server_get_msg(w->gmerlin->remote)))
    {
    gmerlin_handle_remote(w->gmerlin, msg);

    }
  return TRUE;
  }

static gboolean crossing_callback(GtkWidget *widget,
                                  GdkEventCrossing *event,
                                  gpointer data)
  {
  player_window_t * w = (player_window_t *)data;
  if(event->detail == GDK_NOTIFY_INFERIOR)
    return FALSE;

  //  fprintf(stderr, "crossing callback %d %d %d\n",
  //          event->detail, event->type, w->mouse_inside);
 
 
  w->mouse_inside = (event->type == GDK_ENTER_NOTIFY) ? 1 : 0;
  //  fprintf(stderr, "Set background...");

  g_signal_handler_block(w->window, w->enter_notify_id);
  g_signal_handler_block(w->window, w->leave_notify_id); 

  set_background(w);

  g_signal_handler_unblock(w->window, w->enter_notify_id);   
  g_signal_handler_unblock(w->window, w->leave_notify_id); 

  //  fprintf(stderr, "Done\n");
  return FALSE;
  }

void player_window_create(gmerlin_t * g)
  {
  player_window_t * ret;

  ret = calloc(1, sizeof(*ret));
  ret->gmerlin = g;

  g->player_window = ret;
  
  
  ret->msg_queue = bg_msg_queue_create();

  bg_player_add_message_queue(g->player,
                              ret->msg_queue);

  g_timeout_add(DELAY_TIME, idle_callback, (gpointer)ret);
  
  /* Create objects */
  
  ret->window = bg_gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(ret->window), FALSE);

  ret->accel_group = gtk_accel_group_new();

  
  gtk_window_add_accel_group (GTK_WINDOW(ret->window), ret->gmerlin->accel_group);
  gtk_window_add_accel_group (GTK_WINDOW(ret->window), ret->accel_group);
  
  ret->layout = gtk_layout_new(NULL, NULL);
  
  /* Set attributes */

  gtk_widget_set_events(ret->window,
                        GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    
  gtk_widget_set_events(ret->layout,
                        GDK_BUTTON1_MOTION_MASK|
                        GDK_BUTTON2_MOTION_MASK|
                        GDK_BUTTON3_MOTION_MASK|
                        GDK_BUTTON_PRESS_MASK);
  /* Set Callbacks */

  g_signal_connect(G_OBJECT(ret->window), "realize",
                   G_CALLBACK(realize_callback), (gpointer*)ret);

  ret->enter_notify_id =
    g_signal_connect(G_OBJECT(ret->window), "enter-notify-event",
                     G_CALLBACK(crossing_callback), (gpointer*)ret);
  ret->leave_notify_id =
    g_signal_connect(G_OBJECT(ret->window), "leave-notify-event",
                     G_CALLBACK(crossing_callback), (gpointer*)ret);
  

  
  g_signal_connect(G_OBJECT(ret->layout), "realize",
                   G_CALLBACK(realize_callback), (gpointer*)ret);

  g_signal_connect(G_OBJECT(ret->layout), "motion-notify-event",
                   G_CALLBACK(motion_callback), (gpointer*)ret);


  
  g_signal_connect(G_OBJECT(ret->layout), "button-press-event",
                   G_CALLBACK(button_press_callback), (gpointer*)ret);

  /* Create child objects */

  ret->play_button = bg_gtk_button_create();
  ret->stop_button = bg_gtk_button_create();
  ret->next_button = bg_gtk_button_create();
  ret->prev_button = bg_gtk_button_create();
  ret->pause_button = bg_gtk_button_create();
  ret->menu_button = bg_gtk_button_create();
  ret->close_button = bg_gtk_button_create();

  ret->seek_slider = bg_gtk_slider_create();
  ret->volume_slider = bg_gtk_slider_create();
  
  ret->main_menu = main_menu_create(g);
  
  main_menu_finalize(ret->main_menu, g);
  
  ret->display = display_create(g);
  
  /* Set callbacks */

  bg_gtk_slider_set_change_callback(ret->seek_slider,
                                    seek_change_callback, ret);
  
  bg_gtk_slider_set_release_callback(ret->seek_slider,
                                     seek_release_callback, ret);

  bg_gtk_slider_set_change_callback(ret->volume_slider,
                                    volume_change_callback, ret);

  bg_gtk_slider_set_scroll_callback(ret->volume_slider,
                                    slider_scroll_callback, ret);
  bg_gtk_slider_set_scroll_callback(ret->seek_slider,
                                    slider_scroll_callback, ret);

  
  bg_gtk_button_set_callback(ret->play_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->stop_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->pause_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->next_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->prev_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->close_button, gmerlin_button_callback, ret);

  bg_gtk_button_set_callback_2(ret->next_button, gmerlin_button_callback_2, ret);
  bg_gtk_button_set_callback_2(ret->prev_button, gmerlin_button_callback_2, ret);
  
  bg_gtk_button_set_menu(ret->menu_button,
                         main_menu_get_widget(ret->main_menu));

  /* Set tooltips */
  
  bg_gtk_tooltips_set_tip(bg_gtk_button_get_widget(ret->play_button),
                          "Play", PACKAGE);
  bg_gtk_tooltips_set_tip(bg_gtk_button_get_widget(ret->stop_button),
                       "Stop", PACKAGE);
  bg_gtk_tooltips_set_tip(bg_gtk_button_get_widget(ret->pause_button),
                       "Pause", PACKAGE);
  bg_gtk_tooltips_set_tip(bg_gtk_button_get_widget(ret->next_button),
                       "Left button: Next track\nRight button: Next chapter",
                       PACKAGE);
  bg_gtk_tooltips_set_tip(bg_gtk_button_get_widget(ret->prev_button),
                       "Left button: Previous track\nRight button: Previous chapter",
                       PACKAGE);
  
  bg_gtk_tooltips_set_tip(bg_gtk_button_get_widget(ret->menu_button),
                          "Main menu", PACKAGE);
  bg_gtk_tooltips_set_tip(bg_gtk_button_get_widget(ret->close_button),
                          "Quit program", PACKAGE);

  bg_gtk_tooltips_set_tip(bg_gtk_slider_get_slider_widget(ret->volume_slider),
                          "Volume", PACKAGE);

  bg_gtk_tooltips_set_tip(bg_gtk_slider_get_slider_widget(ret->seek_slider),
                          "Seek", PACKAGE);
  
  /* Pack Objects */

  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_button_get_widget(ret->play_button),
                 0, 0);
  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_button_get_widget(ret->stop_button),
                 0, 0);
  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_button_get_widget(ret->pause_button),
                 0, 0);
  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_button_get_widget(ret->next_button),
                 0, 0);
  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_button_get_widget(ret->prev_button),
                 0, 0);
  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_button_get_widget(ret->close_button),
                 0, 0);
  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_button_get_widget(ret->menu_button),
                 0, 0);
  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_slider_get_widget(ret->seek_slider),
                 0, 0);
  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 bg_gtk_slider_get_widget(ret->volume_slider),
                 0, 0);

  gtk_layout_put(GTK_LAYOUT(ret->layout),
                 display_get_widget(ret->display),
                 0, 0);
  
  gtk_widget_show(ret->layout);
  gtk_container_add(GTK_CONTAINER(ret->window), ret->layout);
    
  }

void player_window_show(player_window_t * win)
  {
  gtk_window_move(GTK_WINDOW(win->window),
                  win->window_x,
                  win->window_y);
  gtk_widget_show(win->window);
  }

void player_window_destroy(player_window_t * win)
  {
  /* Fetch parameters */
  
  bg_cfg_section_get(win->gmerlin->display_section,
                     display_get_parameters(win->display),
                     display_get_parameter, (void*)(win->display));

  bg_msg_queue_destroy(win->msg_queue);

  bg_gtk_slider_destroy(win->seek_slider);
  bg_gtk_slider_destroy(win->volume_slider);

  main_menu_destroy(win->main_menu);

  if(win->background_pixbuf)
    g_object_unref(win->background_pixbuf);
  if(win->background_pixbuf_highlight)
    g_object_unref(win->background_pixbuf_highlight);
  
  free(win);
  }



void player_window_skin_load(player_window_skin_t * s,
                             xmlDocPtr doc, xmlNodePtr node)
  {
  xmlNodePtr child;
  char * tmp_string;
  child = node->children;
  while(child)
    {
    if(!child->name)
      {
      child = child->next;
      continue;
      }
    else if(!BG_XML_STRCMP(child->name, "BACKGROUND"))
      {
      tmp_string = (char*)xmlNodeListGetString(doc, child->children, 1);
      s->background = bg_strdup(s->background, tmp_string);
      xmlFree(tmp_string);
      }
    else if(!BG_XML_STRCMP(child->name, "BACKGROUND_HIGHLIGHT"))
      {
      tmp_string = (char*)xmlNodeListGetString(doc, child->children, 1);
      s->background_highlight = bg_strdup(s->background_highlight, tmp_string);
      xmlFree(tmp_string);
      }

    else if(!BG_XML_STRCMP(child->name, "DISPLAY"))
      display_skin_load(&s->display, doc, child);
    else if(!BG_XML_STRCMP(child->name, "PLAYBUTTON"))
      bg_gtk_button_skin_load(&s->play_button, doc, child);
    else if(!BG_XML_STRCMP(child->name, "PAUSEBUTTON"))
      bg_gtk_button_skin_load(&s->pause_button, doc, child);
    else if(!BG_XML_STRCMP(child->name, "NEXTBUTTON"))
      bg_gtk_button_skin_load(&s->next_button, doc, child);
    else if(!BG_XML_STRCMP(child->name, "PREVBUTTON"))
      bg_gtk_button_skin_load(&s->prev_button, doc, child);
    else if(!BG_XML_STRCMP(child->name, "STOPBUTTON"))
      bg_gtk_button_skin_load(&s->stop_button, doc, child);
    else if(!BG_XML_STRCMP(child->name, "MENUBUTTON"))
      bg_gtk_button_skin_load(&s->menu_button, doc, child);
    else if(!BG_XML_STRCMP(child->name, "CLOSEBUTTON"))
      bg_gtk_button_skin_load(&s->close_button, doc, child);
    else if(!BG_XML_STRCMP(child->name, "SEEKSLIDER"))
      bg_gtk_slider_skin_load(&s->seek_slider, doc, child);
    else if(!BG_XML_STRCMP(child->name, "VOLUMESLIDER"))
      bg_gtk_slider_skin_load(&s->volume_slider, doc, child);
    else if(!BG_XML_STRCMP(child->name, "DISPLAY"))
      display_skin_load(&s->display, doc, child);
    child = child->next;
    }
  }

void player_window_skin_destroy(player_window_skin_t * s)
  {
  if(s->background)
    free(s->background);
  if(s->background_highlight)
    free(s->background_highlight);
  
  bg_gtk_button_skin_free(&s->play_button);
  bg_gtk_button_skin_free(&s->stop_button);
  bg_gtk_button_skin_free(&s->pause_button);
  bg_gtk_button_skin_free(&s->next_button);
  bg_gtk_button_skin_free(&s->prev_button);
  bg_gtk_button_skin_free(&s->close_button);
  bg_gtk_button_skin_free(&s->menu_button);
  bg_gtk_slider_skin_free(&s->seek_slider);
  bg_gtk_slider_skin_free(&s->volume_slider);
  
  }
