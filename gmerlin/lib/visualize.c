#include <string.h>

#include <gavl/gavl.h>

#include <config.h>
#include <translation.h>

#include <pluginregistry.h>
#include <visualize.h>
#include <log.h>
#define LOG_DOMAIN "visualizer"

typedef struct
  {
  gavl_audio_converter_t * cnv;
  gavl_audio_frame_t     * frame_in;
  gavl_audio_frame_t     * frame_out;
  int do_convert;
  int do_stop;
  pthread_mutex_t mutex;
  } audio_buffer_t;

static audio_buffer_t * audio_buffer_create()
  {
  audio_buffer_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cnv = gavl_audio_converter_create();
  pthread_mutex_init(&(ret->mutex),(pthread_mutexattr_t *)0);
  return ret;
  }

static void audio_buffer_cleanup(audio_buffer_t * b)
  {
  if(b->do_convert && b->frame_in)
    {
    gavl_audio_frame_destroy(b->frame_in);
    b->frame_in = (gavl_audio_frame_t*)0;
    }
  
  if(b->frame_out)
    {
    gavl_audio_frame_destroy(b->frame_out);
    b->frame_out = (gavl_audio_frame_t*)0;
    }
  }

static void audio_buffer_init(audio_buffer_t * b,
                              const gavl_audio_format_t * in_format,
                              const gavl_audio_format_t * out_format)
  {
  /* Cleanup */
  audio_buffer_cleanup(b);
  b->do_convert = gavl_audio_converter_init(b->cnv, in_format, out_format);

  b->frame_out = gavl_audio_frame_create(out_format);
  
  if(b->do_convert)
    b->frame_in = gavl_audio_frame_create(in_format);
  
  }

static void audio_buffer_destroy(audio_buffer_t * b)
  {

  }

static void audio_buffer_put_audio(audio_buffer_t * b, gavl_audio_frame_t * f)
  {
  
  }

static int audio_buffer_get_audio(audio_buffer_t * b, gavl_audio_frame_t * f)
  {
  return 0;
  }

static void audio_buffer_stop(audio_buffer_t * b)
  {
  
  }


struct bg_visualizer_s
  {
  bg_plugin_registry_t * plugin_reg;
  
  gavl_audio_converter_t * audio_cnv;
  gavl_video_converter_t * video_cnv;

  int do_convert_audio;
  int do_convert_video;
  
  bg_plugin_handle_t * vis_handle;
  bg_plugin_handle_t * ov_handle;

  bg_ov_plugin_t * ov_plugin;
  bg_visualization_plugin_t * vis_plugin;
  
  bg_parameter_info_t * params;
  
  gavl_video_format_t video_format_in;
  gavl_video_format_t video_format_in_real;
  gavl_video_format_t video_format_out;

  gavl_audio_format_t audio_format_in;
  gavl_audio_format_t audio_format_out;
  
  pthread_t video_thread;
  pthread_t audio_thread;
  
  pthread_mutex_t stop_mutex;
  int do_stop;
  
  pthread_mutex_t audio_mutex;
  int audio_changed;

  gavl_audio_frame_t * audio_frame_in;
  gavl_audio_frame_t * audio_frame_out_1;
  gavl_audio_frame_t * audio_frame_out_2;

  gavl_video_frame_t * video_frame_in;
  gavl_video_frame_t * video_frame_out;

  gavl_timer_t * timer;
  pthread_mutex_t time_mutex;
  
  int64_t video_time; /* In timescale tics */
  int64_t audio_time; /* In timescale tics */
  
  int audio_pos; /* Inside frame */
  int last_samples_read;
  
  int enable;
  };


bg_visualizer_t * bg_visualizer_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_visualizer_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = plugin_reg;
  ret->audio_cnv = gavl_audio_converter_create();
  ret->video_cnv = gavl_video_converter_create();

  pthread_mutex_init(&(ret->stop_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->audio_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->time_mutex),(pthread_mutexattr_t *)0);
  ret->timer = gavl_timer_create();
  
  return ret;
  }

