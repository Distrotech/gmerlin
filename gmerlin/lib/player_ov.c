/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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
        if(ctx->osd_ovl)
          bg_osd_set_volume_changed(ctx->osd,
                                    (arg_f - BG_PLAYER_VOLUME_MIN)/
                                    (-BG_PLAYER_VOLUME_MIN),
                                    time);
        break;
      default:
        break;
      }

    bg_msg_queue_unlock_read(ctx->msg_queue);
    }
  }

void bg_player_ov_create(bg_player_t * player)
  {
  bg_player_video_stream_t * s = &player->video_stream;
  
  s->callbacks.accel_callback    = accel_callback;
  s->callbacks.button_callback = button_callback;

  s->callbacks.brightness_callback = brightness_callback;
  s->callbacks.saturation_callback = saturation_callback;
  s->callbacks.contrast_callback   = contrast_callback;
  
  s->callbacks.data      = player;
  s->callbacks.accel_map = s->accel_map;
  
  }

void bg_player_add_accelerators(bg_player_t * player,
                                const bg_accelerator_t * list)
  {
  bg_accelerator_map_append_array(player->video_stream.accel_map, list);
  }


void bg_player_ov_standby(bg_player_video_stream_t * ctx)
  {
  if(!ctx->ov)
    return;
  bg_ov_show_window(ctx->ov, 0);
  }


void bg_player_ov_set_plugin(bg_player_t * player, bg_plugin_handle_t * handle)
  {
  bg_player_video_stream_t * ctx;

  ctx = &player->video_stream;

  if(ctx->ov)
    {
    bg_ov_destroy(ctx->ov);
    ctx->ov = NULL;
    }

  if(handle)
    {
    ctx->ov = bg_ov_create(handle);
    bg_ov_set_callbacks(ctx->ov, &ctx->callbacks);

    /* ov holds a private reference */
    bg_plugin_unref(handle);
    }
  }

void bg_player_ov_destroy(bg_player_t * player)
  {
  bg_player_video_stream_t * ctx = &player->video_stream;
  
  if(ctx->ov)
    bg_ov_destroy(ctx->ov);
  }

int bg_player_ov_init(bg_player_video_stream_t * vs)
  {
  int result;
  
  result = bg_ov_open(vs->ov, &vs->output_format, 1);
  
  bg_ov_set_window_title(vs->ov, "Video output");
  
  if(result)
    bg_ov_show_window(vs->ov, 1);
  
  else if(!result)
    return result;
  
  memset(&vs->osd_format, 0, sizeof(vs->osd_format));
  
  bg_osd_init(vs->osd, &vs->output_format,
              &vs->osd_format);
  /* Fixme: Lets just hope, that the OSD format doesn't get changed
     by this call. Otherwise, we would need a gavl_video_converter */

  vs->osd_id = bg_ov_add_overlay_stream(vs->ov,  &vs->osd_format);
  
  /* create_overlay needs the lock again */

  /* Create frame */
  
  if(vs->osd_id >= 0)
    {
    vs->osd_ovl = bg_ov_create_overlay(vs->ov, vs->osd_id);
    bg_osd_set_overlay(vs->osd, vs->osd_ovl);
    }
  vs->frames_written = 0;
  return result;
  }

static int overlay_is_expired(gavl_overlay_t * ovl,
                              gavl_time_t frame_time)
  {
  if((ovl->frame->duration > 0) &&
     (ovl->frame->timestamp + ovl->frame->duration > frame_time))
    return 1;
  return 0;
  }

static int overlay_is_early(gavl_overlay_t * ovl,
                            gavl_time_t frame_time)
  {
  if(ovl->frame->timestamp < frame_time)
    return 1;
  return 0;
  }

