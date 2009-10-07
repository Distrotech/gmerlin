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
#include <x11/x11.h>
#include <x11/x11_window_private.h>

int bg_x11_window_add_overlay_stream(bg_x11_window_t * w,
                                     gavl_video_format_t * format)
  {
  w->overlay_streams =
    realloc(w->overlay_streams,
            (w->num_overlay_streams+1) * sizeof(*(w->overlay_streams)));
  memset(&w->overlay_streams[w->num_overlay_streams], 0,
         sizeof(w->overlay_streams[w->num_overlay_streams]));
  
  /* Initialize */
  gavl_video_format_copy(&w->overlay_streams[w->num_overlay_streams].format,
                         format); 
  if(w->current_driver->driver->add_overlay_stream)
    {
    w->current_driver->driver->add_overlay_stream(w->current_driver);
    }
  else
    {
    w->overlay_streams[w->num_overlay_streams].ctx = gavl_overlay_blend_context_create();
    gavl_overlay_blend_context_init(w->overlay_streams[w->num_overlay_streams].ctx,
                                    &(w->video_format),
                                    &w->overlay_streams[w->num_overlay_streams].format);
    }
  
  gavl_video_format_copy(format,
                         &w->overlay_streams[w->num_overlay_streams].format); 
  
  w->num_overlay_streams++;
  return w->num_overlay_streams - 1;
  }

void bg_x11_window_set_overlay(bg_x11_window_t * w, int stream,
                               gavl_overlay_t * ovl)
  {
  int i;
  w->overlay_streams[stream].ovl = ovl;
  if(w->current_driver->driver->set_overlay)
    w->current_driver->driver->set_overlay(w->current_driver, stream, ovl);
  else
    gavl_overlay_blend_context_set_overlay(w->overlay_streams[stream].ctx, ovl);
  w->has_overlay = 0;
  for(i = 0; i < w->num_overlay_streams; i++)
    {
    if(w->overlay_streams[i].ovl)
      {
      w->has_overlay = 1;
      break;
      }
    }
  }

gavl_overlay_t *
bg_x11_window_create_overlay(bg_x11_window_t * w, int stream)
  {
  gavl_overlay_t * ret;
  ret = calloc(1, sizeof(*ret));
  if(w->current_driver->driver->create_overlay)
    ret->frame = w->current_driver->driver->create_overlay(w->current_driver,
                                                           stream);
  else
    ret->frame = gavl_video_frame_create(&w->overlay_streams[stream].format);
  return ret;
  }

void bg_x11_window_destroy_overlay(bg_x11_window_t * w, int stream,
                                   gavl_overlay_t * ovl)
  {
  if(w->current_driver->driver->destroy_overlay)
    w->current_driver->driver->destroy_overlay(w->current_driver,
                                               stream, ovl->frame);
  else
    gavl_video_frame_destroy(ovl->frame);
  free(ovl);
  }
