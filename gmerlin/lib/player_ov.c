/*****************************************************************
 
  player_ov.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <keycodes.h>
#include <accelerator.h>

#include <player.h>
#include <playerprivate.h>

#include <log.h>
#define LOG_DOMAIN "player.video_output"

struct bg_player_ov_context_s
  {
  bg_plugin_handle_t * plugin_handle;
  bg_ov_plugin_t     * plugin;
  void               * priv;
  bg_player_t        * player;
  bg_ov_callbacks_t callbacks;
  gavl_video_frame_t * frame;
  gavl_time_t frame_time;

  gavl_video_frame_t * still_frame;
  pthread_mutex_t     still_mutex;
  
  int still_shown;

  gavl_overlay_t       current_subtitle;
  gavl_overlay_t     * next_subtitle;
  int subtitle_id; /* Stream id for subtitles in the output plugin */
  int has_subtitle;

  bg_osd_t * osd;
  int osd_id;
  gavl_overlay_t * osd_ovl;
  
  bg_msg_queue_t * msg_queue;
  
  int64_t frames_written;
  
  bg_accelerator_map_t * accel_map;
  };

/* Callback functions */
#if 0
static void key_callback(void * data, int key, int mask)
  {
  bg_player_ov_context_t * ctx = (bg_player_ov_context_t*)data;

  switch(key)
    {
    /* Take the keys, we can handle from here */
    case BG_KEY_LEFT:
      if(mask == BG_KEY_SHIFT_MASK)
        {
        bg_player_set_volume_rel(ctx->player,  -1.0);
        return;
        }
      else if(mask == BG_KEY_CONTROL_MASK)
        {
        bg_player_seek_rel(ctx->player,   -2 * GAVL_TIME_SCALE );
        return;
        }
      break;
    case BG_KEY_RIGHT:
      if(mask == BG_KEY_SHIFT_MASK)
        {
        bg_player_set_volume_rel(ctx->player,  1.0);
        return;
        }
      else if(mask == BG_KEY_CONTROL_MASK)
        {
        bg_player_seek_rel(ctx->player,   2 * GAVL_TIME_SCALE );
        return;
        }
      break;
    case BG_KEY_0:
      if(!mask)
        {
        bg_player_seek(ctx->player, 0 );
        return;
        }
      break;
    case BG_KEY_SPACE:
      if(!mask)
        {
        bg_player_pause(ctx->player);
        return;
        }
      break;
    case BG_KEY_M:
      if(mask == BG_KEY_CONTROL_MASK)
        {
        bg_player_toggle_mute(ctx->player);
        return;
        }
      break;
    case BG_KEY_PAGE_UP:
      if(mask == (BG_KEY_SHIFT_MASK | BG_KEY_CONTROL_MASK))
        {
        bg_player_prev_chapter(ctx->player);
        return;
        }
      break;
    case BG_KEY_PAGE_DOWN:
      if(mask == (BG_KEY_SHIFT_MASK | BG_KEY_CONTROL_MASK))
        {
        bg_player_next_chapter(ctx->player);
        return;
        }
      break;
    default:
      break;
    }
  /* Broadcast event if we didn't handle it */
  bg_player_key_pressed(ctx->player, key, mask);
  }
#else

static int accel_callback(void * data, int id)
  {
  bg_player_ov_context_t * ctx = (bg_player_ov_context_t*)data;
  bg_player_accel_pressed(ctx->player, id);
  return 1;
  }
#endif


static int
button_callback(void * data, int x, int y, int button, int mask)
  {
  bg_player_ov_context_t * ctx = (bg_player_ov_context_t*)data;

  if(button == 4)
    {
    bg_player_seek_rel(ctx->player, 2 * GAVL_TIME_SCALE );
    return 1;
    }
  else if(button == 5)
    {
    bg_player_seek_rel(ctx->player, - 2 * GAVL_TIME_SCALE );
    return 1;
    }
  return 0;
  }

static void brightness_callback(void * data, float val)
  {
  bg_player_ov_context_t * ctx = (bg_player_ov_context_t*)data;
  bg_osd_set_brightness_changed(ctx->osd, val, ctx->frame_time);
  }

static void saturation_callback(void * data, float val)
  {
  bg_player_ov_context_t * ctx = (bg_player_ov_context_t*)data;
  bg_osd_set_saturation_changed(ctx->osd, val, ctx->frame_time);
  }

