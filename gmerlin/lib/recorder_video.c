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

#include <config.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h> // access()

#include <gmerlin/translation.h>
#include <gmerlin/utils.h>

#include <gmerlin/recorder.h>
#include <recorder_private.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "recorder.video"

#define FRAMERATE_INTERVAL 10

#include <gavl/metatags.h>

static int create_snapshot_cb(void * data, const char * filename)
  {
  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;

  int overwrite;

  pthread_mutex_lock(&rec->snapshot_mutex);
  overwrite = vs->flags & STREAM_SNAPSHOT_OVERWRITE;
  pthread_mutex_unlock(&rec->snapshot_mutex);
  
  if(!overwrite && !access(filename, R_OK))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Won't save snapshot %s (file exists)",
           filename);
    return 0;
    }
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Saving snapshot %s", filename);
  return 1;
  }

static int ov_button_callback(void * data, int x, int y, int button, int mask)
  {
  bg_recorder_t * rec = data;
  bg_recorder_msg_button_press(rec, x, y, button, mask);
  return 1;
  }

static int ov_button_release_callback(void * data, int x, int y,
                                      int button, int mask)
  {
  bg_recorder_t * rec = data;
  bg_recorder_msg_button_release(rec, x, y, button, mask);
  return 1;
  }
  
static int ov_motion_callback(void * data, int x, int y, int mask)
  {
  bg_recorder_t * rec = data;
  bg_recorder_msg_motion(rec, x, y, mask);
  return 1;
  }


void bg_recorder_create_video(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;

  vs->monitor_cnv = gavl_video_converter_create();
  vs->enc_cnv = gavl_video_converter_create();
  vs->snapshot_cnv = gavl_video_converter_create();
  
  bg_gavl_video_options_init(&vs->opt);
  
  vs->fc = bg_video_filter_chain_create(&vs->opt, rec->plugin_reg);

  vs->th = bg_thread_create(rec->tc);
  vs->timer = gavl_timer_create();
  pthread_mutex_init(&vs->config_mutex, NULL);

  vs->snapshot_cb.create_output_file = create_snapshot_cb;
  vs->snapshot_cb.data = rec;

  vs->monitor_cb.button_callback = ov_button_callback;
  vs->monitor_cb.button_release_callback = ov_button_release_callback;
  vs->monitor_cb.motion_callback = ov_motion_callback;
  vs->monitor_cb.data = rec;

  pthread_mutex_init(&vs->eof_mutex, NULL);
  }

void bg_recorder_video_set_eof(bg_recorder_video_stream_t * s, int eof)
  {
  pthread_mutex_lock(&s->eof_mutex);
  s->eof = eof;
  pthread_mutex_unlock(&s->eof_mutex);
  }

int  bg_recorder_video_get_eof(bg_recorder_video_stream_t * s)
  {
  int ret;
  pthread_mutex_lock(&s->eof_mutex);
  ret = s->eof;
  pthread_mutex_unlock(&s->eof_mutex);
  return ret;
  }


