#include <npupp.h>
#include <npapi.h>
#include <config.h>

#include <pthread.h>

#include <gmerlin/player.h>

#include <gtk/gtk.h>
#include <gmerlin/gui_gtk/scrolltext.h>
#include <gmerlin/gui_gtk/slider.h>
#include <gmerlin/gui_gtk/button.h>
#include <gmerlin/gui_gtk/display.h>

typedef struct bg_mozilla_s        bg_mozilla_t;
typedef struct bg_mozilla_widget_s bg_mozilla_widget_t;
typedef struct plugin_window_s plugin_window_t;
typedef struct bg_mozilla_buffer_s  bg_mozilla_buffer_t;

#define BUFFER_SIZE (1024) /* Need to finetune */

#define STATE_IDLE     0
#define STATE_STARTING 1
#define STATE_PLAYING  2

#define MODE_GENERIC   0
#define MODE_REAL      1
#define MODE_QUICKTIME 2
#define MODE_WMP       3

typedef struct
  {
  int mode;
  char * src;
  } bg_mozilla_embed_info_t;

void bg_mozilla_embed_info_set_parameter(bg_mozilla_embed_info_t *,
                                         const char * name,
                                         const char * value);

int bg_mozilla_embed_info_check(bg_mozilla_embed_info_t *);
void bg_mozilla_embed_info_free(bg_mozilla_embed_info_t *);

struct bg_mozilla_s
  {
  int state;

  bg_player_t * player;
  bg_cfg_registry_t * cfg_reg;
  bg_plugin_registry_t * plugin_reg;

  char * orig_url;
  char * new_url;
  char * new_mimetype;
  
  int is_local;

  bg_mozilla_embed_info_t ei;
  
  struct
    {
    char * name;
    char * url;
    } * url_list;
  int num_urls;
  int current_url;

  bg_msg_queue_t * msg_queue;
  
  /* Gtk Stuff */
  bg_mozilla_widget_t * widget;
  
  GdkNativeWindow window;
  
  plugin_window_t * plugin_window;
  
  bg_plugin_handle_t * oa_handle;
  bg_plugin_handle_t * ov_handle;
  
  const bg_plugin_info_t * ov_info;
  
  char * display_string;
  
  bg_mozilla_buffer_t * buffer;
  
  int playing;
  
  pthread_t start_thread;
  
  int start_finished;
  pthread_mutex_t start_finished_mutex;

  /* Configuration sections */
  bg_cfg_section_t * gui_section;
  };

plugin_window_t * bg_mozilla_plugin_window_create(bg_mozilla_t * m);
void bg_mozilla_plugin_window_show(plugin_window_t *);
  
bg_mozilla_t * gmerlin_mozilla_create();
void gmerlin_mozilla_destroy(bg_mozilla_t*);

void gmerlin_mozilla_set_window(bg_mozilla_t*,
                                GdkNativeWindow window_id);

void gmerlin_mozilla_set_oa_plugin(bg_mozilla_t*,
                                   const bg_plugin_info_t * info);
void gmerlin_mozilla_set_ov_plugin(bg_mozilla_t*,
                                   const bg_plugin_info_t * info);
void gmerlin_mozilla_set_vis_plugin(bg_mozilla_t*,
                                   const bg_plugin_info_t * info);

void gmerlin_mozilla_set_stream(bg_mozilla_t * m,
                                const char * url, const char * mimetype);
void gmerlin_mozilla_start(bg_mozilla_t * m);


/* GUI */

typedef struct
  {
  GtkWidget * menu;
  
  struct
    {
    GtkWidget * copy;
    GtkWidget * launch;
    GtkWidget * menu;
    } url_menu;
  
  GtkWidget * url_item;
  
  struct
    {
    GtkWidget * preferences;
    GtkWidget * plugins;
    GtkWidget * menu;
    } config_menu;
  
  GtkWidget * config_item;
  
  } main_menu_t;

typedef struct
  {
  bg_gtk_button_skin_t play_button;
  bg_gtk_button_skin_t pause_button;
  bg_gtk_button_skin_t stop_button;
  
  bg_gtk_slider_skin_t seek_slider;
  // bg_gtk_slider_skin_t volume_slider;

  char * directory;
  } bg_mozilla_widget_skin_t;

char * bg_mozilla_widget_skin_load(bg_mozilla_widget_skin_t * s,
                                   char * directory);

void bg_mozilla_widget_skin_destroy(bg_mozilla_widget_skin_t *);


struct bg_mozilla_widget_s
  {
  GtkWidget * plug;
  GtkWidget * box;
  main_menu_t menu;
  
  /* Top parent */
  GtkWidget * socket;
  GtkWidget * button;
  GtkWidget * controls;
  
  bg_mozilla_t * m;
  
  int width, height;

  float fg_normal[3];
  float bg[3];
  
  guint resize_id;
  guint idle_id;
  guint popup_time;
  
  int idle_counter;
  int toolbar_pos;
  int toolbar_height;
  
  int toolbar_state;
  
  bg_gtk_scrolltext_t * scrolltext;
  bg_gtk_slider_t     * seek_slider;
  gavl_time_t duration;

  bg_gtk_button_t     * pause_button;
  bg_gtk_button_t     * stop_button;
  bg_gtk_button_t     * play_button;

  
  bg_mozilla_widget_skin_t skin;
  char * skin_directory;
  int seek_active;
  };

bg_mozilla_widget_t * bg_mozilla_widget_create(bg_mozilla_t * m);
void bg_mozilla_widget_set_window(bg_mozilla_widget_t * m,
                                  GdkNativeWindow window_id);

void bg_mozilla_widget_set_parameter(void * m, const char * name,
                                     const bg_parameter_value_t * val);

bg_parameter_info_t * bg_mozilla_widget_get_parameters(void * m);

void bg_mozilla_widget_init_menu(bg_mozilla_widget_t * m);

void bg_mozilla_widget_destroy(bg_mozilla_widget_t *);

/* Buffer (passes data from the Browser to the player */

bg_mozilla_buffer_t * bg_mozilla_buffer_create( );
void bg_mozilla_buffer_destroy(bg_mozilla_buffer_t *);

/* A final call with 0 len signals EOF */
int bg_mozilla_buffer_write(bg_mozilla_buffer_t *,
                            void * data, int len);

int bg_mozilla_buffer_read(void *, uint8_t * data, int len);

