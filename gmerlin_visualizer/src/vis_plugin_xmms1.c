#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "vis_plugin.h"
#include <xmms/plugin.h>
#include <gmerlin/utils.h>

static VisPlugin * load_vis_plugin(char * filename)
  {
  void * plugin_handle;
  void *(*gpi) (void);

  VisPlugin * ret;
  
  plugin_handle = dlopen(filename, RTLD_NOW);
  if(!plugin_handle)
    {
    fprintf(stderr, "%s\n", dlerror());
    return (VisPlugin*)0;
    }
  gpi = dlsym(plugin_handle, "get_vplugin_info");

  if(!gpi)
    {
    dlclose(plugin_handle);
    return (VisPlugin*)0;
    }
  ret = (VisPlugin*)gpi(); 

  ret->handle = plugin_handle;
  ret->filename = bg_strdup(ret->filename, filename);
  
  return ret;
  }

#if 0
void load_plugin(xesd_plugin_info * info)
  {
  if(info->plugin)
    return;
  info->plugin = load_vis_plugin(info->filename);
  }

void unload_plugin(xesd_plugin_info * info)
  {
  g_free(info->plugin->filename);
  dlclose(info->plugin->handle);
  info->plugin = NULL;
  }
#endif

static void configure_xmms1(struct vis_plugin_handle_s* handle)
  {
  VisPlugin * plugin = (VisPlugin*)(handle->priv);
  if(plugin->configure)
    plugin->configure();
  }

static void about_xmms1(struct vis_plugin_handle_s*handle)
  {
  VisPlugin * plugin = (VisPlugin*)(handle->priv);
  if(plugin->about)
    plugin->about();
  }

static void start_xmms1(struct vis_plugin_handle_s*handle)
  {
  VisPlugin * plugin = (VisPlugin*)(handle->priv);
  if(plugin->init)
    plugin->init();

  if(plugin->playback_start)
    plugin->playback_start();
  }

static void stop_xmms1(struct vis_plugin_handle_s * handle)
  {
  VisPlugin * plugin = (VisPlugin*)(handle->priv);

  if(plugin->playback_stop)
    plugin->playback_stop();

  if(plugin->cleanup)
    plugin->cleanup();

  }

static void render_xmms1(struct vis_plugin_handle_s * handle,
                         vis_plugin_audio_t * audio_frame)
  {
  VisPlugin * plugin = (VisPlugin*)(handle->priv);

  plugin = (VisPlugin*)handle->priv;
  
  if(plugin->render_pcm)
    {
    if(plugin->num_pcm_chs_wanted == 1)
      plugin->render_pcm(audio_frame->pcm_data_mono);
    else if(plugin->num_pcm_chs_wanted == 2)
      plugin->render_pcm(audio_frame->pcm_data);
    }
  if(plugin->render_freq)
    {
    if(plugin->num_freq_chs_wanted == 1)
      plugin->render_freq(audio_frame->freq_data_mono);
    else
      plugin->render_freq(audio_frame->freq_data);
    }
  }

void unload_xmms1(vis_plugin_handle_t * handle)
  {
  VisPlugin * plugin = (VisPlugin*)(handle->priv);
  free(plugin->filename);
  dlclose(plugin->handle);
  free(handle);
  }


vis_plugin_handle_t * vis_plugin_load_xmms1(const vis_plugin_info_t * info)
  {
  vis_plugin_handle_t * ret = calloc(1, sizeof(*ret));
  ret->info = info;
  ret->priv = load_vis_plugin(info->module_filename);

  ret->configure = configure_xmms1;
  ret->about     = about_xmms1;
  ret->start     = start_xmms1;
  ret->stop      = stop_xmms1;
  ret->render    = render_xmms1;
  ret->unload    = unload_xmms1;
  return ret;
  }

vis_plugin_info_t * vis_plugins_find_xmms1(vis_plugin_info_t * list,
                                           vis_plugin_info_t ** _infos_from_file)
  {
  struct stat st;
  char * filename;
  char * pos;
  VisPlugin * plugin;
  
  struct dirent * entry;

  vis_plugin_info_t * tmp_info;
  DIR * dir;
  vis_plugin_info_t * infos_from_file = *_infos_from_file;
  
  filename = malloc(PATH_MAX + 1);

  dir = opendir(XMMS_VISUALIZATION_PLUGIN_DIR);
  
  while(1)
    {
    if(!(entry = readdir(dir)))
      break;

    pos = strchr(entry->d_name, '.');
    if(!pos)
      continue;
    if(strcmp(pos, ".so"))
      continue;

    if(strncmp(entry->d_name, "lib", 3))
      continue;
    
    sprintf(filename, "%s/%s", XMMS_VISUALIZATION_PLUGIN_DIR, entry->d_name);

    if(stat(filename, &st))
      continue;
        
    /* Look for a plugin in the file */
    
    tmp_info = infos_from_file;

    while(tmp_info)
      {
      // fprintf(stderr, "%s %s\n", filename, tmp_info->module_filename);
      
      if((tmp_info->type == VIS_PLUGIN_TYPE_XMMS1) &&
         !strcmp(filename, tmp_info->module_filename))
        {
        if(st.st_mtime == tmp_info->module_time)
          {
          fprintf(stderr, "%s already in registry\n", tmp_info->long_name);
          infos_from_file = vis_plugin_list_remove(infos_from_file, tmp_info);
          break;
          }
        else
          {
          fprintf(stderr, "reloading %s\n", tmp_info->module_filename);
          infos_from_file = vis_plugin_list_remove(infos_from_file, tmp_info);
          vis_plugin_info_destroy(tmp_info);
          tmp_info = (vis_plugin_info_t*)0;
          break;
          }
        }
      tmp_info = tmp_info->next;
      }

    if(tmp_info)
      {
      list = vis_plugin_list_add(list, tmp_info);
      }
    else
      {
      plugin = load_vis_plugin(filename);
      if(!plugin)
        continue;
      else
        {
        tmp_info = calloc(1, sizeof(*tmp_info));
        tmp_info->module_filename = bg_strdup(tmp_info->module_filename, filename);
        tmp_info->long_name = bg_strdup(tmp_info->long_name, plugin->description);
        tmp_info->name = bg_strdup(tmp_info->name, plugin->description);
        tmp_info->module_time = st.st_mtime;
        
        if(plugin->configure)
          tmp_info->flags |= VIS_PLGUIN_HAS_CONFIGURE;
        if(plugin->about)
          tmp_info->flags |= VIS_PLGUIN_HAS_ABOUT;
        tmp_info->type = VIS_PLUGIN_TYPE_XMMS1;
        free(plugin->filename);
        dlclose(plugin->handle);
        
        list = vis_plugin_list_add(list, tmp_info);
        }
      }
    }
  

  free(filename);
  closedir(dir);

  *_infos_from_file = infos_from_file;

  return list;
  }
