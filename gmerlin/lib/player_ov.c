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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gmerlin/keycodes.h>
#include <gmerlin/accelerator.h>

#include <gmerlin/player.h>
#include <playerprivate.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "player.video_output"

// #define DUMP_TIMESTAMPS

struct bg_player_ov_context_s
  {
  
  };

static int accel_callback(void * data, int id)
  {
  bg_player_t * p = data;
  bg_player_accel_pressed(p, id);
  return 1;
  }

static int
button_callback(void * data, int x, int y, int button, int mask)
  {
  bg_player_t * p = data;

  if(button == 4)
    {
    bg_player_seek_rel(p, 2 * GAVL_TIME_SCALE );
    return 1;
    }
  else if(button == 5)
    {
    bg_player_seek_rel(p, - 2 * GAVL_TIME_SCALE );
    return 1;
    }
  return 0;
  }

static void brightness_callback(void * data, float val)
  {
  bg_player_t * p = data;

  bg_osd_set_brightness_changed(p->video_stream.osd, val,
                                p->video_stream.frame_time);
  }

static void saturation_callback(void * data, float val)
  {
  bg_player_t * p = data;

  bg_osd_set_saturation_changed(p->video_stream.osd, val,
                                p->video_stream.frame_time);
  }

static void contrast_callback(void * data, float val)
  {
  bg_player_t * p = data;

  bg_osd_set_contrast_changed(p->video_stream.osd, val,
                              p->video_stream.frame_time);
  }

static void
handle_messages(bg_player_video_stream_t * ctx, gavl_time_t time)
  {
  bg_msg_t * msg;
  int id;
  float arg_f;
  
  while((msg = bg_msg_queue_try_lock_read(ctx->msg_queue)))
    {
    id = bg_msg_get_id(msg);
    switch(id)
      {
      case BG_PLAYER_MSG_VOLUME_CHANGED:
        arg_f = bg_msg_get_arg_float(msg, 0);
        bg_osd_set_volume_changed(ctx->osd,
                                  (arg_f - BG_PLAYER_VOLUME_MIN)/(-BG_PLAYER_VOLUME_MIN),
                                  time);
        break;
      default:
        break;
      }

    bg_msg_queue_unlock_read(ctx->msg_queue);
    }
  }

/* Create frame */

static gavl_video_frame_t * create_frame(bg_player_video_stream_t * s)
  {
  gavl_video_frame_t * ret;

  if(s->plugin->create_frame)
    {
    bg_plugin_lock(s->plugin_handle);
    ret = s->plugin->create_frame(s->priv);
    bg_plugin_unlock(s->plugin_handle);
    }
  else
    ret = gavl_video_frame_create(&s->output_format);

  gavl_video_frame_clear(ret, &(s->output_format));
  
  return (void*)ret;
  }

static void destroy_frame(bg_player_video_stream_t * s,
                          gavl_video_frame_t * frame)
  {
  if(s->plugin->destroy_frame)
    {
    bg_plugin_lock(s->plugin_handle);
    s->plugin->destroy_frame(s->priv, frame);
    bg_plugin_unlock(s->plugin_handle);
    }
  else
    gavl_video_frame_destroy(frame);
  }

static gavl_overlay_t * create_overlay(bg_player_video_stream_t * vs,
                                       int id)
  {
  gavl_overlay_t * ret;
  
  if(vs->plugin->create_overlay)
    {
    bg_plugin_lock(vs->plugin_handle);
    ret = vs->plugin->create_overlay(vs->priv, id);
    bg_plugin_unlock(vs->plugin_handle);
    }
  else
    {
    ret = calloc(1, sizeof(*ret));

    if(id == vs->subtitle_id)
      ret->frame =
        gavl_video_frame_create(&(vs->ss->output_format));
    else
      ret->frame =
        gavl_video_frame_create(&(vs->osd_format));
    }

  if(id == vs->subtitle_id)
    gavl_video_frame_clear(ret->frame,
                           &(vs->ss->output_format));
  else
    gavl_video_frame_clear(ret->frame,
                           &(vs->osd_format));
    
  
  return ret;
  }

static void destroy_overlay(bg_player_video_stream_t * vs,
                            int id, gavl_overlay_t * ovl)
  {
  if(vs->plugin->destroy_overlay)
    {
    bg_plugin_lock(vs->plugin_handle);
    vs->plugin->destroy_overlay(vs->priv, id, ovl);
    bg_plugin_unlock(vs->plugin_handle);
    }
  else
    {
    gavl_video_frame_destroy(ovl->frame);
    free(ovl);
    }
  }



