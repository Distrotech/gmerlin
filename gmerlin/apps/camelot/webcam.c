/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/

#include <pthread.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <config.h>
#include <gmerlin/utils.h>
#include <gmerlin/plugin.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/filters.h>
#include <gmerlin/translation.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "webcam"

#include "webcam.h"
#include "webcam_priv.h"


#define FRAMERATE_INTERVAL 10


static int read_func(void * data, gavl_video_frame_t * f, int stream)
  {
  gmerlin_webcam_t * w = data;
  bg_plugin_lock(w->input_handle);
  
  if(!w->input->read_video(w->input_handle->priv, f, 0))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "read_frame failed");
    }
  bg_plugin_unlock(w->input_handle);
  return 1;
  }

void gmerlin_webcam_get_message_queues(gmerlin_webcam_t * cam,
                                       bg_msg_queue_t ** cmd_queue,
                                       bg_msg_queue_t ** msg_queue)
  {
  *cmd_queue = cam->cmd_queue;
  *msg_queue = cam->msg_queue;
  }

gmerlin_webcam_t * gmerlin_webcam_create(bg_plugin_registry_t * plugin_reg)
  {
  gmerlin_webcam_t * ret;

  ret = calloc(1, sizeof(*ret));
  pthread_mutex_init(&ret->mutex, NULL);

  ret->plugin_reg = plugin_reg;

  bg_gavl_video_options_init(&ret->opt);
  
  ret->fc = bg_video_filter_chain_create(&ret->opt, ret->plugin_reg);
  
  ret->cmd_queue = bg_msg_queue_create();
  ret->msg_queue = bg_msg_queue_create();
  
  ret->monitor_cnv = gavl_video_converter_create();
  ret->capture_cnv = gavl_video_converter_create();

  ret->timer = gavl_timer_create();

#ifdef HAVE_V4L
  ret->vloopback = bg_vloopback_create();
  pthread_mutex_init(&ret->vloopback_mutex, NULL);
#endif
  
  return ret;
  }

void gmerlin_webcam_destroy(gmerlin_webcam_t * w)
  {
  /* First, we destroy the video frames. This must be done while the
     plugins aren't unrefed */

  if(w->input_frame)
    gavl_video_frame_destroy(w->input_frame);
  if(w->capture_frame)
    gavl_video_frame_destroy(w->capture_frame);

  if(w->monitor_frame)
    {
    if(w->monitor->destroy_frame)
      w->monitor->destroy_frame(w->monitor_handle->priv, w->monitor_frame);
    else
      gavl_video_frame_destroy(w->monitor_frame);
    }

  if(w->capture_directory)
    free(w->capture_directory);

  if(w->capture_namebase)
    free(w->capture_namebase);
    
  if(w->input_handle)
    bg_plugin_unref(w->input_handle);
  if(w->monitor_handle)
    bg_plugin_unref(w->monitor_handle);
  if(w->capture_handle)
    bg_plugin_unref(w->capture_handle);
  bg_msg_queue_destroy(w->cmd_queue);
  bg_msg_queue_destroy(w->msg_queue);

  if(w->fc)
    bg_video_filter_chain_destroy(w->fc);

  bg_gavl_video_options_free(&w->opt);
  
  if(w->capture_cnv)
    gavl_video_converter_destroy(w->capture_cnv);
  if(w->monitor_cnv)
    gavl_video_converter_destroy(w->monitor_cnv);
  
  gavl_timer_destroy(w->timer);
  pthread_mutex_destroy(&w->mutex);
  
#ifdef HAVE_V4L
  bg_vloopback_destroy(w->vloopback);
  pthread_mutex_destroy(&w->vloopback_mutex);
#endif  
  
  free(w);
  }

#define BUFFER_SIZE 256

