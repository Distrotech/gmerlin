#include <string.h>

#include <gavl/gavl.h>

#include <gmerlin/cfg_registry.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>

#include <track.h>
#include <project.h>
#include <editops.h>

static void edit_callback_stub(bg_nle_project_t * p,
                               bg_nle_edit_op_t op,
                               void * op_data,
                               void * user_data)
  {
  
  }

const bg_parameter_info_t bg_nle_performance_parameters[] =
  {
    {
      .name        = "proxy_width_theshold",
      .long_name   = TRS("Width threshold"),
      .type        = BG_PARAMETER_INT,
      .val_min     = { .val_i = 1 },
      .val_max     = { .val_i = 1000000 },
      .val_default = { .val_i = 2048 },
      .help_string = TRS("Maximum image width to process without proxy editing"),
    },
    {
      .name        = "editing_quality",
      .long_name   = TRS("Editing quality"),
      .type        = BG_PARAMETER_SLIDER_INT,
      .val_min     = { .val_i = GAVL_QUALITY_FASTEST },
      .val_max     = { .val_i = GAVL_QUALITY_BEST    },
      .val_default = { .val_i = GAVL_QUALITY_FASTEST },
    },
    {
      .name        = "render_quality",
      .long_name   = TRS("Render quality"),
      .type        = BG_PARAMETER_SLIDER_INT,
      .val_min     = { .val_i = GAVL_QUALITY_FASTEST },
      .val_max     = { .val_i = GAVL_QUALITY_BEST    },
      .val_default = { .val_i = GAVL_QUALITY_BEST },
    },
    {
      .name        = "max_file_cache",
      .long_name   = TRS("File cache"),
      .type        = BG_PARAMETER_INT,
      .val_min     = { .val_i = 0 },
      .val_max     = { .val_i = 1000 },
      .val_default = { .val_i = 10 },
      .help_string = TRS("Number of media files to keep open during rendering"),
    },
    { /* */ },
  };

static const bg_parameter_info_t cache_parameters[] =
  {
    {
      .name        = "cache_directory",
      .long_name   = TRS("Cache directory"),
      .type        = BG_PARAMETER_DIRECTORY,
      .help_string = TRS("Directory for the cache"),
    },
    { /* */ }
  };

bg_parameter_info_t * bg_nle_project_create_cache_parameters()
  {
  bg_parameter_info_t * ret;
  ret = bg_parameter_info_copy_array(cache_parameters);
  ret[0].val_default.val_str =
    bg_sprintf("%s/.gmerlin/gmerlerra/cache", getenv("HOME"));
  return ret;
  }

void bg_nle_project_create_sections(bg_nle_project_t * ret)
  {
  ret->audio_track_section = bg_cfg_section_find_subsection(ret->section,
                                                            "audio_track");
  
  ret->video_track_section = bg_cfg_section_find_subsection(ret->section,
                                                            "video_track");
  ret->audio_outstream_section = bg_cfg_section_find_subsection(ret->section,
                                                            "audio_outstream");
  ret->video_outstream_section = bg_cfg_section_find_subsection(ret->section,
                                                            "video_outstream");
  ret->performance_section = bg_cfg_section_find_subsection(ret->section,
                                                            "performance");
  ret->cache_section = bg_cfg_section_find_subsection(ret->section,
                                                      "cache");
  }

bg_nle_project_t * bg_nle_project_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_nle_project_t * ret;
  bg_cfg_section_t * section;
  ret = calloc(1, sizeof(*ret));
  ret->visible.end = GAVL_TIME_SCALE * 10;
  ret->selection.end = -1;

  ret->in_out.start = -1;
  ret->in_out.end   = -1;

  ret->media_list = bg_nle_media_list_create(plugin_reg);
  ret->plugin_reg = plugin_reg;

  ret->section = bg_cfg_section_create(NULL);

  bg_nle_project_create_sections(ret);
  
  
  /* Audio track defaults */
  section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_track_audio_parameters);
  bg_cfg_section_transfer(section, ret->audio_track_section);
  bg_cfg_section_destroy(section);

  /* Video track defaults */
  section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_track_video_parameters);
  bg_cfg_section_transfer(section, ret->video_track_section);
  bg_cfg_section_destroy(section);

  /* Audio outstream defaults */
  section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_outstream_audio_parameters);
  bg_cfg_section_transfer(section, ret->audio_outstream_section);
  bg_cfg_section_destroy(section);

  /* Video outstream defaults */
  section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_outstream_video_parameters);
  bg_cfg_section_transfer(section, ret->video_outstream_section);
  bg_cfg_section_destroy(section);

  /* Performance */
  section =
    bg_cfg_section_create_from_parameters("",
                                          bg_nle_performance_parameters);
  bg_cfg_section_transfer(section, ret->performance_section);
  bg_cfg_section_destroy(section);

  /* Cache */

  ret->cache_parameters = bg_nle_project_create_cache_parameters();
  bg_ensure_directory(ret->cache_parameters[0].val_default.val_str);
  
  section =
    bg_cfg_section_create_from_parameters("",
                                          ret->cache_parameters);
  bg_cfg_section_transfer(section, ret->cache_section);
  bg_cfg_section_destroy(section);

  bg_cfg_section_apply(ret->cache_section,
                       ret->cache_parameters,
                       bg_nle_project_set_cache_parameter,
                       ret);
  
  /* */

  ret->edit_callback = edit_callback_stub;
  
  return ret;
  }

void bg_nle_project_set_cache_parameter(void * data, const char * name,
                                        const bg_parameter_value_t * val)
  {
  bg_nle_project_t * p = data;
  if(!name)
    return;
  if(!strcmp(name, "cache_directory"))
    {
    p->media_list->cache_dir =
      bg_strdup(p->media_list->cache_dir, val->val_str);
    }
  }