void bg_visualizer_destroy(bg_visualizer_t * v)
  {
  pthread_mutex_destroy(&(v->stop_mutex));
  pthread_mutex_destroy(&(v->audio_mutex));
  pthread_mutex_destroy(&(v->time_mutex));

  gavl_audio_converter_destroy(v->audio_cnv);
  gavl_video_converter_destroy(v->video_cnv);
  
  gavl_timer_destroy(v->timer);
  
  free(v);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      name:       "enable",
      long_name:  TRS("Enable visualizations"),
      type:       BG_PARAMETER_CHECKBUTTON,
    },
    {
      name:       "plugin",
      long_name:  TRS("Plugin"),
      type:       BG_PARAMETER_MULTI_MENU,
    },
    {
      name:        "width",
      long_name:   TRS("Width"),
      type:        BG_PARAMETER_INT,
      val_min:     { val_i: 16 },
      val_max:     { val_i: 32768 },
      val_default: { val_i: 320 },
      help_string: TRS("Desired image with, the visualization plugin might override this for no apparent reason"),
    },
    {
      name:       "height",
      long_name:  TRS("Height"),
      type:       BG_PARAMETER_INT,
      val_min:    { val_i: 16 },
      val_max:    { val_i: 32768 },
      val_default: { val_i: 240 },
      help_string: TRS("Desired image height, the visualization plugin might override this for no apparent reason"),
    },
    {
      name:        "framerate",
      long_name:   TRS("Framerate"),
      type:        BG_PARAMETER_FLOAT,
      val_min:     { val_f: 0.1 },
      val_max:     { val_f: 200.0 },
      val_default: { val_f: 30.0 },
      num_digits: 2,
    },
    { /* End of parameters */ }
  };

static void create_params(bg_visualizer_t * v)
  {
  v->params = bg_parameter_info_copy_array(parameters);
  bg_plugin_registry_set_parameter_info(v->plugin_reg,
                                        BG_PLUGIN_VISUALIZATION,
                                        BG_PLUGIN_VISUALIZE_FRAME |
                                        BG_PLUGIN_VISUALIZE_GL,
                                        &(v->params[1]));
  }

bg_parameter_info_t * bg_visualizer_get_parameters(bg_visualizer_t * v)
  {
  if(!v->params)
    create_params(v);
  return v->params;
  }

void bg_visualizer_set_parameter(void * priv,
                                 const char * name,
                                 const bg_parameter_value_t * val)
  {
  const bg_plugin_info_t * info;
  bg_visualizer_t * v;
  if(!name)
    return;
  
  v = (bg_visualizer_t*)priv;
  
  //  fprintf(stderr, "bg_visualizer_set_parameter: %s\n", name);

  if(!strcmp(name, "enable"))
    {
    v->enable = val->val_i;
    }
  else if(!strcmp(name, "plugin"))
    {
    if(v->vis_handle && strcmp(v->vis_handle->info->name, val->val_str))
      {
      bg_plugin_unref(v->vis_handle);
      v->vis_handle = (bg_plugin_handle_t*)0;
      }
    if(!v->vis_handle)
      {
      info = bg_plugin_find_by_name(v->plugin_reg, val->val_str);
      v->vis_handle = bg_plugin_load(v->plugin_reg, info);
      v->vis_plugin =
        (bg_visualization_plugin_t*)(v->vis_handle->plugin);
      }
    }
  else if(!strcmp(name, "width"))
    {
    v->video_format_in.image_width = val->val_i;
    v->video_format_in.frame_width = val->val_i;
    }
  else if(!strcmp(name, "height"))
    {
    v->video_format_in.image_height = val->val_i;
    v->video_format_in.frame_height = val->val_i;
    }
  else if(!strcmp(name, "framerate"))
    {
    v->video_format_in.frame_duration = 100;
    v->video_format_in.timescale      = val->val_f * 100;
    }
  else if(v->vis_handle && v->vis_handle->plugin->set_parameter)
    {
    v->vis_handle->plugin->set_parameter(v->vis_handle->priv,
                                         name, val);
    }
  }

static int delay_print_counter = 0;

