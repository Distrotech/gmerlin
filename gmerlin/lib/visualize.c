#include <string.h>

#include <gavl/gavl.h>

#include <config.h>
#include <translation.h>

#include <pluginregistry.h>
#include <visualize.h>
#include <utils.h>
#include <subprocess.h>
#include <msgqueue.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/signal.h>

#include <log.h>

#define LOG_DOMAIN "visualizer"

/*
 * gmerlin_visualize_slave
 *   -w "window_id"
 *   -p "plugin_module"
 *   -o "output_module"
 */

struct bg_visualizer_s
  {
  bg_msg_t * msg;
  bg_plugin_registry_t * plugin_reg;

  bg_plugin_handle_t * ov_handle;
  bg_ov_plugin_t * ov_plugin;
  
  pthread_mutex_t mutex;
  
  int changed;
  
  bg_subprocess_t * proc;
  const bg_plugin_info_t * vis_info;
  const bg_plugin_info_t * ov_info;

  int image_width;
  int image_height;
  float framerate;
  float gain;
  
  sigset_t oldset;
  gavl_audio_format_t audio_format;
  
  const char * display_string;
  };

static int proc_write_func(void * data, uint8_t * ptr, int len)
  {
  bg_visualizer_t * v = (bg_visualizer_t *)data;
  return write(v->proc->stdin_fd, ptr, len);
  }

static void write_message(bg_visualizer_t * v)
  {
  if(!v->proc)
    {
    bg_msg_free(v->msg);
    return;
    }
  if(!bg_msg_write(v->msg, proc_write_func, v))
    {
    bg_subprocess_close(v->proc);
    v->proc = (bg_subprocess_t*)0;
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Visualization process crashed, restart to try again");
    }
  bg_msg_free(v->msg);
  }

bg_visualizer_t *
bg_visualizer_create(bg_plugin_registry_t * plugin_reg)
  {
  bg_visualizer_t * ret;
  ret = calloc(1, sizeof(*ret));

  ret->plugin_reg = plugin_reg;

  pthread_mutex_init(&(ret->mutex),(pthread_mutexattr_t *)0);
  
  ret->msg = bg_msg_create();
  return ret;
  }

void bg_visualizer_destroy(bg_visualizer_t * v)
  {
  pthread_mutex_destroy(&(v->mutex));
  
  
  free(v);
  }


