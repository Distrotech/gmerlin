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

#include "player.h"
#include "playerprivate.h"

struct bg_player_ov_context_s
  {
  bg_plugin_handle_t * plugin_handle;
  bg_ov_plugin_t     * plugin;
  void               * priv;
  bg_player_t           * player;
  int do_sync;
  bg_ov_callbacks_t callbacks;
  
  gavl_video_frame_t  * logo_frame;
  gavl_video_format_t   logo_format;

  const char * error_msg;
  };

/* Callback functions */

static void key_callback(void * data, int key)
  {
  fprintf(stderr, "Key callback %c\n", key);
  }

static void button_callback(void * data, int x, int y, int button)
  {
  bg_player_ov_context_t * ctx = (bg_player_ov_context_t*)data;

  if(button == 4)
    bg_player_seek_rel(ctx->player, 2 * GAVL_TIME_SCALE );
  else if(button == 5)
    bg_player_seek_rel(ctx->player, - 2 * GAVL_TIME_SCALE );
    
  
  fprintf(stderr, "Button callback %d %d (Button %d)\n", x, y, button);
  }

/* Create frame */

void * bg_player_ov_create_frame(void * data)
  {
  void * ret;
  bg_player_ov_context_t * ctx;
  ctx = (bg_player_ov_context_t *)data;

  if(ctx->plugin->alloc_frame)
    {
    bg_plugin_lock(ctx->plugin_handle);
    ret = ctx->plugin->alloc_frame(ctx->priv);
    bg_plugin_unlock(ctx->plugin_handle);
    }
  else
    ret = gavl_video_frame_create(&(ctx->player->video_stream.output_format));
  gavl_video_frame_clear(ret, &(ctx->player->video_stream.output_format));
  return ret;
  }

void bg_player_ov_destroy_frame(void * data, void * frame)
  {
  bg_player_ov_context_t * ctx;
  ctx = (bg_player_ov_context_t *)data;
  
  if(ctx->plugin->free_frame)
    {
    bg_plugin_lock(ctx->plugin_handle);
    ctx->plugin->free_frame(ctx->priv, (gavl_video_frame_t*)frame);
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
  
  /* Load output plugin */
  
  ctx->callbacks.key_callback    = key_callback;
  ctx->callbacks.button_callback = button_callback;
  ctx->callbacks.data = ctx;
  player->ov_context = ctx;
  }

void bg_player_ov_standby(bg_player_ov_context_t * ctx)
  {
  //  fprintf(stderr, "bg_player_ov_standby\n");
  
  if(!ctx->plugin_handle)
    return;

  bg_plugin_lock(ctx->plugin_handle);
  if((ctx->plugin->put_still) && ctx->logo_frame)
    ctx->plugin->put_still(ctx->priv,
                           &(ctx->logo_format),
                           ctx->logo_frame);
  
  bg_plugin_unlock(ctx->plugin_handle);
  }

void bg_player_ov_set_logo(bg_player_ov_context_t * ctx,
                           gavl_video_format_t * format,
                           gavl_video_frame_t * frame)
  {
  if(ctx->logo_frame)
    gavl_video_frame_destroy(ctx->logo_frame);
  
  ctx->logo_frame = frame;

  gavl_video_format_copy(&(ctx->logo_format), format);;
  
  }

void bg_player_ov_set_plugin(bg_player_t * player, bg_plugin_handle_t * handle)
  {
  bg_player_ov_context_t * ctx;

  ctx = player->ov_context;

  if(ctx->plugin_handle)
    bg_plugin_unref(ctx->plugin_handle);
  

  ctx->plugin_handle = handle;

  bg_plugin_ref(ctx->plugin_handle);

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

  if(ctx->logo_frame)
    gavl_video_frame_destroy(ctx->logo_frame);
  
  free(ctx);
  }

int bg_player_ov_init(bg_player_ov_context_t * ctx)
  {
  int result;
  
  gavl_video_format_copy(&(ctx->player->video_stream.output_format),
                         &(ctx->player->video_stream.input_format));

  bg_plugin_lock(ctx->plugin_handle);
  result = ctx->plugin->open(ctx->priv,
                             &(ctx->player->video_stream.output_format),
                             "Video output");
  if(result && ctx->plugin->show_window)
    ctx->plugin->show_window(ctx->priv, 1);

  else if(!result)
    {
    if(ctx->plugin->common.get_error)
      ctx->error_msg = ctx->plugin->common.get_error(ctx->priv);
    }
  
  bg_plugin_unlock(ctx->plugin_handle);
  return result;
  }

void bg_player_ov_cleanup(bg_player_ov_context_t * ctx)
  {
  bg_plugin_lock(ctx->plugin_handle);
  ctx->plugin->close(ctx->priv);
  bg_plugin_unlock(ctx->plugin_handle);
  }

void bg_player_ov_sync(bg_player_t * p)
  {
  p->ov_context->do_sync  = 1;
  }


const char * bg_player_ov_get_error(bg_player_ov_context_t * ctx)
  {
  return ctx->error_msg;
  }


void * bg_player_ov_thread(void * data)
  {
  bg_player_ov_context_t * ctx;
  gavl_video_frame_t * frame;
  gavl_time_t diff_time;
  gavl_time_t current_time;
  bg_fifo_state_t state;
  
  ctx = (bg_player_ov_context_t*)data;
  

  while(1)
    {
    if(!bg_player_keep_going(ctx->player))
      break;
    frame = bg_fifo_lock_read(ctx->player->video_stream.fifo, &state);
    if(!frame)
      break;

    bg_player_time_get(ctx->player, 1, &current_time);
#if 1
    fprintf(stderr, "F: %f, C: %f\n",
            gavl_time_to_seconds(frame->time),
            gavl_time_to_seconds(current_time));
#endif

    if(!ctx->do_sync)
      {
      diff_time = frame->time - current_time;
      
      /* Wait until we can display the frame */
      if(diff_time > 0)
        gavl_time_delay(&diff_time);
    
      /* TODO: Drop frames */
      else if(diff_time < -100000)
        {
        }
      }
    //    fprintf(stderr, "Frame time: %lld\n", frame->time);
    bg_plugin_lock(ctx->plugin_handle);
    ctx->plugin->put_video(ctx->priv, frame);

    if(ctx->do_sync)
      {
      bg_player_time_set(ctx->player, frame->time);
      //      fprintf(stderr, "OV Resync %lld\n", frame->time);
      ctx->do_sync = 0;
      }
    
    bg_plugin_unlock(ctx->plugin_handle);
    bg_fifo_unlock_read(ctx->player->video_stream.fifo);
    }
  
    //  fprintf(stderr, "ov thread finisheded\n");
  return NULL;
  }