static char * create_filename(gmerlin_webcam_t * cam)
  {
  char mask[16];
  char buf[BUFFER_SIZE];
  char * pos;
  char * end;
  char * filename;
  int have_time = 0;
  time_t t;
  struct tm time_date;
  
  filename = bg_sprintf("%s/", cam->capture_directory);
  
  pos = cam->capture_namebase;
  
  while(1)
    {
    end = pos;

    while((*end != '%') && (*end != '\0'))
      {
      end++;
      }

    if(end - pos)
      filename = bg_strncat(filename, pos, end);
    
    if(*end == '\0')
      break;

    pos = end;
    
    /* Insert frame count */
    if(isdigit(pos[1]) && (pos[2] == 'n'))
      {
      mask[0] = '%';
      mask[1] = '0';
      mask[2] = pos[1];
      mask[3] = 'd';
      mask[4] = '\0';
      sprintf(buf, mask, cam->capture_frame_counter);
      filename = bg_strcat(filename, buf);
      pos += 3;
      }
    /* Insert date */
    else if(pos[1] == 'd')
      {
      if(!have_time)
        {
        time(&t);
        localtime_r(&t, &time_date);
        have_time = 1;
        }
      strftime(buf, BUFFER_SIZE, "%Y-%m-%d", &time_date);
      filename = bg_strcat(filename, buf);
      pos += 2;
      }
    /* Insert date */
    else if(pos[1] == 't')
      {
      if(!have_time)
        {
        time(&t);
        localtime_r(&t, &time_date);
        have_time = 1;
        }
      strftime(buf, BUFFER_SIZE, "%H-%M-%S", &time_date);
      filename = bg_strcat(filename, buf);
      pos += 2;
      }
    else
      {
      filename = bg_strcat(filename, "%");
      pos++;
      }
    }
  
  return filename;
  }

static void do_capture(gmerlin_webcam_t * cam)
  {
  bg_msg_t * msg;
  char * filename = NULL;

  //   char * capture_directory;
  //   char * capture_namebase;
    
  if(!cam->capture)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "do_capture: No plugin loaded");
    goto fail;
    }
  
  /* Construct filename */

  filename = create_filename(cam);
  
  if(!cam->capture_initialized)
    {
    if(cam->capture_frame)
      gavl_video_frame_destroy(cam->capture_frame);
    gavl_video_format_copy(&cam->capture_format, &cam->input_format);
    }
  
  if(!cam->capture->write_header(cam->capture_handle->priv,
                                 filename, &cam->capture_format, NULL))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Saving %s failed", filename);
    goto fail;
    }

  if(!cam->capture_initialized)
    {
    cam->do_convert_capture =
      gavl_video_converter_init(cam->capture_cnv,
                      &cam->input_format, &cam->capture_format);
    cam->capture_frame = gavl_video_frame_create(&cam->capture_format);
    }

  if(cam->do_convert_capture)
    gavl_video_convert(cam->capture_cnv, cam->input_frame, cam->capture_frame);
  else
    gavl_video_frame_copy(&cam->input_format,
                          cam->capture_frame, cam->input_frame);

  cam->capture->write_image(cam->capture_handle->priv, cam->capture_frame);

  /* Update capture frame counter */
  
  cam->capture_frame_counter++;
  msg = bg_msg_queue_lock_write(cam->msg_queue);
  bg_msg_set_id(msg, MSG_FRAME_COUNT);
  bg_msg_set_arg_int(msg, 0, cam->capture_frame_counter);
  bg_msg_queue_unlock_write(cam->msg_queue);
  cam->capture_initialized = 1;
  
  fail:
  if(filename)
    free(filename);
  }

static void open_monitor(gmerlin_webcam_t * cam)
  {
  if(!cam->input_open)
    return;
  gavl_video_format_copy(&cam->monitor_format, &cam->input_format);

  /* Open monitor */
  
  if(!cam->monitor->open(cam->monitor_handle->priv, &cam->monitor_format, 1))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Opening monitor plugin failed");
    }
  cam->monitor->set_window_title(cam->monitor_handle->priv,
                                 "Camelot");
  
  if(cam->monitor->show_window)
    cam->monitor->show_window(cam->monitor_handle->priv, 1);
  
  //  gavl_video_format_dump(&cam->monitor_format);

  /* Fire up video converter */
  
  cam->do_convert_monitor =
    gavl_video_converter_init(cam->monitor_cnv, &cam->input_format,
                    &cam->monitor_format);
  
  /* Allocate video image */
  
  if(cam->monitor->create_frame)
    cam->monitor_frame = cam->monitor->create_frame(cam->monitor_handle->priv);
  else
    cam->monitor_frame = gavl_video_frame_create(&cam->monitor_format);
  cam->monitor_open = 1;

  //  fprintf(stderr, "Monitor format:\n");
  //  gavl_video_format_dump(&cam->monitor_format);
  
  }

static void close_monitor(gmerlin_webcam_t * cam)
  {
  if(!cam->monitor_open)
    return;

  if(cam->monitor->destroy_frame)
    cam->monitor->destroy_frame(cam->monitor_handle->priv, cam->monitor_frame);
  else
    gavl_video_frame_destroy(cam->monitor_frame);
  
  cam->monitor_frame = NULL;
  cam->monitor->close(cam->monitor_handle->priv);
  if(cam->monitor->show_window)
    cam->monitor->show_window(cam->monitor_handle->priv, 0);
  cam->monitor_open = 0;
  }

