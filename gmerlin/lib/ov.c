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

#include <gmerlin/pluginregistry.h>
#include <gmerlin/ov.h>

struct bg_ov_s
  {
  bg_plugin_handle_t * h;
  bg_ov_plugin_t * plugin;
  void * priv;
  gavl_video_format_t format;
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
  bg_plugin_unref(ov->h);
  }

void bg_ov_set_window(bg_ov_t * ov, const char * window_id)
  {
  if(ov->plugin->set_window)
    ov->plugin->set_window(ov->priv, window_id);
  }

const char * bg_ov_get_window(bg_ov_t * ov)
  {
  if(ov->plugin->get_window)
    return ov->plugin->get_window(ov->priv);
  }

void bg_ov_set_window_options(bg_ov_t * ov, const char * name, 
                           const char * klass, 
                           const gavl_video_frame_t * icon,
                           const gavl_video_format_t * icon_format)
  {
  if(ov->plugin->set_window_options)
    return ov->plugin->set_window_options(ov->priv,
                                          name, klass, icon, icon_format);
  }

void bg_ov_set_window_title(bg_ov_t * ov, const char * title)
  {
  if(ov->plugin->set_window_title)
    return ov->plugin->set_window_title(ov->priv, title);
  
  }

void bg_ov_set_callbacks(bg_ov_t * ov, bg_ov_callbacks_t * callbacks)
  {
  if(ov->plugin->set_callbacks)
    ov->plugin->set_callbacks(ov->priv, callbacks);
  }


int  bg_ov_open(bg_ov_t * ov, gavl_video_format_t * format, int keep_aspect)
  {
  int ret = ov->plugin->open(ov->priv, format, keep_aspect);
  gavl_video_format_copy(&ov->format, format);
  return ret;
  }
  
gavl_video_frame_t * bg_ov_get_frame(bg_ov_t * ov)
  {
  return ov->plugin->get_frame(ov->priv);
  }

int bg_ov_add_overlay_stream(bg_ov_t * ov, gavl_video_format_t * format)
  {
  int ret;
  if(!ov->plugin->add_overlay_stream ||
     ((ret = ov->plugin->add_overlay_stream(ov, format)) < 0))
    {
    /* Software overlay */
    }
  else
    {
    return ret;
    }
  }
  
gavl_overlay_t * bg_ov_create_overlay(bg_ov_t * ov, int id)
  {
  
  }

void bg_ov_set_overlay(bg_ov_t * ov, int stream, gavl_overlay_t * ovl)
  {

  }

void bg_ov_put_video(bg_ov_t * ov, gavl_video_frame_t*frame)
  {
  ov->plugin->put_video(ov->priv, frame);
  }
  
void bg_ov_put_still(bg_ov_t * ov, gavl_video_frame_t*frame)
  {
  
  }

void bg_ov_handle_events(bg_ov_t * ov)
  {
  
  }

void bg_ov_update_aspect(bg_ov_t * ov, int pixel_width, int pixel_height)
  {

  }

void bg_ov_destroy_overlay(bg_ov_t * ov, int id, gavl_overlay_t * ovl)
  {

  }
  
void bg_ov_close(bg_ov_t * ov)
  {

  }

void bg_ov_show_window(bg_ov_t * ov, int show)
  {
  
  }
