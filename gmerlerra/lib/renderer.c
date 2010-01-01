#include <string.h>

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#include <gmerlin/pluginregistry.h>
#include <renderer.h>

struct bg_nle_renderer_s
  {
  bg_track_info_t info;

  struct
    {
    bg_nle_renderer_outstream_audio_t * s;
    bg_nle_outstream_t * os;
    } * audio_streams;

  struct
    {
    bg_nle_renderer_outstream_video_t * s;
    bg_nle_outstream_t * os;
    } * video_streams;
  
  int num_audio_streams;
  int num_video_streams;

  struct
    {
    bg_nle_renderer_instream_audio_t * s;
    bg_nle_track_t * t;
    } * audio_istreams;

  struct
    {
    bg_nle_renderer_instream_video_t * s;
    bg_nle_track_t * t;
    } * video_istreams;
  
  int num_audio_istreams;
  int num_video_istreams;
  
  bg_nle_project_t * p;
  bg_plugin_registry_t * plugin_reg;

  int quality;

  gavl_audio_options_t * aopt;
  gavl_video_options_t * vopt;

  bg_nle_file_cache_t * file_cache;
  
  };

bg_nle_renderer_t * bg_nle_renderer_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_nle_renderer_t * ret = calloc(1, sizeof(*ret));
  ret->plugin_reg = plugin_reg;

  ret->aopt = gavl_audio_options_create();
  ret->vopt = gavl_video_options_create();
  
  return ret;
  }

static void cleanup_renderer(bg_nle_renderer_t * r)
  {
  bg_track_info_free(&r->info);
  }

int bg_nle_renderer_open(bg_nle_renderer_t * r, bg_nle_project_t * p)
  {
  int i;
  int test_duration;
  
  r->p = p;
  
  r->file_cache = bg_nle_file_cache_create(r->p);
  
  /* Cleanup from previous playback */
  
  /* Apply parameters */
  
  /* Create streams */

  r->num_audio_streams = 0;
  r->num_video_streams = 0;
  
  for(i = 0; i < p->num_outstreams; i++)
    {
    switch(p->outstreams[i]->type)
      {
      case BG_NLE_TRACK_AUDIO:
        r->num_audio_streams++;
        break;
      case BG_NLE_TRACK_VIDEO:
        r->num_video_streams++;
        break;
      case BG_NLE_TRACK_NONE:
        break;
      }
    }

  for(i = 0; i < p->num_tracks; i++)
    {
    switch(p->tracks[i]->type)
      {
      case BG_NLE_TRACK_AUDIO:
        r->num_audio_istreams++;
        break;
      case BG_NLE_TRACK_VIDEO:
        r->num_video_istreams++;
        break;
      case BG_NLE_TRACK_NONE:
        break;
      }
    }
 
  r->audio_streams = calloc(r->num_audio_streams, sizeof(*r->audio_streams));
  r->video_streams = calloc(r->num_video_streams, sizeof(*r->video_streams));

  r->audio_istreams = calloc(r->num_audio_istreams, sizeof(*r->audio_istreams));
  r->video_istreams = calloc(r->num_video_istreams, sizeof(*r->video_istreams));

  /* Connect project pointers */
  for(i = 0; i < r->num_audio_streams; i++)
    {
    r->audio_streams[i].os =
      bg_nle_project_find_outstream(p, BG_NLE_TRACK_AUDIO, i);
    }
  for(i = 0; i < r->num_video_streams; i++)
    {
    r->video_streams[i].os =
      bg_nle_project_find_outstream(p, BG_NLE_TRACK_VIDEO, i);
    }

  for(i = 0; i < r->num_audio_istreams; i++)
    {
    r->audio_istreams[i].t =
      bg_nle_project_find_track(p, BG_NLE_TRACK_AUDIO, i);
    }
  for(i = 0; i < r->num_video_istreams; i++)
    {
    r->video_istreams[i].t =
      bg_nle_project_find_track(p, BG_NLE_TRACK_VIDEO, i);
    }
  
  /* Create track info */
  
  r->info.flags = BG_TRACK_SEEKABLE | BG_TRACK_PAUSABLE;
  r->info.num_audio_streams = r->num_audio_streams;
  r->info.num_video_streams = r->num_video_streams;
  r->info.audio_streams = calloc(r->info.num_audio_streams,
                                  sizeof(*r->info.audio_streams));
  r->info.video_streams = calloc(r->info.num_video_streams,
                                  sizeof(*r->info.video_streams));

  r->info.duration = 0;
  
  for(i = 0; i < p->num_outstreams; i++)
    {
    test_duration = bg_nle_outstream_duration(p->outstreams[i]);
    if(test_duration > r->info.duration)
      r->info.duration = test_duration;
    }
  
  return 1;
  }

int bg_nle_renderer_set_audio_stream(bg_nle_renderer_t * r, int stream,
                                     bg_stream_action_t action)
  {
  if(action == BG_STREAM_ACTION_DECODE)
    {
    bg_nle_outstream_t * os = 
      bg_nle_project_find_outstream(r->p,
                                    BG_NLE_TRACK_AUDIO,
                                    stream);
    
    r->audio_streams[stream].s =
      bg_nle_renderer_outstream_audio_create(r->p, os, r->aopt,
                                             &r->info.audio_streams[stream].format);
    r->audio_streams[stream].os = os;
    }
  return 1;
  }