static void * audio_thread_func(void * data)
  {
  int do_stop;
  bg_visualizer_t * v;
  
  gavl_time_t current_time;
  gavl_time_t diff_time;
  gavl_time_t audio_time_unscaled;
  
  v = (bg_visualizer_t*)data;

  
  
  while(1)
    {
    /* Check if we should stop */
    pthread_mutex_lock(&v->stop_mutex);
    do_stop = v->do_stop;
    pthread_mutex_unlock(&v->stop_mutex);
    if(do_stop)
      break;
    
    /* Check if there is new audio */

    pthread_mutex_lock(&v->audio_mutex);
    if(v->audio_changed)
      {
      if(v->do_convert_audio)
        gavl_audio_convert(v->audio_cnv,
                           v->audio_frame_in,
                           v->audio_frame_out_1);
      else
        gavl_audio_frame_copy(&v->audio_format_out, 
                              v->audio_frame_out_1,
                              v->audio_frame_in,
                              0, 0, v->audio_format_in.samples_per_frame,
                              v->audio_frame_in->valid_samples);
      v->audio_changed = 0;
      v->audio_time = v->audio_frame_in->timestamp;
      v->audio_pos = 0;
      }
    pthread_mutex_unlock(&v->audio_mutex);

    
    
    /* Update audio */

    pthread_mutex_lock(&(v->time_mutex));
    current_time = gavl_timer_get(v->timer);
    pthread_mutex_unlock(&(v->time_mutex));
    
    audio_time_unscaled = gavl_time_unscale(v->audio_format_in.samplerate,
                                            v->audio_time + v->audio_pos);
    
    diff_time = audio_time_unscaled - current_time;

    if(delay_print_counter < 10)
      {
      fprintf(stderr, "Delay %lld %lld -> %lld\n",
              audio_time_unscaled, current_time, diff_time);
      delay_print_counter++;
      }
    
    if(diff_time > GAVL_TIME_SCALE / 1000)
      {
      gavl_time_delay(&diff_time);
      }
    /* Update audio */
    
    bg_plugin_lock(v->vis_handle);
    v->vis_plugin->update(v->vis_handle->priv, v->audio_frame_out_1);
    bg_plugin_unlock(v->vis_handle);
    
    v->audio_pos += v->audio_format_out.samples_per_frame;
    }
  
  return (void*)0;
  }

static void * video_thread_func(void * data)
  {
  int do_stop;
  bg_visualizer_t * v;
  
  gavl_time_t current_time;
  gavl_time_t diff_time;
  gavl_time_t video_time_unscaled;
  
  v = (bg_visualizer_t*)data;

  while(1)
    {
    /* Check if we should stop */
    pthread_mutex_lock(&v->stop_mutex);
    do_stop = v->do_stop;
    pthread_mutex_unlock(&v->stop_mutex);
    if(do_stop)
      break;
    
    /* Draw frame */

    bg_plugin_lock(v->vis_handle);

    if(!(v->vis_handle->info->flags & BG_PLUGIN_VISUALIZE_FRAME))
      v->vis_plugin->draw_frame(v->vis_handle->priv,
                                (gavl_video_frame_t*)0);
    else if(v->do_convert_video)
      {
      v->vis_plugin->draw_frame(v->vis_handle->priv, v->video_frame_in);
      gavl_video_convert(v->video_cnv, v->video_frame_in, v->video_frame_out);
      }
    else
      v->vis_plugin->draw_frame(v->vis_handle->priv, v->video_frame_out);
    
    bg_plugin_unlock(v->vis_handle);
    
    /* Wait until we can show the frame */
    
    pthread_mutex_lock(&(v->time_mutex));
    current_time = gavl_timer_get(v->timer);
    pthread_mutex_unlock(&(v->time_mutex));

    video_time_unscaled = gavl_time_unscale(v->video_format_out.timescale,
                                            v->video_time);
    
    diff_time = video_time_unscaled - current_time;
    if(diff_time > GAVL_TIME_SCALE / 1000)
      gavl_time_delay(&diff_time);

    /* Show frame */
    
    if(v->vis_handle->info->flags & BG_PLUGIN_VISUALIZE_FRAME)
      {
      bg_plugin_lock(v->ov_handle);
      v->ov_plugin->put_video(v->ov_handle->priv, v->video_frame_out);

      fprintf(stderr, "Show frame\n");
      
      v->ov_plugin->handle_events(v->ov_handle->priv);
      bg_plugin_unlock(v->ov_handle);
      }
    else
      {
      bg_plugin_lock(v->vis_handle);
      v->vis_plugin->show_frame(v->vis_handle->priv);
      bg_plugin_unlock(v->vis_handle);

      bg_plugin_lock(v->ov_handle);
      v->ov_plugin->handle_events(v->ov_handle->priv);
      bg_plugin_unlock(v->ov_handle);
      }
    
    v->video_time += v->video_format_out.frame_duration;
    }
  
  return (void*)0;
  }

