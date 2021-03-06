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

typedef struct bg_osd_s bg_osd_t;

bg_osd_t * bg_osd_create();
void bg_osd_destroy(bg_osd_t*); 

const bg_parameter_info_t * bg_osd_get_parameters(bg_osd_t*);
void bg_osd_set_parameter(void * data, const char * name,
                          const bg_parameter_value_t * val);

// void bg_osd_set_overlay(bg_osd_t*, gavl_overlay_t *);

void bg_osd_init(bg_osd_t * osd, gavl_video_sink_t * sink, 
                 const gavl_video_format_t * frame_format);

void bg_osd_update(bg_osd_t * osd);

// int bg_osd_overlay_valid(bg_osd_t*);

void bg_osd_set_volume_changed(bg_osd_t*, float val);
void bg_osd_set_brightness_changed(bg_osd_t*, float val);
void bg_osd_set_contrast_changed(bg_osd_t*, float val);
void bg_osd_set_saturation_changed(bg_osd_t*, float val);

// int bg_osd_changed(bg_osd_t * osd);
