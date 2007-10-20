#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

#include <gavl/gavl.h>

#include <config.h>
#include <translation.h>

#include <pluginregistry.h>
#include <visualize.h>
#include <utils.h>



#include <log.h>

#define LOG_DOMAIN "visualizer_slave"

/* Messages from the application to the visualizer */

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

typedef struct
  {
  audio_buffer_t * audio_buffer;
  
  gavl_video_converter_t * video_cnv;

  int do_convert_video;
    
  bg_ov_plugin_t * ov_plugin;
  bg_visualization_plugin_t * vis_plugin;

  void * ov_priv;
  void * vis_priv;
  
  gavl_video_format_t video_format_in;
  gavl_video_format_t video_format_in_real;
  gavl_video_format_t video_format_out;
  
  pthread_t video_thread;
  
  pthread_mutex_t running_mutex;
  pthread_mutex_t stop_mutex;
  pthread_mutex_t vis_mutex;
  pthread_mutex_t ov_mutex;
  
  int do_stop;

  gavl_video_frame_t * video_frame_in;
  gavl_video_frame_t * video_frame_out;

  gavl_timer_t * timer;
  
  gavl_time_t last_frame_time;

  gavl_audio_format_t audio_format_in;
  gavl_audio_format_t audio_format_out;
  

  int do_ov;

  void * ov_module;
  void * vis_module;

  char * window_id;
  
  gavl_audio_frame_t * read_frame;
  
  } bg_visualizer_slave_t;

static void init_plugin(bg_visualizer_slave_t * v);

static bg_plugin_common_t *
load_plugin(const char * filename, void ** handle, void ** priv)
  {
  int (*get_plugin_api_version)();
  bg_plugin_common_t * ret;
  
  *handle = dlopen(filename, RTLD_NOW | RTLD_GLOBAL);
  if(!(*handle))
    {
    fprintf(stderr, "Cannot dlopen plugin module %s: %s", filename,
           dlerror());
    goto fail;
    }
  
  get_plugin_api_version = dlsym(*handle, "get_plugin_api_version");
  if(!get_plugin_api_version)
    {
    fprintf(stderr, 
            "cannot get API version: %s", dlerror());
    goto fail;
    }
  if(get_plugin_api_version() != BG_PLUGIN_API_VERSION)
    {
    fprintf(stderr, 
            "Wrong API version: %s", dlerror());
    goto fail;
    }
  ret = dlsym(*handle, "the_plugin");
  if(!ret)
    {
    fprintf(stderr, 
            "No symbol the_plugin: %s", dlerror());
    goto fail;
    }
  *priv = ret->create();
  return ret;
  fail:
  return (bg_plugin_common_t *)0;
  }
                                

static bg_visualizer_slave_t *
bg_visualizer_slave_create(int argc, char ** argv)
  {
  int i;
  bg_visualizer_slave_t * ret;
  char * window_id = (char*)0;
  char * plugin_module = (char*)0;
  char * ov_module = (char*)0;

  /* Handle arguments and load plugins */
  i = 1;
  while(i < argc)
    {
    if(!strcmp(argv[i], "-w"))
      {
      window_id = argv[i+1];
      i += 2;
      }
    else if(!strcmp(argv[i], "-p"))
      {
      plugin_module = argv[i+1];
      i += 2;
      }
    else if(!strcmp(argv[i], "-o"))
      {
      ov_module = argv[i+1];
      i += 2;
      }
    }

  /* Sanity checks */
  if(!window_id)
    {
    fprintf(stderr, "No window ID given\n");
    return (bg_visualizer_slave_t *)0;
    }
  if(!plugin_module)
    {
    fprintf(stderr, "No plugin given\n");
    return (bg_visualizer_slave_t *)0;
    }
  
  ret = calloc(1, sizeof(*ret));
  ret->audio_buffer = audio_buffer_create();
  ret->window_id = window_id;
  
  pthread_mutex_init(&(ret->stop_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->running_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->vis_mutex),(pthread_mutexattr_t *)0);
  pthread_mutex_init(&(ret->ov_mutex),(pthread_mutexattr_t *)0);
  
  ret->timer = gavl_timer_create();

  /* Load ov module */
  if(ov_module)
    {
    ret->do_ov = 1;
    ret->video_cnv = gavl_video_converter_create();
    
    ret->ov_plugin =
      (bg_ov_plugin_t*)load_plugin(ov_module, &ret->ov_module,
                                   &ret->ov_priv);
    if(!ret->ov_plugin)
      return (bg_visualizer_slave_t*)0;
    ret->ov_plugin->set_window(ret->ov_priv, ret->window_id);
    }

  ret->vis_plugin =
    (bg_visualization_plugin_t*)load_plugin(plugin_module, &ret->vis_module,
                                 &ret->vis_priv);
  if(!ret->vis_plugin)
    return (bg_visualizer_slave_t*)0;
  
  
  return ret;
  }

