#include <gui_gtk/button.h>
#include <gui_gtk/slider.h>
#include <msgqueue.h>

typedef struct
  {
  char * background;
 
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

GtkWidget * main_menu_get_widget(main_menu_t *);

void main_menu_destroy(main_menu_t *);

void
main_menu_update_streams(main_menu_t *,
                         int num_audio_streams,
                         int num_video_streams,
                         int num_subpicture_streams,
                         int num_programs);

void main_menu_set_tree_window_item(main_menu_t * m, int state);
void main_menu_set_info_window_item(main_menu_t * m, int state);
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
  } player_window_t;

player_window_t * player_window_create(gmerlin_t*);

void player_window_show(player_window_t * win);

void player_window_set_skin(player_window_t * win,
                            player_window_skin_t*,
                            const char * directory);

void player_window_destroy(player_window_t * win);

