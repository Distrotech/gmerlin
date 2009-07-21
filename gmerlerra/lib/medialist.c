#include <stdlib.h>
#include <string.h>
#include <medialist.h>

#include <gmerlin/utils.h>

bg_nle_media_list_t *
bg_nle_media_list_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_nle_media_list_t * ret;

  ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;
  return ret;
  }

void bg_nle_media_list_insert(bg_nle_media_list_t * list,
                              bg_nle_file_t * file, int index)
  {
  if(list->num_files+1 > list->files_alloc)
    {
    list->files_alloc += 16;
    list->files = realloc(list->files, list->files_alloc * sizeof(*list->files));
    }
  if(index < list->num_files)
    {
    memmove(list->files + list->num_files + 1,
            list->files + list->num_files,
            (list->num_files - index) * sizeof(*list->files));
    }
  list->files[index] = file;
  list->num_files++;
  }

void bg_nle_media_list_delete(bg_nle_media_list_t * list,
                              int index)
  {
  if(index < list->num_files - 1)
    {
    memmove(list->files + index,
            list->files + index+1,
            (list->num_files - 1 - index) * sizeof(*list->files));
    
    }
  list->num_files--;
  }

bg_nle_file_t *
bg_nle_media_list_load_file(bg_nle_media_list_t * list,
                            const char * file,
                            const char * plugin)
  {
  bg_plugin_handle_t * handle = NULL;
  bg_input_plugin_t  * input;
  int num_tracks, track;
  const bg_plugin_info_t * info;
  bg_nle_file_t * ret;
  bg_track_info_t * ti;
  
  if(plugin)
    info = bg_plugin_find_by_name(list->plugin_reg, file);
  else
    info = NULL;

  if(!bg_input_plugin_load(list->plugin_reg,
                           file, info,
                           &handle,
                           (bg_input_callbacks_t*)0,
                           0 /* int prefer_edl */ ))
    return NULL;

  input = (bg_input_plugin_t*)handle->plugin;

  if(input->get_num_tracks)
    num_tracks = input->get_num_tracks(handle->priv);
  else
    num_tracks = 1;

  track = 0;
  if(num_tracks > 1)
    {
    /* TODO: Ask which track to load */
    }

  
  ti = input->get_track_info(handle->priv, track);
  
  ret = calloc(1, sizeof(*ret));
  
  ret->filename = bg_strdup(ret->filename, file);
  ret->num_audio_streams = ti->num_audio_streams;
  ret->num_video_streams = ti->num_video_streams;
  ret->track = track;
  ret->duration = ti->duration;

  ret->plugin = bg_strdup(ret->plugin, handle->info->name);

  /* Section */
  ret->section = 
    bg_plugin_registry_get_section(list->plugin_reg,
                                   ret->plugin);
  
  if(ti->name)  
    ret->name = bg_strdup(ret->name, ti->name);
  else
    ret->name = bg_get_track_name_default(ret->filename, ret->track, num_tracks);
  
  bg_plugin_unref(handle);
  return ret;
  }

bg_nle_file_t *
bg_nle_media_list_find_file(bg_nle_media_list_t * list,
                            const char * filename, int track)
  {
  return NULL;
  }

void bg_nle_media_list_destroy(bg_nle_media_list_t * list)
  {
  free(list);
  }

void bg_nle_file_destroy(bg_nle_file_t * file)
  {
  if(file->name) free(file->name);
  if(file->filename) free(file->filename);
  if(file->section) bg_cfg_section_destroy(file->section);
  free(file);
  }
