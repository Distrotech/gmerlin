#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gmerlin.h"
#include <utils.h>

#define DELAY_TIME 10

#define BACKGROUND_WINDOW GTK_LAYOUT(win->layout)->bin_window
#define MASK_WIDNOW       win->window->window

#define BACKGROUND_WIDGET win->layout
#define MASK_WIDGET       win->window

static void set_background(player_window_t * win)
  {
  GdkBitmap * mask;
  GdkPixmap * pixmap;
  int width, height;
  
  if(!win->background_pixbuf)
    return;
  
  width = gdk_pixbuf_get_width(win->background_pixbuf);
  height = gdk_pixbuf_get_height(win->background_pixbuf);

  gtk_widget_set_size_request(win->window, width, height);
  
  gdk_pixbuf_render_pixmap_and_mask(win->background_pixbuf,
                                    &pixmap, &mask, 0x80);
  
  if(pixmap && BACKGROUND_WINDOW)
    {
    gdk_window_set_back_pixmap(BACKGROUND_WINDOW, pixmap, FALSE);
    }
  if(mask && MASK_WIDNOW)
    {
    gdk_window_shape_combine_mask(MASK_WIDNOW, mask, 0, 0);
    }
  }

void player_window_set_skin(player_window_t * win,
                            player_window_skin_t * s,
                            const char * directory)
  {
  int x, y;
  char * tmp_path;
  
  if(win->background_pixbuf)
    {
    g_object_unref(G_OBJECT(win->background_pixbuf));
    }

  tmp_path = bg_sprintf("%s/%s", directory, s->background);
  
  win->background_pixbuf = gdk_pixbuf_new_from_file(tmp_path, NULL);
  
  free(tmp_path);
  set_background(win);

  /* Apply the button skins */

  bg_gtk_button_set_skin(win->play_button, &(s->play_button), directory);
  bg_gtk_button_get_coords(win->play_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                  bg_gtk_button_get_widget(win->play_button),
                  x, y);

  bg_gtk_button_set_skin(win->stop_button, &(s->stop_button), directory);
  bg_gtk_button_get_coords(win->stop_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->stop_button),
                 x, y);

  bg_gtk_button_set_skin(win->pause_button, &(s->pause_button), directory);
  bg_gtk_button_get_coords(win->pause_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->pause_button),
                 x, y);
  
  bg_gtk_button_set_skin(win->next_button, &(s->next_button), directory);
  bg_gtk_button_get_coords(win->next_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->next_button),
                 x, y);
  
  bg_gtk_button_set_skin(win->prev_button, &(s->prev_button), directory);
  bg_gtk_button_get_coords(win->prev_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->prev_button),
                 x, y);

  bg_gtk_button_set_skin(win->close_button, &(s->close_button), directory);
  bg_gtk_button_get_coords(win->close_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->close_button),
                 x, y);

  bg_gtk_button_set_skin(win->menu_button, &(s->menu_button), directory);
  bg_gtk_button_get_coords(win->menu_button, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_button_get_widget(win->menu_button),
                 x, y);

  bg_gtk_slider_set_skin(win->seek_slider, &(s->seek_slider), directory);
  bg_gtk_slider_get_coords(win->seek_slider, &x, &y);
  gtk_layout_move(GTK_LAYOUT(win->layout),
                 bg_gtk_slider_get_widget(win->seek_slider),
                 x, y);

  display_set_skin(win->display, &(s->display));
  display_get_coords(win->display, &x, &y);
  
  gtk_layout_move(GTK_LAYOUT(win->layout),
                  display_get_widget(win->display),
                  x, y);


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
  
  win->x = (int)(evt->x);
  win->y = (int)(evt->y);
  
  return TRUE;
  }


static gboolean motion_callback(GtkWidget * w, GdkEventMotion * evt,
                                gpointer data)
  {
  player_window_t * win;

  win = (player_window_t*)data;
  gtk_window_move(GTK_WINDOW(win->window),
                  (int)(evt->x_root) - win->x,
                  (int)(evt->y_root) - win->y);
  
  return TRUE;
  }

