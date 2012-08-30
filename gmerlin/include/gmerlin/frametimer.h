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

typedef struct bg_frame_timer_s bg_frame_timer_t;

bg_frame_timer_t * bg_frame_timer_create(float framerate,
                                         uint32_t * timescale);

void bg_frame_timer_destroy(bg_frame_timer_t *);

void bg_frame_timer_update(bg_frame_timer_t *,
                           gavl_video_frame_t * frame);

/* Wait for the next frame to capture */
void bg_frame_timer_wait(bg_frame_timer_t *);


