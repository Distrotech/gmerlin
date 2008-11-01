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
#include <gmerlin/gui_gtk/infowindow.h>
#include <gmerlin/cfg_dialog.h>

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
#define MODE_VLC       4

#define URL_MODE_STREAM   0
#define URL_MODE_LOCAL    1
#define URL_MODE_REDIRECT 2

typedef struct
  {
  int mode;
  char * src;
  char * target;
  char * type;
  } bg_mozilla_embed_info_t;

void bg_mozilla_embed_info_set_parameter(bg_mozilla_embed_info_t *,
                                         const char * name,
                                         const char * value);

int bg_mozilla_embed_info_check(bg_mozilla_embed_info_t *);
void bg_mozilla_embed_info_free(bg_mozilla_embed_info_t *);

struct bg_mozilla_s
  {
  int state;

  int player_state;
  
  bg_player_t * player;
  bg_cfg_registry_t * cfg_reg;
  bg_plugin_registry_t * plugin_reg;

  char * uri; /* Directly from browser */
  
  char * url;
  char * mimetype;
  int64_t total_bytes;
  
  char * current_url;
  
  int url_mode;
  
  bg_mozilla_embed_info_t ei;

#if 0  
  struct
    {
    char * name;
    char * url;
    } * url_list;
  int num_urls;
  int current_url;
#endif
  
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
  bg_cfg_section_t * infowindow_section;
  
  /* Config dialog */
  bg_dialog_t * cfg_dialog;
  bg_gtk_info_window_t * info_window;

  void (*reload_url)(struct bg_mozilla_s*);

  /* Filled by browser interface (plugin.c) */
  void * instance;
  void * scriptable;
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

int gmerlin_mozilla_set_stream(bg_mozilla_t * m,
                               const char * url, const char * mimetype, int64_t total_bytes);
void gmerlin_mozilla_start(bg_mozilla_t * m);

void gmerlin_mozilla_create_dialog(bg_mozilla_t * g);

void gmerlin_mozilla_create_scriptable(bg_mozilla_t * g);
void gmerlin_mozilla_init_scriptable();

/* GUI */

typedef struct
  {
  GtkWidget * menu;
  
  struct
    {
    GtkWidget * copy;
    GtkWidget * launch;
    GtkWidget * info;
    GtkWidget * menu;
    } url_menu;
  
  GtkWidget * url_item;
  
  struct
    {
    GtkWidget * preferences;
    GtkWidget * plugins;
    GtkWidget * menu;
    } config_menu;

  GtkWidget * fullscreen;
  GtkWidget * windowed;
  
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

typedef struct
  {
  GtkWidget * window;
  GtkWidget * box;
  GtkWidget * socket;
  int width;
  int height;
  guint resize_id;
  } bg_mozilla_window_t;

struct bg_mozilla_widget_s
  {
  main_menu_t menu;
  
  /* Top parent */
  bg_mozilla_window_t normal_win;
  bg_mozilla_window_t fullscreen_win;
  bg_mozilla_window_t * current_win;
  
  GtkWidget * controls;
  
  bg_mozilla_t * m;
  
  int width, height;

  float fg_normal[3];
  float fg_error[3];
  float bg[3];
  
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
  int autohide_toolbar;
  int can_pause;
  };

bg_mozilla_widget_t * bg_mozilla_widget_create(bg_mozilla_t * m);
void bg_mozilla_widget_set_window(bg_mozilla_widget_t * m,
                                  GdkNativeWindow window_id);

void bg_mozilla_widget_toggle_fullscreen(bg_mozilla_widget_t * m);

const bg_parameter_info_t * bg_mozilla_widget_get_parameters(bg_mozilla_widget_t * m);

void bg_mozilla_widget_set_parameter(void * priv, const char * name,
                                     const bg_parameter_value_t * v);

void bg_mozilla_widget_init_menu(bg_mozilla_widget_t * m);

void bg_mozilla_widget_destroy(bg_mozilla_widget_t *);

/* Buffer (passes data from the Browser to the player */

bg_mozilla_buffer_t * bg_mozilla_buffer_create( );
void bg_mozilla_buffer_destroy(bg_mozilla_buffer_t *);

/* A final call with 0 len signals EOF */
int bg_mozilla_buffer_write(bg_mozilla_buffer_t *,
                            void * data, int len);

int bg_mozilla_buffer_read(void *, uint8_t * data, int len);

/* Browser functions wrapped by struct */

NPError bg_NPN_GetURL(NPP instance, 
                      const char* url,
                      const char* target);

NPError bg_NPN_DestroyStream(NPP     instance, 
                          NPStream* stream, 
                             NPError   reason);

void *bg_NPN_MemAlloc (uint32 size);

void bg_NPN_MemFree(void* ptr);

NPError bg_NPN_GetValue(NPP instance, NPNVariable variable, void * value);

NPError bg_NPN_SetValue(NPP instance, NPPVariable variable, void * value);

void bg_NPN_InvalidateRect(NPP instance, NPRect * invalidRect);

void bg_NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion);

void bg_NPN_ForceRedraw(NPP instance);

void bg_NPN_ReleaseVariantValue(NPVariant * variant);

NPIdentifier bg_NPN_GetStringIdentifier(const NPUTF8 * name);

void bg_NPN_GetStringIdentifiers(const NPUTF8 ** names,
                              int32_t nameCount,
                                 NPIdentifier * identifiers);

NPIdentifier bg_NPN_GetIntIdentifier(int32_t intid);

bool bg_NPN_IdentifierIsString(NPIdentifier * identifier);

NPUTF8 * bg_NPN_UTF8FromIdentifier(NPIdentifier identifier);

int32_t bg_NPN_IntFromIdentifier(NPIdentifier identifier);

NPObject * bg_NPN_CreateObject(NPP npp, NPClass * aClass);

NPObject * bg_NPN_RetainObject(NPObject * npobj);

void bg_NPN_ReleaseObject(NPObject * npobj);

bool bg_NPN_Invoke(NPP npp, NPObject * npobj, NPIdentifier methodName,
                   const NPVariant * args, uint32_t argCount, NPVariant * result);

bool bg_NPN_InvokeDefault(NPP npp, NPObject * npobj, const NPVariant * args,
                          uint32_t argCount, NPVariant * result);

bool bg_NPN_Evaluate(NPP npp, NPObject * npobj, NPString * script,
                     NPVariant * result);

bool bg_NPN_GetProperty(NPP npp, NPObject * npobj, NPIdentifier propertyName,
                        NPVariant * result);

bool bg_NPN_SetProperty(NPP npp, NPObject * npobj, NPIdentifier propertyName,
                        const NPVariant * result);

bool bg_NPN_RemoveProperty(NPP npp, NPObject * npobj, NPIdentifier propertyName);

bool bg_NPN_HasProperty(NPP npp, NPObject * npobj, NPIdentifier propertyName);

bool bg_NPN_HasMethod(NPP npp, NPObject * npobj, NPIdentifier methodName);

void bg_mozilla_init_browser_funcs(NPNetscapeFuncs * funcs);

/* Commands (from the script or the GUI) */

void bg_mozilla_play(bg_mozilla_t * m);
void bg_mozilla_stop(bg_mozilla_t * m);
void bg_mozilla_pause(bg_mozilla_t * m);
