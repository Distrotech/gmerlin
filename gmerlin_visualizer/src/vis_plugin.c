#include <stdlib.h>

#include <gmerlin/utils.h>
#include "vis_plugin.h"



vis_plugin_info_t * vis_plugin_list_add(vis_plugin_info_t * l, vis_plugin_info_t * info)
  {
  vis_plugin_info_t * tmp_info;
  info->next = (vis_plugin_info_t*)0;
  if(!l)
    return info;

  tmp_info = l;
  while(tmp_info->next)
    tmp_info = tmp_info->next;
  tmp_info->next = info;
  return l;
  }

vis_plugin_info_t * vis_plugin_list_remove(vis_plugin_info_t * l, vis_plugin_info_t * info)
  {
  vis_plugin_info_t * tmp_info;
  
  if(info == l)
    {
    l = l->next;
    return l;
    }
  
  tmp_info = l;
  while(tmp_info->next != info)
    tmp_info = tmp_info->next;

  tmp_info->next = info->next;
  return l;
  }

vis_plugin_info_t * vis_plugins_find()
  {
  vis_plugin_info_t * ret = (vis_plugin_info_t*)0;
  char * tmp_string;
  vis_plugin_info_t * info_from_file = (vis_plugin_info_t *)0;
  
  /* 1. Load file */
  tmp_string = bg_search_file_read("visualizer", "vis_plugins.xml");
  if(tmp_string)
    {
    info_from_file = vis_plugins_load(tmp_string);
    free(tmp_string);
    }
#ifdef HAVE_XMMS1

  ret = vis_plugins_find_xmms1(ret, &info_from_file);
  
#endif // HAVE_XMMS1
  
  /* Save the entries */
  vis_plugins_save(ret);
  return ret;
  }

void vis_plugin_info_destroy(vis_plugin_info_t * info)
  {
  if(info->name)
      free(info->name);
  if(info->module_filename)
    free(info->module_filename);
  free(info);
  }

void vis_plugin_infos_destroy(vis_plugin_info_t * info)
  {
  vis_plugin_info_t * tmp_info;

  while(info);
    {
    tmp_info = info->next;
    vis_plugin_info_destroy(info);
    info = tmp_info;
    }
  }

vis_plugin_handle_t * vis_plugin_load(const vis_plugin_info_t * info)
  {
  switch(info->type)
    {
    // #ifdef HAVE_XMMS1
    case VIS_PLUGIN_TYPE_XMMS1:
      return vis_plugin_load_xmms1(info);
      break;
      // #endif
    case VIS_PLUGIN_TYPE_UNDEFINED:
      break;
    }
  return (vis_plugin_handle_t *)0;
  }
