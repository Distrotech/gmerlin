/*****************************************************************
 * gmerlin-mozilla - A gmerlin based mozilla plugin
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

#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#define LOG_DOMAIN "widget"

#define TOOLBAR_VISIBLE 0
#define TOOLBAR_HIDING  1
#define TOOLBAR_HIDDEN  2

void bg_mozilla_play(bg_mozilla_t * m)
  {
  if(m->player_state == BG_PLAYER_STATE_PLAYING)
    return;
  
  if(m->player_state == BG_PLAYER_STATE_PAUSED)
    {
    bg_player_pause(m->player);
    return;
    }

  switch(m->url_mode)
    {
    case URL_MODE_LOCAL:
      gmerlin_mozilla_start(m);
      break;
    case URL_MODE_REDIRECT:
      m->url = bg_strdup(m->url, m->current_url);
      gmerlin_mozilla_start(m);
      break;
    case URL_MODE_STREAM:
      if(m->reload_url)
        m->reload_url(m);
      break;
    }

  }

void bg_mozilla_stop(bg_mozilla_t * m)
  {
  bg_msg_t * msg;
  bg_player_stop(m->player);
  while((msg = bg_msg_queue_lock_read(m->msg_queue)))
    {
    bg_mozilla_handle_message(m, msg);
    bg_msg_queue_unlock_read(m->msg_queue);
    if(m->player_state == BG_PLAYER_STATE_STOPPED ||
       m->player_state == BG_PLAYER_STATE_CHANGING)
      break;
    }
  }

void bg_mozilla_pause(bg_mozilla_t * m)
  {
  bg_player_pause(m->player);
  }


static void gmerlin_button_callback(bg_gtk_button_t * b, void * data)
  {
  bg_mozilla_widget_t * w;
  w = data;

  if(b == w->stop_button)
    {
    bg_mozilla_stop(w->m);
    }
  if(b == w->play_button)
    {
    bg_mozilla_play(w->m);
    }
  if(b == w->pause_button)
    {
    bg_mozilla_pause(w->m);
    }
  if(b == w->volume_button)
    {
    int x, y;
    GtkWidget * wid = bg_gtk_button_get_widget(b);
    gdk_window_get_origin(wid->window, &x, &y);
    gdk_window_move_resize(w->volume_window->window, x, y - 80, 20, 80);
    gtk_widget_show(w->volume_window);
    gdk_window_move(w->volume_window->window, x, y - 80);
    }
  }

static void show_toolbar(bg_mozilla_widget_t * w)
  {
  w->toolbar_state = TOOLBAR_VISIBLE;
  w->toolbar_pos = w->current_win->height - w->toolbar_height;
  gtk_fixed_move(GTK_FIXED(w->current_win->box), w->controls, 0,
                 w->toolbar_pos);
  }

static void resize_toolbar(bg_mozilla_widget_t * w)
  {
  bg_mozilla_window_t * win;
  win = w->current_win;
  
  gtk_widget_set_size_request(bg_gtk_button_get_widget(w->stop_button),
                              20, 20);
  gtk_widget_set_size_request(bg_gtk_button_get_widget(w->play_button),
                              20, 20);
  gtk_widget_set_size_request(bg_gtk_button_get_widget(w->volume_button),
                              20, 20);
  gtk_widget_set_size_request(bg_gtk_button_get_widget(w->pause_button),
                              20, 20);
  gtk_widget_set_size_request(bg_gtk_scrolltext_get_widget(w->scrolltext),
                              win->width-40, 20);

  gtk_fixed_move(GTK_FIXED(w->controls),
                 bg_gtk_button_get_widget(w->volume_button),
                 win->width-20, 0);
  
  if(w->duration != GAVL_TIME_UNDEFINED)
    {
    gtk_widget_show(bg_gtk_slider_get_widget(w->seek_slider));
    gtk_widget_set_size_request(bg_gtk_slider_get_widget(w->seek_slider),
                                win->width, 20);
    w->toolbar_height = 40;
    }
  else
    {
    gtk_widget_hide(bg_gtk_slider_get_widget(w->seek_slider));
    w->toolbar_height = 20;
    }
  
  if(win->resize_id > 0)
    g_signal_handler_block(win->box, win->resize_id);
  
  if(w->controls)
    {
    gtk_widget_set_size_request(w->controls, win->width, w->toolbar_height);
    w->toolbar_pos = win->height - w->toolbar_height;
    gtk_fixed_move(GTK_FIXED(win->box), w->controls, 0,
                   w->toolbar_pos);
    }
  if(win->resize_id > 0)
    g_signal_handler_unblock(win->box, win->resize_id);
  }

static void window_size_allocate(GtkWidget     *widget,
                                 GtkAllocation *a,
                                 gpointer       user_data)
  {
  bg_mozilla_widget_t * w;
  int pixbuf_width, pixbuf_height;
  w = user_data;

  if((a->width == w->fullscreen_win.width) && (a->height == w->fullscreen_win.height))
    return;
  w->fullscreen_win.width = a->width;
  w->fullscreen_win.height = a->height;

  pixbuf_width = gdk_pixbuf_get_width(w->logo_pixbuf);
  pixbuf_height = gdk_pixbuf_get_height(w->logo_pixbuf);
  
  gtk_fixed_move(GTK_FIXED(w->fullscreen_win.box),
                w->fullscreen_win.logo_image,
                (w->fullscreen_win.width - pixbuf_width)/2,
                (w->fullscreen_win.height - pixbuf_height)/2);
  }

static void size_allocate(GtkWidget     *widget,
                          GtkAllocation *a,
                          gpointer       user_data)
  {
  bg_mozilla_widget_t * w;
  bg_mozilla_window_t * win;
  int pixbuf_width, pixbuf_height;
  w = user_data;
  
  win = &w->normal_win;
  if((a->width == win->width) && (a->height == win->height))
    return;
  
  win->width = a->width;
  win->height = a->height;

  if(!win->logo_image)
    {
    if(!w->logo_pixbuf_scaled)
      {
      pixbuf_width = gdk_pixbuf_get_width(w->logo_pixbuf);
      pixbuf_height = gdk_pixbuf_get_height(w->logo_pixbuf);

      if(pixbuf_width > win->width)
        {
        pixbuf_height = (pixbuf_height * win->width) / pixbuf_width;
        pixbuf_width = win->width;
        }
      if(pixbuf_height > win->height)
        {
        pixbuf_width = (pixbuf_width * win->height) / pixbuf_height;
        pixbuf_height = win->height;
        }
      
      w->logo_pixbuf_scaled = gdk_pixbuf_scale_simple(w->logo_pixbuf,
                                                      pixbuf_width,
                                                      pixbuf_height,
                                                      GDK_INTERP_BILINEAR);
      }
    win->logo_image = gtk_image_new_from_pixbuf(w->logo_pixbuf_scaled);
    gtk_widget_show(win->logo_image);
    
    
    pixbuf_width = gdk_pixbuf_get_width(w->logo_pixbuf_scaled);
    pixbuf_height = gdk_pixbuf_get_height(w->logo_pixbuf_scaled);
    
    gtk_fixed_put(GTK_FIXED(win->box), win->logo_image,
                  (win->width - pixbuf_width)/2,
                  (win->height - pixbuf_height)/2);
    }
  else
    {
    pixbuf_width = gdk_pixbuf_get_width(w->logo_pixbuf_scaled);
    pixbuf_height = gdk_pixbuf_get_height(w->logo_pixbuf_scaled);
    
    gtk_fixed_move(GTK_FIXED(win->box), win->logo_image,
                   (win->width - pixbuf_width)/2,
                   (win->height - pixbuf_height)/2);
    }
  

  g_signal_handler_block(win->box, win->resize_id);
  if(win == w->current_win)
    {
    resize_toolbar(w);
    }
  
  if(win->socket)
    {
    gtk_widget_set_size_request(win->socket, a->width, a->height);
    }
  g_signal_handler_unblock(win->box, win->resize_id);
  }

static gboolean popup_menu(void * data)
  {
  bg_mozilla_widget_t * w;
  w = data;
  gtk_menu_popup(GTK_MENU(w->menu.menu),
                 (GtkWidget *)0,
                 (GtkWidget *)0,
                 (GtkMenuPositionFunc)0,
                 (gpointer)0,
                 0,
                 gtk_get_current_event_time()
                 // w->popup_time
                 );
  return FALSE;
  }
     
static gboolean open_link(void * data)
  {
  bg_mozilla_widget_t * w;
  w = data;
  w->m->open_url(w->m, w->m->ei.href);
  return FALSE;
  }


static gboolean button_press_callback(GtkWidget * wid, GdkEventButton * evt,
                                      gpointer data)
  {
  bg_mozilla_widget_t * w;
  w = data;
  w->idle_counter = 0;

  GTK_WIDGET_SET_FLAGS(w->current_win->socket, GTK_CAN_FOCUS);
  gtk_widget_grab_focus(w->current_win->socket);
  

  show_toolbar(w);
  return TRUE;
  }

static gboolean button_release_callback(GtkWidget * wid, GdkEventButton * evt,
                                        gpointer data)
  {
#if 1
  bg_mozilla_widget_t * w;
  w = data;
  //  show_toolbar(w);
  w->idle_counter = 0;
  if(evt->button == 3)
    {
    w->popup_time = evt->time;
    g_idle_add(popup_menu, w);
    return TRUE;
    }
  if(evt->button == 1)
    {
    if((w->m->ei.href) &&
       (w->m->open_url))
      {
      g_idle_add(open_link, w);
      }
    }

#endif
  return TRUE;
  }

static gboolean key_press_callback(GtkWidget * wid, GdkEventKey * evt,
                                   gpointer data)
  {
  bg_mozilla_widget_t * w;
  w = data;

  switch(evt->keyval)
    {
    case GDK_Menu:
      gtk_menu_popup(GTK_MENU(w->menu.menu),
                     (GtkWidget *)0,
                     (GtkWidget *)0,
                     (GtkMenuPositionFunc)0,
                     (gpointer)0,
                     0,
                     // gtk_get_current_event_time()
                     evt->time
                     );
      return TRUE;
      break;
    }
  
  return FALSE;
  }

/* Slider callbacks */