/* Gmerlin callbacks */

static void seek_change_callback(bg_gtk_slider_t * slider, float perc,
                                 void * data)
  {
  player_window_t * win = (player_window_t *)data;

  win->seek_active = 1;
  
  //  player_window_t * win = (player_window_t *)data;
  //  fprintf(stderr, "Seek change callback %f\n", perc);

  display_set_time(win->display, (int)(perc * (float)win->duration + 0.5));
  }

static void seek_release_callback(bg_gtk_slider_t * slider, float perc,
                                  void * data)
  {
  player_window_t * win = (player_window_t *)data;
  //  player_window_t * win = (player_window_t *)data;
  fprintf(stderr, "Seek release callback %f\n", perc);
  bg_player_seek(win->gmerlin->player, perc);
  
  }

static void gmerlin_button_callback(bg_gtk_button_t * b, void * data)
  {
  player_window_t * win = (player_window_t *)data;
  if(b == win->play_button)
    {
    fprintf(stderr, "Play button clicked\n");

    gmerlin_play(win->gmerlin, BG_PLAYER_IGNORE_IF_PLAYING);
    
    }
  else if(b == win->pause_button)
    {
    bg_player_pause(win->gmerlin->player);
    }
  else if(b == win->stop_button)
    {
    fprintf(stderr, "Stop button clicked\n");
    bg_player_stop(win->gmerlin->player);
    }
  else if(b == win->next_button)
    {
    fprintf(stderr, "Next button clicked\n");

    bg_media_tree_next(win->gmerlin->tree, 1);
    //    fprintf(stderr, "Handle: %p plugin %p\n", handle, handle->plugin);

    gmerlin_play(win->gmerlin, BG_PLAYER_IGNORE_IF_STOPPED);
    }
  else if(b == win->prev_button)
    {
    fprintf(stderr, "Prev button clicked\n");
    bg_media_tree_previous(win->gmerlin->tree, 1);

    gmerlin_play(win->gmerlin, BG_PLAYER_IGNORE_IF_STOPPED);
    }
  else if(b == win->close_button)
    {
    fprintf(stderr, "Close button clicked\n");
    gtk_main_quit();
    }
  }