static void build_pipeline(gmerlin_webcam_t * cam)
  {
  cam->read_func = read_func;
  cam->read_data = cam;
  cam->read_stream = 0;
  
  bg_video_filter_chain_connect_input(cam->fc, cam->read_func,cam->read_data,
                                      cam->read_stream);

  bg_video_filter_chain_reset(cam->fc);
  
  if(bg_video_filter_chain_init(cam->fc, &cam->cam_format, &cam->input_format))
    {
    bg_video_filter_chain_set_out_format(cam->fc, &cam->input_format);
    cam->read_func = bg_video_filter_chain_read;
    cam->read_data = cam->fc;
    cam->read_stream = 0;
    }

  if(cam->input_frame)
    gavl_video_frame_destroy(cam->input_frame);
  
  cam->input_frame = gavl_video_frame_create(&cam->input_format);
  
  //  fprintf(stderr, "Cam format:\n");
  //  gavl_video_format_dump(&cam->cam_format);
  
  //  fprintf(stderr, "Input format:\n");
  //  gavl_video_format_dump(&cam->input_format);

#ifdef HAVE_V4L
  bg_vloopback_set_format(cam->vloopback, &cam->input_format);
#endif
  
  }

static void open_input(gmerlin_webcam_t * cam)
  {
  bg_msg_t * msg;
  if(!cam->input->open(cam->input_handle->priv, NULL, &cam->cam_format))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Initializing %s failed, check settings",
           cam->input_handle->info->long_name);
    msg = bg_msg_queue_lock_write(cam->msg_queue);
    bg_msg_set_id(msg, MSG_ERROR);
    bg_msg_queue_unlock_write(cam->msg_queue);

    cam->input_open = 0;
    return;
    }

  build_pipeline(cam);
  cam->input_open = 1;
  }

static void close_input(gmerlin_webcam_t * cam)
  {
  if(!cam->input_open)
    return;
  cam->input->close(cam->input_handle->priv);
  cam->input_open = 0;
  }

static void handle_cmd(gmerlin_webcam_t * cam, bg_msg_t * msg)
  {
  bg_plugin_handle_t * h;
  int arg_i;
  float arg_f;
  
  switch(bg_msg_get_id(msg))
    {
    case CMD_QUIT:
      cam->do_quit = 1;
      break;
    case CMD_SET_MONITOR:               /* ARG0: int */
      arg_i = bg_msg_get_arg_int(msg, 0);
      if(arg_i == cam->do_monitor)
        return;
      if(arg_i)
        open_monitor(cam);
      else
        close_monitor(cam);
      cam->do_monitor = arg_i; 
      break;
    case CMD_SET_MONITOR_PLUGIN:        /* ARG0: plugin_handle */
      if(cam->monitor)
        {
        close_monitor(cam);
        bg_plugin_unref(cam->monitor_handle);
        }
      h = bg_msg_get_arg_ptr_nocopy(msg, 0);

      cam->monitor_handle = h;
      bg_plugin_ref(cam->monitor_handle);
      cam->monitor = (bg_ov_plugin_t*)cam->monitor_handle->plugin;

      if(cam->do_monitor)
        open_monitor(cam);
      break;
    case CMD_SET_INPUT_PLUGIN:          /* ARG0: plugin_handle */
      if(cam->do_monitor)
        close_monitor(cam);
      
      if(cam->input)
        {
        close_input(cam);
        bg_plugin_unref(cam->input_handle);
        }
      h = bg_msg_get_arg_ptr_nocopy(msg, 0);
      cam->input_handle = h;

      bg_plugin_ref(cam->input_handle);

      cam->input = (bg_recorder_plugin_t*)cam->input_handle->plugin;
      open_input(cam);

      if(cam->do_monitor)
        open_monitor(cam);
      
      break;
    case CMD_INPUT_REOPEN:
      if(cam->do_monitor)
        close_monitor(cam);
      close_input(cam);
      open_input(cam);
      
      if(cam->do_monitor)
        open_monitor(cam);

      cam->capture_initialized = 0;

      break;
    case CMD_SET_CAPTURE_PLUGIN:
      if(cam->capture_handle)
        {
        bg_plugin_unref(cam->capture_handle);
        }
      h = bg_msg_get_arg_ptr_nocopy(msg, 0);

      cam->capture_handle = h;
      bg_plugin_ref(cam->capture_handle);
      cam->capture = (bg_image_writer_plugin_t*)cam->capture_handle->plugin;

      cam->capture_initialized = 0;
      break;
    case CMD_SET_CAPTURE_AUTO:
      arg_i = bg_msg_get_arg_int(msg, 0);
      if(!cam->auto_capture && arg_i)
        cam->next_capture_time = gavl_timer_get(cam->timer);
      cam->auto_capture = arg_i;
      break;
    case CMD_SET_CAPTURE_INTERVAL:      /* ARG0: float */
      arg_f = bg_msg_get_arg_float(msg, 0);
      cam->capture_interval = (gavl_time_t)(GAVL_TIME_SCALE * arg_f);
      break;
    case CMD_SET_CAPTURE_DIRECTORY:     /* ARG0: string */
      if(cam->capture_directory)
        free(cam->capture_directory);
      cam->capture_directory = bg_msg_get_arg_string(msg, 0);
      break;
    case CMD_SET_CAPTURE_NAMEBASE:      /* ARG0: string */
      if(cam->capture_namebase)
        free(cam->capture_namebase);
      cam->capture_namebase = bg_msg_get_arg_string(msg, 0);
      break;
    case CMD_SET_CAPTURE_FRAME_COUNTER: /* ARG0: int */
      arg_i = bg_msg_get_arg_int(msg, 0);
      cam->capture_frame_counter = arg_i;
      break;
    case CMD_DO_CAPTURE:
      do_capture(cam);
      break;
#ifdef HAVE_V4L
    case CMD_SET_VLOOPBACK:
      arg_i = bg_msg_get_arg_int(msg, 0);
      cam->do_vloopback = 0;
      if(arg_i)
        {
        if(bg_vloopback_open(cam->vloopback))
          cam->do_vloopback = 1;
        }
      else
        {
        bg_vloopback_close(cam->vloopback);
        cam->do_vloopback = 0;
        }
      break;
#endif
    }
  }