void bg_recorder_destroy_video(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  
  gavl_video_converter_destroy(vs->monitor_cnv);
  gavl_video_converter_destroy(vs->enc_cnv);
  gavl_video_converter_destroy(vs->snapshot_cnv);
  
  bg_video_filter_chain_destroy(vs->fc);
  bg_thread_destroy(vs->th);
  gavl_timer_destroy(vs->timer);
  pthread_mutex_destroy(&vs->config_mutex);

  if(vs->monitor_handle)
    bg_plugin_unref(vs->monitor_handle);
  if(vs->input_handle)
    bg_plugin_unref(vs->input_handle);
  if(vs->snapshot_handle)
    bg_plugin_unref(vs->snapshot_handle);
  
  bg_gavl_video_options_free(&vs->opt);
  pthread_mutex_destroy(&vs->eof_mutex);
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
    if((rec->flags & FLAG_RUNNING) &&
       (!!(vs->flags & STREAM_ACTIVE) != val->val_i))
      bg_recorder_interrupt(rec);

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
    
    if(rec->flags & FLAG_RUNNING)
      bg_recorder_interrupt(rec);

    if(vs->input_handle)
      bg_plugin_unref(vs->input_handle);
    
    info = bg_plugin_find_by_name(rec->plugin_reg, val->val_str);
    vs->input_handle = bg_plugin_load(rec->plugin_reg, info);
    vs->input_plugin = (bg_recorder_plugin_t*)(vs->input_handle->plugin);
    if(vs->input_plugin->set_callbacks)
      vs->input_plugin->set_callbacks(vs->input_handle->priv, &rec->recorder_cb);
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
    if(!!(vs->flags & STREAM_MONITOR) != val->val_i)
      bg_recorder_interrupt(rec);

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
    
    bg_recorder_interrupt(rec);
    
    if(vs->monitor_handle)
      bg_plugin_unref(vs->monitor_handle);
    
    info = bg_plugin_find_by_name(rec->plugin_reg, val->val_str);

    vs->monitor_handle = bg_ov_plugin_load(rec->plugin_reg,
                                           info,
                                           rec->display_string);
    vs->monitor_plugin = (bg_ov_plugin_t*)(vs->monitor_handle->plugin);

    if(vs->monitor_plugin->set_callbacks)
      vs->monitor_plugin->set_callbacks(vs->monitor_handle->priv,
                                        &vs->monitor_cb);

    if(vs->monitor_plugin->show_window && rec->display_string)
      vs->monitor_plugin->show_window(vs->monitor_handle->priv, 1);
    }
  else if(vs->monitor_handle && vs->monitor_plugin->common.set_parameter)
    {
    vs->monitor_plugin->common.set_parameter(vs->monitor_handle->priv, name, val);
    }
  }

static const bg_parameter_info_t snapshot_parameters[] =
  {
    {
      .name = "snapshot_auto",
      .long_name = TRS("Automatic"),
      .type = BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name = "snapshot_interval",
      .long_name = TRS("Snapshot interval"),
      .type = BG_PARAMETER_FLOAT,
      .val_default = { .val_f = 5.0 },
      .val_min     = { .val_f = 0.5 },
      .val_max     = { .val_f = 10000.0 },
      .num_digits  = 1,
    },
    {
      .name = "snapshot_overwrite",
      .long_name = TRS("Overwrite existing files"),
      .type = BG_PARAMETER_CHECKBUTTON,
    },
    {
      .name      = "plugin",
      .long_name = TRS("Plugin"),
      .type      = BG_PARAMETER_MULTI_MENU,
    },
    { /* End */ }
  };

const bg_parameter_info_t *
bg_recorder_get_video_snapshot_parameters(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  if(!vs->snapshot_parameters)
    {
    vs->snapshot_parameters = bg_parameter_info_copy_array(snapshot_parameters);
    bg_plugin_registry_set_parameter_info(rec->plugin_reg,
                                          BG_PLUGIN_IMAGE_WRITER,
                                          BG_PLUGIN_FILE,
                                          &vs->snapshot_parameters[3]);
    }
  return vs->snapshot_parameters;
  }