void bg_nle_project_destroy(bg_nle_project_t * p)
  {
  int i;

  /* Free tracks */
  for(i = 0; i < p->num_tracks; i++)
    bg_nle_track_destroy(p->tracks[i]);
  
  if(p->tracks)
    free(p->tracks);

  for(i = 0; i < p->num_outstreams; i++)
    bg_nle_outstream_destroy(p->outstreams[i]);
  
  if(p->outstreams)
    free(p->outstreams);

  if(p->cache_parameters)
    bg_parameter_info_destroy_array(p->cache_parameters);
    
  free(p);
  }

void bg_nle_project_set_edit_callback(bg_nle_project_t * p,
                                      bg_nle_edit_callback callback,
                                      void * callback_data)
  {
  p->edit_callback = callback;
  p->edit_callback_data = callback_data;
  }

void bg_nle_project_set_pre_edit_callback(bg_nle_project_t * p,
                                          bg_nle_pre_edit_callback callback,
                                          void * callback_data)
  {
  p->pre_edit_callback = callback;
  p->pre_edit_callback_data = callback_data;
  }

void bg_nle_project_append_track(bg_nle_project_t * p,
                                     bg_nle_track_t * t)
  {
  bg_nle_project_insert_track(p, t, p->num_tracks);
  }

void bg_nle_project_insert_track(bg_nle_project_t * p,
                                     bg_nle_track_t * t, int pos)
  {
  if(p->num_tracks+1 > p->tracks_alloc)
    {
    p->tracks_alloc += 16;
    p->tracks = realloc(p->tracks,
                        sizeof(*p->tracks) *
                        (p->tracks_alloc));
    }

  if(pos < p->num_tracks)
    {
    memmove(p->tracks + pos + 1, p->tracks + pos,
            (p->num_tracks - pos)*sizeof(*p->tracks));
    }
  
  p->tracks[pos] = t;
  p->num_tracks++;

  t->p = p;
  
  }

void bg_nle_project_append_outstream(bg_nle_project_t * p,
                                     bg_nle_outstream_t * t)
  {
  bg_nle_project_insert_outstream(p, t, p->num_outstreams);
  }

void bg_nle_project_insert_outstream(bg_nle_project_t * p,
                                     bg_nle_outstream_t * t, int pos)
  {
  if(t->flags & BG_NLE_TRACK_PLAYBACK)
    {
    switch(t->type)
      {
      case BG_NLE_TRACK_AUDIO:
        p->current_audio_outstream = t;
        break;
      case BG_NLE_TRACK_VIDEO:
        p->current_video_outstream = t;
        break;
      case BG_NLE_TRACK_NONE:
        break;
      }
    }
  
  if(p->num_outstreams+1 > p->outstreams_alloc)
    {
    p->outstreams_alloc += 16;
    p->outstreams = realloc(p->outstreams,
                        sizeof(*p->outstreams) *
                        (p->outstreams_alloc));
    }

  if(pos < p->num_outstreams)
    {
    memmove(p->outstreams + pos + 1, p->outstreams + pos,
            (p->num_outstreams - pos)*sizeof(*p->outstreams));
    }
  
  p->outstreams[pos] = t;
  p->num_outstreams++;

  t->p = p;
  
  }


static void resolve_source_tracks(bg_nle_outstream_t * s,
                                  bg_nle_track_t ** tracks, int num_tracks)
  {
  int i, j;
  s->source_tracks = malloc(s->source_tracks_alloc *
                            sizeof(*s->source_tracks));

  for(i = 0; i < s->num_source_tracks; i++)
    {
    for(j = 0; j < num_tracks; j++)
      {
      if(s->source_track_ids[i] == tracks[j]->id)
        {
        s->source_tracks[i] = tracks[j];
        break;
        }
      }
    }
  }


void bg_nle_project_resolve_ids(bg_nle_project_t * p)
  {
  int i;
  /* Set the source tracks of the outstreams */

  for(i = 0; i < p->num_outstreams; i++)
    {
    resolve_source_tracks(p->outstreams[i],
                          p->tracks, p->num_tracks);
    }
  
  }

int bg_nle_project_outstream_index(bg_nle_project_t * p,
                                   bg_nle_outstream_t * outstream)
  {
  int i;
  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i] == outstream)
      return i;
    }
  return -1;
  }

int bg_nle_project_track_index(bg_nle_project_t * p,
                               bg_nle_track_t * track)
  {
  int i;
  
  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i] == track)
      return i;
    }
  return -1;
  }

bg_nle_outstream_t * bg_nle_project_find_outstream(bg_nle_project_t * p,
                                                   bg_nle_track_type_t type,
                                                   int index)
  {
  int count = 0, i;

  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i]->type == type)
      {
      if(count == index)
        return p->outstreams[i];
      else
        count++;
      }
    }
  return NULL;
  }

bg_nle_track_t * bg_nle_project_find_track(bg_nle_project_t * p,
                                           bg_nle_track_type_t type,
                                           int index)
  {
  int count = 0, i;

  for(i = 0; i < p->num_tracks; i++)
    {
    if(p->tracks[i]->type == type)
      {
      if(count == index)
        return p->tracks[i];
      else
        count++;
      }
    }
  return NULL;
  }


int bg_nle_project_num_outstreams(bg_nle_project_t * p,
                                  bg_nle_track_type_t type)
  {
  int i;
  int ret = 0;

  for(i = 0; i < p->num_outstreams; i++)
    {
    if(p->outstreams[i]->type == type)
      ret++;
    }                                          
  return ret;
  }
