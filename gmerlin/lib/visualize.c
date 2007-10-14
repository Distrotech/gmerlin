#include <string.h>

#include <gavl/gavl.h>

#include <config.h>
#include <translation.h>

#include <pluginregistry.h>
#include <visualize.h>
#include <utils.h>

#include <log.h>

#define LOG_DOMAIN "visualizer"

/* Messages from the application to the visualizer */

#define BG_VIS_MSG_AUDIO_FORMAT 0
#define BG_VIS_MSG_AUDIO_DATA   1
#define BG_VIS_MSG_RENDERSIZE   2
#define BG_VIS_MSG_FPS          3
#define BG_VIS_MSG_STOP         4
#define BG_VIS_MSG_SETPARAM     5

/* Messages from the visualizer to the application */

#define BG_VIS_MSG_LOG          0

/*
 * gmerlin_visualize_slave
 *   -w "window_id"
 *   -a "application"
 *   -p "plugin_module"
 *   -o "output_module"
 */


typedef struct
  {
  gavl_audio_converter_t * cnv;
  gavl_audio_frame_t     * in_frame_1;
  gavl_audio_frame_t     * in_frame_2;
  pthread_mutex_t in_mutex;
  
  int do_convert;
  
  gavl_audio_frame_t * out_frame;
  
  gavl_audio_format_t in_format;
  gavl_audio_format_t out_format;
  
  int last_samples_read;
  int frame_done;
  
  gavl_volume_control_t * gain_control;
  pthread_mutex_t gain_mutex;
  
  } audio_buffer_t;

static audio_buffer_t * audio_buffer_create()
  {
  audio_buffer_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->cnv = gavl_audio_converter_create();
  pthread_mutex_init(&(ret->in_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->gain_mutex),(pthread_mutexattr_t *)0);
  
  ret->gain_control = gavl_volume_control_create();
  
  return ret;
  }

static void audio_buffer_cleanup(audio_buffer_t * b)
  {
  if(b->in_frame_1)
    {
    gavl_audio_frame_destroy(b->in_frame_1);
    b->in_frame_1 = (gavl_audio_frame_t*)0;
    }
  if(b->in_frame_2)
    {
    gavl_audio_frame_destroy(b->in_frame_2);
    b->in_frame_2 = (gavl_audio_frame_t*)0;
    }
  if(b->out_frame)
    {
    gavl_audio_frame_destroy(b->out_frame);
    b->out_frame = (gavl_audio_frame_t*)0;
    }
  b->last_samples_read = 0;
  b->frame_done = 0;
  }


static void audio_buffer_destroy(audio_buffer_t * b)
  {
  audio_buffer_cleanup(b);
  gavl_audio_converter_destroy(b->cnv);
  gavl_volume_control_destroy(b->gain_control);
  pthread_mutex_destroy(&b->in_mutex);
  pthread_mutex_destroy(&b->gain_mutex);
  free(b);
  }

static void audio_buffer_init(audio_buffer_t * b,
                              const gavl_audio_format_t * in_format,
                              const gavl_audio_format_t * out_format)
  {
  gavl_audio_format_t frame_format;
  /* Cleanup */
  audio_buffer_cleanup(b);
#if 0
  fprintf(stderr, "audio_buffer_init\n");
  fprintf(stderr, "in_format:\n");
  gavl_audio_format_dump(in_format);
  fprintf(stderr, "out_format:\n");
  gavl_audio_format_dump(out_format);
#endif
  gavl_audio_format_copy(&b->in_format, in_format);
  gavl_audio_format_copy(&b->out_format, out_format);

  /* For visualizations, we ignore the samplerate completely.
     Perfect synchronization is mathematically impossible anyway. */
  
  b->out_format.samplerate = b->in_format.samplerate;
  
  b->do_convert = gavl_audio_converter_init(b->cnv,
                                            &b->in_format,
                                            &b->out_format);

  b->in_frame_1 = gavl_audio_frame_create(&b->in_format);

  gavl_audio_format_copy(&frame_format, out_format);
  frame_format.samples_per_frame = b->in_format.samples_per_frame;
  
  b->in_frame_2 = gavl_audio_frame_create(&frame_format);
  
  b->out_frame = gavl_audio_frame_create(&b->out_format);
  
  gavl_volume_control_set_format(b->gain_control, &frame_format);
  }

