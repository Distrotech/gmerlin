#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>
#include <gdk/gdkkeysyms.h>

#define TOOLBAR_VISIBLE 0
#define TOOLBAR_HIDING  1
#define TOOLBAR_HIDDEN  2

static void show_toolbar(bg_mozilla_widget_t * w)
  {
  w->toolbar_state = TOOLBAR_VISIBLE;
  w->toolbar_pos = w->height - w->toolbar_height;
  gtk_fixed_move(GTK_FIXED(w->box), w->controls, 0,
                 w->toolbar_pos);
  }

static void size_allocate(GtkWidget     *widget,
                          GtkAllocation *a,
                          gpointer       user_data)
  {
  bg_mozilla_widget_t * w;
  w = user_data;

  if((a->width == w->width) && (a->height == w->height))
    return;

  w->width = a->width;
  w->height = a->height;
  w->toolbar_height = 40;
  fprintf(stderr, "Embed size allocate: %dx%d+%d+%d\n",
          a->width, a->height, a->x, a->y);
  
  gtk_widget_set_size_request(bg_gtk_button_get_widget(w->stop_button),
                              20, 20);
  
  gtk_widget_set_size_request(bg_gtk_scrolltext_get_widget(w->scrolltext),
                              a->width-20, 20);
  gtk_widget_set_size_request(bg_gtk_slider_get_widget(w->seek_slider),
                              a->width, 20);
  
                              
  g_signal_handler_block(w->box, w->resize_id);

  if(w->socket)
    gtk_widget_set_size_request(w->socket, a->width, a->height);
  if(w->controls)
    {
    gtk_widget_set_size_request(w->controls, a->width, w->toolbar_height);
    w->toolbar_pos = a->height - w->toolbar_height;
    gtk_fixed_move(GTK_FIXED(w->box), w->controls, 0,
                   w->toolbar_pos);
    //    gtk_container_check_resize(GTK_CONTAINER(w->table));
    }
  g_signal_handler_unblock(w->box, w->resize_id);
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
     



static gboolean button_press_callback(GtkWidget * wid, GdkEventButton * evt,
                                      gpointer data)
  {
  bg_mozilla_widget_t * w;
  w = data;
  w->idle_counter = 0;
  fprintf(stderr, "button_press_callback %d\n", evt->button);

  GTK_WIDGET_SET_FLAGS(w->socket, GTK_CAN_FOCUS);
  gtk_widget_grab_focus(w->socket);
  
#if 0
  if(evt->button == 3)
    {
    gtk_menu_popup(GTK_MENU(w->menu.menu),
                   (GtkWidget *)0,
                   (GtkWidget *)0,
                   (GtkMenuPositionFunc)0,
                   (gpointer)0,
                   3,
                   // gtk_get_current_event_time()
                   evt->time
                   );
    return TRUE;
    }
#endif
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
  fprintf(stderr, "button_release_callback %d\n", evt->button);
  if(evt->button == 3)
    {
    w->popup_time = evt->time;
    g_idle_add(popup_menu, w);
    return TRUE;
    }
#endif
  return TRUE;
  }

static gboolean key_press_callback(GtkWidget * wid, GdkEventKey * evt,
                                   gpointer data)
  {
  bg_mozilla_widget_t * w;
  w = data;
  fprintf(stderr, "key_press_callback\n");

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
      break;
    }
  
  return TRUE;
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
  bg_player_seek(win->m->player, time);
  
  }

static void
slider_scroll_callback(bg_gtk_slider_t * slider, int up, void * data)
  {
  bg_mozilla_widget_t * win = (bg_mozilla_widget_t *)data;

#if 0  
  if(slider == win->volume_slider)
    {
    if(up)
      bg_player_set_volume_rel(win->m->player, 1.0);
    else
      bg_player_set_volume_rel(win->m->player, -1.0);
    }
  else
#endif
    if(slider == win->seek_slider)
    {
    if(up)
      bg_player_seek_rel(win->m->player, 2 * GAVL_TIME_SCALE);
    else
      bg_player_seek_rel(win->m->player, -2 * GAVL_TIME_SCALE);
    }
  
  }


bg_mozilla_widget_t * bg_mozilla_widget_create(bg_mozilla_t * m)
  {
  bg_mozilla_widget_t * w;
  w = calloc(1, sizeof(*w));
  w->m = m;
  w->fg_normal[0] = 1.0;
  w->fg_normal[1] = 0.5;
  w->fg_normal[2] = 0.0;
  
  bg_mozilla_widget_init_menu(w);
  
  w->skin_directory =
    bg_mozilla_widget_skin_load(&w->skin,
                                (char *)0);
  fprintf(stderr, "Loaded skin %s\n", w->skin_directory);
  return w;
  }

