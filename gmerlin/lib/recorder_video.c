/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <config.h>
#include <string.h>

#include <gmerlin/translation.h>

#include <gmerlin/recorder.h>
#include <recorder_private.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "recorder.video"

#define FRAMERATE_INTERVAL 10


void bg_recorder_create_video(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;

  vs->monitor_cnv = gavl_video_converter_create();
  vs->output_cnv = gavl_video_converter_create();

  bg_gavl_video_options_init(&(vs->opt));
  
  vs->fc = bg_video_filter_chain_create(&(vs->opt), rec->plugin_reg);

  vs->th = bg_player_thread_create(rec->tc);
  vs->timer = gavl_timer_create();
  pthread_mutex_init(&vs->config_mutex, NULL);
  }

void bg_recorder_destroy_video(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  
  gavl_video_converter_destroy(vs->monitor_cnv);
  gavl_video_converter_destroy(vs->output_cnv);
  bg_video_filter_chain_destroy(vs->fc);
  bg_player_thread_destroy(vs->th);
  gavl_timer_destroy(vs->timer);
  pthread_mutex_destroy(&vs->config_mutex);

  if(vs->monitor_handle)
    bg_plugin_unref(vs->monitor_handle);
  if(vs->input_handle)
    bg_plugin_unref(vs->input_handle);
  
  }

static const bg_parameter_info_t parameters[] =
  {
    {
      .name = "do_video",
      .long_name = TRS("Record video"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name      = "plugin",
      .long_name = TRS("Plugin"),
      .type      = BG_PARAMETER_MULTI_MENU,
      .flags     = BG_PARAMETER_PLUGIN,
    },
    {
      .name        = "limit_fps",
      .long_name   = TRS("fps limit"),
      .type        = BG_PARAMETER_FLOAT,
      .val_min     = { .val_f = 1.0 },
      .val_max     = { .val_f = 100.0 },
      .val_default = { .val_f = 25.0 },
      .num_digits  = 2,
      .help_string = TRS("Specify the maximum framerate for input plugins, which can capture images really fast"),
    },
    { },
  };

const bg_parameter_info_t *
bg_recorder_get_video_parameters(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  if(!vs->parameters)
    {
    vs->parameters = bg_parameter_info_copy_array(parameters);
    
    bg_plugin_registry_set_parameter_info(rec->plugin_reg,
                                          BG_PLUGIN_RECORDER_VIDEO,
                                          BG_PLUGIN_RECORDER,
                                          &vs->parameters[1]);
    }
  
  return vs->parameters;
  }


void
bg_recorder_set_video_parameter(void * data,
                                const char * name,
                                const bg_parameter_value_t * val)
  {
  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;
  
  if(!name)
    return;
  
  //  if(name)
  //    fprintf(stderr, "bg_recorder_set_video_parameter %s\n", name);

  if(!strcmp(name, "do_video"))
    {
    if(rec->running && (!!(vs->flags & STREAM_ACTIVE) != val->val_i))
      bg_recorder_stop(rec);

    if(val->val_i)
      vs->flags |= STREAM_ACTIVE;
    else
      vs->flags &= ~STREAM_ACTIVE;

    }
  else if(!strcmp(name, "plugin"))
    {
    const bg_plugin_info_t * info;
    
    if(vs->input_handle &&
       !strcmp(vs->input_handle->info->name, val->val_str))
      return;
    
    if(rec->running)
      bg_recorder_stop(rec);

    if(vs->input_handle)
      bg_plugin_unref(vs->input_handle);
    
    info = bg_plugin_find_by_name(rec->plugin_reg, val->val_str);
    vs->input_handle = bg_plugin_load(rec->plugin_reg, info);
    vs->input_plugin = (bg_recorder_plugin_t*)(vs->input_handle->plugin);
    }
  else if(!strcmp(name, "limit_fps"))
    {
    pthread_mutex_lock(&vs->config_mutex);
    vs->limit_timescale      = (int)(val->val_f * 100.0);
    vs->limit_duration = 100;
    pthread_mutex_unlock(&vs->config_mutex);
    }
  else if(vs->input_handle && vs->input_plugin->common.set_parameter)
    {
    vs->input_plugin->common.set_parameter(vs->input_handle->priv, name, val);
    }

  
  }

/* Monitor */

static const bg_parameter_info_t monitor_parameters[] =
  {
    {
      .name = "do_monitor",
      .long_name = TRS("Enable monitor"),
      .type = BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 1 },
    },
    {
      .name      = "plugin",
      .long_name = TRS("Plugin"),
      .type      = BG_PARAMETER_MULTI_MENU,
      .flags     = BG_PARAMETER_PLUGIN,
    },
    { },
  };