static void audio_buffer_put(audio_buffer_t * b,
                             const gavl_audio_frame_t * f)
  {
  //  fprintf(stderr, "audio buffer_put...");
  pthread_mutex_lock(&b->in_mutex);
  b->in_frame_1->valid_samples =
    gavl_audio_frame_copy(&b->in_format,
                          b->in_frame_1,
                          f,
                          0, /* dst_pos */
                          0, /* src_pos */
                          b->in_format.samples_per_frame, /* dst_size */
                          f->valid_samples /* src_size */ ); 
  pthread_mutex_unlock(&b->in_mutex);
  //  fprintf(stderr, "audio buffer_put done\n");
  }

static void audio_buffer_set_gain(audio_buffer_t * b, float gain)
  {
  pthread_mutex_lock(&b->gain_mutex);
  gavl_volume_control_set_volume(b->gain_control, gain);
  pthread_mutex_unlock(&b->gain_mutex);
  }

static gavl_audio_frame_t * audio_buffer_get(audio_buffer_t * b)
  {
  int samples_copied;
  /* Check if there is new audio */
  pthread_mutex_lock(&b->in_mutex);

  if(b->in_frame_1->valid_samples)
    {
    if(b->do_convert)
      {
      gavl_audio_convert(b->cnv, b->in_frame_1, b->in_frame_2);
      samples_copied = b->in_frame_1->valid_samples;
      }
    else
      samples_copied =
        gavl_audio_frame_copy(&b->in_format,
                              b->in_frame_2,
                              b->in_frame_1,
                              0, /* dst_pos */
                              0, /* src_pos */
                              b->in_format.samples_per_frame, /* dst_size */
                              b->in_frame_1->valid_samples    /* src_size */ ); 
    
    b->in_frame_2->valid_samples = samples_copied;
    b->last_samples_read         = samples_copied;
    b->in_frame_1->valid_samples = 0;
    
    pthread_mutex_lock(&b->gain_mutex);
    gavl_volume_control_apply(b->gain_control, b->in_frame_2);
    pthread_mutex_unlock(&b->gain_mutex);
    }
  pthread_mutex_unlock(&b->in_mutex);
  
  /* If the frame was output the last time, set valid_samples to 0 */
  if(b->frame_done)
    {
    b->out_frame->valid_samples = 0;
    b->frame_done = 0;
    }
  
  /* Copy to output frame and check if there are enough samples */
  
  samples_copied =
    gavl_audio_frame_copy(&b->out_format,
                          b->out_frame,
                          b->in_frame_2,
                          b->out_frame->valid_samples, /* dst_pos */
                          b->last_samples_read - b->in_frame_2->valid_samples, /* src_pos */
                          b->out_format.samples_per_frame - b->out_frame->valid_samples, /* dst_size */
                          b->in_frame_2->valid_samples /* src_size */ ); 
  
  b->out_frame->valid_samples += samples_copied;
  b->in_frame_2->valid_samples -= samples_copied;
    
  if(b->out_frame->valid_samples == b->out_format.samples_per_frame)
    {
    b->frame_done = 1;
    return b->out_frame;
    }
  return (gavl_audio_frame_t*)0;
  }

struct bg_visualizer_s
  {
  bg_plugin_registry_t * plugin_reg;

  audio_buffer_t * audio_buffer;
  
  gavl_video_converter_t * video_cnv;

  int do_convert_video;
  
  bg_plugin_handle_t * vis_handle;
  bg_plugin_handle_t * ov_handle;

  bg_ov_plugin_t * ov_plugin;
  bg_visualization_plugin_t * vis_plugin;
  
  bg_parameter_info_t * params;
  
  gavl_video_format_t video_format_in;
  gavl_video_format_t video_format_in_real;
  gavl_video_format_t video_format_out;
  
  pthread_t video_thread;
  
  pthread_mutex_t stop_mutex;
  int do_stop;

  gavl_video_frame_t * video_frame_in;
  gavl_video_frame_t * video_frame_out;

  gavl_timer_t * timer;
  
  gavl_time_t last_frame_time;

  gavl_audio_format_t audio_format_in;
  gavl_audio_format_t audio_format_out;
  
  pthread_mutex_t running_mutex;
  
  int enable;
  int changed;
  
  int is_open;
  
  char * vis_plugin_name;
  };