void bg_player_ov_create(bg_player_t * player)
  {
  bg_player_video_stream_t * ctx = &player->video_stream;
  ctx = calloc(1, sizeof(*ctx));
  
  ctx->msg_queue = bg_msg_queue_create();
  ctx->accel_map = bg_accelerator_map_create();
  
  pthread_mutex_init(&(ctx->still_mutex),(pthread_mutexattr_t *)0);
  
  /* Load output plugin */
  
  ctx->callbacks.accel_callback    = accel_callback;
  ctx->callbacks.button_callback = button_callback;

  ctx->callbacks.brightness_callback = brightness_callback;
  ctx->callbacks.saturation_callback = saturation_callback;
  ctx->callbacks.contrast_callback   = contrast_callback;
  
  ctx->callbacks.data      = player;
  ctx->callbacks.accel_map = ctx->accel_map;
  
  ctx->osd = bg_osd_create();
  }

void bg_player_add_accelerators(bg_player_t * player, const bg_accelerator_t * list)
  {
  if(player->video_stream.plugin_handle)
    bg_plugin_lock(player->video_stream.plugin_handle);

  bg_accelerator_map_append_array(player->video_stream.accel_map, list);
  
  if(player->video_stream.plugin_handle)
    bg_plugin_unlock(player->video_stream.plugin_handle);
  }


void bg_player_ov_standby(bg_player_video_stream_t * ctx)
  {
  if(!ctx->plugin_handle)
    return;

  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->show_window)
    ctx->plugin->show_window(ctx->priv, 0);
  bg_plugin_unlock(ctx->plugin_handle);
  }


void bg_player_ov_set_plugin(bg_player_t * player, bg_plugin_handle_t * handle)
  {
  bg_player_video_stream_t * ctx;

  ctx = &player->video_stream;

  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  

  ctx->plugin_handle = handle;


  ctx->plugin = (bg_ov_plugin_t*)(ctx->plugin_handle->plugin);
  ctx->priv = ctx->plugin_handle->priv;

  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin->set_callbacks)
    ctx->plugin->set_callbacks(ctx->priv, &(ctx->callbacks));
  bg_plugin_unlock(ctx->plugin_handle);

  }

void bg_player_ov_destroy(bg_player_t * player)
  {
  bg_player_video_stream_t * ctx = &player->video_stream;
  
  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  bg_osd_destroy(ctx->osd);
  
  bg_accelerator_map_destroy(ctx->accel_map);

  bg_msg_queue_destroy(ctx->msg_queue);
  
  }

int bg_player_ov_init(bg_player_video_stream_t * vs)
  {
  int result;
  
  vs->next_subtitle    = (gavl_overlay_t*)0;
  vs->has_subtitle = 0;

  bg_plugin_lock(vs->plugin_handle);
  result = vs->plugin->open(vs->priv,
                             &(vs->output_format), 1);
  
  vs->plugin->set_window_title(vs->priv, "Video output");
  
  if(result && vs->plugin->show_window)
    vs->plugin->show_window(vs->priv, 1);
  else if(!result)
    return result;
  
  memset(&(vs->osd_format), 0, sizeof(vs->osd_format));
  
  bg_osd_init(vs->osd, &(vs->output_format),
              &vs->osd_format);
  /* Fixme: Lets just hope, that the OSD format doesn't get changed
     by this call. Otherwise, we would need a gavl_video_converter */
  vs->osd_id = vs->plugin->add_overlay_stream(vs->priv,
                                                &vs->osd_format);

  /* create_overlay needs the lock again */
  bg_plugin_unlock(vs->plugin_handle);
  
  
  vs->osd_ovl = create_overlay(vs, vs->osd_id);
  bg_osd_set_overlay(vs->osd, vs->osd_ovl);
  vs->frames_written = 0;
  return result;
  }

void bg_player_ov_update_still(bg_player_video_stream_t * ctx)
  {

  pthread_mutex_lock(&ctx->still_mutex);
  
  if(!ctx->still_frame)
    {
    ctx->still_frame = create_frame(ctx);
    }
  
  if(ctx->frame)
    {
    gavl_video_frame_copy(&(ctx->output_format),
                          ctx->still_frame, ctx->frame);
    }
  else
    {
    gavl_video_frame_clear(ctx->still_frame,
                           &(ctx->output_format));
    }
  
  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->put_still(ctx->priv, ctx->still_frame);
  bg_plugin_unlock(ctx->plugin_handle);
  
  pthread_mutex_unlock(&ctx->still_mutex);
  }

