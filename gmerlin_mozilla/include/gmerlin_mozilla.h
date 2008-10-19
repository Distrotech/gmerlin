#include <npupp.h>
#include <npapi.h>
#include <config.h>

#include <gmerlin/player.h>

#include <gtk/gtk.h>

typedef struct bg_mozilla_s        bg_mozilla_t;
typedef struct bg_mozilla_widget_s bg_mozilla_widget_t;
typedef struct plugin_window_s plugin_window_t;

bg_mozilla_widget_t * bg_mozilla_widget_create(bg_mozilla_t * m);
void bg_mozilla_widget_set_window(bg_mozilla_widget_t * m,
                                  GdkNativeWindow window_id);

void bg_mozilla_widget_init_menu(bg_mozilla_widget_t * m);

void bg_mozilla_widget_destroy(bg_mozilla_widget_t *);

struct bg_mozilla_s
  {
  bg_player_t * player;
  bg_cfg_registry_t * cfg_reg;
  bg_plugin_registry_t * plugin_reg;

  char * orig_url;
  
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
void gmerlin_mozilla_set_url(bg_mozilla_t * m, const char * url);



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
  
  guint resize_id;
  guint idle_id;
  guint popup_time;
  
  int idle_counter;
  int toolbar_pos;
  int toolbar_height;
  
  int toolbar_state;

  };

