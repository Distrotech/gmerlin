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

typedef struct bg_vloopback_s bg_vloopback_t;

bg_vloopback_t * bg_vloopback_create();
void bg_vloopback_destroy(bg_vloopback_t *);

int bg_vloopback_open(bg_vloopback_t *);
void bg_vloopback_close(bg_vloopback_t *);

void bg_vloopback_set_format(bg_vloopback_t *, const gavl_video_format_t * format);
void bg_vloopback_put_frame(bg_vloopback_t *, gavl_video_frame_t * frame);

const bg_parameter_info_t * bg_vloopback_get_parameters(bg_vloopback_t *);
void bg_vloopback_set_parameter(void * data, const char * name,
                                const bg_parameter_value_t * val);

int bg_vloopback_changed(bg_vloopback_t *);