void bg_player_ov_cleanup(bg_player_video_stream_t * ctx)
  {
  pthread_mutex_lock(&ctx->still_mutex);
  if(ctx->still_frame)
    {
    destroy_frame(ctx, ctx->still_frame);
    ctx->still_frame = (gavl_video_frame_t*)0;
    }
  pthread_mutex_unlock(&ctx->still_mutex);
  
  if(ctx->current_subtitle.frame)
    {
    gavl_video_frame_destroy(ctx->current_subtitle.frame);
    ctx->current_subtitle.frame = (gavl_video_frame_t*)0;
    }
  if(ctx->osd_ovl)
    {
    destroy_overlay(ctx, ctx->osd_id, ctx->osd_ovl);
    ctx->osd_ovl = (gavl_overlay_t*)0;
    }
  
  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->close(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  }

void bg_player_ov_reset(bg_player_t * player)
  {
  bg_player_video_stream_t * ctx;
  ctx = &player->video_stream;
  if(ctx->has_subtitle)
    {
    ctx->plugin->set_overlay(ctx->priv, ctx->subtitle_id, (gavl_overlay_t*)0);
    ctx->has_subtitle = 0;
    }
  ctx->next_subtitle = (gavl_overlay_t*)0;
  ctx->frame = NULL;
  }

void bg_player_ov_update_aspect(bg_player_video_stream_t * ctx,
                                int pixel_width, int pixel_height)
  {
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin && ctx->plugin->update_aspect)
    ctx->plugin->update_aspect(ctx->priv, pixel_width, pixel_height);
  bg_plugin_unlock(ctx->plugin_handle);
  }

/* Set this extra because we must initialize subtitles after the video output */
void bg_player_ov_set_subtitle_format(bg_player_video_stream_t * ctx)
  {
  gavl_video_format_copy(&ctx->ss->output_format,
                         &ctx->ss->input_format);
  
  /* Add subtitle stream for plugin */
  
  ctx->subtitle_id =
    ctx->plugin->add_overlay_stream(ctx->priv,
                                    &ctx->ss->output_format);
  
  /* Allocate private overlay frame */
  ctx->current_subtitle.frame =
    gavl_video_frame_create(&ctx->ss->output_format);
  }