static void * thread_func(void * data)
  {
  bg_msg_t * msg;
  gavl_time_t frame_time;
  gavl_time_t last_frame_time;
  gmerlin_webcam_t * w;
  float frame_rate;
  
  w = (gmerlin_webcam_t *)data;

  last_frame_time = gavl_timer_get(w->timer);
  w->frame_counter = 0;
  gavl_timer_start(w->timer);
  
  while(1)
    {
    /* Get frame from camera */
    if(w->input_open)
      {
      if(!w->read_func(w->read_data, w->input_frame, w->read_stream))
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "read_frame failed");
      frame_time = gavl_timer_get(w->timer);
      
      /* Send to monitor plugin */
      if(w->do_monitor && w->monitor_open)
        {
        if(w->do_convert_monitor)
          gavl_video_convert(w->monitor_cnv, w->input_frame, w->monitor_frame);
        else
          gavl_video_frame_copy(&w->input_format,
                                w->monitor_frame, w->input_frame);
        bg_plugin_lock(w->monitor_handle);
        w->monitor->put_video(w->monitor_handle->priv, w->monitor_frame);
        if(w->monitor->handle_events)
          w->monitor->handle_events(w->monitor_handle->priv);
        bg_plugin_unlock(w->monitor_handle);
        
        }
      /* Check if we have to do auto capture */
      if(w->capture && w->auto_capture &&
         (frame_time >= w->next_capture_time))
        {
        do_capture(w);
        w->next_capture_time += w->capture_interval;
        }
#ifdef HAVE_V4L
      pthread_mutex_lock(&w->vloopback_mutex);
      if(w->do_vloopback)
        bg_vloopback_put_frame(w->vloopback, w->input_frame);
      pthread_mutex_unlock(&w->vloopback_mutex);
#endif      
      /* Calculate framerate */
      
      w->frame_counter++;
      if(w->frame_counter == FRAMERATE_INTERVAL)
        w->frame_counter = 0;
      
      if(!w->frame_counter)
        {
        frame_rate = (double)(FRAMERATE_INTERVAL*GAVL_TIME_SCALE) /
          (double)(frame_time - last_frame_time);
        
        msg = bg_msg_queue_lock_write(w->msg_queue);
        bg_msg_set_id(msg, MSG_FRAMERATE);
        bg_msg_set_arg_float(msg, 0, frame_rate);
        bg_msg_queue_unlock_write(w->msg_queue);
        last_frame_time = frame_time;

        }
      }
    else
      {
      gavl_time_t delay_time = GAVL_TIME_SCALE / 100;
      gavl_time_delay(&delay_time);
      }
    
    /* Check for commands */
    
    while((msg = bg_msg_queue_try_lock_read(w->cmd_queue)))
      {
      handle_cmd(w, msg);
      bg_msg_queue_unlock_read(w->cmd_queue);
      }
    if(w->do_quit)
      break;
    }
  
  return NULL;
  }