static bg_parameter_info_t parameters[] =
  {
    {
      name:        "width",
      long_name:   TRS("Width"),
      type:        BG_PARAMETER_INT,
      flags:      BG_PARAMETER_SYNC,
      val_min:     { val_i: 16 },
      val_max:     { val_i: 32768 },
      val_default: { val_i: 320 },
      help_string: TRS("Desired image with, the visualization plugin might override this for no apparent reason"),
    },
    {
      name:       "height",
      long_name:  TRS("Height"),
      type:       BG_PARAMETER_INT,
      flags:      BG_PARAMETER_SYNC,
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

bg_parameter_info_t * bg_visualizer_get_parameters(bg_visualizer_t * v)
  {
  return parameters;
  }

static void set_ov_parameter(void * data,
                             const char * name,
                             const bg_parameter_value_t * val)
  {
  const bg_parameter_info_t * info;
  bg_visualizer_t * v = (bg_visualizer_t*)data;

  bg_msg_set_id(v->msg, BG_VIS_MSG_OV_PARAM);
  
  if(name)
    {
    info = bg_parameter_find(v->ov_info->parameters,
                             name);
    if(!info)
      return;
    bg_msg_set_parameter(v->msg, name, info->type, val);
    }
  else
    bg_msg_set_parameter(v->msg, name, 0, NULL);
  
  write_message(v);
  }

static void set_vis_parameter(void * data,
                              const char * name,
                              const bg_parameter_value_t * val)
  {
  const bg_parameter_info_t * info;
  bg_visualizer_t * v = (bg_visualizer_t*)data;

  bg_msg_set_id(v->msg, BG_VIS_MSG_VIS_PARAM);
  
  if(name)
    {
    info = bg_parameter_find(v->vis_info->parameters,
                             name);
    if(!info)
      return;
    bg_msg_set_parameter(v->msg, name, info->type, val);
    }
  else
    bg_msg_set_parameter(v->msg, name, 0, NULL);
  
  write_message(v);
  
  }

static void set_gain(bg_visualizer_t * v)
  {
  bg_msg_set_id(v->msg, BG_VIS_MSG_GAIN);
  bg_msg_set_arg_float(v->msg, 0, v->gain);
  write_message(v);
  }

void bg_visualizer_set_vis_plugin(bg_visualizer_t * v,
                                  const bg_plugin_info_t * info)
  {
  if(!v->vis_info || strcmp(v->vis_info->name, info->name))
    {
    v->changed = 1;
    v->vis_info = info;
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN, "Got visualization plugin %s",
           v->vis_info->name);
    }
  }

void bg_visualizer_set_vis_parameter(void * priv,
                                     const char * name,
                                     const bg_parameter_value_t * val)
  {
  bg_visualizer_t * v;
  v = (bg_visualizer_t *)priv;
  pthread_mutex_lock(&v->mutex);
  if(v->proc)
    set_vis_parameter(v, name, val);
  pthread_mutex_unlock(&v->mutex);
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
  
  if(!strcmp(name, "width"))
    {
    if(v->image_width != val->val_i)
      {
      v->image_width = val->val_i;
      v->changed = 1;
      }
    }
  else if(!strcmp(name, "height"))
    {
    if(v->image_height != val->val_i)
      {
      v->image_height = val->val_i;
      v->changed = 1;
      }
    }
  else if(!strcmp(name, "framerate"))
    {
    v->framerate = val->val_f;
    }
  else if(!strcmp(name, "gain"))
    {
    v->gain = val->val_f;
    
    pthread_mutex_lock(&v->mutex);
    if(v->proc)
      set_gain(v);
    pthread_mutex_unlock(&v->mutex);
    }
  }

int bg_visualizer_stop(bg_visualizer_t * v)
  {
  pthread_mutex_lock(&v->mutex);
  
  if(!v->proc)
    {
    pthread_mutex_unlock(&v->mutex);
    return 0;
    }
  
  /* Stop process */
  bg_msg_set_id(v->msg, BG_VIS_MSG_QUIT);
  write_message(v);

  bg_subprocess_close(v->proc);
  v->proc = (bg_subprocess_t*)0;

  /* Restore signal mask */
  pthread_sigmask(SIG_SETMASK, &v->oldset, NULL);
  pthread_mutex_unlock(&v->mutex);
  return 1;
  }


int bg_visualizer_start(bg_visualizer_t * v)
  {
  char * command;
  bg_cfg_section_t * section;
  sigset_t newset;
  pthread_mutex_lock(&v->mutex);
  
  if(v->proc)
    {
    pthread_mutex_unlock(&v->mutex);
    return 0;
    }
  
  if(!v->vis_info)
    {
    pthread_mutex_unlock(&v->mutex);
    return 0;
    }

  /* Block SIGPIPE */
  sigemptyset(&newset);
  sigaddset(&newset, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &newset, &v->oldset);
  
  /* Start process */
    
  if(v->vis_info->flags & BG_PLUGIN_VISUALIZE_FRAME)
    {
    command = bg_sprintf("gmerlin_visualizer_slave  -w %s -o %s -p %s",
                         v->display_string,
                         v->ov_info->module_filename,
                         v->vis_info->module_filename);
    }
  else
    {
    command = bg_sprintf("gmerlin_visualizer_slave -w %s -p %s",
                         v->display_string,
                         v->vis_info->module_filename);
    }
  v->proc = bg_subprocess_create(command, 1, 0, 0);
  if(!v->proc)
    return 0;

  free(command);
  
  /* Audio format */
  bg_msg_set_id(v->msg, BG_VIS_MSG_AUDIO_FORMAT);
  bg_msg_set_arg_audio_format(v->msg, 0, &v->audio_format);
  write_message(v);
  
  /* default ov parameters */
  if(v->ov_info->parameters &&
     (v->vis_info->flags & BG_PLUGIN_VISUALIZE_FRAME))
    {
    section = bg_plugin_registry_get_section(v->plugin_reg,
                                             v->ov_info->name);
    bg_cfg_section_apply(section,
                         v->ov_info->parameters,
                         set_ov_parameter, v);
    }
  
  /* default vis parameters */
  if(v->vis_info->parameters)
    {
    section = bg_plugin_registry_get_section(v->plugin_reg,
                                             v->vis_info->name);
    bg_cfg_section_apply(section,
                         v->vis_info->parameters,
                         set_vis_parameter, v);
    }
  
  /* Gain */
  set_gain(v);
  /* Image size */

  bg_msg_set_id(v->msg, BG_VIS_MSG_IMAGE_SIZE);
  bg_msg_set_arg_int(v->msg, 0, v->image_width);
  bg_msg_set_arg_int(v->msg, 1, v->image_height);
  
  write_message(v);

  /* FPS*/
  bg_msg_set_id(v->msg,   BG_VIS_MSG_FPS);
  bg_msg_set_arg_float(v->msg, 0, v->framerate);
  write_message(v);
  
  /* All done, start */
  bg_msg_set_id(v->msg, BG_VIS_MSG_START);
  bg_msg_set_arg_float(v->msg, 0, v->gain);
  write_message(v);

  /* Show window */
  if(v->ov_plugin)
    {
    v->ov_plugin->show_window(v->ov_handle->priv, 1);
    v->ov_plugin->set_window_title(v->ov_handle->priv,
                                   v->vis_info->long_name);
    }
  pthread_mutex_unlock(&v->mutex);
  return 1;
  }

void bg_visualizer_set_audio_format(bg_visualizer_t * v,
                                    const gavl_audio_format_t * format)
  {
  pthread_mutex_lock(&v->mutex);
  
  gavl_audio_format_copy(&v->audio_format, format);

  if(v->proc)
    {
    bg_msg_set_id(v->msg, BG_VIS_MSG_AUDIO_FORMAT);
    bg_msg_set_arg_audio_format(v->msg, 0, &v->audio_format);
    write_message(v);
    }
  
  pthread_mutex_unlock(&v->mutex);
  
  //  fprintf(stderr, "bg_visualizer_set_audio_format done\n");
  }

void bg_visualizer_open_plugin(bg_visualizer_t * v,
                               const gavl_audio_format_t * format,
                               bg_plugin_handle_t * ov_handle)
  {
  /* Set audio format */
  
  gavl_audio_format_copy(&v->audio_format, format);
  
  /* Set ov plugin */
  v->ov_handle = ov_handle;
  v->ov_plugin = (bg_ov_plugin_t*)(ov_handle->plugin);
  
  v->ov_info = v->ov_handle->info;
  
  v->display_string = v->ov_plugin->get_window(v->ov_handle->priv);
  
  v->changed = 0;
  
  bg_visualizer_start(v);
  
  }

void bg_visualizer_open_id(bg_visualizer_t * v,
                           const gavl_audio_format_t * format,
                           const bg_plugin_info_t * ov_info,
                           const char * display_name)
  {
  /* Set audio format */
  
  gavl_audio_format_copy(&v->audio_format, format);
  
  /* Set ov plugin */
  v->ov_handle = (bg_plugin_handle_t*)0;
  v->ov_plugin = (bg_ov_plugin_t*)0;
  
  v->ov_info = ov_info;
  v->display_string = display_name;
  
  v->changed = 0;
  
  bg_visualizer_start(v);
  
  }


void bg_visualizer_update(bg_visualizer_t * v,
                          const gavl_audio_frame_t * frame)
  {
  pthread_mutex_lock(&v->mutex);

  if(!v->proc)
    {
    if(v->ov_plugin)
      v->ov_plugin->handle_events(v->ov_handle->priv);
    pthread_mutex_unlock(&v->mutex);
    return;
    }
  
  bg_msg_set_id(v->msg, BG_VIS_MSG_AUDIO_DATA);

  if(!bg_msg_write_audio_frame(v->msg,
                               &v->audio_format,
                               frame,
                               proc_write_func, v))
    {
    bg_subprocess_close(v->proc);
    v->proc = (bg_subprocess_t*)0;
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Visualization process crashed, restart to try again");
    }
  bg_msg_free(v->msg);

  if(v->ov_plugin)
    v->ov_plugin->handle_events(v->ov_handle->priv);
  
  pthread_mutex_unlock(&v->mutex);
  }


void bg_visualizer_close(bg_visualizer_t * v)
  {
  bg_visualizer_stop(v);
  }

int bg_visualizer_need_restart(bg_visualizer_t * v)
  {
  return v->changed;
  }