static void bg_visualizer_slave_destroy(bg_visualizer_slave_t * v)
  {
  pthread_mutex_destroy(&(v->stop_mutex));

  if(v->video_cnv)
    gavl_video_converter_destroy(v->video_cnv);

  audio_buffer_destroy(v->audio_buffer);
  gavl_timer_destroy(v->timer);
  pthread_mutex_destroy(&(v->running_mutex));
  
  /* Close OV Plugin */
  if(v->do_ov)
    {
    if(v->video_frame_out)
      {
      if(v->ov_plugin->destroy_frame)
        v->ov_plugin->destroy_frame(v->ov_priv,
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
    v->ov_plugin->close(v->ov_priv);
    }
  /* Close vis plugin */
  v->vis_plugin->close(v->vis_priv);

  
  free(v);
  }

static void * video_thread_func(void * data)
  {
  int do_stop;
  bg_visualizer_slave_t * v;
  gavl_audio_frame_t * audio_frame;
  gavl_time_t diff_time, current_time;
  
  v = (bg_visualizer_slave_t*)data;
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
    pthread_mutex_lock(&v->vis_mutex);
    
    /* Check if we should update audio */

    audio_frame = audio_buffer_get(v->audio_buffer);
    if(audio_frame)
      v->vis_plugin->update(v->vis_priv, audio_frame);
    
    /* Draw frame */
    
    if(!(v->do_ov))
      v->vis_plugin->draw_frame(v->vis_priv,
                                (gavl_video_frame_t*)0);
    else if(v->do_convert_video)
      {
      v->vis_plugin->draw_frame(v->vis_priv, v->video_frame_in);
      gavl_video_convert(v->video_cnv, v->video_frame_in, v->video_frame_out);
      }
    else
      v->vis_plugin->draw_frame(v->vis_priv, v->video_frame_out);
    
    pthread_mutex_unlock(&v->vis_mutex);
    
    /* Wait until we can show the frame */
    current_time = gavl_timer_get(v->timer);
    
    diff_time = v->last_frame_time +
      v->video_format_out.frame_duration - current_time;
    
    if(diff_time > GAVL_TIME_SCALE / 1000)
      gavl_time_delay(&diff_time);
    
    /* Show frame */
    
    if(v->do_ov)
      {
      pthread_mutex_lock(&v->ov_mutex);
      v->ov_plugin->put_video(v->ov_priv, v->video_frame_out);
      v->last_frame_time = gavl_timer_get(v->timer);
      
      v->ov_plugin->handle_events(v->ov_priv);
      pthread_mutex_unlock(&v->ov_mutex);
      }
    else
      {
      pthread_mutex_lock(&v->vis_mutex);
      v->vis_plugin->show_frame(v->vis_priv);
      v->last_frame_time = gavl_timer_get(v->timer);
      pthread_mutex_unlock(&v->vis_mutex);
      }
    }
  
  pthread_mutex_unlock(&v->running_mutex);
  return (void*)0;
  }

static int bg_visualizer_slave_stop(bg_visualizer_slave_t * v)
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

static int bg_visualizer_slave_start(bg_visualizer_slave_t * v)
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

static void
bg_visualizer_slave_set_audio_format(bg_visualizer_slave_t * v,
                                     const gavl_audio_format_t * format)
  {
  int was_running;
  was_running = bg_visualizer_slave_stop(v);
  pthread_mutex_lock(&v->audio_buffer->in_mutex);
  
  gavl_audio_format_copy(&v->audio_format_in, format);

  if(was_running)
    audio_buffer_init(v->audio_buffer, &v->audio_format_in, &v->audio_format_out);
  pthread_mutex_unlock(&v->audio_buffer->in_mutex);
  if(was_running)
    bg_visualizer_slave_start(v);
  }

static void cleanup_plugin(bg_visualizer_slave_t * v)
  {
  }

static void init_plugin(bg_visualizer_slave_t * v)
  {
  gavl_audio_format_copy(&v->audio_format_out, &v->audio_format_in);

  /* Set members, which might be missing */
  v->video_format_in.pixel_width  = 1;
  v->video_format_in.pixel_height = 1;
  
  /* Set video format */
  gavl_video_format_copy(&v->video_format_in_real, &v->video_format_in);
  
  /* Open visualizer plugin */
  
  if(v->do_ov)
    {
    v->vis_plugin->open_ov(v->vis_priv, &v->audio_format_out,
                           v->ov_plugin,
                           v->ov_priv,
                           &v->video_format_in_real);
    
    gavl_video_format_copy(&v->video_format_out, &v->video_format_in_real);
    
    /* Open OV Plugin */
    v->ov_plugin->open(v->ov_priv, &v->video_format_out);
    
    /* Initialize video converter */
    
    v->do_convert_video =
      gavl_video_converter_init(v->video_cnv, &v->video_format_in_real,
                                &v->video_format_out);

    if(v->ov_plugin->create_frame)
      v->video_frame_out = v->ov_plugin->create_frame(v->ov_priv);
    else
      v->video_frame_out = gavl_video_frame_create(&v->video_format_out);
    
    if(v->do_convert_video)
      v->video_frame_in = gavl_video_frame_create(&v->video_format_in_real);
    }
  else
    {
    v->vis_plugin->open_win(v->vis_priv, &v->audio_format_out,
                            v->window_id);
    
    gavl_video_format_copy(&v->video_format_out, &v->video_format_in);
    }
  
  audio_buffer_init(v->audio_buffer, &v->audio_format_in, &v->audio_format_out);

  }


static int msg_read_callback(void * priv, uint8_t * data, int len)
  {
  return read(STDIN_FILENO, data, len);
  }

int main(int argc, char ** argv)
  {
  gavl_audio_format_t audio_format;
  gavl_audio_frame_t * audio_frame = (gavl_audio_frame_t *)0;
  float arg_f;
  
  int keep_going;
  bg_visualizer_slave_t * s;
  bg_msg_t * msg;

  char * parameter_name = (char*)0;
  bg_parameter_value_t parameter_value;

  bg_parameter_type_t parameter_type;
  memset(&parameter_value, 0, sizeof(parameter_value));
  
  if(isatty(fileno(stdin)))
    {
    fprintf(stderr, "This program is not meant to be started from the commandline.\nThe official frontend API for visualizatons is in " PREFIX "/include/gmerlin/visualize.h\n");
    return -1;
    }
  fprintf(stderr, "slave started\n");
        
  s = bg_visualizer_slave_create(argc, argv);

  msg = bg_msg_create();

  keep_going = 1;
  
  while(keep_going)
    {
    
    if(!bg_msg_read(msg, msg_read_callback, (void*)0))
      break;
    
    switch(bg_msg_get_id(msg))
      {
      case BG_VIS_MSG_AUDIO_FORMAT:
        fprintf(stderr, "Got audio format\n");
        bg_msg_get_arg_audio_format(msg, 0, &audio_format);
        gavl_audio_format_dump(&audio_format);
        bg_visualizer_slave_set_audio_format(s, &audio_format);
        if(audio_frame)
          gavl_audio_frame_destroy(audio_frame);
        audio_frame = gavl_audio_frame_create(&audio_format);
        break;
      case BG_VIS_MSG_AUDIO_DATA:
        bg_msg_read_audio_frame(msg,
                                &audio_format,
                                audio_frame,
                                msg_read_callback,
                                (void*)0);
        audio_buffer_put(s->audio_buffer,
                         audio_frame);
        break;
      case BG_VIS_MSG_VIS_PARAM:
        bg_msg_get_parameter(msg,
                             &parameter_name,
                             &parameter_type,
                             &parameter_value);
        
        fprintf(stderr, "Got vis parameter %s\n", parameter_name);
        
        pthread_mutex_lock(&s->vis_mutex);
        s->vis_plugin->common.set_parameter(s->vis_priv,
                                            parameter_name,
                                            &parameter_value);
        pthread_mutex_unlock(&s->vis_mutex);
        if(parameter_name)
          {
          free(parameter_name);
          parameter_name = (char*)0;
          bg_parameter_value_free(&parameter_value,
                                  parameter_type);
          }
        
        break;
      case BG_VIS_MSG_OV_PARAM:
        bg_msg_get_parameter(msg,
                             &parameter_name,
                             &parameter_type,
                             &parameter_value);
        fprintf(stderr, "Got ov parameter %s\n", parameter_name);
        
        pthread_mutex_lock(&s->ov_mutex);


        s->ov_plugin->common.set_parameter(s->ov_priv,
                                           parameter_name,
                                           &parameter_value);
        pthread_mutex_unlock(&s->ov_mutex);
        if(parameter_name)
          {
          free(parameter_name);
          parameter_name = (char*)0;
          bg_parameter_value_free(&parameter_value,
                                  parameter_type);
          }
        break;
      case BG_VIS_MSG_GAIN:
        arg_f = bg_msg_get_arg_float(msg, 0);
        fprintf(stderr, "Got gain %f\n",arg_f);
        audio_buffer_set_gain(s->audio_buffer, arg_f);
        break;
      case BG_VIS_MSG_FPS:
        s->video_format_in.timescale = GAVL_TIME_SCALE;
        s->video_format_in.frame_duration =
          (int)(GAVL_TIME_SCALE / bg_msg_get_arg_float(msg, 0));
        fprintf(stderr, "Got fps %d/%d\n",
                s->video_format_in.timescale,
                s->video_format_in.frame_duration);
        break;
      case BG_VIS_MSG_IMAGE_SIZE:
        s->video_format_in.image_width =
          bg_msg_get_arg_int(msg, 0);
        s->video_format_in.image_height =
          bg_msg_get_arg_int(msg, 1);

        s->video_format_in.frame_width =
          s->video_format_in.image_width;
        s->video_format_in.frame_height =
          s->video_format_in.image_height;
        fprintf(stderr, "Got image size %dx%d\n",
                s->video_format_in.frame_width,
                s->video_format_in.frame_height);
        break;
      case BG_VIS_MSG_START:
        fprintf(stderr, "Starting thread\n");
        init_plugin(s);
        bg_visualizer_slave_start(s);
        break;
      case BG_VIS_MSG_QUIT:
        keep_going = 0;
        break;
      }
    }
  bg_visualizer_slave_stop(s);
  cleanup_plugin(s);
  
  bg_visualizer_slave_destroy(s);
  bg_msg_free(msg);
  return 0;
  }