int bg_nle_renderer_set_video_stream(bg_nle_renderer_t * r, int stream,
                                     bg_stream_action_t action)
  {
  if(action == BG_STREAM_ACTION_DECODE)
    {
    bg_nle_outstream_t * os = 
      bg_nle_project_find_outstream(r->p,
                                    BG_NLE_TRACK_VIDEO,
                                    stream);
    
    r->video_streams[stream].s =
      bg_nle_renderer_outstream_video_create(r->p, os, r->vopt,
                                             &r->info.video_streams[stream].format);
    r->video_streams[stream].os = os;
    }
  return 1;
  }

int bg_nle_renderer_start(bg_nle_renderer_t * r)
  {
  int i, j, k;
  for(i = 0; i < r->num_audio_streams; i++)
    {
    if(!r->audio_streams[i].s)
      continue;

    /* Connect source tracks */
    for(j = 0; j < r->audio_streams[i].os->num_source_tracks; j++)
      {
      for(k = 0; k < r->num_audio_istreams; k++)
        {
        if(r->audio_istreams[k].t ==
           r->audio_streams[i].os->source_tracks[j])
          {
          if(!r->audio_istreams[k].s)
            {
            r->audio_istreams[k].s =
              bg_nle_renderer_instream_audio_create(r->p,
                                                    r->audio_istreams[k].t,
                                                    r->aopt, r->file_cache);
            }
          bg_nle_renderer_outstream_audio_add_istream(r->audio_streams[i].s,
                                                      r->audio_istreams[k].s,
                                                      r->audio_istreams[k].t);
          break;
          }
        }
      }
    }

  for(j = 0; j < r->video_streams[i].os->num_source_tracks; j++)
    {
    for(k = 0; k < r->num_video_istreams; k++)
      {
      if(r->video_istreams[k].t ==
         r->video_streams[i].os->source_tracks[j])
        {
        if(!r->video_istreams[k].s)
          {
          r->video_istreams[k].s =
            bg_nle_renderer_instream_video_create(r->p,
                                                  r->video_istreams[k].t,
                                                  r->vopt, r->file_cache);
          }
        bg_nle_renderer_outstream_video_add_istream(r->video_streams[i].s,
                                                    r->video_istreams[k].s,
                                                    r->video_istreams[k].t);
        break;
        }
      }
    }
  return 1;
  }


void bg_nle_renderer_destroy(bg_nle_renderer_t * r)
  {
  gavl_audio_options_destroy(r->aopt);
  gavl_video_options_destroy(r->vopt);
  free(r);
  }

bg_track_info_t * bg_nle_renderer_get_track_info(bg_nle_renderer_t * r)
  {
  return &r->info;
  }

int bg_nle_renderer_read_video(bg_nle_renderer_t *r ,
                               gavl_video_frame_t * ret, int stream)
  {
  return bg_nle_renderer_outstream_video_read(r->video_streams[stream].s,
                                              ret);
  }

int bg_nle_renderer_read_audio(bg_nle_renderer_t * r,
                               gavl_audio_frame_t * ret,
                               int stream, int num_samples)
  {
  return bg_nle_renderer_outstream_audio_read(r->audio_streams[stream].s,
                                              ret, num_samples);
  }

void bg_nle_renderer_seek(bg_nle_renderer_t * r, gavl_time_t time)
  {
  int i;

  for(i = 0; i < r->num_audio_streams; i++)
    bg_nle_renderer_outstream_audio_seek(r->audio_streams[i].s, time);
  for(i = 0; i < r->num_video_streams; i++)
    bg_nle_renderer_outstream_video_seek(r->video_streams[i].s, time);
  
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name = "render_quality",
      .long_name = TRS("Render quality"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      /* End of parameters */ 
    },
    
  };

const bg_parameter_info_t *
bg_nle_renderer_get_parameters(bg_nle_renderer_t * r)
  {
  return parameters;
  }

void bg_nle_renderer_set_parameter(void * data, const char * name,
                                   const bg_parameter_value_t * val)
  {
  if(!name)
    return;
  if(!strcmp(name, "render_quality"))
    {
    
    }
      
  }


/* bg_nle_plugin_t */

typedef struct
  {
  bg_nle_renderer_t * renderer;

  bg_nle_project_t * project;

  bg_plugin_registry_t * plugin_reg;
  bg_plugin_registry_t * plugin_reg_priv;

  bg_cfg_registry_t * cfg_reg;
  
  } bg_nle_plugin_t;

const bg_parameter_info_t *
bg_nle_plugin_get_parameters(void* priv)
  {
  bg_nle_plugin_t * p = priv;
  return bg_nle_renderer_get_parameters(p->renderer);
  }

void bg_nle_plugin_set_parameter(void * priv, const char * name,
                                 const bg_parameter_value_t * val)
  {
  bg_nle_plugin_t * p = priv;
  bg_nle_renderer_set_parameter(p->renderer, name, val);
  }