static void seek_change_callback(bg_gtk_slider_t * slider, float perc,
                                 void * data)
  {
  bg_mozilla_widget_t * win = (bg_mozilla_widget_t *)data;

  win->seek_active = 1;
  
  //  bg_mozilla_widget_t * win = (bg_mozilla_widget_t *)data;

  //  display_set_time(win->display, (gavl_time_t)(perc *
  //                                               (float)win->duration + 0.5));
  }

static void seek_release_callback(bg_gtk_slider_t * slider, float perc,
                                  void * data)
  {
  gavl_time_t time;
  bg_mozilla_widget_t * win = (bg_mozilla_widget_t *)data;

  time = (gavl_time_t)(perc * (double)win->duration);
  
  //  bg_mozilla_widget_t * win = (bg_mozilla_widget_t *)data;
  bg_player_seek(win->m->player, time, GAVL_TIME_SCALE);
  
  }

static void
slider_scroll_callback(bg_gtk_slider_t * slider, int up, void * data)
  {
  bg_mozilla_widget_t * win = (bg_mozilla_widget_t *)data;

  if(slider == win->volume_slider)
    {
    if(up)
      bg_player_set_volume_rel(win->m->player, 1.0);
    else
      bg_player_set_volume_rel(win->m->player, -1.0);
    }
  else
    if(slider == win->seek_slider)
    {
    if(up)
      bg_player_seek_rel(win->m->player, 2 * GAVL_TIME_SCALE);
    else
      bg_player_seek_rel(win->m->player, -2 * GAVL_TIME_SCALE);
    }
  
  }

