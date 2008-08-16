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

typedef struct bg_image_transform_s bg_image_transform_t;

typedef int (*bg_image_transform_func)(void * priv,
                                       double xdst,
                                       double ydst,
                                       double * xsrc,
                                       double * ysrc);

bg_image_transform_t * bg_image_transform_create();
void bg_image_transform_destroy(bg_image_transform_t *);

/* Format can get changed */

void bg_image_transform_init(bg_image_transform_t *,
                             gavl_video_format_t * format,
                             bg_image_transform_func, void * priv);

const bg_parameter_info_t * bg_image_transform_get_parameters(bg_image_transform_t *);
void bg_image_transform_set_parameter(void * t, const char * name,
                                      const bg_parameter_value_t * val);


void bg_image_transform_transform(bg_image_transform_t * t,
                                  gavl_video_frame_t * in_frame,
                                  gavl_video_frame_t * out_frame);

int bg_image_transform_need_restart(bg_image_transform_t * t);