#if 0
static void ping_func(bg_player_video_stream_t * ctx)
  {
  pthread_mutex_lock(&ctx->still_mutex);
  
  if(!ctx->still_shown)
    {
    if(ctx->frame)
      {
      ctx->still_frame = create_frame(ctx);
      
      gavl_video_frame_copy(&(ctx->output_format),
                            ctx->still_frame, ctx->frame);
      //      fprintf(stderr, "Unlock read\n");
      
      bg_plugin_lock(ctx->plugin_handle);
      ctx->plugin->put_still(ctx->priv, ctx->still_frame);
      bg_plugin_unlock(ctx->plugin_handle);
      }
    
    ctx->still_shown = 1;
    }
  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->handle_events(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  
  pthread_mutex_unlock(&ctx->still_mutex);
  }
#endif

#if 0
static void handle_subtitle(bg_player_video_stream_t * ctx)
  {
  gavl_overlay_t tmp_overlay;
  
  /* Try to get next subtitle */
  if(!vs->next_subtitle)
    {
    vs->next_subtitle =
      bg_fifo_try_lock_read(ctx->player->subtitle_stream.fifo,
                            &state);
    if(ctx->next_subtitle)
      ctx->next_subtitle_time =
        gavl_time_unscale(ctx->player->subtitle_stream.output_format.timescale,
                          ctx->next_subtitle->frame->timestamp);
    }
  /* Check if the overlay is expired */
  if(ctx->has_subtitle)
    {
    if(bg_overlay_too_old(ctx->frame_time,
                          ctx->current_subtitle_time,
                          ctx->current_subtitle_duration))
      {
      ctx->plugin->set_overlay(ctx->priv, ctx->subtitle_id, (gavl_overlay_t*)0);
      ctx->has_subtitle = 0;
      }
    }
      
  /* Check if new overlay should be used */
      
  if(ctx->next_subtitle)
    {
    if(!bg_overlay_too_new(ctx->frame_time, ctx->next_subtitle_time))
      {
      memcpy(&tmp_overlay, ctx->next_subtitle, sizeof(tmp_overlay));
      memcpy(ctx->next_subtitle, &(ctx->current_subtitle),
             sizeof(tmp_overlay));
      memcpy(&(ctx->current_subtitle), &tmp_overlay, sizeof(tmp_overlay));

      ctx->current_subtitle_time =
        gavl_time_unscale(ctx->player->subtitle_stream.output_format.timescale,
                          ctx->current_subtitle.frame->timestamp);

      ctx->current_subtitle_duration =
        gavl_time_unscale(ctx->player->subtitle_stream.output_format.timescale,
                          ctx->current_subtitle.frame->duration);
          
      ctx->plugin->set_overlay(ctx->priv, ctx->subtitle_id,
                               &(ctx->current_subtitle));
          
      ctx->has_subtitle = 1;
      ctx->next_subtitle = (gavl_overlay_t*)0;
      ctx->next_subtitle_time = GAVL_TIME_UNDEFINED;
      bg_fifo_unlock_read(ctx->player->subtitle_stream.fifo);
      }
    }
  }

#endif

void * bg_player_ov_thread(void * data)
  {
  int state;
  
  bg_player_video_stream_t * ctx;
  gavl_time_t diff_time;
  gavl_time_t current_time;
  
  bg_player_t * p = data;
  
  ctx = &p->video_stream;

  bg_player_add_message_queue(p, ctx->msg_queue);

  bg_player_thread_wait_for_start(ctx->th);
  
  while(1)
    {
    if(!bg_player_thread_check(ctx->th))
      break;

    if(!bg_player_read_video(p, ctx->frame, &state))
      {
      if(state == BG_PLAYER_STATE_PLAYING)
        {
        /* EOF */
        }
      else
        {
        /* Wait for restart or quit */
        }
      }
    pthread_mutex_lock(&ctx->still_mutex);
    if(ctx->still_frame)
      {
      destroy_frame(data, ctx->still_frame);
      ctx->still_frame = (gavl_video_frame_t*)0;
      }
    pthread_mutex_unlock(&ctx->still_mutex);
    
    ctx->still_shown = 0;
    
    /* Get frame time */

    ctx->frame_time =
      gavl_time_unscale(ctx->output_format.timescale,
                        ctx->frame->timestamp);
    
    /* Subtitle handling */
    if(DO_SUBTITLE(p->flags))
      {
      //      handle_subtitle(ctx);
      }
    /* Handle message */
    handle_messages(ctx, ctx->frame_time);
    
    /* Display OSD */

    if(bg_osd_overlay_valid(ctx->osd, ctx->frame_time))
      ctx->plugin->set_overlay(ctx->priv, ctx->osd_id, ctx->osd_ovl);
    else
      ctx->plugin->set_overlay(ctx->priv, ctx->osd_id, (gavl_overlay_t*)0);
    
    /* Check Timing */
    bg_player_time_get(p, 1, &current_time);

    diff_time =  ctx->frame_time - current_time;
    
    
#ifdef DUMP_TIMESTAMPS
    bg_debug("C: %"PRId64", F: %"PRId64", D: %"PRId64"\n",
             current_time, ctx->frame_time, diff_time);
#endif
    /* Wait until we can display the frame */
    if(diff_time > 0)
      gavl_time_delay(&diff_time);
    
    /* TODO: Drop frames */
    else if(diff_time < -100000)
      {
      }

    if(p->time_update_mode == TIME_UPDATE_FRAME)
      {
      bg_player_broadcast_time(p, ctx->frame_time);
      }
    
    bg_plugin_lock(ctx->plugin_handle);
    ctx->plugin->put_video(ctx->priv, ctx->frame);
    ctx->plugin->handle_events(ctx->priv);
    ctx->frames_written++;
    
    bg_plugin_unlock(ctx->plugin_handle);
    }

  bg_player_delete_message_queue(p,
                              ctx->msg_queue);
  return NULL;
  }

const bg_parameter_info_t * bg_player_get_osd_parameters(bg_player_t * p)
  {
  return bg_osd_get_parameters(p->video_stream.osd);
  }

void bg_player_set_osd_parameter(void * data, const char * name, const bg_parameter_value_t*val)
  {
  bg_player_t * p = data;
  bg_osd_set_parameter(p->video_stream.osd, name, val);
  }
