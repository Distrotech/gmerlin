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

#ifndef __BG_OV_H_
#define __BG_OV_H_

typedef struct bg_ov_s bg_ov_t;

bg_ov_t * bg_ov_create(bg_plugin_handle_t * h);

bg_plugin_handle_t * bg_ov_get_plugin(bg_ov_t *);

void bg_ov_destroy(bg_ov_t * ov);

void bg_ov_set_window(bg_ov_t * ov, const char * window_id);
  
const char * bg_ov_get_window(bg_ov_t * ov);


void bg_ov_set_window_options(bg_ov_t * ov, const char * name, 
                           const char * klass, 
                           const gavl_video_frame_t * icon,
                           const gavl_video_format_t * icon_format);

void bg_ov_set_window_title(bg_ov_t * ov, const char * title);

void bg_ov_set_callbacks(bg_ov_t * ov, bg_ov_callbacks_t * callbacks);
  
int  bg_ov_open(bg_ov_t * ov, gavl_video_format_t * format, int keep_aspect);
  
gavl_video_frame_t * bg_ov_get_frame(bg_ov_t * ov);
  
int bg_ov_add_overlay_stream(bg_ov_t * ov, gavl_video_format_t * format);
  
gavl_overlay_t * bg_ov_create_overlay(bg_ov_t * ov, int id);
 
void bg_ov_set_overlay(bg_ov_t * ov, int stream, gavl_overlay_t * ovl);

void bg_ov_put_video(bg_ov_t * ov, gavl_video_frame_t*frame);
  
void bg_ov_put_still(bg_ov_t * ov, gavl_video_frame_t*frame);

void bg_ov_handle_events(bg_ov_t * ov);

void bg_ov_update_aspect(bg_ov_t * ov, int pixel_width, int pixel_height);
  
void bg_ov_destroy_overlay(bg_ov_t * ov, int id, gavl_overlay_t * ovl);
  
void bg_ov_close(bg_ov_t * ov);

void bg_ov_show_window(bg_ov_t * ov, int show);


#endif // __BG_OV_H_