void
bg_recorder_set_video_snapshot_parameter(void * data,
                                         const char * name,
                                         const bg_parameter_value_t * val)
  {
  bg_recorder_t * rec;
  bg_recorder_video_stream_t * vs;

  if(!name)
    return;

  rec = data;
  vs = &rec->vs;
  
  if(!strcmp(name, "snapshot_auto"))
    {
    pthread_mutex_lock(&rec->snapshot_mutex);
    if(val->val_i)
      vs->flags |= STREAM_SNAPSHOT_AUTO;
    else
      vs->flags &= ~STREAM_SNAPSHOT_AUTO;
    pthread_mutex_unlock(&rec->snapshot_mutex);
    }
  else if(!strcmp(name, "snapshot_overwrite"))
    {
    if(val->val_i)
      vs->flags |= STREAM_SNAPSHOT_OVERWRITE;
    else
      vs->flags &= ~STREAM_SNAPSHOT_OVERWRITE;
    }
  else if(!strcmp(name, "snapshot_interval"))
    vs->snapshot_interval = gavl_seconds_to_time(val->val_f);
  else if( !strcmp(name, "plugin"))
    {
    const bg_plugin_info_t * info;
    
    if(vs->snapshot_handle &&
       !strcmp(vs->snapshot_handle->info->name, val->val_str))
      return;
    
    bg_recorder_interrupt(rec);
    
    if(vs->snapshot_handle)
      bg_plugin_unref(vs->snapshot_handle);
    
    info = bg_plugin_find_by_name(rec->plugin_reg, val->val_str);

    vs->snapshot_handle = bg_plugin_load(rec->plugin_reg,
                                         info);
    vs->snapshot_plugin = (bg_image_writer_plugin_t*)(vs->snapshot_handle->plugin);

    if(vs->snapshot_plugin->set_callbacks)
      vs->snapshot_plugin->set_callbacks(vs->snapshot_handle->priv,
                                         &vs->snapshot_cb);
    
    }
  else
    {
    vs->snapshot_plugin->common.set_parameter(vs->snapshot_handle->priv,
                                              name, val);
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
  int need_restart;
  
  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;

  if(!name)
    {
    bg_recorder_resume(rec);
    return;
    }

  bg_video_filter_chain_lock(vs->fc);
  bg_video_filter_chain_set_parameter(vs->fc, name, val);
  
  if(bg_video_filter_chain_need_restart(vs->fc))
    need_restart = 1;
  else
    need_restart = 0;

  bg_video_filter_chain_unlock(vs->fc);
  
  if(need_restart)
    bg_recorder_interrupt(rec);
  }

#define BUFFER_SIZE 256

static char * create_snapshot_filename(bg_recorder_t * rec, int * have_count)
  {
  char mask[16];
  char buf[BUFFER_SIZE];
  char * pos;
  char * end;
  char * filename;
  int have_time = 0;
  time_t t;
  struct tm time_date;
  
  filename = bg_sprintf("%s/", rec->snapshot_directory);
  
  pos = rec->snapshot_filename_mask;

  if(have_count)
    *have_count = 0;
  
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
      sprintf(buf, mask, rec->vs.snapshot_counter);
      filename = bg_strcat(filename, buf);
      pos += 3;

      if(have_count)
        *have_count = 1;
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

static void check_snapshot(bg_recorder_t * rec)
  {
  int doit = 0;
  char * filename;
  gavl_time_t frame_time;
  
  bg_recorder_video_stream_t * vs = &rec->vs;

  frame_time =
    gavl_time_unscale(vs->pipe_format.timescale,
                      vs->pipe_frame->timestamp);
  
  /* Check whether to make a snapshot */

  pthread_mutex_lock(&rec->snapshot_mutex);
  if(rec->snapshot)
    {
    doit = 1;
    rec->snapshot = 0;
    }
  
  if(!doit &&
     ((vs->flags & STREAM_SNAPSHOT_AUTO) &&
      (!(vs->flags & STREAM_SNAPSHOT_INIT) ||
       frame_time >= vs->last_snapshot_time + vs->snapshot_interval)))
    {
    doit = 1;
    }
  pthread_mutex_unlock(&rec->snapshot_mutex);
  
  if(!doit)
    return;
  
  filename = create_snapshot_filename(rec, NULL);
  
  /* Initialize snapshot plugin */
  if(!(vs->flags & STREAM_SNAPSHOT_INIT))
    gavl_video_format_copy(&vs->snapshot_format,
                           &vs->pipe_format);

  if(!vs->snapshot_plugin->write_header(vs->snapshot_handle->priv,
                                        filename,
                                        &vs->snapshot_format,
                                        &rec->m))
    return;
  
  if(!(vs->flags & STREAM_SNAPSHOT_INIT))
    {
    vs->do_convert_snapshot =
      gavl_video_converter_init(vs->snapshot_cnv,
                                &vs->pipe_format,
                                &vs->snapshot_format);

    if(vs->do_convert_snapshot)
      vs->snapshot_frame = gavl_video_frame_create(&vs->snapshot_format);
    vs->flags |= STREAM_SNAPSHOT_INIT;
    }

  if(vs->do_convert_snapshot)
    {
    gavl_video_convert(vs->snapshot_cnv, vs->pipe_frame,
                       vs->snapshot_frame);
    vs->snapshot_plugin->write_image(vs->snapshot_handle->priv,
                                     vs->snapshot_frame);
    }
  else
    {
    vs->snapshot_plugin->write_image(vs->snapshot_handle->priv,
                                     vs->pipe_frame);
    }
  vs->snapshot_counter++;
  vs->last_snapshot_time = frame_time;
  }

void * bg_recorder_video_thread(void * data)
  {
  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;
  gavl_video_frame_t * monitor_frame = NULL;
  gavl_time_t idle_time = GAVL_TIME_SCALE / 100; // 10 ms
  bg_thread_wait_for_start(vs->th);

  gavl_timer_set(vs->timer, 0);
  gavl_timer_start(vs->timer);
  
  while(1)
    {
    if(!bg_thread_check(vs->th))
      break;

    if(bg_recorder_video_get_eof(vs))
      {
      gavl_time_delay(&idle_time);
      continue;
      }
    
    vs->pipe_frame = NULL;
    
    if(vs->flags & STREAM_MONITOR)
      {
      monitor_frame =
        vs->monitor_plugin->get_frame(vs->monitor_handle->priv);
      if(!vs->do_convert_monitor)
        vs->pipe_frame = monitor_frame;
      }

    if(!vs->pipe_frame)
      vs->pipe_frame = vs->pipe_frame_priv;
    
    if(!vs->in_func(vs->in_data, vs->pipe_frame, vs->in_stream))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Read failed (device unplugged?)");
      bg_recorder_video_set_eof(vs, 1);
      continue; // Need to go to bg_thread_check to stop the thread cleanly
      }
    /* Check whether to make a snapshot */
    check_snapshot(rec);
    
    /* Monitor */
    if(vs->flags & STREAM_MONITOR)
      {
      if(vs->do_convert_monitor)
        gavl_video_convert(vs->monitor_cnv, vs->pipe_frame, monitor_frame);
      
      vs->monitor_plugin->put_frame(vs->monitor_handle->priv, monitor_frame);
      }
    if(vs->monitor_plugin && vs->monitor_plugin->handle_events)
      vs->monitor_plugin->handle_events(vs->monitor_handle->priv);

    /* Encoding */
    if(vs->flags & STREAM_ENCODE_OPEN)
      {
      bg_recorder_update_time(rec,
                              gavl_time_unscale(vs->pipe_format.timescale,
                                                vs->pipe_frame->timestamp));
      if(vs->do_convert_enc)
        {
        gavl_video_convert(vs->enc_cnv, vs->pipe_frame, vs->enc_frame);
        bg_encoder_write_video_frame(rec->enc, vs->enc_frame, vs->enc_index);
        }
      else
        bg_encoder_write_video_frame(rec->enc, vs->pipe_frame, vs->enc_index);
      }
    
    /* */
    
    }
  gavl_timer_stop(vs->timer);
  
  return NULL;
  }

static int read_video_internal(void * data, gavl_video_frame_t * frame, int stream)
  {
  gavl_time_t time_after, cur_time;
  
  int ret;

  bg_recorder_t * rec = data;
  bg_recorder_video_stream_t * vs = &rec->vs;
  
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
  if(!vs->input_plugin->open(vs->input_handle->priv, NULL, &vs->input_format, &vs->m))
    return 0;
  bg_metadata_date_now(&vs->m, GAVL_META_DATE_CREATE);
  
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
  
  bg_video_filter_chain_init(vs->fc, &vs->input_format, &vs->pipe_format);
  
  /* Set up monitoring */

  if(vs->flags & STREAM_MONITOR)
    {
    gavl_video_format_copy(&vs->monitor_format, &vs->pipe_format);
    vs->monitor_plugin->open(vs->monitor_handle->priv, &vs->monitor_format, 1);

    vs->do_convert_monitor = gavl_video_converter_init(vs->monitor_cnv, &vs->pipe_format,
                                                       &vs->monitor_format);
    vs->flags |= STREAM_MONITOR_OPEN;

    if(vs->monitor_plugin->show_window && !rec->display_string)
      {
      vs->monitor_plugin->show_window(vs->monitor_handle->priv, 1);
      if(vs->monitor_plugin->set_window_title)
        vs->monitor_plugin->set_window_title(vs->monitor_handle->priv, "Gmerlin recorder "VERSION);
      }
    }
  else
    vs->do_convert_monitor = 0;
  
  /* Set up encoding */

  if(vs->flags & STREAM_ENCODE)
    {
    vs->enc_index =
      bg_encoder_add_video_stream(rec->enc, &vs->m, &vs->pipe_format, 0);
    }
  
  /* Create frames */

#if 0
  if(vs->flags & STREAM_MONITOR)
    {
    if(vs->monitor_plugin->create_frame)
      vs->monitor_frame = vs->monitor_plugin->create_frame(vs->monitor_handle->priv);
    else
      vs->monitor_frame = gavl_video_frame_create(&vs->monitor_format);
    }
#endif
  if(vs->do_convert_monitor || !(vs->flags & STREAM_MONITOR))
    vs->pipe_frame_priv = gavl_video_frame_create(&vs->pipe_format);
  
  /* Initialize snapshot counter */
  
  
  return 1;
  }

void bg_recorder_video_finalize_encode(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  bg_encoder_get_video_format(rec->enc, vs->enc_index, &vs->enc_format);

  /*
   *  The encoder might have changed the framerate.
   *  This affects the pipe_format as well, but not the other formats
   */
  
  vs->pipe_format.framerate_mode = vs->enc_format.framerate_mode;
  vs->pipe_format.timescale      = vs->enc_format.timescale;
  vs->pipe_format.frame_duration = vs->enc_format.frame_duration;
  
  bg_video_filter_chain_set_out_format(vs->fc,
                                       &vs->pipe_format);
  
  vs->do_convert_enc = gavl_video_converter_init(vs->enc_cnv, &vs->pipe_format,
                                                 &vs->enc_format);

  if(vs->do_convert_enc)
    vs->enc_frame = gavl_video_frame_create(&vs->enc_format);


  
  
  vs->flags |= STREAM_ENCODE_OPEN;
  
  }


void bg_recorder_video_cleanup(bg_recorder_t * rec)
  {
  bg_recorder_video_stream_t * vs = &rec->vs;
  if(vs->flags & STREAM_INPUT_OPEN)
    vs->input_plugin->close(vs->input_handle->priv);

  if(vs->pipe_frame_priv)
    gavl_video_frame_destroy(vs->pipe_frame_priv);
  
  vs->pipe_frame_priv = NULL;
  
  if(vs->flags & STREAM_MONITOR_OPEN)
    vs->monitor_plugin->close(vs->monitor_handle->priv);

  if(vs->enc_frame)
    {
    gavl_video_frame_destroy(vs->enc_frame);
    vs->enc_frame = NULL;
    }

  vs->flags &= ~(STREAM_INPUT_OPEN |
                 STREAM_ENCODE_OPEN |
                 STREAM_MONITOR_OPEN |
                 STREAM_SNAPSHOT_INIT);
  
  }