static void volume_change_callback(bg_gtk_slider_t * slider, float perc,
                                   void * data)
  {
  float volume;
  bg_mozilla_widget_t * win = (bg_mozilla_widget_t *)data;
  
  volume = BG_PLAYER_VOLUME_MIN - BG_PLAYER_VOLUME_MIN * perc;
  if(volume > 0.0)
    volume = 0.0;
  
  bg_player_set_volume(win->m->player, volume);
  win->m->volume = volume;
  }



bg_mozilla_widget_t * bg_mozilla_widget_create(bg_mozilla_t * m)
  {
  bg_mozilla_widget_t * w;
  w = calloc(1, sizeof(*w));
  w->m = m;
  w->accel_group = gtk_accel_group_new();
  
  bg_mozilla_widget_init_menu(w);
  
  w->skin_directory =
    bg_mozilla_widget_skin_load(&w->skin,
                                (char *)0);
  return w;
  }

void bg_mozilla_handle_message(bg_mozilla_t * m,
                               bg_msg_t * msg)
  {
  gavl_time_t time;
  int id;
  int arg_i;
  float arg_f;
  char * tmp_string;
  char time_str[GAVL_TIME_STRING_LEN];
  char duration_str[GAVL_TIME_STRING_LEN];
  bg_mozilla_widget_t * w = m->widget;
  
  id = bg_msg_get_id(msg);
  
  switch(id)
    {
    case BG_PLAYER_MSG_TIME_CHANGED:
      time = bg_msg_get_arg_time(msg, 0);
      
      gavl_time_prettyprint(time, time_str);
      //      gavl_time_prettyprint(duration_str, time);
      if(w->duration != GAVL_TIME_UNDEFINED)
        {
        gavl_time_prettyprint(w->duration, duration_str);
        tmp_string = bg_sprintf("%s / %s", time_str, duration_str);
        
        bg_gtk_slider_set_pos(w->seek_slider,
                              gavl_time_to_seconds(time)/
                              gavl_time_to_seconds(w->duration));
        }
      else
        tmp_string = bg_sprintf("%s", time_str);
      
      bg_gtk_scrolltext_set_text(w->scrolltext, tmp_string,
                                 w->fg_normal, w->bg);
      free(tmp_string);
      break;
    case BG_PLAYER_MSG_VOLUME_CHANGED:
      break;
    case BG_PLAYER_MSG_STATE_CHANGED:
      arg_i = bg_msg_get_arg_int(msg, 0);
      
      switch(arg_i)
        {
        case BG_PLAYER_STATE_PAUSED:
          gtk_widget_hide(bg_gtk_button_get_widget(w->pause_button));
          gtk_widget_show(bg_gtk_button_get_widget(w->play_button));
          break;
        case BG_PLAYER_STATE_SEEKING:
          break;
        case BG_PLAYER_STATE_ERROR:
          bg_mozilla_widget_set_error(w);
          break;
        case BG_PLAYER_STATE_BUFFERING:
          arg_f = bg_msg_get_arg_float(msg, 1);
          tmp_string = bg_sprintf("Buffering: %d %%", (int)(100.0 * arg_f + 0.5));
          bg_gtk_scrolltext_set_text(w->scrolltext, tmp_string,
                                     w->fg_normal, w->bg);
          free(tmp_string);
          break;
        case BG_PLAYER_STATE_STARTING:
          break;
        case BG_PLAYER_STATE_PLAYING:
          gtk_widget_hide(w->normal_win.logo_image);
          gtk_widget_hide(w->fullscreen_win.logo_image);
          gtk_widget_show(w->normal_win.socket);
          gtk_widget_show(w->fullscreen_win.socket);
          gtk_widget_show(w->controls);
          gdk_window_raise(w->controls->window);
          
          w->can_pause = bg_msg_get_arg_int(msg, 1);
          
          if(w->can_pause)
            gtk_widget_show(bg_gtk_button_get_widget(w->pause_button));
          else
            gtk_widget_show(bg_gtk_button_get_widget(w->stop_button));
          
          gtk_widget_hide(bg_gtk_button_get_widget(w->play_button));
          resize_toolbar(w);
          break;
        case BG_PLAYER_STATE_STOPPED:
        case BG_PLAYER_STATE_CHANGING:
          if(w->m->player_state != BG_PLAYER_STATE_STOPPED)
            {
            if(w->m->buffer)
              {
              bg_mozilla_buffer_destroy(w->m->buffer); 
              w->m->buffer = (bg_mozilla_buffer_t*)0;
              }
            gtk_widget_hide(w->normal_win.socket);
            gtk_widget_hide(w->fullscreen_win.socket);
            
            gtk_widget_show(w->normal_win.logo_image);
            gtk_widget_show(w->fullscreen_win.logo_image);
            
            if(w->can_pause)
              gtk_widget_hide(bg_gtk_button_get_widget(w->pause_button));
            else
              gtk_widget_hide(bg_gtk_button_get_widget(w->stop_button));
            gtk_widget_show(bg_gtk_button_get_widget(w->play_button));
            
            w->m->state = STATE_IDLE;
            bg_gtk_scrolltext_set_text(w->scrolltext, TR("Stopped"),
                                       w->fg_normal, w->bg);
            }
          break;
        }
      w->m->player_state = arg_i;
      break;
    case BG_PLAYER_MSG_TRACK_DURATION:
      w->duration = bg_msg_get_arg_time(msg, 0);
      arg_i = bg_msg_get_arg_int(msg, 1);
      if(arg_i)
        bg_gtk_slider_set_state(w->seek_slider,
                                BG_GTK_SLIDER_ACTIVE);
      else if(w->duration != GAVL_TIME_UNDEFINED)
        {
        bg_gtk_slider_set_state(w->seek_slider,
                                BG_GTK_SLIDER_INACTIVE);
        }
      else
        bg_gtk_slider_set_state(w->seek_slider,
                                BG_GTK_SLIDER_HIDDEN);
    }
  }

