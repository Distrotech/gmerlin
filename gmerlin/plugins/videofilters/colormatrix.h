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

typedef struct bg_colormatrix_s bg_colormatrix_t;

bg_colormatrix_t * bg_colormatrix_create();

void bg_colormatrix_destroy(bg_colormatrix_t *);

void bg_colormatrix_set_rgba(bg_colormatrix_t *, float coeffs[4][5]);
void bg_colormatrix_set_yuva(bg_colormatrix_t *, float coeffs[4][5]);

void bg_colormatrix_set_rgb(bg_colormatrix_t *, float coeffs[3][4]);
void bg_colormatrix_set_yuv(bg_colormatrix_t *, float coeffs[3][4]);

/* Constants for flags */
#define BG_COLORMATRIX_FORCE_ALPHA (1<<0)

void bg_colormatrix_init(bg_colormatrix_t *,
                         gavl_video_format_t * format, int flags,
                         gavl_video_options_t * opt);

void bg_colormatrix_process(bg_colormatrix_t *,
                            gavl_video_frame_t * frame);