static void handle_message(bg_mozilla_widget_t * w,
                           bg_msg_t * msg)
  {
  gavl_time_t time;
  int id;
  int arg_i;
  float arg_f;
  char * tmp_string;
  char time_str[GAVL_TIME_STRING_LEN];
  char duration_str[GAVL_TIME_STRING_LEN];
  
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
          fprintf(stderr, "State: Paused\n");
          break;
        case BG_PLAYER_STATE_SEEKING:
          fprintf(stderr, "State: Seeking\n");
          break;
        case BG_PLAYER_STATE_ERROR:
          fprintf(stderr, "State: Error\n");
          break;
        case BG_PLAYER_STATE_BUFFERING:
          arg_f = bg_msg_get_arg_float(msg, 1);
          // fprintf(stderr, "State: Buffering\n");
          tmp_string = bg_sprintf("Buffering: %d %%", (int)(100.0 * arg_f + 0.5));
          bg_gtk_scrolltext_set_text(w->scrolltext, tmp_string,
                                     w->fg_normal, w->bg);
          free(tmp_string);
          break;
        case BG_PLAYER_STATE_PLAYING:
          fprintf(stderr, "State: Playing\n");
          break;
        case BG_PLAYER_STATE_STOPPED:
          fprintf(stderr, "State: Stopped\n");
          break;
        case BG_PLAYER_STATE_CHANGING:
          fprintf(stderr, "State: Changing\n");
          break;
        }
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
      fprintf(stderr, "Joining start thread...");
      pthread_join(w->m->start_thread, (void**)0);
      fprintf(stderr, "done\n");
      w->m->state = STATE_PLAYING;
      }
    pthread_mutex_unlock(&w->m->start_finished_mutex);
    }
  else if(w->m->state == STATE_PLAYING)
    {
    while((msg = bg_msg_queue_try_lock_read(w->m->msg_queue)))
      {
      handle_message(w, msg);
      bg_msg_queue_unlock_read(w->m->msg_queue);
      }
    }
  
  w->idle_counter++;

  if((w->toolbar_state == TOOLBAR_VISIBLE) &&
     (w->idle_counter > 150))
    w->toolbar_state = TOOLBAR_HIDING;

  if(w->toolbar_state == TOOLBAR_HIDING)
    {
    w->toolbar_pos+=2;
    gtk_fixed_move(GTK_FIXED(w->box), w->controls, 0,
                   w->toolbar_pos);
    if(w->toolbar_pos == w->height)
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

#if 1
static void grab_notify_callback(GtkWidget *widget,
                                 gboolean   was_grabbed,
                                 gpointer   data)
  {
  bg_mozilla_widget_t * w = data;
  fprintf(stderr, "grab notify %d\n", was_grabbed);
  if(!was_grabbed)
    {
    GTK_WIDGET_SET_FLAGS(w->socket, GTK_CAN_FOCUS);
    gtk_widget_grab_focus(w->socket);
    }
  }
#endif
static void focus_callback(GtkWidget *widget,
                           GdkEventFocus *event,
                           gpointer   data)
  {
  //  bg_mozilla_widget_t * w = data;
  if(event->in)
    fprintf(stderr, "Focus in\n");
  else
    fprintf(stderr, "Focus out\n");
  }

static void create_controls(bg_mozilla_widget_t * w)
  {
  w->controls = gtk_fixed_new();

  gtk_fixed_set_has_window(GTK_FIXED(w->controls), TRUE);
  
  //  g_signal_connect(G_OBJECT(w->controls),
  //                   "size-allocate",
  //                   G_CALLBACK(size_allocate), NULL);

  /* Prepare for reparenting */
  g_object_ref(w->controls);
  
  //  w->button = gtk_button_new_with_label("Bla");
  //  gtk_widget_show(w->button);
  
  //  gtk_container_add(GTK_CONTAINER(w->controls),
  //                  w->button);
    //  
  //  g_signal_connect(G_OBJECT(w->button),
  //                   "size-allocate",
  //                   G_CALLBACK(size_allocate_test), NULL);

  w->scrolltext = bg_gtk_scrolltext_create(0, 0);
  bg_gtk_scrolltext_set_text(w->scrolltext, "Gmerlin mozilla plugin",
                             w->fg_normal, w->bg);
#if 1
  w->stop_button = bg_gtk_button_create();
  w->pause_button = bg_gtk_button_create();
  w->play_button = bg_gtk_button_create();

  bg_gtk_button_set_skin(w->stop_button,
                         &w->skin.stop_button, w->skin_directory);
  bg_gtk_button_set_skin(w->pause_button,
                         &w->skin.pause_button, w->skin_directory);
  bg_gtk_button_set_skin(w->play_button,
                         &w->skin.play_button, w->skin_directory);
#endif
  
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
  
  gtk_fixed_put(GTK_FIXED(w->controls),
                bg_gtk_scrolltext_get_widget(w->scrolltext),
                20, 0);
  gtk_fixed_put(GTK_FIXED(w->controls),
                bg_gtk_slider_get_widget(w->seek_slider),
                0, 20);
  
  
  }

void bg_mozilla_widget_set_window(bg_mozilla_widget_t * w,
                                  GdkNativeWindow window_id)
  {
  GdkDisplay * dpy;
  GdkColor gdk_black = { 0, 0, 0, 0 };
  
  if(w->plug)
    gtk_widget_destroy(w->plug);
  
  if(w->socket)
    return;
  
  w->socket = gtk_socket_new();
  
  //  gtk_widget_set_events(w->socket, GDK_BUTTON_PRESS_MASK);
  //  g_signal_connect(w->socket, "button-press-event",
  //                   G_CALLBACK(button_press_callback), w);
  
  gtk_widget_modify_bg(w->socket, GTK_STATE_NORMAL, &gdk_black);
  gtk_widget_modify_bg(w->socket, GTK_STATE_ACTIVE, &gdk_black);
  gtk_widget_modify_bg(w->socket, GTK_STATE_PRELIGHT, &gdk_black);
  gtk_widget_modify_bg(w->socket, GTK_STATE_SELECTED, &gdk_black);
  gtk_widget_modify_bg(w->socket, GTK_STATE_INSENSITIVE, &gdk_black);
  
  gtk_widget_show(w->socket);

  create_controls(w);
  
  w->box = gtk_fixed_new();
  
  gtk_fixed_put(GTK_FIXED(w->box), w->socket, 0, 0);
  gtk_fixed_put(GTK_FIXED(w->box), w->controls, 0, 0);
  //  gtk_fixed_put(GTK_FIXED(w->box), w->button, 10, 10);
  
  w->resize_id = g_signal_connect(G_OBJECT(w->box),
                                  "size-allocate",
                                  G_CALLBACK(size_allocate), w);
  
  gtk_widget_show(w->box);
    
  w->plug = gtk_plug_new(window_id);

  gtk_widget_add_events(w->socket,
                        GDK_BUTTON_PRESS_MASK|
                        GDK_BUTTON_RELEASE_MASK|
                        GDK_KEY_PRESS_MASK|
                        GDK_FOCUS_CHANGE_MASK);
  g_signal_connect(w->socket, "button-press-event",
                   G_CALLBACK(button_press_callback), w);
  g_signal_connect(w->socket, "button-release-event",
                   G_CALLBACK(button_release_callback), w);
  g_signal_connect(w->socket, "key-press-event",
                   G_CALLBACK(key_press_callback), w);
  g_signal_connect(G_OBJECT(w->socket), "plug-added",
                   G_CALLBACK(plug_added_callback),
                   w);
  g_signal_connect(G_OBJECT(w->socket), "grab-notify",
                   G_CALLBACK(grab_notify_callback),
                   w);
  g_signal_connect(G_OBJECT(w->socket), "focus-in-event",
                   G_CALLBACK(focus_callback),
                   w);
  g_signal_connect(G_OBJECT(w->socket), "focus-out-event",
                   G_CALLBACK(focus_callback),
                   w);

  //  gtk_widget_realize(w->plug);

  //  gtk_container_add(GTK_CONTAINER(w->plug), w->table);
  gtk_container_add(GTK_CONTAINER(w->plug), w->box);
  gtk_widget_show(w->plug);
  gtk_widget_show(w->controls);
  
  /* Get display string */
  gtk_widget_realize(w->socket);

  dpy = gdk_display_get_default();
  
  w->m->display_string = 
    bg_sprintf("%s:%08lx:", gdk_display_get_name(dpy),
               (long unsigned int)gtk_socket_get_id(GTK_SOCKET(w->socket)));

  if(w->m->ov_info)
    {
    if(w->m->ov_handle)
      {
      bg_plugin_unref(w->m->ov_handle);
      w->m->ov_handle = NULL;
      }
    
    w->m->ov_handle = bg_ov_plugin_load(w->m->plugin_reg, w->m->ov_info,
                                        w->m->display_string);
    //    fprintf(stderr, "Loaded OV %s\n", w->m->ov_handle->info->name);
    bg_player_set_ov_plugin(w->m->player, w->m->ov_handle);
    }
  w->idle_id = g_timeout_add(50, idle_callback, (gpointer)w);
  }

void bg_mozilla_widget_destroy(bg_mozilla_widget_t * m)
  {
  gtk_widget_destroy(m->socket);
  g_object_unref(m->controls);
  g_source_remove(m->idle_id);
  free(m);
  }