static gboolean idle_callback(void * data)
  {
  bg_msg_t * msg;
  bg_mozilla_widget_t * w = data;

  if(w->m->state == STATE_STARTING)
    {
    pthread_mutex_lock(&w->m->start_finished_mutex);
    if(w->m->start_finished)
      {
      w->m->start_finished = 0;
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Joining start thread...");
      pthread_join(w->m->start_thread, (void**)0);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Joining start thread done");
      if(!w->m->ti)
        w->m->state = STATE_ERROR;
      else
        w->m->state = STATE_PLAYING;
      }
    pthread_mutex_unlock(&w->m->start_finished_mutex);
    }
  while((msg = bg_msg_queue_try_lock_read(w->m->msg_queue)))
    {
    bg_mozilla_handle_message(w->m, msg);
    bg_msg_queue_unlock_read(w->m->msg_queue);
    }
  
  w->idle_counter++;

  if((w->toolbar_state == TOOLBAR_VISIBLE) &&
     (w->idle_counter > 150) &&
     w->autohide_toolbar &&
     (w->m->state == STATE_PLAYING))
    w->toolbar_state = TOOLBAR_HIDING;
  
  if(w->toolbar_state == TOOLBAR_HIDING)
    {
    w->toolbar_pos+=2;
    gtk_fixed_move(GTK_FIXED(w->current_win->box), w->controls, 0,
                   w->toolbar_pos);
    if(w->toolbar_pos == w->current_win->height)
      w->toolbar_state = TOOLBAR_HIDDEN;
    }
  
  return TRUE;
  }

