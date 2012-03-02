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

#include <config.h>
#include <stdlib.h>
#include <string.h>

#include <gmerlin/pluginregistry.h>
#include <gmerlin/ov.h>

#define LOCK(p) bg_plugin_lock(p->h)
#define UNLOCK(p) bg_plugin_unlock(p->h)

struct bg_ov_s
  {
  bg_plugin_handle_t * h;
  bg_ov_plugin_t * plugin;
  void * priv;
  gavl_video_format_t format;

  int has_hw_overlay;
  int is_open;

  struct
    {
    gavl_overlay_blend_context_t * ctx;
    gavl_overlay_t * ovl;
    gavl_video_format_t format;
    } * ovl_str;
  int num_ovl_str;
  gavl_video_frame_t * still_frame;
  int still_mode;
  };

bg_ov_t * bg_ov_create(bg_plugin_handle_t * h)
  {
  bg_ov_t * ret = calloc(1, sizeof(*ret));

  ret->h = h;
  bg_plugin_ref(h);
  
  ret->priv = h->priv;
  ret->plugin = (bg_ov_plugin_t*)(ret->plugin);

  
  return ret;
  }

void bg_ov_destroy(bg_ov_t * ov)
  {
  if(ov->is_open)
    {
    LOCK(ov);
    ov->plugin->close(ov->priv);
    UNLOCK(ov);
    }
  bg_plugin_unref(ov->h);
  }

void bg_ov_set_window(bg_ov_t * ov, const char * window_id)
  {
  if(ov->plugin->set_window)
    {
    LOCK(ov);
    ov->plugin->set_window(ov->priv, window_id);
    UNLOCK(ov);
    }
  }
const char * bg_ov_get_window(bg_ov_t * ov)
  {
  const char * ret;
  if(ov->plugin->get_window)
    {
    LOCK(ov);
    ret = ov->plugin->get_window(ov->priv);
    UNLOCK(ov);
    return ret;
    }
  return NULL;
  }

void bg_ov_set_window_options(bg_ov_t * ov, const char * name, 
                           const char * klass, 
                           const gavl_video_frame_t * icon,
                           const gavl_video_format_t * icon_format)
  {
  if(ov->plugin->set_window_options)
    {
    LOCK(ov);
    ov->plugin->set_window_options(ov->priv,
                                   name, klass, icon, icon_format);
    UNLOCK(ov);
    }
  }

void bg_ov_set_window_title(bg_ov_t * ov, const char * title)
  {
  if(ov->plugin->set_window_title)
    {
    LOCK(ov);
    ov->plugin->set_window_title(ov->priv, title);
    UNLOCK(ov);
    }
  }

void bg_ov_set_callbacks(bg_ov_t * ov, bg_ov_callbacks_t * callbacks)
  {
  if(ov->plugin->set_callbacks)
    {
    LOCK(ov);
    ov->plugin->set_callbacks(ov->priv, callbacks);
    UNLOCK(ov);
    }
  }

int  bg_ov_open(bg_ov_t * ov, gavl_video_format_t * format, int keep_aspect)
  {
  int ret;

  LOCK(ov);
  ret = ov->plugin->open(ov->priv, format, keep_aspect);
  UNLOCK(ov);
  gavl_video_format_copy(&ov->format, format);
  if(ret)
    ov->is_open = 1;
  return ret;
  }
  
gavl_video_frame_t * bg_ov_get_frame(bg_ov_t * ov)
  {
  gavl_video_frame_t * ret;
  LOCK(ov);
  ret = ov->plugin->get_frame(ov->priv);
  UNLOCK(ov);
  return ret;
  }

