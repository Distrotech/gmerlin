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

#include <gui_gtk/button.h>
#include <gui_gtk/slider.h>
#include <gmerlin/msgqueue.h>

typedef struct
  {
  char * background;
  char * background_highlight;
 
  bg_gtk_button_skin_t play_button;
  bg_gtk_button_skin_t pause_button;
  bg_gtk_button_skin_t prev_button;
  bg_gtk_button_skin_t next_button;
  bg_gtk_button_skin_t stop_button;
  bg_gtk_button_skin_t menu_button;
  bg_gtk_button_skin_t close_button;

  bg_gtk_slider_skin_t seek_slider;
  bg_gtk_slider_skin_t volume_slider;

  display_skin_t display;
  
  } player_window_skin_t;

void player_window_skin_load(player_window_skin_t *,
                            xmlDocPtr doc, xmlNodePtr node);

void player_window_skin_destroy(player_window_skin_t *);

/* Player window */

typedef struct main_menu_s main_menu_t;

main_menu_t * main_menu_create(gmerlin_t * gmerlin);

/* Set the plugins and send everything to the player */
void main_menu_finalize(main_menu_t * ret, gmerlin_t * gmerlin);


void main_menu_destroy(main_menu_t *);

GtkWidget * main_menu_get_widget(main_menu_t *);

void main_menu_set_num_chapters(main_menu_t * m,
                                int num, int scale);

void main_menu_set_num_streams(main_menu_t *,
                               int audio_streams,
                               int video_streams,
                               int subtitle_streams);

void main_menu_set_chapter_info(main_menu_t * m, int chapter,
                                const char * name,
                                gavl_time_t time);

void main_menu_chapter_changed(main_menu_t * m, int chapter);

void main_menu_set_audio_info(main_menu_t *, int stream,
                              const char * info,
                              const char * language);

void main_menu_set_video_info(main_menu_t *, int stream,
                              const char * info,
                              const char * language);

void main_menu_set_subtitle_info(main_menu_t *, int stream,
                                 const char * info,
                                 const char * language);

void
main_menu_update_streams(main_menu_t *,
                         int num_audio_streams,
                         int num_video_streams,
                         int num_subpicture_streams,
                         int num_programs);

void
main_menu_set_audio_index(main_menu_t *, int);

void
main_menu_set_video_index(main_menu_t *, int);

void
main_menu_set_subtitle_index(main_menu_t *, int);


void main_menu_set_tree_window_item(main_menu_t * m, int state);
void main_menu_set_info_window_item(main_menu_t * m, int state);
void main_menu_set_log_window_item(main_menu_t * m, int state);
void main_menu_set_plugin_window_item(main_menu_t * m, int state);

typedef struct player_window_s
  {
  bg_msg_queue_t       * cmd_queue;
  bg_msg_queue_t       * msg_queue;
  
  gmerlin_t * gmerlin;
  
  /* Window stuff */
  
  GtkWidget * window;
  GtkWidget * layout;

  /* For moving the window */
  
  int mouse_x;
  int mouse_y;

  int window_x;
  int window_y;
    
  /* Background */
  
  GdkPixbuf * background_pixbuf;
  GdkPixbuf * background_pixbuf_highlight;
  int mouse_inside;
  
  /* GUI Elements */

  bg_gtk_button_t * play_button;
  bg_gtk_button_t * stop_button;
  bg_gtk_button_t * pause_button;
  bg_gtk_button_t * next_button;
  bg_gtk_button_t * prev_button;
  bg_gtk_button_t * close_button;
  bg_gtk_button_t * menu_button;

  bg_gtk_slider_t * seek_slider;
  bg_gtk_slider_t * volume_slider;
  
  main_menu_t * main_menu;

  display_t * display;
  gavl_time_t duration;

  int seek_active;
  
  float volume;

  int msg_queue_locked;

  /* For the player window only (NOT for album windows) */
  GtkAccelGroup *accel_group;

  /* For avoiding infinite recursions */
  guint enter_notify_id;
  guint leave_notify_id;
  
  } player_window_t;

void player_window_create(gmerlin_t*);

void player_window_push_accel(player_window_t * w, int accel);


void player_window_show(player_window_t * win);

void player_window_set_skin(player_window_t * win,
                            player_window_skin_t*,
                            const char * directory);

void player_window_destroy(player_window_t * win);