static void plug_added_callback(GtkWidget * w, gpointer data)
  {
  /* Seems that this is switched off, when an earlier client exited */
  GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS);
  gtk_widget_grab_focus(w);
  }


static void volume_leave_notify_callback(GtkWidget * w, GdkEventCrossing *event,
                                         gpointer data)
  {
  bg_mozilla_widget_t * wid = data;
  gtk_widget_hide(wid->volume_window);
  }

static void create_controls(bg_mozilla_widget_t * w)
  {
  w->controls = gtk_fixed_new();
  
  
  gtk_fixed_set_has_window(GTK_FIXED(w->controls), TRUE);
  
  /* Prepare for reparenting */
  g_object_ref(w->controls);
  
  w->scrolltext = bg_gtk_scrolltext_create(0, 0);
  bg_gtk_scrolltext_set_text(w->scrolltext, "Gmerlin mozilla plugin",
                             w->fg_normal, w->bg);
  w->stop_button = bg_gtk_button_create();
  w->pause_button = bg_gtk_button_create();
  w->play_button = bg_gtk_button_create();
  w->volume_button = bg_gtk_button_create();
    
  gtk_widget_hide(bg_gtk_button_get_widget(w->play_button));
  gtk_widget_hide(bg_gtk_button_get_widget(w->pause_button));
  
  bg_gtk_button_set_callback(w->stop_button, gmerlin_button_callback, w);
  bg_gtk_button_set_callback(w->pause_button, gmerlin_button_callback, w);
  bg_gtk_button_set_callback(w->play_button, gmerlin_button_callback, w);
  bg_gtk_button_set_callback(w->volume_button, gmerlin_button_callback, w);
  
  bg_gtk_button_set_skin(w->stop_button,
                         &w->skin.stop_button, w->skin_directory);
  bg_gtk_button_set_skin(w->pause_button,
                         &w->skin.pause_button, w->skin_directory);
  bg_gtk_button_set_skin(w->play_button,
                         &w->skin.play_button, w->skin_directory);
  bg_gtk_button_set_skin(w->volume_button,
                         &w->skin.volume_button, w->skin_directory);
  
  w->seek_slider = bg_gtk_slider_create();
  
  bg_gtk_slider_set_change_callback(w->seek_slider,
                                    seek_change_callback, w);
  
  bg_gtk_slider_set_release_callback(w->seek_slider,
                                     seek_release_callback, w);

  bg_gtk_slider_set_scroll_callback(w->seek_slider,
                                    slider_scroll_callback, w);
  
  bg_gtk_slider_set_skin(w->seek_slider, &w->skin.seek_slider, w->skin_directory);

  gtk_fixed_put(GTK_FIXED(w->controls),
                bg_gtk_button_get_widget(w->stop_button),
                0, 0);
#if 1
  gtk_fixed_put(GTK_FIXED(w->controls),
                bg_gtk_button_get_widget(w->play_button),
                0, 0);
  gtk_fixed_put(GTK_FIXED(w->controls),
                bg_gtk_button_get_widget(w->pause_button),
                0, 0);
#endif

  gtk_fixed_put(GTK_FIXED(w->controls),
                bg_gtk_scrolltext_get_widget(w->scrolltext),
                20, 0);
  gtk_fixed_put(GTK_FIXED(w->controls),
                bg_gtk_slider_get_widget(w->seek_slider),
                0, 20);
  gtk_fixed_put(GTK_FIXED(w->controls),
                bg_gtk_button_get_widget(w->volume_button),
                0, 0);

  /* Volume slider */
  w->volume_window = gtk_window_new(GTK_WINDOW_POPUP);

  gtk_widget_set_events(w->volume_window, GDK_LEAVE_NOTIFY_MASK);
  g_signal_connect(w->volume_window, "leave-notify-event",
                   G_CALLBACK(volume_leave_notify_callback), w);
  
  w->volume_slider = bg_gtk_slider_create();
  
  bg_gtk_slider_set_skin(w->volume_slider,
                         &w->skin.volume_slider,
                         w->skin_directory);
  
  bg_gtk_slider_set_pos(w->volume_slider,
                        (w->m->volume - BG_PLAYER_VOLUME_MIN)/
                        (-BG_PLAYER_VOLUME_MIN));
  
  bg_gtk_slider_set_scroll_callback(w->volume_slider,
                                    slider_scroll_callback, w);
  bg_gtk_slider_set_change_callback(w->volume_slider,
                                    volume_change_callback, w);
  gtk_container_add(GTK_CONTAINER(w->volume_window),
                    bg_gtk_slider_get_widget(w->volume_slider));
  gtk_widget_realize(w->volume_window);
  
  }