static void init_plugin(bg_visualizer_t * v);
static void cleanup_plugin(bg_visualizer_t * v);


bg_visualizer_t *
bg_visualizer_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_visualizer_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = plugin_reg;
  ret->video_cnv = gavl_video_converter_create();

  ret->audio_buffer = audio_buffer_create();
  
  pthread_mutex_init(&(ret->stop_mutex),(pthread_mutexattr_t *)0);
  ret->timer = gavl_timer_create();

  pthread_mutex_init(&(ret->running_mutex),(pthread_mutexattr_t *)0);
  
  return ret;
  }

void bg_visualizer_destroy(bg_visualizer_t * v)
  {
  pthread_mutex_destroy(&(v->stop_mutex));
  
  gavl_video_converter_destroy(v->video_cnv);
  audio_buffer_destroy(v->audio_buffer);
  gavl_timer_destroy(v->timer);
  pthread_mutex_destroy(&(v->running_mutex));
  
  if(v->vis_handle)
    bg_plugin_unref(v->vis_handle);

  if(v->vis_plugin_name)
    free(v->vis_plugin_name);
  
  free(v);
  }

static bg_parameter_info_t parameters[] =
  {
    {
      name:       "enable",
      long_name:  TRS("Enable visualizations"),
      type:       BG_PARAMETER_CHECKBUTTON,
      flags:      BG_PARAMETER_SYNC,
    },
    {
      name:       "plugin",
      long_name:  TRS("Plugin"),
      type:       BG_PARAMETER_MULTI_MENU,
      flags:      BG_PARAMETER_SYNC,
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
    {
      name:        "gain",
      long_name:   TRS("Gain"),
      type:        BG_PARAMETER_SLIDER_FLOAT,
      flags:       BG_PARAMETER_SYNC,
      val_min:     { val_f: -10.0 },
      val_max:     { val_f:  10.0 },
      val_default: { val_f:   0.0 },
      num_digits: 2,
      help_string: TRS("Gain (in dB) to apply to the audio samples before it is sent to the visualization plugin"),
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
  bg_visualizer_t * v;
  if(!name)
    return;
  
  v = (bg_visualizer_t*)priv;
  
  //  fprintf(stderr, "bg_visualizer_set_parameter: %s\n", name);
  
  if(!strcmp(name, "enable"))
    {
    if(v->enable != val->val_i)
      {
      v->changed = 1;
      v->enable = val->val_i;
      }
    }
  else if(!strcmp(name, "plugin"))
    {
    if(!v->vis_plugin_name || strcmp(v->vis_plugin_name, val->val_str))
      v->changed = 1;
    v->vis_plugin_name = bg_strdup(v->vis_plugin_name, val->val_str);
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
    v->video_format_in.frame_duration = (int)((float)GAVL_TIME_SCALE / val->val_f);
    v->video_format_in.timescale      = GAVL_TIME_SCALE;
    }
  else if(!strcmp(name, "gain"))
    {
    audio_buffer_set_gain(v->audio_buffer, val->val_f);
    }
  else if(v->vis_handle && v->vis_handle->plugin->set_parameter)
    {
    v->vis_handle->plugin->set_parameter(v->vis_handle->priv,
                                         name, val);
    }
  }

static void * video_thread_func(void * data)
  {
  int do_stop;
  bg_visualizer_t * v;
  gavl_audio_frame_t * audio_frame;
  gavl_time_t diff_time, current_time;
  
  v = (bg_visualizer_t*)data;
  pthread_mutex_lock(&v->running_mutex);
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

    /* Check if we should update audio */

    audio_frame = audio_buffer_get(v->audio_buffer);
    if(audio_frame)
      v->vis_plugin->update(v->vis_handle->priv,
                            audio_frame);
    
    /* Draw frame */
    
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
    current_time = gavl_timer_get(v->timer);
    
    diff_time = v->last_frame_time +
      v->video_format_out.frame_duration - current_time;
    
    if(diff_time > GAVL_TIME_SCALE / 1000)
      gavl_time_delay(&diff_time);
    
    /* Show frame */
    
    if(v->vis_handle->info->flags & BG_PLUGIN_VISUALIZE_FRAME)
      {
      bg_plugin_lock(v->ov_handle);
      v->ov_plugin->put_video(v->ov_handle->priv, v->video_frame_out);
      v->last_frame_time = gavl_timer_get(v->timer);
      
      v->ov_plugin->handle_events(v->ov_handle->priv);
      bg_plugin_unlock(v->ov_handle);
      }
    else
      {
      bg_plugin_lock(v->vis_handle);
      v->vis_plugin->show_frame(v->vis_handle->priv);
      v->last_frame_time = gavl_timer_get(v->timer);
      bg_plugin_unlock(v->vis_handle);

      bg_plugin_lock(v->ov_handle);
      v->ov_plugin->handle_events(v->ov_handle->priv);
      bg_plugin_unlock(v->ov_handle);
      }
    
    }
  
  pthread_mutex_unlock(&v->running_mutex);
  return (void*)0;
  }

int bg_visualizer_stop(bg_visualizer_t * v)
  {
  if(!pthread_mutex_trylock(&v->running_mutex))
    {
    pthread_mutex_unlock(&v->running_mutex);
    return 0;
    }
  
  /* Join threads */
  
  pthread_mutex_lock(&v->stop_mutex);
  v->do_stop = 1;
  pthread_mutex_unlock(&v->stop_mutex);
  
  pthread_join(v->video_thread, (void**)0);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Joined thread");
  gavl_timer_stop(v->timer);
  return 1;
  }

int bg_visualizer_start(bg_visualizer_t * v)
  {
  if(pthread_mutex_trylock(&v->running_mutex))
    return 0;
  
  pthread_mutex_unlock(&v->running_mutex);
  
  v->do_stop = 0;
  v->last_frame_time = 0; 
  gavl_timer_set(v->timer, 0);
  gavl_timer_start(v->timer);
  
  pthread_create(&(v->video_thread), (pthread_attr_t*)0, video_thread_func, v);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Started thread");
  return 1;
  }

void bg_visualizer_set_audio_format(bg_visualizer_t * v,
                                    const gavl_audio_format_t * format)
  {
  int was_running;
  //  fprintf(stderr, "bg_visualizer_set_audio_format...");
  was_running = bg_visualizer_stop(v);
  pthread_mutex_lock(&v->audio_buffer->in_mutex);
  
  gavl_audio_format_copy(&v->audio_format_in, format);
  audio_buffer_init(v->audio_buffer, &v->audio_format_in, &v->audio_format_out);
  pthread_mutex_unlock(&v->audio_buffer->in_mutex);
  if(was_running)
    bg_visualizer_start(v);
  //  fprintf(stderr, "bg_visualizer_set_audio_format done\n");
  }

static void cleanup_plugin(bg_visualizer_t * v)
  {
  /* Close OV Plugin */
  if(v->vis_handle->info->flags & BG_PLUGIN_VISUALIZE_FRAME)
    {
    if(v->video_frame_out)
      {
      if(v->ov_plugin->free_frame)
        v->ov_plugin->free_frame(v->ov_handle->priv,
                                 v->video_frame_out);
      else
        gavl_video_frame_destroy(v->video_frame_out);
      v->video_frame_out = (gavl_video_frame_t*)0;
      }
    if(v->video_frame_in)
      {
      gavl_video_frame_destroy(v->video_frame_in);
      v->video_frame_in = (gavl_video_frame_t*)0;
      }
    v->ov_plugin->close(v->ov_handle->priv);
    }
  /* Close vis plugin */
  v->vis_plugin->close(v->vis_handle->priv);
  v->is_open = 0;
  }

static void init_plugin(bg_visualizer_t * v)
  {
  const bg_plugin_info_t * info;
  gavl_audio_format_copy(&v->audio_format_out, &v->audio_format_in);

  /* Set members, which might be missing */
  v->video_format_in.pixel_width  = 1;
  v->video_format_in.pixel_height = 1;
  
  /* Set video format */
  gavl_video_format_copy(&v->video_format_in_real, &v->video_format_in);
  
  /* Check whether to reopen the visualization plugin */
  
  if(v->vis_handle && strcmp(v->vis_plugin_name, v->vis_handle->info->name))
    {
    if(v->is_open)
      cleanup_plugin(v);
    
    bg_plugin_unref(v->vis_handle);
    v->vis_handle = (bg_plugin_handle_t*)0;
    v->vis_plugin = (bg_visualization_plugin_t*)0;
    }

  if(!v->vis_handle)
    {
    info = bg_plugin_find_by_name(v->plugin_reg, v->vis_plugin_name);
    v->vis_handle = bg_plugin_load(v->plugin_reg, info);
    v->vis_plugin =
      (bg_visualization_plugin_t*)(v->vis_handle->plugin);
    }
  
  /* Open visualizer plugin */
  
  v->vis_plugin->open(v->vis_handle->priv, v->ov_plugin,
                      v->ov_handle->priv,
                      &v->audio_format_out,
                      &v->video_format_in_real);
  if(v->vis_handle->info->flags & BG_PLUGIN_VISUALIZE_FRAME)
    {
    gavl_video_format_copy(&v->video_format_out, &v->video_format_in_real);
    
    /* Open OV Plugin */
    v->ov_plugin->open(v->ov_handle->priv, &v->video_format_out);
    
    /* Initialize video converter */
    
    v->do_convert_video =
      gavl_video_converter_init(v->video_cnv, &v->video_format_in_real,
                                &v->video_format_out);

    if(v->ov_plugin->alloc_frame)
      v->video_frame_out = v->ov_plugin->alloc_frame(v->ov_handle->priv);
    else
      v->video_frame_out = gavl_video_frame_create(&v->video_format_out);
    
    if(v->do_convert_video)
      v->video_frame_in = gavl_video_frame_create(&v->video_format_in_real);
    }
  else
    gavl_video_format_copy(&v->video_format_out, &v->video_format_in);
  
  v->ov_plugin->set_window_title(v->ov_handle->priv,
                                 v->vis_handle->info->long_name);
  
  v->ov_plugin->show_window(v->ov_handle->priv, 1);

  v->is_open = 1;
  
  }

void bg_visualizer_open(bg_visualizer_t * v,
                        const gavl_audio_format_t * format,
                        bg_plugin_handle_t * ov_handle)
  {
  /* Set audio format */
  
  gavl_audio_format_copy(&v->audio_format_in, format);
  
  /* Set ov plugin */
  v->ov_handle = ov_handle;
  v->ov_plugin = (bg_ov_plugin_t*)(ov_handle->plugin);
  
  init_plugin(v);
  
  audio_buffer_init(v->audio_buffer, &v->audio_format_in, &v->audio_format_out);

  v->changed = 0;
  
  bg_visualizer_start(v);
  
  }

void bg_visualizer_update(bg_visualizer_t * v, const gavl_audio_frame_t * frame)
  {
  audio_buffer_put(v->audio_buffer, frame);
  }


void bg_visualizer_close(bg_visualizer_t * v)
  {
  fprintf(stderr, "Close visualizer\n");
  bg_visualizer_stop(v);
  cleanup_plugin(v);
  }

int bg_visualizer_is_enabled(bg_visualizer_t * v)
  {
  return v->enable;
  }

int bg_visualizer_need_restart(bg_visualizer_t * v)
  {
  return v->changed;
  }
