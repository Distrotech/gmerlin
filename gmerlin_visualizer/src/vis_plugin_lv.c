#include <stdlib.h>
#include "vis_plugin.h"
#include <gmerlin/utils.h>

#include <libvisual/libvisual.h>

/* Plugin handle */

typedef struct
  {
  VisActor *actor;

  
  } lv_priv_t;

static void configure_lv(struct vis_plugin_handle_s* handle)
  {

  }

static void about_lv(struct vis_plugin_handle_s*handle)
  {

  }
#if 0
static void start_lv(struct vis_plugin_handle_s*handle)
  {
  
  }

static void stop_lv(struct vis_plugin_handle_s * handle)
  {

  }
#endif

static void render_lv(struct vis_plugin_handle_s * handle,
                         vis_plugin_audio_t * audio_frame)
  {
  lv_priv_t * priv; 
  priv = (lv_priv_t *)priv;

  }

void unload_lv(vis_plugin_handle_t * handle)
  {
  lv_priv_t * priv; 
  priv = (lv_priv_t *)priv;
  }

vis_plugin_handle_t * vis_plugin_load_lv(const vis_plugin_info_t * info)
  {
  lv_priv_t * priv; 
  vis_plugin_handle_t * ret;

  ret = calloc(1, sizeof(*ret));
  priv = calloc(1, sizeof(*priv));

  /* Initialize libnvisual stuff */

  priv->actor = visual_actor_new(info->name);
  
  ret->info = info;
  ret->priv = priv;
  
  //  ret->priv = load_vis_plugin(info->module_filename);
  
  ret->configure = configure_lv;
  ret->about     = about_lv;
  //  ret->start     = start_lv;
  //  ret->stop      = stop_lv;
  ret->render    = render_lv;
  ret->unload    = unload_lv;
  return ret;

  }

vis_plugin_info_t * vis_plugins_find_lv(vis_plugin_info_t * list,
                                        vis_plugin_info_t ** _infos_from_file)
  {
  VisPluginRef * plugin_ref;
  VisList * actors;
  vis_plugin_info_t * tmp_info;

  vis_plugin_info_t * infos_from_file = *_infos_from_file;
  const char * name = (const char*)0;
  
  actors = visual_actor_get_list();
    
  name = visual_actor_get_next_by_name_nogl(name);

  while(name)
    {
    plugin_ref = visual_plugin_find(actors, name);
    fprintf(stderr, "Found actor %s (%s)\n", plugin_ref->info->name, name);

    tmp_info = infos_from_file;
    while(tmp_info)
      {
      
      if((tmp_info->type == VIS_PLUGIN_TYPE_LIBVISUAL) &&
         !strcmp(name, tmp_info->name))
        {
        infos_from_file = vis_plugin_list_remove(infos_from_file, tmp_info);
        break;
        }
      tmp_info = tmp_info->next;
      }
    if(tmp_info)
      list = vis_plugin_list_add(list, tmp_info);
    else
      {
      tmp_info = calloc(1, sizeof(*tmp_info));

      tmp_info->long_name = bg_strdup(tmp_info->long_name, plugin_ref->info->name);
      tmp_info->name = bg_strdup(tmp_info->name, name);

      tmp_info->type = VIS_PLUGIN_TYPE_LIBVISUAL;
      list = vis_plugin_list_add(list, tmp_info);
      }
    
    
    name = visual_actor_get_next_by_name_nogl(name);
    }

  *_infos_from_file = infos_from_file;
  return list;
  }