static void handle_subtitle(bg_player_t * p)
  {
  bg_player_video_stream_t * s = &p->video_stream;
  gavl_overlay_t * swp;
  
  /* Check if subtitle expired */
  if(s->subtitle_active &&
     !overlay_is_expired(s->ss->current_subtitle, s->frame_time))
    {
    bg_ov_set_overlay(s->ov, s->subtitle_id, NULL);
    
    /* Make invalid */
    s->ss->current_subtitle->frame->timestamp = GAVL_TIME_UNDEFINED;
    s->subtitle_active = 0;
    
    /* Swap */
    swp = s->ss->current_subtitle;
    s->ss->current_subtitle = s->ss->next_subtitle;
    s->ss->next_subtitle = swp;
    }
  
  /* Try to read as many subtitles as possible */
  
  if((s->ss->current_subtitle->frame->timestamp == GAVL_TIME_UNDEFINED) &&
     bg_player_has_subtitle(p) && !s->ss->eof)
    {
    if(!bg_player_read_subtitle(p, s->ss->current_subtitle))
      s->ss->eof = 1;
    }
  if((s->ss->next_subtitle->frame->timestamp == GAVL_TIME_UNDEFINED) &&
     bg_player_has_subtitle(p) && !s->ss->eof)
    {
    if(!bg_player_read_subtitle(p, s->ss->next_subtitle))
      s->ss->eof = 1;
    }
  
  /* Check if the current subtitle became valid */
  if(!s->subtitle_active &&
     (s->ss->current_subtitle->frame->timestamp != GAVL_TIME_UNDEFINED) &&
     !overlay_is_early(s->ss->current_subtitle, s->frame_time))
    {
    bg_ov_set_overlay(s->ov, s->subtitle_id, s->ss->current_subtitle);
    s->subtitle_active = 1;
    }
  /* Check if the next subtitle became valid */
  else if((s->ss->next_subtitle->frame->timestamp != GAVL_TIME_UNDEFINED) &&
          !overlay_is_early(s->ss->next_subtitle, s->frame_time))
    {
    if(s->subtitle_active)
      {
      s->ss->current_subtitle->frame->timestamp =
        GAVL_TIME_UNDEFINED;
      }
    
    /* Swap */
    swp = s->ss->current_subtitle;
    s->ss->current_subtitle = s->ss->next_subtitle;
    s->ss->next_subtitle = swp;

    bg_ov_set_overlay(s->ov, s->subtitle_id, s->ss->current_subtitle);
    s->subtitle_active = 1;
    }
  }


void bg_player_ov_update_still(bg_player_t * p)
  {
  gavl_video_frame_t * frame;
  bg_player_video_stream_t * s = &p->video_stream;
  
  frame = bg_ov_get_frame(s->ov);
  if(!bg_player_read_video(p, frame))
    return;
  s->frame_time =
    gavl_time_unscale(s->output_format.timescale,
                      frame->timestamp);
  
  if(DO_SUBTITLE(p->flags))
    handle_subtitle(p);
  
  handle_messages(s, s->frame_time);
  
  bg_ov_put_still(s->ov, frame);
  bg_ov_handle_events(s->ov);
  }

void bg_player_ov_cleanup(bg_player_video_stream_t * s)
  {
  if(s->osd_ovl)
    {
    bg_ov_destroy_overlay(s->ov, s->osd_id, s->osd_ovl);
    s->osd_ovl = NULL;
    }

  //  destroy_frame(s, s->frame);
  //  s->frame = NULL;

  if(s->ss->subtitles[0])
    {
    bg_ov_destroy_overlay(s->ov, s->subtitle_id, s->ss->subtitles[0]);
    s->ss->subtitles[0] = NULL;
    }
  if(s->ss->subtitles[1])
    {
    bg_ov_destroy_overlay(s->ov, s->subtitle_id, s->ss->subtitles[1]);
    s->ss->subtitles[1] = NULL;
    }
  
  bg_ov_close(s->ov);
  
  }

void bg_player_ov_reset(bg_player_t * p)
  {
  bg_player_video_stream_t * s = &p->video_stream;

  if(DO_SUBTITLE(p->flags))
    {
    if(s->subtitle_active)
      bg_ov_set_overlay(s->ov, s->subtitle_id, NULL);
    s->subtitle_active = 0;
    s->ss->current_subtitle->frame->timestamp = GAVL_TIME_UNDEFINED;
    s->ss->next_subtitle->frame->timestamp = GAVL_TIME_UNDEFINED;
    }
  }

void bg_player_ov_update_aspect(bg_player_video_stream_t * ctx,
                                int pixel_width, int pixel_height)
  {
  bg_ov_update_aspect(ctx->ov, pixel_width, pixel_height);
  }

