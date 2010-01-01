#include <filecache.h>

struct bg_nle_file_cache_s
  {
  bg_nle_file_handle_t * cache;
  int cache_size;
  int cache_size_max;
  bg_nle_project_t * p;
  gavl_timer_t * timer;
  };

bg_nle_file_cache_t * bg_nle_file_cache_create(bg_nle_project_t * p)
  {
  bg_nle_file_cache_t * ret = calloc(1, sizeof(*ret));
  ret->timer = gavl_timer_create();
  gavl_timer_start(ret->timer);
  return ret;
  }

static bg_nle_file_handle_t *
remove_from_list(bg_nle_file_handle_t * list,
                 bg_nle_file_handle_t * e)
  {
  bg_nle_file_handle_t * ptr;
  if(e == list)
    return list->next;
  ptr = list;
  while((ptr->next != e) && ptr->next)
    ptr = ptr->next;
  ptr->next = e->next;
  return list;
  }

static void set_stream(bg_nle_file_handle_t * h, bg_nle_track_type_t type, int stream)
  {
  switch(type)
    {
    case BG_NLE_TRACK_NONE:
      break;
    case BG_NLE_TRACK_AUDIO:
      h->plugin->set_audio_stream(h->h->priv, stream, BG_STREAM_ACTION_DECODE);
      break;
    case BG_NLE_TRACK_VIDEO:
      h->plugin->set_video_stream(h->h->priv, stream, BG_STREAM_ACTION_DECODE);
      break;
    }
  h->type = type;
  h->stream = stream;
  if(h->plugin->start)
    h->plugin->start(h->h->priv);
  }

bg_nle_file_handle_t * bg_nle_file_cache_load(bg_nle_file_cache_t * c,
                                              bg_nle_id_t id,
                                              bg_nle_track_type_t type,
                                              int stream)
  {
  const bg_plugin_info_t * info;
  bg_nle_file_handle_t * exact = NULL;
  bg_nle_file_handle_t * inexact = NULL;
  
  bg_nle_file_handle_t * h = c->cache;
  int i;
  
  /* 1. Try to find an open file */

  while(h)
    {
    if(h->file->id == id)
      {
      inexact = h;

      if((h->type == type) && (h->stream == stream))
        {
        exact = h;
        break;
        }
      }
    h = h->next;
    }

  if(exact)
    {
    c->cache = remove_from_list(c->cache, exact);
    c->cache_size--;
    return exact;
    }
  else if(inexact && inexact->plugin->set_track)
    {
    c->cache = remove_from_list(c->cache, inexact);
    c->cache_size--;
    inexact->plugin->set_track(inexact->h->priv, inexact->file->track);

    set_stream(inexact, type, stream);
    
    return inexact;
    }

  /* Find the file in the media list */
  
  h = calloc(1, sizeof(*h));

  for(i = 0; i < c->p->media_list->num_files; i++)
    {
    if(c->p->media_list->files[i]->id == id)
      h->file = c->p->media_list->files[i];
    }

  if(!h->file)
    {
    free(h);
    return NULL;
    }

  info = bg_plugin_find_by_name(c->p->plugin_reg, h->file->plugin);
  
  h->h = bg_plugin_load(c->p->plugin_reg, info);
  h->plugin = (bg_input_plugin_t*)h->h->plugin;
  
  if(h->file->section && h->h->info->parameters)
    {
    bg_cfg_section_apply(h->file->section, h->h->info->parameters,
                         h->plugin->common.set_parameter, h->h->priv);
    }

  if(!h->plugin->open(h->h->priv, h->file->filename))
    goto fail;

  if(h->plugin->set_track)
    h->plugin->set_track(h->h->priv, h->file->track);
  
  h->ti = h->plugin->get_track_info(h->h->priv, h->file->track);
  
  set_stream(h, type, stream);
  
  return h;
  
  fail:
  
  free(h);
  return NULL;
  
  }

static void destroy_handle(bg_nle_file_handle_t * h)
  {
  bg_plugin_unref(h->h);
  free(h);
  }

void bg_nle_file_cache_unload(bg_nle_file_cache_t * c, bg_nle_file_handle_t * h)
  {
  if(!c->cache_size_max)
    {
    destroy_handle(h);
    return;
    }
  else if(c->cache_size_max == 1)
    {
    if(c->cache)
      destroy_handle(c->cache);
    c->cache = h;
    h->next = NULL;
    c->cache_size = 1;
    }
  
  h->next = c->cache;
  c->cache = h;
  
  c->cache_size++;
  if(c->cache_size >= c->cache_size_max)
    {
    bg_nle_file_handle_t * tmp;
    
    tmp = c->cache;

    while(tmp->next->next)
      tmp = tmp->next;

    destroy_handle(tmp->next);
    tmp->next = NULL;
    }
  }


void bg_nle_file_cache_destroy(bg_nle_file_cache_t * c)
  {
  bg_nle_file_handle_t * tmp;
  if(c->cache)
    {
    while(c->cache)
      {
      tmp = c->cache->next;
      destroy_handle(c->cache);
      c->cache = tmp;
      }
    }
  free(c);
  }
