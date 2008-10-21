#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>
#include <gdk/gdkkeysyms.h>

// #include <ovBox.h>
// #include <drawer.h>

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
  w->toolbar_height = 32;
  fprintf(stderr, "Embed size allocate: %dx%d+%d+%d\n",
          a->width, a->height, a->x, a->y);
  
  g_signal_handler_block(w->box, w->resize_id);

  if(w->socket)
    gtk_widget_set_size_request(w->socket, a->width, a->height);
  if(w->controls)
    {
    gtk_widget_set_size_request(w->controls, a->width, w->toolbar_height);
    w->toolbar_pos = a->height - w->toolbar_height;
    gtk_fixed_move(GTK_FIXED(w->box), w->controls, 0,
                   w->toolbar_pos);
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

bg_mozilla_widget_t * bg_mozilla_widget_create(bg_mozilla_t * m)
  {
  bg_mozilla_widget_t * w;
  w = calloc(1, sizeof(*w));
  w->m = m;
  bg_mozilla_widget_init_menu(w);
  return w;
  }

static void handle_message(bg_mozilla_widget_t * w,
                           bg_msg_t * msg)
  {
  gavl_time_t time;
  int id;
  int arg_i;
  id = bg_msg_get_id(msg);
  
  switch(id)
    {
    case BG_PLAYER_MSG_TIME_CHANGED:
      time = bg_msg_get_arg_time(msg, 0);
      fprintf(stderr, "Time: %f\n", gavl_time_to_seconds(time));
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
          fprintf(stderr, "State: Buffering\n");
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
      }
    pthread_mutex_unlock(&w->m->start_finished_mutex);
    w->m->state = STATE_PLAYING;
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
  
  w->controls = gtk_event_box_new();
  //  g_signal_connect(G_OBJECT(w->controls),
  //                   "size-allocate",
  //                   G_CALLBACK(size_allocate), NULL);

  /* Prepare for reparenting */
  g_object_ref(w->controls);
  
  w->button = gtk_button_new_with_label("Bla");
  gtk_widget_show(w->button);
  
  gtk_container_add(GTK_CONTAINER(w->controls),
                    w->button);
  
  //  g_signal_connect(G_OBJECT(w->button),
  //                   "size-allocate",
  //                   G_CALLBACK(size_allocate_test), NULL);
  
  w->box = gtk_fixed_new();
  
  gtk_fixed_put(GTK_FIXED(w->box), w->socket, 0, 0);
  gtk_fixed_put(GTK_FIXED(w->box), w->controls, 0, 0);
  //  gtk_fixed_put(GTK_FIXED(w->box), w->button, 10, 10);

  w->resize_id = g_signal_connect(G_OBJECT(w->box),
                                  "size-allocate",
                                  G_CALLBACK(size_allocate), w);
  
  //  gtk_widget_set_size_request(w->box, 320, 240);
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