/* Set this extra because we must initialize subtitles after the video output */
void bg_player_ov_set_subtitle_format(bg_player_video_stream_t * s)
  {
  gavl_video_format_copy(&s->ss->output_format,
                         &s->ss->input_format);
  
  /* Add subtitle stream for plugin */
  
  s->subtitle_id =
    bg_ov_add_overlay_stream(s->ov,
                                    &s->ss->output_format);
  
  /* Allocate overlay frames */
  
  s->ss->subtitles[0] = bg_ov_create_overlay(s->ov, s->subtitle_id);
  s->ss->subtitles[1] = bg_ov_create_overlay(s->ov, s->subtitle_id);

  s->ss->subtitles[0]->frame->timestamp = GAVL_TIME_UNDEFINED;  
  s->ss->subtitles[1]->frame->timestamp = GAVL_TIME_UNDEFINED;  
  
  s->ss->current_subtitle = s->ss->subtitles[0];
  s->ss->next_subtitle    = s->ss->subtitles[1];
  }

void bg_player_ov_handle_events(bg_player_video_stream_t * s)
  {
  bg_ov_handle_events(s->ov);
  handle_messages(s, s->frame_time);
  }

static void wait_or_skip(bg_player_t * p, gavl_time_t diff_time)
  {
  bg_player_video_stream_t * s;
  s = &p->video_stream;
  
  if(diff_time > 0)
    {
    gavl_time_delay(&diff_time);

    if(s->skip)
      {
      
      s->skip -= diff_time;
      if(s->skip < 0)
        s->skip = 0;

      // fprintf(stderr, "Diff time: %ld, skip: %ld\n", diff_time, s->skip);
      }
    }
  /* TODO: Drop frames */
  else if(diff_time < -100000) // 100 ms
    {
    s->skip -= diff_time;
    // fprintf(stderr, "Diff time: %ld, skip: %ld\n", diff_time, s->skip);
    }
  }

void * bg_player_ov_thread(void * data)
  {
  bg_player_video_stream_t * s;
  gavl_time_t diff_time;
  gavl_time_t current_time;
  
  bg_player_t * p = data;
  gavl_video_frame_t * frame;
  
  s = &p->video_stream;

  bg_player_add_message_queue(p, s->msg_queue);

  bg_player_thread_wait_for_start(s->th);

  
  while(1)
    {
    if(!bg_player_thread_check(s->th))
      {
      break;
      }
    
    frame = bg_ov_get_frame(s->ov);
    
    if(!bg_player_read_video(p, frame))
      {
      bg_player_video_set_eof(p);
      if(!bg_player_thread_wait_for_start(s->th))
        {
        break;
        }
      continue;
      }
    
    /* Get frame time */

    s->frame_time =
      gavl_time_unscale(s->output_format.timescale,
                        frame->timestamp);

    pthread_mutex_lock(&p->config_mutex);
    s->frame_time += p->sync_offset;
    pthread_mutex_unlock(&p->config_mutex);
    
    /* Subtitle handling */
    if(DO_SUBTITLE(p->flags))
      {
      handle_subtitle(p);
      }
    /* Handle message */
    handle_messages(s, s->frame_time);
    
    /* Display OSD */

    if(s->osd_id >= 0)
      {
      if(bg_osd_overlay_valid(s->osd, s->frame_time))
        bg_ov_set_overlay(s->ov, s->osd_id, s->osd_ovl);
      else
        bg_ov_set_overlay(s->ov, s->osd_id, NULL);
      }
    
    /* Check Timing */
    bg_player_time_get(p, 1, &current_time);

    diff_time =  s->frame_time - current_time;
    
#ifdef DUMP_TIMESTAMPS
    bg_debug("C: %"PRId64", F: %"PRId64", D: %"PRId64"\n",
             current_time, s->frame_time, diff_time);
#endif
    /* Wait until we can display the frame */

    wait_or_skip(p, diff_time);
    
    if(p->time_update_mode == TIME_UPDATE_FRAME)
      {
      bg_player_broadcast_time(p, s->frame_time);
      }
    
    bg_ov_put_video(s->ov, frame);
    bg_ov_handle_events(s->ov);
    s->frames_written++;
    }
  
  bg_player_delete_message_queue(p, s->msg_queue);
  return NULL;
  }

const bg_parameter_info_t * bg_player_get_osd_parameters(bg_player_t * p)
  {
  return bg_osd_get_parameters(p->video_stream.osd);
  }

void bg_player_set_osd_parameter(void * data,
                                 const char * name,
                                 const bg_parameter_value_t*val)
  {
  bg_player_t * p = data;
  bg_osd_set_parameter(p->video_stream.osd, name, val);
  }