void bg_nle_plugin_destroy(void * priv)
  {
  bg_nle_plugin_t * p = priv;
  if(p->renderer)
    bg_nle_renderer_destroy(p->renderer);
  if(p->plugin_reg_priv)
    bg_plugin_registry_destroy(p->plugin_reg_priv);
  if(p->cfg_reg)
    bg_cfg_registry_destroy(p->cfg_reg);
  if(p->project)
    bg_nle_project_destroy(p->project);
  
  }

void * bg_nle_plugin_create(bg_nle_project_t * p,
                            bg_plugin_registry_t * plugin_reg)
  {
  bg_nle_plugin_t * ret = calloc(1, sizeof(*ret));
  
  if(plugin_reg)
    ret->plugin_reg = plugin_reg;
  else
    {
    char * tmp_path;
    bg_cfg_section_t * cfg_section;
    bg_plugin_registry_options_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.dont_save = 1;
    opt.blacklist = (char*[]){ "i_gmerlerra", NULL };
    
    ret->cfg_reg = bg_cfg_registry_create();
    tmp_path = bg_search_file_read("gmerlerra", "config.xml");
    bg_cfg_registry_load(ret->cfg_reg, tmp_path);
    if(tmp_path)
      free(tmp_path);
    cfg_section = bg_cfg_registry_find_section(ret->cfg_reg, "plugins");
    ret->plugin_reg_priv = bg_plugin_registry_create_with_options(cfg_section, &opt);
    ret->plugin_reg = ret->plugin_reg_priv;
    }

  ret->renderer = bg_nle_renderer_create(ret->plugin_reg);
  
  if(p)
    bg_nle_renderer_open(ret->renderer, p);
  
  return ret;
  }

static char const * const extensions = "gmprj";

const char * bg_nle_plugin_get_extensions(void * priv)
  {
  return extensions;
  }
  
  
void bg_nle_plugin_set_callbacks(void * priv, bg_input_callbacks_t * callbacks)
  {
  //  bg_nle_plugin_t * p = priv;
  }

  
int bg_nle_plugin_open(void * priv, const char * arg)
  {
  bg_nle_plugin_t * p = priv;

  if(p->project)
    bg_nle_project_destroy(p->project);

  p->project = bg_nle_project_load(arg, p->plugin_reg);
  if(!p->project)
    return 0;
  return bg_nle_renderer_open(p->renderer, p->project);
  }



//  const bg_edl_t * (*get_edl)(void * priv);
    
int bg_nle_plugin_get_num_tracks(void * priv)
  {
  return 1;
  }
  
bg_track_info_t * bg_nle_plugin_get_track_info(void * priv, int track)
  {
  bg_nle_plugin_t * p = priv;
  return bg_nle_renderer_get_track_info(p->renderer);
  }

  
int bg_nle_plugin_set_track(void * priv, int track)
  {
  return 1;
  }
    
  
int bg_nle_plugin_set_audio_stream(void * priv, int stream,
                                   bg_stream_action_t action)
  {
  bg_nle_plugin_t * p = priv;
  return bg_nle_renderer_set_audio_stream(p->renderer, stream, action);
  }

int bg_nle_plugin_set_video_stream(void * priv, int stream,
                                   bg_stream_action_t action)
  {
  bg_nle_plugin_t * p = priv;
  return bg_nle_renderer_set_video_stream(p->renderer, stream, action);
  }
  
#if 0
int bg_nle_plugin_set_subtitle_stream(void * priv, int stream, bg_stream_action_t action)
  {
  bg_nle_plugin_t * p = priv;

  }
#endif
  
int bg_nle_plugin_start(void * priv)
  {
  bg_nle_plugin_t * p = priv;
  return bg_nle_renderer_start(p->renderer);
  }


gavl_frame_table_t * bg_nle_plugin_get_frame_table(void * priv, int stream)
  {
  bg_nle_plugin_t * p = priv;
  return NULL;
  }

int bg_nle_plugin_read_audio(void * priv, gavl_audio_frame_t* frame, int stream,
                             int num_samples)
  {
  bg_nle_plugin_t * p = priv;
  return bg_nle_renderer_read_audio(p->renderer, frame, stream, num_samples);
  }

int bg_nle_plugin_read_video(void * priv, gavl_video_frame_t* frame, int stream)
  {
  bg_nle_plugin_t * p = priv;
  return bg_nle_renderer_read_video(p->renderer, frame, stream);
  }

void bg_nle_plugin_skip_video(void * priv, int stream, int64_t * time, int scale, int exact)
  {
  bg_nle_plugin_t * p = priv;
  
  }
  
void bg_nle_plugin_seek(void * priv, int64_t * time, int scale)
  {
  bg_nle_plugin_t * p = priv;
  gavl_time_t time_unscaled = gavl_time_unscale(scale, *time);
  bg_nle_renderer_seek(p->renderer, time_unscaled);
  }


void bg_nle_plugin_stop(void * priv)
  {

  }

#if 0
void bg_nle_close(void * priv)
  {
  
  }
#endif