static void contrast_callback(void * data, float val)
  {
  bg_player_ov_context_t * ctx = (bg_player_ov_context_t*)data;
  bg_osd_set_contrast_changed(ctx->osd, val, ctx->frame_time);
  }

static void
handle_messages(bg_player_ov_context_t * ctx, gavl_time_t time)
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

int  bg_player_ov_has_plugin(bg_player_ov_context_t * ctx)
  {
  return (ctx->plugin_handle ? 1 : 0);
  }

bg_plugin_handle_t * bg_player_ov_get_plugin(bg_player_ov_context_t * ctx)
  {
  return ctx->plugin_handle;
  }


/* Create frame */

void * bg_player_ov_create_frame(void * data)
  {
  gavl_video_frame_t * ret;
  bg_player_ov_context_t * ctx;
  ctx = (bg_player_ov_context_t *)data;

  if(ctx->plugin->create_frame)
    {
    bg_plugin_lock(ctx->plugin_handle);
    ret = ctx->plugin->create_frame(ctx->priv);
    bg_plugin_unlock(ctx->plugin_handle);
    }
  else
    ret = gavl_video_frame_create(&(ctx->player->video_stream.output_format));

  gavl_video_frame_clear(ret, &(ctx->player->video_stream.output_format));
  
  return (void*)ret;
  }

void bg_player_ov_destroy_frame(void * data, void * frame)
  {
  bg_player_ov_context_t * ctx;
  ctx = (bg_player_ov_context_t *)data;
  
  if(ctx->plugin->destroy_frame)
    {
    bg_plugin_lock(ctx->plugin_handle);
    ctx->plugin->destroy_frame(ctx->priv, (gavl_video_frame_t*)frame);
    bg_plugin_unlock(ctx->plugin_handle);
    }
  else
    gavl_video_frame_destroy((gavl_video_frame_t*)frame);
  }

void bg_player_ov_create(bg_player_t * player)
  {
  bg_player_ov_context_t * ctx;
  ctx = calloc(1, sizeof(*ctx));
  ctx->player = player;

  ctx->msg_queue = bg_msg_queue_create();
  ctx->accel_map = bg_accelerator_map_create();
  
  pthread_mutex_init(&(ctx->still_mutex),(pthread_mutexattr_t *)0);
  
  /* Load output plugin */
  
  ctx->callbacks.accel_callback    = accel_callback;
  ctx->callbacks.button_callback = button_callback;

  ctx->callbacks.brightness_callback = brightness_callback;
  ctx->callbacks.saturation_callback = saturation_callback;
  ctx->callbacks.contrast_callback   = contrast_callback;
  
  ctx->callbacks.data      = ctx;
  ctx->callbacks.accel_map = ctx->accel_map;
  
  player->ov_context = ctx;

  ctx->osd = bg_osd_create();
  }

void bg_player_add_accelerators(bg_player_t * player, const bg_accelerator_t * list)
  {
  if(player->ov_context->plugin_handle)
    bg_plugin_lock(player->ov_context->plugin_handle);

  bg_accelerator_map_append_array(player->ov_context->accel_map, list);
  
  if(player->ov_context->plugin_handle)
    bg_plugin_unlock(player->ov_context->plugin_handle);
  }


void bg_player_ov_standby(bg_player_ov_context_t * ctx)
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
  bg_player_ov_context_t * ctx;

  ctx = player->ov_context;

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
  bg_player_ov_context_t * ctx;
  
  ctx = player->ov_context;
  
  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  bg_osd_destroy(ctx->osd);

  bg_msg_queue_destroy(ctx->msg_queue);
  
  free(ctx);
  }

int bg_player_ov_init(bg_player_ov_context_t * ctx)
  {
  gavl_video_format_t osd_format;
  int result;
  
  ctx->next_subtitle    = (gavl_overlay_t*)0;
  ctx->has_subtitle = 0;
  
  bg_plugin_lock(ctx->plugin_handle);
  result = ctx->plugin->open(ctx->priv,
                             &(ctx->player->video_stream.output_format));
  
  ctx->plugin->set_window_title(ctx->priv, "Video output");
  
  if(result && ctx->plugin->show_window)
    ctx->plugin->show_window(ctx->priv, 1);
  else if(!result)
    return result;
  
  memset(&(osd_format), 0, sizeof(osd_format));
  
  bg_osd_init(ctx->osd, &(ctx->player->video_stream.output_format),
              &osd_format);

  ctx->osd_id = ctx->plugin->add_overlay_stream(ctx->priv,
                                                      &osd_format);
  ctx->osd_ovl = bg_osd_get_overlay(ctx->osd);
  ctx->frames_written = 0;
  bg_plugin_unlock(ctx->plugin_handle);
  return result;
  }