static gboolean plug_removed_callback(GtkWidget * w, gpointer data)
  {
  /* Reuse socket */
  return TRUE;
  }

static void init_window(bg_mozilla_widget_t * w,
                        bg_mozilla_window_t * win, int fullscreen)
  {
  GdkColor gdk_black = { 0, 0, 0, 0 };

  gtk_widget_modify_bg(win->window, GTK_STATE_NORMAL, &gdk_black);
  gtk_widget_modify_bg(win->window, GTK_STATE_ACTIVE, &gdk_black);
  gtk_widget_modify_bg(win->window, GTK_STATE_PRELIGHT, &gdk_black);
  gtk_widget_modify_bg(win->window, GTK_STATE_SELECTED, &gdk_black);
  gtk_widget_modify_bg(win->window, GTK_STATE_INSENSITIVE, &gdk_black);
  
  win->socket = gtk_socket_new();
  gtk_widget_modify_bg(win->socket, GTK_STATE_NORMAL, &gdk_black);
  gtk_widget_modify_bg(win->socket, GTK_STATE_ACTIVE, &gdk_black);
  gtk_widget_modify_bg(win->socket, GTK_STATE_PRELIGHT, &gdk_black);
  gtk_widget_modify_bg(win->socket, GTK_STATE_SELECTED, &gdk_black);
  gtk_widget_modify_bg(win->socket, GTK_STATE_INSENSITIVE, &gdk_black);
  // gtk_widget_show(win->socket);

  
  
  win->box = gtk_fixed_new();
  gtk_fixed_put(GTK_FIXED(win->box), win->socket, 0, 0);
  
  if(!fullscreen)
    {
    win->resize_id = g_signal_connect(G_OBJECT(win->box),
                                      "size-allocate",
                                      G_CALLBACK(size_allocate), w);
    }
  
  gtk_widget_show(win->box);
  
  gtk_widget_add_events(win->socket,
                        GDK_BUTTON_PRESS_MASK|
                        GDK_BUTTON_RELEASE_MASK|
                        GDK_KEY_PRESS_MASK|
                        GDK_FOCUS_CHANGE_MASK);

  gtk_widget_add_events(win->window,
                        GDK_BUTTON_PRESS_MASK|
                        GDK_BUTTON_RELEASE_MASK|
                        GDK_KEY_PRESS_MASK);


  g_signal_connect(win->socket, "button-press-event",
                   G_CALLBACK(button_press_callback), w);
  g_signal_connect(win->socket, "button-release-event",
                   G_CALLBACK(button_release_callback), w);

  g_signal_connect(win->window, "button-press-event",
                   G_CALLBACK(button_press_callback), w);
  g_signal_connect(win->window, "button-release-event",
                   G_CALLBACK(button_release_callback), w);
  

  g_signal_connect(win->socket, "key-press-event",
                   G_CALLBACK(key_press_callback), w);
  g_signal_connect(win->socket, "plug-added",
                   G_CALLBACK(plug_added_callback),
                   w);
#if 0
  g_signal_connect(G_OBJECT(win->socket), "grab-notify",
                   G_CALLBACK(grab_notify_callback),
                   w);
  g_signal_connect(G_OBJECT(win->socket), "focus-in-event",
                   G_CALLBACK(focus_callback),
                   w);
  g_signal_connect(G_OBJECT(win->socket), "focus-out-event",
                   G_CALLBACK(focus_callback),
                   w);
#endif
  g_signal_connect(G_OBJECT(win->socket), "plug-removed",
                   G_CALLBACK(plug_removed_callback),
                   w);
  
  gtk_container_add(GTK_CONTAINER(win->window), win->box);
  
  }


