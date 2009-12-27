#include <stdlib.h>
#include <string.h>

#include <config.h>

#include <medialist.h>

#include <gmerlin/utils.h>
#include <gmerlin/log.h>

#define LOG_DOMAIN "medialist"

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

char * bg_nle_media_list_get_frame_table_filename(bg_nle_file_t * file, int stream)
  {
  return bg_sprintf("%s/%s/frametable_%02d", file->cache_dir, file->filename_hash, stream);
  }

bg_nle_file_t *
bg_nle_media_list_load_file(bg_nle_media_list_t * list,
                            const char * file,
                            const char * plugin)
  {
  int i, keep_going;
  bg_plugin_handle_t * handle = NULL;
  bg_input_plugin_t  * input;
  int num_tracks, track;
  const bg_plugin_info_t * info;
  bg_nle_file_t * ret = NULL;
  bg_track_info_t * ti;
  char * tmp_string;
  
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

  if(input->set_track)
    {
    input->set_track(handle->priv, track);
    }
  
  /* Check out streams */
  
  
  
  ti = input->get_track_info(handle->priv, track);

  if(!input->get_frame_table && ti->num_video_streams)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Plugin %s doesn't export video frame tables, try i_avdec", handle->info->name);
    goto fail;
    }
  
  if(input->set_audio_stream)
    {
    for(i = 0; i < ti->num_audio_streams; i++)
      input->set_audio_stream(handle->priv, i, BG_STREAM_ACTION_DECODE);
    }
  
  if(input->set_video_stream)
    {
    for(i = 0; i < ti->num_video_streams; i++)
      input->set_video_stream(handle->priv, i, BG_STREAM_ACTION_DECODE);
    }

  if((input->start) && !input->start(handle->priv))
    goto fail;
    
  ret = calloc(1, sizeof(*ret));

  ret->cache_dir = bg_strdup(ret->cache_dir, list->cache_dir);
  ret->filename = bg_strdup(ret->filename, file);
  
  bg_get_filename_hash(ret->filename, ret->filename_hash);
  
  tmp_string = bg_sprintf("%s/%s", ret->cache_dir, ret->filename_hash);
  if(!bg_ensure_directory(tmp_string))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot create cache directory %s, check permissions",
           tmp_string);
    free(tmp_string);
    goto fail;
    }
  free(tmp_string);
  
  ret->num_audio_streams = ti->num_audio_streams;

  ret->audio_streams = calloc(ret->num_audio_streams,
                              sizeof(*ret->audio_streams));
  
  for(i = 0; i < ret->num_audio_streams; i++)
    {
    ret->audio_streams[i].timescale  = ti->audio_streams[i].format.samplerate;
    ret->audio_streams[i].start_time = ti->audio_streams[i].pts_offset;
    ret->audio_streams[i].duration   = ti->audio_streams[i].duration;
    }
  
  ret->num_video_streams = ti->num_video_streams;
  ret->video_streams = calloc(ret->num_video_streams,
                              sizeof(*ret->video_streams));
  
  for(i = 0; i < ret->num_video_streams; i++)
    {
    ret->video_streams[i].timescale  = ti->video_streams[i].format.timescale;
    
    ret->video_streams[i].frametable = input->get_frame_table(handle->priv, i);
    if(!ret->video_streams[i].frametable)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Video stream %d has no frame table. Try i_avdec in sample accurate mode",
             i+1);
      goto fail;
      }
    
    tmp_string = bg_nle_media_list_get_frame_table_filename(ret, i);
    gavl_frame_table_save(ret->video_streams[i].frametable, tmp_string);
    free(tmp_string);
    }
  
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
  
  /* Get ID */

  do{
    ret->id++;
    keep_going = 0;
    for(i = 0; i < list->num_files; i++)
      {
      if(list->files[i]->id == ret->id)
        {
        keep_going = 1;
        break;
        }
      }
    }while(keep_going);

  return ret;

  fail:
  if(ret)
    bg_nle_file_destroy(ret);
  if(handle)
    bg_plugin_unref(handle);
  return NULL;
  }

bg_nle_file_t *
bg_nle_media_list_find_file(bg_nle_media_list_t * list,
                            const char * filename, int track)
  {
  return NULL;
  }

void bg_nle_file_destroy(bg_nle_file_t * file)
  {
  int i;
  if(file->name) free(file->name);
  if(file->filename) free(file->filename);
  if(file->section) bg_cfg_section_destroy(file->section);
  if(file->cache_dir) free(file->cache_dir);

  if(file->video_streams)
    {
    for(i = 0; i < file->num_video_streams; i++)
      {
      if(file->video_streams[i].frametable)
        gavl_frame_table_destroy(file->video_streams[i].frametable);
      }
    free(file->video_streams);
    }
  if(file->audio_streams)
    free(file->audio_streams);
  
  free(file);
  }

void bg_nle_media_list_destroy(bg_nle_media_list_t * list)
  {
  int i;
  for(i = 0; i < list->num_files; i++)
    bg_nle_file_destroy(list->files[i]);
  if(list->files)
    free(list->files);
  free(list);
  }


bg_plugin_handle_t * bg_nle_media_list_open_file(bg_nle_media_list_t * list,
                                                 bg_nle_file_t * file)
  {
  const bg_plugin_info_t * info;
  bg_plugin_handle_t * handle;
  bg_input_plugin_t * plugin;
  
  /* Get plugin */
  info = bg_plugin_find_by_name(list->plugin_reg, file->plugin);
  if(!info)
    {
    /* Plugin not found */
    return NULL;
    }

  /* Load plugin */
  handle = bg_plugin_load(list->plugin_reg, info);
  if(!handle)
    {
    /* Plugin could not be loaded */
    return NULL;
    }
  plugin = (bg_input_plugin_t *)handle->plugin;
  
  /* Set parameters */
  if(file->section)
    {
    bg_cfg_section_apply(file->section,
                         plugin->common.get_parameters(handle->priv),
                         plugin->common.set_parameter,
                         handle->priv);
    }

  /* Open */

  if(!plugin->open(handle->priv, file->filename))
    {
    /* Open failed */
    bg_plugin_unref(handle);
    return NULL;
    }
  
  return handle;
  }
