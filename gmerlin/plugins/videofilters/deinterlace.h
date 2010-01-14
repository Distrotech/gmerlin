/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#if 0
#include <gmerlin/plugin.h>

typedef struct bg_deinterlacer_s bg_deinterlacer_t;

bg_deinterlacer_t * bg_deinterlacer_create();

void bg_deinterlacer_destroy(bg_deinterlacer_t * d);

void bg_deinterlacer_init(bg_deinterlacer_t * di,
                          gavl_video_format_t * format,
                          gavl_video_options_t * opt);

void bg_deinterlacer_get_output_format(bg_deinterlacer_t * di,
                                       gavl_video_format_t * format);

void bg_deinterlacer_connect_input(void * priv,
                                   bg_read_video_func_t func,
                                   void * data, int stream);

int bg_deinterlacer_read(void * priv, gavl_video_frame_t * frame, int stream);


void bg_deinterlacer_reset(bg_deinterlacer_t * di);

void bg_deinterlacer_set_mode(bg_deinterlacer_t * di, int mode);

#endif