const bg_parameter_info_t *
bg_recorder_get_video_monitor_parameters(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  if(!vs->monitor_parameters)
    {
    vs->monitor_parameters = bg_parameter_info_copy_array(monitor_parameters);
    
    bg_plugin_registry_set_parameter_info(rec->plugin_reg,
                                          BG_PLUGIN_OUTPUT_VIDEO,
                                          BG_PLUGIN_PLAYBACK,
                                          &vs->monitor_parameters[1]);
    }
  
  return vs->monitor_parameters;
  }

void
bg_recorder_set_video_monitor_parameter(void * data,
                                        const char * name,
                                        const bg_parameter_value_t * val)
  {
  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;
  
  if(!name)
    return;
  
  //  if(name)
  //    fprintf(stderr, "bg_recorder_set_video_monitor_parameter %s\n", name);

  if(!strcmp(name, "do_monitor"))
    {
    if(rec->running && (!!(vs->flags & STREAM_MONITOR) != val->val_i))
      bg_recorder_stop(rec);

    if(val->val_i)
      vs->flags |= STREAM_MONITOR;
    else
      vs->flags &= ~STREAM_MONITOR;
    }
  else if(!strcmp(name, "plugin"))
    {
    const bg_plugin_info_t * info;
    
    if(vs->monitor_handle &&
       !strcmp(vs->monitor_handle->info->name, val->val_str))
      return;
    
    if(rec->running)
      bg_recorder_stop(rec);
    
    if(vs->monitor_handle)
      bg_plugin_unref(vs->monitor_handle);
    
    info = bg_plugin_find_by_name(rec->plugin_reg, val->val_str);

    vs->monitor_handle = bg_ov_plugin_load(rec->plugin_reg,
                                           info,
                                           rec->display_string);
    vs->monitor_plugin = (bg_ov_plugin_t*)(vs->monitor_handle->plugin);

    vs->monitor_plugin->show_window(vs->monitor_handle->priv, 1);
    

    }
  else if(vs->monitor_handle && vs->monitor_plugin->common.set_parameter)
    {
    vs->monitor_plugin->common.set_parameter(vs->monitor_handle->priv, name, val);
    }
  }

const bg_parameter_info_t *
bg_recorder_get_video_filter_parameters(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  return bg_video_filter_chain_get_parameters(vs->fc);
  }

void
bg_recorder_set_video_filter_parameter(void * data,
                                       const char * name,
                                       const bg_parameter_value_t * val)
  {
  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;

  if(!name)
    return;
  
  //  if(name)
  //    fprintf(stderr, "bg_recorder_set_video_filter_parameter %s\n", name);
  
  }

void * bg_recorder_video_thread(void * data)
  {
  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;

  bg_player_thread_wait_for_start(vs->th);

  gavl_timer_set(vs->timer, 0);
  gavl_timer_start(vs->timer);
  
  while(1)
    {
    if(!bg_player_thread_check(vs->th))
      break;

    if(!vs->in_func(vs->in_data, vs->pipe_frame, vs->in_stream))
      break; /* Should never happen */
    
    /* Monitor */
    if(vs->flags & STREAM_MONITOR)
      {
      if(vs->do_convert_monitor)
        gavl_video_convert(vs->monitor_cnv, vs->pipe_frame, vs->monitor_frame);
      
      vs->monitor_plugin->put_video(vs->monitor_handle->priv, vs->monitor_frame);
      }
    if(vs->monitor_plugin && vs->monitor_plugin->handle_events)
      vs->monitor_plugin->handle_events(vs->monitor_handle->priv);

    /* */
    
    }
  gavl_timer_stop(vs->timer);
  
  return NULL;
  }