void bg_mozilla_widget_set_window(bg_mozilla_widget_t * w,
                                  GdkNativeWindow window_id)
  {
  char * tmp_string;
  GdkDisplay * dpy;
  
  if(w->normal_win.window)
    gtk_widget_destroy(w->normal_win.window);
  
  if(w->normal_win.socket)
    return;
  
  //  gtk_widget_set_events(w->socket, GDK_BUTTON_PRESS_MASK);
  //  g_signal_connect(w->socket, "button-press-event",
  //                   G_CALLBACK(button_press_callback), w);
  w->normal_win.window = gtk_plug_new(window_id);
  
  w->fullscreen_win.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_add_accel_group(GTK_WINDOW(w->fullscreen_win.window),
                             w->accel_group);
  
  // gtk_window_fullscreen(GTK_WINDOW(w->fullscreen_win.window));
  // gtk_window_set_resizable(GTK_WINDOW(w->fullscreen_win.window), FALSE);
  
  init_window(w, &w->normal_win, 0);
  init_window(w, &w->fullscreen_win, 1);
  w->current_win = &w->normal_win;

  gtk_container_set_resize_mode(GTK_CONTAINER(w->fullscreen_win.box),
                                GTK_RESIZE_QUEUE);
  
  create_controls(w);

  if(w->skin.logo)
    {
    tmp_string = bg_sprintf("%s/%s", w->skin_directory,
                           w->skin.logo);
    w->logo_pixbuf = gdk_pixbuf_new_from_file(tmp_string, NULL);
    free(tmp_string);
    }

  w->fullscreen_win.logo_image = gtk_image_new_from_pixbuf(w->logo_pixbuf);
  gtk_widget_show(w->fullscreen_win.logo_image);
    
  gtk_fixed_put(GTK_FIXED(w->fullscreen_win.box),
                w->fullscreen_win.logo_image, 0, 0);
  
  gtk_fixed_put(GTK_FIXED(w->current_win->box), w->controls, 0, 0);
  
  //  gtk_widget_realize(w->plug);

  //  gtk_container_add(GTK_CONTAINER(w->plug), w->table);
  gtk_widget_show(w->normal_win.window);
  w->current_win = &w->normal_win;
  
  gtk_widget_show(w->controls);
  
  /* Get display string */
  gtk_widget_realize(w->normal_win.socket);
  gtk_widget_realize(w->fullscreen_win.socket);
  
  dpy = gdk_display_get_default();
  
  w->m->display_string = 
    bg_sprintf("%s:%08lx:%08lx", gdk_display_get_name(dpy),
               (long unsigned int)gtk_socket_get_id(GTK_SOCKET(w->normal_win.socket)),
               (long unsigned int)gtk_socket_get_id(GTK_SOCKET(w->fullscreen_win.socket)));

  
  if(w->m->ov_info)
    {
    if(w->m->ov_handle)
      {
      bg_plugin_unref(w->m->ov_handle);
      w->m->ov_handle = NULL;
      }
    
    w->m->ov_handle = bg_ov_plugin_load(w->m->plugin_reg, w->m->ov_info,
                                        w->m->display_string);
    bg_player_set_ov_plugin(w->m->player, w->m->ov_handle);
    }
  w->idle_id = g_timeout_add(50, idle_callback, (gpointer)w);

  /* If we have the url already, we start playing */
  if(w->m->url && (w->m->url_mode == URL_MODE_LOCAL))
    gmerlin_mozilla_start(w->m);
  
  }