void bg_player_ov_update_still(bg_player_ov_context_t * ctx)
  {
  bg_fifo_state_t state;

  pthread_mutex_lock(&ctx->still_mutex);

  if(ctx->frame)
    bg_fifo_unlock_read(ctx->player->video_stream.fifo);

  ctx->frame = bg_fifo_lock_read(ctx->player->video_stream.fifo, &state);

  if(!ctx->still_frame)
    {
    ctx->still_frame = bg_player_ov_create_frame(ctx);
    }
  
  if(ctx->frame)
    {
    gavl_video_frame_copy(&(ctx->player->video_stream.output_format),
                          ctx->still_frame, ctx->frame);
    bg_fifo_unlock_read(ctx->player->video_stream.fifo);
    ctx->frame = (gavl_video_frame_t*)0;
    }
  else
    {
    gavl_video_frame_clear(ctx->still_frame, &(ctx->player->video_stream.output_format));
    }
  
  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->put_still(ctx->priv, ctx->still_frame);
  bg_plugin_unlock(ctx->plugin_handle);

  
  pthread_mutex_unlock(&ctx->still_mutex);
  }

void bg_player_ov_cleanup(bg_player_ov_context_t * ctx)
  {
  pthread_mutex_lock(&ctx->still_mutex);
  if(ctx->still_frame)
    {
    bg_player_ov_destroy_frame(ctx, ctx->still_frame);
    ctx->still_frame = (gavl_video_frame_t*)0;
    }
  if(ctx->current_subtitle.frame)
    {
    gavl_video_frame_destroy(ctx->current_subtitle.frame);
    ctx->current_subtitle.frame = (gavl_video_frame_t*)0;
    }
  
  pthread_mutex_unlock(&ctx->still_mutex);

  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->close(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);

  }

void bg_player_ov_reset(bg_player_t * player)
  {
  bg_player_ov_context_t * ctx;
  ctx = player->ov_context;

  if(ctx->has_subtitle)
    {
    ctx->plugin->set_overlay(ctx->priv, ctx->subtitle_id, (gavl_overlay_t*)0);
    ctx->has_subtitle = 0;
    }
  
  ctx->next_subtitle = (gavl_overlay_t*)0;
  }

void bg_player_ov_update_aspect(bg_player_ov_context_t * ctx,
                                int pixel_width, int pixel_height)
  {
  bg_plugin_lock(ctx->plugin_handle);
  if(ctx->plugin && ctx->plugin->update_aspect)
    ctx->plugin->update_aspect(ctx->priv, pixel_width, pixel_height);
  bg_plugin_unlock(ctx->plugin_handle);
  }

/* Set this extra because we must initialize subtitles after the video output */
void bg_player_ov_set_subtitle_format(void * data)
  {
  bg_player_ov_context_t * ctx;
  ctx = (bg_player_ov_context_t*)data;
  
  gavl_video_format_copy(&ctx->player->subtitle_stream.output_format,
                         &ctx->player->subtitle_stream.input_format);
  
  /* Add subtitle stream for plugin */
  
  ctx->subtitle_id =
    ctx->plugin->add_overlay_stream(ctx->priv,
                                    &ctx->player->subtitle_stream.output_format);
  
  /* Allocate private overlay frame */
  ctx->current_subtitle.frame =
    gavl_video_frame_create(&ctx->player->subtitle_stream.output_format);
  }