void gmerlin_webcam_run(gmerlin_webcam_t * w)
  {
  pthread_create(&w->thread, NULL,
                 thread_func, w);
  w->running = 1;
  }

void gmerlin_webcam_quit(gmerlin_webcam_t * w)
  {
  bg_msg_t * msg;
  msg = bg_msg_queue_lock_write(w->cmd_queue);
  bg_msg_set_id(msg, CMD_QUIT);
  bg_msg_queue_unlock_write(w->cmd_queue);
  
  pthread_join(w->thread, NULL);
  w->running = 0;
  w->do_quit = 0;
  }


const bg_parameter_info_t *
gmerlin_webcam_get_filter_parameters(gmerlin_webcam_t * w)
  {
  return bg_video_filter_chain_get_parameters(w->fc);
  }

void gmerlin_webcam_set_filter_parameter(void * data, const char * name,
                                         const bg_parameter_value_t * val)
  {
  int need_rebuild = 0, need_restart = 0;
  gmerlin_webcam_t * w = data;

  //  fprintf(stderr, "Set filter parameter %s\n",
  //          name);
  
  //  pthread_mutex_lock(&p->video_stream.config_mutex);
  //  is_interrupted = p->video_stream.interrupted;
  //  pthread_mutex_unlock(&p->video_stream.config_mutex);
  
  bg_video_filter_chain_lock(w->fc);
  bg_video_filter_chain_set_parameter(w->fc, name, val);
  
  need_rebuild =
    bg_video_filter_chain_need_rebuild(w->fc);
  need_restart =
    bg_video_filter_chain_need_restart(w->fc);
  
  bg_video_filter_chain_unlock(w->fc);
  
  if(w->running && (need_rebuild || need_restart))
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Restarting playback due to changed video filters");
    gmerlin_webcam_quit(w);
    w->interrupted = 1;
    
    if(w->do_monitor)
      close_monitor(w);
    }
  if(!name && w->interrupted)
    {
    build_pipeline(w);
    if(w->do_monitor)
      open_monitor(w);
    w->interrupted = 0;
    gmerlin_webcam_run(w);
    w->capture_initialized = 0;
    //    fprintf(stderr, "Restarting\n");
    }
  }

#ifdef HAVE_V4L
const bg_parameter_info_t *
gmerlin_webcam_get_vloopback_parameters(gmerlin_webcam_t * w)
  {
  return bg_vloopback_get_parameters(w->vloopback);
  }

void gmerlin_webcam_set_vloopback_parameter(void * data, const char * name,
                                            const bg_parameter_value_t * val)
  {
  gmerlin_webcam_t * w = data;
  pthread_mutex_lock(&w->vloopback_mutex);
  bg_vloopback_set_parameter(w->vloopback, name, val);

  if(bg_vloopback_changed(w->vloopback) && w->do_vloopback)
    {
    bg_vloopback_close(w->vloopback);
    if(!bg_vloopback_open(w->vloopback))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Restarting vloopback failed, close client and try again");
      w->do_vloopback = 0;
      }
    }

  pthread_mutex_unlock(&w->vloopback_mutex);
  }
#endif

static const bg_parameter_info_t parameters[] =
  {
    {
      .name = "video_capture",
      .long_name = TRS("Video capture"),
      .type = BG_PARAMETER_SECTION,
    },
    {
      .name      = "max_framerate",
      .long_name = TRS("Maximum framerate"),
      .type      = BG_PARAMETER_INT,
      .val_min       = { .val_i = 1 },
      .val_max       = { .val_i = 100 },
      .val_default   = { .val_i = 25 },
    },
    { /* End */ }
  };

const bg_parameter_info_t *
gmerlin_webcam_get_parameters(gmerlin_webcam_t * w)
  {
  return parameters;
  }

void
gmerlin_webcam_set_parameter(void * data, const char * name,
                             const bg_parameter_value_t * val)
  {
  
  }