static void handle_message(player_window_t * win,
                           bg_msg_t * msg)
  {
  int id;
  int arg_i_1;
  int arg_i_2;
  float arg_f_1;
  char * arg_str_1;
  id = bg_msg_get_id(msg);
  switch(id)
    {
    case BG_PLAYER_MSG_TIME_CHANGED:
      if(!win->seek_active)
        {
        arg_i_1 = bg_msg_get_arg_int(msg, 0);
        display_set_time(win->display, arg_i_1);
        if(win->duration)
          bg_gtk_slider_set_pos(win->seek_slider,
                                (float)(arg_i_1) / (float)(win->duration));
        }
      break;
    case BG_PLAYER_MSG_TRACK_CHANGED:
      break;
    case BG_PLAYER_MSG_STATE_CHANGED:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      
      switch(arg_i_1)
        {
        case BG_PLAYER_STATE_SEEKING:
          win->seek_active = 0;
          break;
        case BG_PLAYER_STATE_ERROR:
          fprintf(stderr, "State Error\n");
          arg_str_1 = bg_msg_get_arg_string(msg, 1);
          display_set_state(win->display, arg_i_1, arg_str_1);
          arg_i_1 = bg_msg_get_arg_int(msg, 2);

          switch(arg_i_1)
            {
            case BG_PLAYER_ERROR_TRACK:
              if(win->gmerlin->playback_flags & PLAYBACK_MARK_ERROR)
                {
                /* Mark track as error in the tree */
                bg_media_tree_mark_error(win->gmerlin->tree, 1);
                }
              if(win->gmerlin->playback_flags & PLAYBACK_SKIP_ERROR)
                {
                /* Next track */
                gmerlin_next_track(win->gmerlin);
                }
              break;
            }
          break;
        case BG_PLAYER_STATE_BUFFERING:
          arg_f_1 = bg_msg_get_arg_float(msg, 1);
          display_set_state(win->display, arg_i_1, &arg_f_1);
          break;
        case BG_PLAYER_STATE_PLAYING:
          arg_i_2 = bg_msg_get_arg_int(msg, 1);
          if(arg_i_2)
            {
            bg_gtk_slider_set_state(win->seek_slider,
                                    BG_GTK_SLIDER_ACTIVE);
            }
          else if(win->duration > 0)
            {
            bg_gtk_slider_set_state(win->seek_slider,
                                    BG_GTK_SLIDER_INACTIVE);
            }
          else
            {
            bg_gtk_slider_set_state(win->seek_slider,
                                    BG_GTK_SLIDER_HIDDEN);
            }
          display_set_state(win->display, arg_i_1, NULL);
          bg_media_tree_mark_error(win->gmerlin->tree, 0);
          break;
        case BG_PLAYER_STATE_STOPPED:
          bg_gtk_slider_set_state(win->seek_slider,
                                  BG_GTK_SLIDER_INACTIVE);
          display_set_state(win->display, arg_i_1, NULL);
          break;
        case BG_PLAYER_STATE_CHANGING:
          arg_i_2 = bg_msg_get_arg_int(msg, 1);
          if(arg_i_2)
            gmerlin_next_track(win->gmerlin);
        }
      break;
    case BG_PLAYER_MSG_TRACK_NAME:
      fprintf(stderr, "BG_PLAYER_MSG_TRACK_NAME\n");
      arg_str_1 = bg_msg_get_arg_string(msg, 0);
      display_set_track_name(win->display, arg_str_1);
      free(arg_str_1);
      break;
    case BG_PLAYER_MSG_TRACK_NUM_STREAMS:
      break;
    case BG_PLAYER_MSG_TRACK_DURATION:
      arg_i_1 = bg_msg_get_arg_int(msg, 0);
      win->duration = arg_i_1;
      break;
    case BG_PLAYER_MSG_AUDIO_STREAM:
      break;
    case BG_PLAYER_MSG_VIDEO_STREAM:
      break;
    case BG_PLAYER_MSG_META_ARTIST:
      break;
    case BG_PLAYER_MSG_META_TITLE:
      break;
    case BG_PLAYER_MSG_META_ALBUM:
      break;
    case BG_PLAYER_MSG_META_GENRE:
      break;
    case BG_PLAYER_MSG_META_COMMENT:
      break;
    case BG_PLAYER_MSG_AUDIO_DESCRIPTION:
      break;
    case BG_PLAYER_MSG_VIDEO_DESCRIPTION:
      break;
    case BG_PLAYER_MSG_SUBPICTURE_DESCRIPTION:
      break;
    case BG_PLAYER_MSG_STREAM_DESCRIPTION:
      break;
    case BG_PLAYER_MSG_META_YEAR:
      break;
    case BG_PLAYER_MSG_META_TRACK:
      break;
    }
  
  }

gboolean idle_callback(gpointer data)
  {
  bg_msg_t * msg;
  
  player_window_t * w = (player_window_t *)data;

  while((msg = bg_msg_queue_try_lock_read(w->msg_queue)))
    {
    handle_message(w, msg);
    bg_msg_queue_unlock_read(w->msg_queue);
    }
  return TRUE;
  }