void bg_visualizer_open(bg_visualizer_t * v,
                        const gavl_audio_format_t * format,
                        bg_plugin_handle_t * ov_handle)
  {
  gavl_audio_format_t tmp_format;
  
  /* Set ov plugin */
  v->ov_handle = ov_handle;
  v->ov_plugin = (bg_ov_plugin_t*)(ov_handle->plugin);
  
  /* Set audio format */
  gavl_audio_format_copy(&v->audio_format_in, format);
  gavl_audio_format_copy(&v->audio_format_out, format);

  /* Set video format */
  gavl_video_format_copy(&v->video_format_in_real, &v->video_format_in);
  
  /* Open visualizer plugin */
  
  v->vis_plugin->open(v->vis_handle->priv, v->ov_plugin,
                      v->ov_handle->priv,
                      &v->audio_format_out,
                      &v->video_format_in_real);

  fprintf(stderr, "Audio Input format:\n");
  gavl_audio_format_dump(&v->audio_format_in);
  fprintf(stderr, "Audio Output format:\n");
  gavl_audio_format_dump(&v->audio_format_out);
  
  if(v->vis_handle->info->flags & BG_PLUGIN_VISUALIZE_FRAME)
    {
    gavl_video_format_copy(&v->video_format_out, &v->video_format_in_real);
    
    /* Open OV Plugin */
    v->ov_plugin->open(v->ov_handle->priv,
                       &v->video_format_out, v->vis_handle->info->long_name);

    fprintf(stderr, "Video Input format:\n");
    gavl_video_format_dump(&v->video_format_in_real);
    fprintf(stderr, "Video Output format:\n");
    gavl_video_format_dump(&v->video_format_out);
    
    /* Initialize video converter */
    
    v->do_convert_video =
      gavl_video_converter_init(v->video_cnv, &v->video_format_in_real,
                                &v->video_format_out);
    if(v->do_convert_video)
      {
      if(v->ov_plugin->alloc_frame)
        v->video_frame_out = v->ov_plugin->alloc_frame(v->ov_handle->priv);
      else
        v->video_frame_out = gavl_video_frame_create(&v->video_format_out);
      v->video_frame_in = gavl_video_frame_create(&v->video_format_in_real);
      }
    else
      {
      if(v->ov_plugin->alloc_frame)
        v->video_frame_out = v->ov_plugin->alloc_frame(v->ov_handle->priv);
      else
        v->video_frame_out = gavl_video_frame_create(&v->video_format_out);
      }
    }

  /* Initialize audio converter */
  v->do_convert_audio = gavl_audio_converter_init(v->audio_cnv,
                                                  &v->audio_format_in,
                                                  &v->audio_format_out);

  /* Create audio frames */
  v->audio_frame_in = gavl_audio_frame_create(&v->audio_format_in);
  
  gavl_audio_format_copy(&tmp_format, &v->audio_format_out);
  tmp_format.samples_per_frame = v->audio_format_in.samples_per_frame;
  
  v->audio_frame_out_1 = gavl_audio_frame_create(&tmp_format);
  v->audio_frame_out_2 = gavl_audio_frame_create(&v->audio_format_out);
    
  v->ov_plugin->show_window(v->ov_handle->priv, 1);
  v->do_stop = 0;
  
  gavl_timer_set(v->timer, 0);
  gavl_timer_start(v->timer);
  
  pthread_create(&(v->audio_thread), (pthread_attr_t*)0, audio_thread_func, v);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Started audio thread");
  pthread_create(&(v->video_thread), (pthread_attr_t*)0, video_thread_func, v);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Started video thread");
  }

void bg_visualizer_update(bg_visualizer_t * v, gavl_audio_frame_t * frame)
  {
  pthread_mutex_lock(&v->audio_mutex);
  v->audio_changed = 1;
  
  v->audio_frame_in->valid_samples
    = gavl_audio_frame_copy(&v->audio_format_in,
                            v->audio_frame_in,
                            frame, 0, 0, v->audio_format_in.samples_per_frame,
                            frame->valid_samples);
  v->audio_frame_in->timestamp = frame->timestamp;
  
  pthread_mutex_unlock(&v->audio_mutex);
  }

void bg_visualizer_close(bg_visualizer_t * v)
  {
  /* Join threads */
  
  pthread_mutex_lock(&v->stop_mutex);
  v->do_stop = 1;
  pthread_mutex_unlock(&v->stop_mutex);

  pthread_join(v->audio_thread, (void**)0);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Joined audio thread");
  pthread_join(v->video_thread, (void**)0);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Joined video thread");
  
  /* Close plugins */
  
  if(v->vis_handle->info->flags & BG_PLUGIN_VISUALIZE_FRAME)
    {
    /* Close OV Plugin */
    v->ov_plugin->close(v->ov_handle->priv);
    }

  v->vis_plugin->close(v->vis_handle->priv);
  gavl_timer_stop(v->timer);
  }

int bg_visualizer_is_enabled(bg_visualizer_t * v)
  {
  return v->enable;
  }