void bg_mozilla_widget_destroy(bg_mozilla_widget_t * m)
  {
  if(m->normal_win.window)
    gtk_widget_destroy(m->normal_win.window);
  if(m->fullscreen_win.window)
    gtk_widget_destroy(m->fullscreen_win.window);
  if(m->controls)
    g_object_unref(m->controls);
  if(m->scrolltext)
    bg_gtk_scrolltext_destroy(m->scrolltext);

  if(m->idle_id > 0)
    g_source_remove(m->idle_id);

  g_object_unref(m->accel_group);

  if(m->logo_pixbuf)
    g_object_unref(m->logo_pixbuf);
  if(m->logo_pixbuf_scaled)
    g_object_unref(m->logo_pixbuf_scaled);
  
  free(m);
  }

void bg_mozilla_widget_toggle_fullscreen(bg_mozilla_widget_t * m)
  {
  /* Unparent toolbar */
  gtk_container_remove(GTK_CONTAINER(m->current_win->box),
                       m->controls);
  
  /* Normal to fullscreen */
  if(m->current_win == &m->normal_win)
    {
    gulong resize_id;
    resize_id = g_signal_connect(G_OBJECT(m->fullscreen_win.window),
                                 "size-allocate",
                                 G_CALLBACK(window_size_allocate), m);
    gtk_widget_show(m->fullscreen_win.window);
    gtk_window_fullscreen(GTK_WINDOW(m->fullscreen_win.window));

    /* HACK: Actually I didn't mess with mozillas event-loop,
       but we need the fullscreen size right after the window was created */
    while(gtk_events_pending())
      gtk_main_iteration();
    
    g_signal_handler_disconnect(G_OBJECT(m->fullscreen_win.window),
                                resize_id);

    gtk_widget_set_size_request(m->fullscreen_win.box,
                                m->fullscreen_win.width,
                                m->fullscreen_win.height);
    gtk_widget_set_size_request(m->fullscreen_win.socket,
                                m->fullscreen_win.width,
                                m->fullscreen_win.height);
    
    m->current_win = &m->fullscreen_win;

    gtk_widget_hide(m->menu.fullscreen);
    gtk_widget_show(m->menu.windowed);
    }
  /* Fullscreen to normal */
  else
    {
    gtk_widget_hide(m->fullscreen_win.window);
    m->current_win = &m->normal_win;
    m->fullscreen_win.width = 0;
    m->fullscreen_win.height = 0;
 
    gtk_widget_hide(m->menu.windowed);
    gtk_widget_show(m->menu.fullscreen);
    }
  
  gtk_fixed_put(GTK_FIXED(m->current_win->box), m->controls, 0, 0);
  resize_toolbar(m);
  }


/* Configuration */

static const bg_parameter_info_t parameters[] =
  {
    {
      .name        = "autohide_toolbar",
      .long_name   = "Hide toolbar",
      .type        = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
      .help_string = TRS("Automatically hide the toolbar after the last mouse click"),
    },
    {
      .name =      "background",
      .long_name = TRS("Background"),
      .type = BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 0.215686, 0.215686, 0.215686, 1.0 } },
    },
    {
      .name =      "foreground_normal",
      .long_name = TRS("Normal foreground"),
      .type = BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 1.0, 1.0, 1.0, 1.0 } },
    },
    {
      .name =      "foreground_error",
      .long_name = TRS("Error foreground"),
      .type = BG_PARAMETER_COLOR_RGB,
      .val_default = { .val_color = { 1.0, 0.5, 0.0, 1.0 } },
    },
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_mozilla_widget_get_parameters(bg_mozilla_widget_t * m)
  {
  return parameters;
  }

void bg_mozilla_widget_set_parameter(void * priv, const char * name,
                                     const bg_parameter_value_t * v)
  {
  bg_mozilla_widget_t * m = priv;

  if(!name)
    return;
  if(!strcmp(name, "autohide_toolbar"))
    m->autohide_toolbar = v->val_i;
  else if(!strcmp(name, "foreground_error"))
    memcpy(m->fg_error, v->val_color, 3 * sizeof(float));
  else if(!strcmp(name, "foreground_normal"))
    memcpy(m->fg_normal, v->val_color, 3 * sizeof(float));
  else if(!strcmp(name, "background"))
    memcpy(m->bg, v->val_color, 3 * sizeof(float));
  
  }

void bg_mozilla_widget_set_error(bg_mozilla_widget_t * m)
  {
  char * msg = bg_log_last_error();
  if(msg)
    {
    bg_gtk_scrolltext_set_text(m->scrolltext, msg,
                               m->fg_error, m->bg);
    free(msg);
    }
  else
    bg_gtk_scrolltext_set_text(m->scrolltext, TR("Error"),
                               m->fg_error, m->bg);
  }
