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

#include <plugin.h>

#define YUVD_MODE_ANTIALIAS 0
#define YUVD_MODE_DEINT_1   1
#define YUVD_MODE_DEINT_2   2

typedef struct yuvdeinterlacer_s yuvdeinterlacer_t;

yuvdeinterlacer_t * yuvdeinterlacer_create();

void yuvdeinterlacer_destroy(yuvdeinterlacer_t * d);

void yuvdeinterlacer_init(yuvdeinterlacer_t * di,
                          gavl_video_format_t * format);

void yuvdeinterlacer_get_output_format(yuvdeinterlacer_t * di,
                                       gavl_video_format_t * format);

void yuvdeinterlacer_connect_input(void * priv,
                                   bg_read_video_func_t func,
                                   void * data, int stream);

int yuvdeinterlacer_read(void * priv, gavl_video_frame_t * frame, int stream);


void yuvdeinterlacer_reset(yuvdeinterlacer_t * di);

void yuvdeinterlacer_set_mode(yuvdeinterlacer_t * di, int mode);