static void ping_func(void * data)
  {
  bg_player_ov_context_t * ctx;
  ctx = (bg_player_ov_context_t*)data;
  
  pthread_mutex_lock(&ctx->still_mutex);
  
  if(!ctx->still_shown)
    {
    if(ctx->frame)
      {
      ctx->still_frame = bg_player_ov_create_frame(data);
      
      gavl_video_frame_copy(&(ctx->player->video_stream.output_format),
                            ctx->still_frame, ctx->frame);
      bg_fifo_unlock_read(ctx->player->video_stream.fifo);
      ctx->frame = (gavl_video_frame_t*)0;
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

void * bg_player_ov_thread(void * data)
  {
  gavl_overlay_t tmp_overlay;
  
  bg_player_ov_context_t * ctx;
  gavl_time_t diff_time;
  gavl_time_t current_time;
  bg_fifo_state_t state;
  
  ctx = (bg_player_ov_context_t*)data;

  bg_player_add_message_queue(ctx->player,
                              ctx->msg_queue);

  

  while(1)
    {
    if(!bg_player_keep_going(ctx->player, ping_func, ctx))
      {
      break;
      }
    if(ctx->frame)
      {
      bg_fifo_unlock_read(ctx->player->video_stream.fifo);
      
      ctx->frame = (gavl_video_frame_t*)0;
      }

    pthread_mutex_lock(&ctx->still_mutex);
    if(ctx->still_frame)
      {
      bg_player_ov_destroy_frame(data, ctx->still_frame);
      ctx->still_frame = (gavl_video_frame_t*)0;
      }
    pthread_mutex_unlock(&ctx->still_mutex);
    
    ctx->still_shown = 0;

    ctx->frame = bg_fifo_lock_read(ctx->player->video_stream.fifo, &state);
    if(!ctx->frame)
      {
      if(state == BG_FIFO_STOPPED) 
        {
        break;
        }
      else if(state == BG_FIFO_PAUSED)
        {
        continue;
        }
      break;
      }

    /* Get frame time */
    ctx->frame_time = gavl_time_unscale(ctx->player->video_stream.output_format.timescale,
                                        ctx->frame->timestamp);

    
    /* Subtitle handling */
    if(DO_SUBTITLE(ctx->player->flags))
      {

      /* Try to get next subtitle */
      if(!ctx->next_subtitle)
        {
        ctx->next_subtitle = bg_fifo_try_lock_read(ctx->player->subtitle_stream.fifo,
                                                  &state);
        }
      /* Check if the overlay is expired */
      if(ctx->has_subtitle)
        {
        if(bg_overlay_too_old(ctx->frame_time,
                              ctx->current_subtitle.frame->timestamp,
                              ctx->current_subtitle.frame->duration))
          {
          ctx->plugin->set_overlay(ctx->priv, ctx->subtitle_id, (gavl_overlay_t*)0);
          ctx->has_subtitle = 0;
          }
        }
      
      /* Check if new overlay should be used */
      
      if(ctx->next_subtitle)
        {
        if(!bg_overlay_too_new(ctx->frame_time,
                               ctx->next_subtitle->frame->timestamp))
          {
          memcpy(&tmp_overlay, ctx->next_subtitle, sizeof(tmp_overlay));
          memcpy(ctx->next_subtitle, &(ctx->current_subtitle),
                 sizeof(tmp_overlay));
          memcpy(&(ctx->current_subtitle), &tmp_overlay, sizeof(tmp_overlay));
          ctx->plugin->set_overlay(ctx->priv, ctx->subtitle_id,
                                   &(ctx->current_subtitle));
          
          ctx->has_subtitle = 1;
          ctx->next_subtitle = (gavl_overlay_t*)0;
          bg_fifo_unlock_read(ctx->player->subtitle_stream.fifo);
          }
        }
      }
    /* Handle message */
    handle_messages(ctx, ctx->frame_time);
    
    /* Display OSD */

    if(bg_osd_overlay_valid(ctx->osd, ctx->frame_time))
      ctx->plugin->set_overlay(ctx->priv, ctx->osd_id, ctx->osd_ovl);
    else
      ctx->plugin->set_overlay(ctx->priv, ctx->osd_id, (gavl_overlay_t*)0);
    
    /* Check Timing */
    bg_player_time_get(ctx->player, 1, &current_time);
    
    diff_time =  ctx->frame_time - current_time;
    
    /* Wait until we can display the frame */
    if(diff_time > 0)
      gavl_time_delay(&diff_time);
    
    /* TODO: Drop frames */
    else if(diff_time < -100000)
      {
      }
    
    bg_plugin_lock(ctx->plugin_handle);
    ctx->plugin->put_video(ctx->priv, ctx->frame);
    ctx->plugin->handle_events(ctx->priv);
    ctx->frames_written++;
    
    bg_plugin_unlock(ctx->plugin_handle);
    }

  bg_player_delete_message_queue(ctx->player,
                              ctx->msg_queue);
  return NULL;
  }

bg_parameter_info_t * bg_player_get_osd_parameters(bg_player_t * p)
  {
  return bg_osd_get_parameters(p->ov_context->osd);
  }

void bg_player_set_osd_parameter(void * data, const char * name, const bg_parameter_value_t*val)
  {
  bg_player_t * p = (bg_player_t *)data;
  bg_osd_set_parameter(p->ov_context->osd, name, val);
  }