player_window_t * player_window_create(gmerlin_t * g)
  {
  player_window_t * ret;

  ret = calloc(1, sizeof(*ret));
  ret->gmerlin = g;

  ret->msg_queue = bg_msg_queue_create();

  bg_player_add_message_queue(g->player,
                              ret->msg_queue);

  g_timeout_add(DELAY_TIME, idle_callback, (gpointer)ret);
  
  /* Create objects */

  
  ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(ret->window), FALSE);
  
  ret->layout = gtk_layout_new((GtkAdjustment*)0,
                               (GtkAdjustment*)0);
  
  /* Set attributes */

  gtk_widget_set_events(ret->layout,
                        GDK_BUTTON1_MOTION_MASK|
                        GDK_BUTTON2_MOTION_MASK|
                        GDK_BUTTON3_MOTION_MASK|
                        GDK_BUTTON_PRESS_MASK);
  /* Set Callbacks */

  g_signal_connect(G_OBJECT(ret->window), "realize",
                   G_CALLBACK(realize_callback), (gpointer*)ret);
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

  ret->seek_slider = bg_gtk_slider_create(0);

  ret->main_menu = main_menu_create(g);

  ret->display = display_create(g);
  
  /* Set callbacks */

  bg_gtk_slider_set_change_callback(ret->seek_slider,
                                    seek_change_callback, ret);
  bg_gtk_slider_set_release_callback(ret->seek_slider,
                                     seek_release_callback, ret);
  
  bg_gtk_button_set_callback(ret->play_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->stop_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->pause_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->next_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->prev_button, gmerlin_button_callback, ret);
  bg_gtk_button_set_callback(ret->close_button, gmerlin_button_callback, ret);

  bg_gtk_button_set_menu(ret->menu_button,
                         main_menu_get_widget(ret->main_menu));
  
  
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
                 display_get_widget(ret->display),
                 0, 0);
  
  gtk_widget_show(ret->layout);
  gtk_container_add(GTK_CONTAINER(ret->window), ret->layout);
    
  return ret;
  }

void player_window_show(player_window_t * win)
  {
  gtk_widget_show(win->window);
  }

void player_window_destroy(player_window_t * win)
  {
  
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
    else if(!strcmp(child->name, "BACKGROUND"))
      {
      tmp_string = xmlNodeListGetString(doc, child->children, 1);
      s->background = bg_strdup(s->background, tmp_string);
      xmlFree(tmp_string);
      }
    else if(!strcmp(child->name, "DISPLAY"))
      display_skin_load(&(s->display), doc, child);
    else if(!strcmp(child->name, "PLAYBUTTON"))
      bg_gtk_button_skin_load(&(s->play_button), doc, child);
    else if(!strcmp(child->name, "PAUSEBUTTON"))
      bg_gtk_button_skin_load(&(s->pause_button), doc, child);
    else if(!strcmp(child->name, "NEXTBUTTON"))
      bg_gtk_button_skin_load(&(s->next_button), doc, child);
    else if(!strcmp(child->name, "PREVBUTTON"))
      bg_gtk_button_skin_load(&(s->prev_button), doc, child);
    else if(!strcmp(child->name, "STOPBUTTON"))
      bg_gtk_button_skin_load(&(s->stop_button), doc, child);
    else if(!strcmp(child->name, "MENUBUTTON"))
      bg_gtk_button_skin_load(&(s->menu_button), doc, child);
    else if(!strcmp(child->name, "CLOSEBUTTON"))
      bg_gtk_button_skin_load(&(s->close_button), doc, child);
    else if(!strcmp(child->name, "SEEKSLIDER"))
      bg_gtk_slider_skin_load(&(s->seek_slider), doc, child);
    else if(!strcmp(child->name, "DISPLAY"))
      display_skin_load(&(s->display), doc, child);
    child = child->next;
    }
  }

void player_window_skin_destroy(player_window_skin_t * s)
  {
  if(s->background)
    free(s->background);
  bg_gtk_button_skin_destroy(&(s->play_button));
  bg_gtk_button_skin_destroy(&(s->stop_button));
  bg_gtk_button_skin_destroy(&(s->pause_button));
  bg_gtk_button_skin_destroy(&(s->next_button));
  bg_gtk_button_skin_destroy(&(s->prev_button));
  bg_gtk_button_skin_destroy(&(s->close_button));
  bg_gtk_button_skin_destroy(&(s->menu_button));
  
  }