int bg_ov_add_overlay_stream(bg_ov_t * ov, gavl_video_format_t * format)
  {
  int ret;
  
  if(ov->plugin->add_overlay_stream)
    {
    /* Try hardware overlay */
    LOCK(ov);
    ret = ov->plugin->add_overlay_stream(ov->priv, format);
    UNLOCK(ov);
    
    if(ret >= 0)
      {
      ov->has_hw_overlay = 1;
      return ret;
      }
    }
  /* Software overlay */
  ov->has_hw_overlay = 0;

  ov->ovl_str = realloc(ov->ovl_str,
                        (ov->num_ovl_str+1)*
                        sizeof(*ov->ovl_str));
  memset(ov->ovl_str + ov->num_ovl_str, 0, sizeof(*ov->ovl_str));
  
  ov->ovl_str[ov->num_ovl_str].ctx =
    gavl_overlay_blend_context_create();
  gavl_overlay_blend_context_init(ov->ovl_str[ov->num_ovl_str].ctx,
                                  &ov->format,
                                  &ov->ovl_str[ov->num_ovl_str].format);

  ov->num_ovl_str++;
  return ov->num_ovl_str-1;
  }
  
gavl_overlay_t * bg_ov_create_overlay(bg_ov_t * ov, int id)
  {
  gavl_overlay_t * ret;
  if(ov->has_hw_overlay)
    {
    LOCK(ov);
    ret = ov->plugin->create_overlay(ov->priv, id);
    UNLOCK(ov);
    return ret;
    }
  else
    {
    ret = calloc(1, sizeof(*ret));
    ret->frame = gavl_video_frame_create(&ov->ovl_str[id].format);
    gavl_video_frame_clear(ret->frame, &ov->ovl_str[id].format);
    return ret;
    }
  }

void bg_ov_set_overlay(bg_ov_t * ov, int stream, gavl_overlay_t * ovl)
  {
  if(ov->has_hw_overlay)
    {
    LOCK(ov);
    ov->plugin->set_overlay(ov->priv, stream, ovl);
    UNLOCK(ov);
    }
  else
    {
    gavl_overlay_blend_context_set_overlay(ov->ovl_str[stream].ctx, ovl);
    ov->ovl_str[stream].ovl = ovl;
    }
  }

void bg_ov_put_video(bg_ov_t * ov, gavl_video_frame_t*frame)
  {
  ov->still_mode = 0;
  LOCK(ov);
  ov->plugin->put_video(ov->priv, frame);
  UNLOCK(ov);
  }
  
void bg_ov_put_still(bg_ov_t * ov, gavl_video_frame_t*frame)
  {
  if(ov->plugin->put_still)
    {
    LOCK(ov);
    ov->plugin->put_still(ov->priv, frame);
    UNLOCK(ov);
    return;
    }
  /* Still frames not supported */
  else
    {
    if(!ov->still_frame)
      ov->still_frame =
        gavl_video_frame_create(&ov->format);
    }
  gavl_video_frame_copy(&ov->format,
                        ov->still_frame,
                        frame);
  ov->still_mode = 1;
  LOCK(ov);
  ov->plugin->put_video(ov->priv, frame);
  UNLOCK(ov);
  }

void bg_ov_handle_events(bg_ov_t * ov)
  {
  if(ov->still_mode)
    {
    gavl_video_frame_t * f;
    LOCK(ov);
    f = ov->plugin->get_frame(ov->priv);
    gavl_video_frame_copy(&ov->format,
                          f, ov->still_frame);
    ov->plugin->put_video(ov->priv, f);
    UNLOCK(ov);
    }
  
  if(ov->plugin->handle_events)
    {
    LOCK(ov);
    ov->plugin->handle_events(ov->priv);
    UNLOCK(ov);
    }
  }

void bg_ov_update_aspect(bg_ov_t * ov, int pixel_width, int pixel_height)
  {
  if(ov->plugin->update_aspect)
    {
    LOCK(ov);
    ov->plugin->update_aspect(ov->priv, pixel_width, pixel_height);
    UNLOCK(ov);
    }

  }

void bg_ov_destroy_overlay(bg_ov_t * ov, int id, gavl_overlay_t * ovl)
  {

  }
  
void bg_ov_close(bg_ov_t * ov)
  {
  LOCK(ov);
  ov->plugin->close(ov->priv);
  UNLOCK(ov);

  if(ov->still_frame)
    {
    gavl_video_frame_destroy(ov->still_frame);
    ov->still_frame = NULL;
    }
  }

void bg_ov_show_window(bg_ov_t * ov, int show)
  {
  if(ov->plugin->show_window)
    {
    LOCK(ov);
    ov->plugin->show_window(ov->priv, show);
    UNLOCK(ov);
    }
  }
