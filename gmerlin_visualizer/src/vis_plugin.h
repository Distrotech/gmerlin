#ifndef __VIS_PLUGIN_H_
#define __VIS_PLUGIN_H_

#include <inttypes.h>
#include "config.h"

#ifdef HAVE_LIBVISUAL
#include <libvisual/libvisual.h>
#endif


typedef enum
  {
    VIS_PLUGIN_TYPE_UNDEFINED = 0,
#ifdef HAVE_XMMS1
    VIS_PLUGIN_TYPE_XMMS1,
#endif
#ifdef HAVE_LIBVISUAL
    VIS_PLUGIN_TYPE_LIBVISUAL,
#endif
  } vis_plugin_type_t;

typedef struct vis_plugin_audio_s
  {
  /* For xmms */
  
  int16_t pcm_data[2][512];
  int16_t pcm_data_mono[2][512];
  
  int16_t freq_data[2][256];
  int16_t freq_data_mono[2][256];

  /* For libvisual */

#ifdef HAVE_LIBVISUAL
  VisAudio * vis_audio;
#endif
    
  } vis_plugin_audio_t;

#define VIS_PLGUIN_HAS_ABOUT     (1<<0)
#define VIS_PLGUIN_HAS_CONFIGURE (1<<1)

typedef struct vis_plugin_info_s
  {
  int flags; /* See defines above */
  
  char * name; /* For interal use only */
  char * long_name;
  
  char * module_filename;
  long module_time;
  
  int enabled;

  vis_plugin_type_t type;
  struct vis_plugin_info_s * next;
  } vis_plugin_info_t;

typedef struct vis_plugin_handle_s
  {
  const vis_plugin_info_t * info;
  void * priv;

  struct vis_plugin_handle_s * next;
  /* Callback functions */

  void (*configure)(struct vis_plugin_handle_s*);
  void (*about)(struct vis_plugin_handle_s*);

  void (*start)(struct vis_plugin_handle_s*);
  void (*stop)(struct vis_plugin_handle_s*);
  void (*render)(struct vis_plugin_handle_s*, vis_plugin_audio_t*);

  void (*unload)(struct vis_plugin_handle_s*);
  
  } vis_plugin_handle_t;

/* Load / Unload plugin */

vis_plugin_handle_t * vis_plugin_load(const vis_plugin_info_t*);

/* Load a plugin registry */

vis_plugin_info_t *
vis_plugin_list_add(vis_plugin_info_t * l, vis_plugin_info_t * info);

vis_plugin_info_t *
vis_plugin_list_remove(vis_plugin_info_t * l, vis_plugin_info_t * info);

void vis_plugin_info_destroy(vis_plugin_info_t * info);

/* Destroy it */

void vis_plugins_destroy(vis_plugin_info_t *);

vis_plugin_info_t * vis_plugins_find();

/* Load / Save */

vis_plugin_info_t * vis_plugins_load();
void vis_plugins_save(vis_plugin_info_t *);

/* API specific stuff */

#ifdef HAVE_XMMS1

vis_plugin_handle_t * vis_plugin_load_xmms1(const vis_plugin_info_t *);
vis_plugin_info_t * vis_plugins_find_xmms1(vis_plugin_info_t * list,
                                           vis_plugin_info_t ** infos_from_file);

#endif // HAVE_XMMS1

#ifdef HAVE_LIBVISUAL

vis_plugin_handle_t * vis_plugin_load_lv(const vis_plugin_info_t *);
vis_plugin_info_t * vis_plugins_find_lv(vis_plugin_info_t * list,
                                        vis_plugin_info_t ** infos_from_file);

#endif // HAVE_XMMS1


#endif // __VIS_PLUGIN_H_