static int read_video_internal(void * data, gavl_video_frame_t * frame, int stream)
  {
  gavl_time_t diff_time, time_after, cur_time, next_frame_time;
  
  int ret;

  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;
  
  /* Limit the captured framerate */
  if(vs->frame_counter)
    {
    pthread_mutex_lock(&vs->config_mutex);
    next_frame_time = vs->last_frame_time +
      gavl_frames_to_time(vs->limit_timescale, vs->limit_duration, 1);
    pthread_mutex_unlock(&vs->config_mutex);
    
    cur_time = gavl_timer_get(vs->timer);
    diff_time = next_frame_time - cur_time - vs->last_capture_duration;
    
    if(diff_time > 0)
      gavl_time_delay(&diff_time);
    }
  
  cur_time = gavl_timer_get(vs->timer);
  ret = vs->input_plugin->read_video(vs->input_handle->priv, frame, 0);
  time_after = gavl_timer_get(vs->timer);

  vs->last_capture_duration = time_after - cur_time;
  vs->frame_counter++;

  /* Calculate actual framerate */
  
  vs->fps_frame_counter++;
  if(vs->fps_frame_counter == FRAMERATE_INTERVAL)
    vs->fps_frame_counter = 0;
  
  if(!vs->fps_frame_counter)
    {
    bg_recorder_msg_framerate(rec, (double)(FRAMERATE_INTERVAL*GAVL_TIME_SCALE) /
                              (double)(time_after - vs->fps_frame_time));
    
    vs->fps_frame_time = time_after;
    }

  vs->last_frame_time = time_after;
  
  return ret;
  }

int bg_recorder_video_init(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;

  vs->frame_counter = 0;
  vs->fps_frame_time = 0;
  vs->fps_frame_counter = 0;

  /* Open input */
  if(!vs->input_plugin->open(vs->input_handle->priv, NULL, &vs->input_format))
    return 0;

  vs->flags |= STREAM_INPUT_OPEN;
  

  vs->in_func   = read_video_internal;
  vs->in_stream = 0;
  vs->in_data   = rec;
  
  /* Set up filter chain */

  bg_video_filter_chain_connect_input(vs->fc,
                                      vs->in_func,
                                      vs->in_data,
                                      vs->in_stream);
  
  vs->in_func = bg_video_filter_chain_read;
  vs->in_data = vs->fc;
  vs->in_stream = 0;
  
  bg_video_filter_chain_init(vs->fc, &(vs->input_format), &(vs->pipe_format));
  
  /* Set up monitoring */

  if(vs->flags & STREAM_MONITOR)
    {
    gavl_video_format_copy(&vs->monitor_format, &vs->pipe_format);
    vs->monitor_plugin->open(vs->monitor_handle->priv, &vs->monitor_format, 1);

    vs->do_convert_monitor = gavl_video_converter_init(vs->monitor_cnv, &vs->pipe_format,
                                                       &vs->monitor_format);
    vs->flags |= STREAM_MONITOR_OPEN;
    }
  else
    vs->do_convert_monitor = 0;
  
  /* Set up output */

  /* Create frames */
  
  if(vs->flags & STREAM_MONITOR)
    {
    if(vs->monitor_plugin->create_frame)
      vs->monitor_frame = vs->monitor_plugin->create_frame(vs->monitor_handle->priv);
    else
      vs->monitor_frame = gavl_video_frame_create(&vs->monitor_format);
    }
  
  if(!vs->do_convert_monitor && (vs->flags & STREAM_MONITOR))
    vs->pipe_frame = vs->monitor_frame;
  else
    vs->pipe_frame = gavl_video_frame_create(&vs->pipe_format);
  
  return 1;
  }

void bg_recorder_video_cleanup(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  if(vs->flags & STREAM_INPUT_OPEN)
    vs->input_plugin->close(vs->input_handle->priv);

  if(vs->pipe_frame && (vs->pipe_frame != vs->monitor_frame))
    {
    gavl_video_frame_destroy(vs->pipe_frame);
    }
  vs->pipe_frame = NULL;
  
  if(vs->monitor_frame)
    {
    if(vs->monitor_plugin->destroy_frame)
      vs->monitor_plugin->destroy_frame(vs->monitor_handle->priv,
                                        vs->monitor_frame);
    else
      gavl_video_frame_destroy(vs->monitor_frame);
    vs->monitor_frame = NULL;
    }
  
  if(vs->flags & STREAM_MONITOR_OPEN)
    vs->monitor_plugin->close(vs->monitor_handle->priv);
  }
