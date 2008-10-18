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


#include <gavl/gavl.h>
#include <gmerlin/cfg_registry.h>

/* Audio monitor, will have level meters and other stuff */

typedef struct bg_gtk_vumeter_s bg_gtk_vumeter_t;

bg_gtk_vumeter_t *
bg_gtk_vumeter_create(int max_num_channels, int vertical);

GtkWidget *
bg_gtk_vumeter_get_widget(bg_gtk_vumeter_t * m);

void bg_gtk_vumeter_set_format(bg_gtk_vumeter_t * m,
                               gavl_audio_format_t * format);

void bg_gtk_vumeter_update(bg_gtk_vumeter_t * m,
                           gavl_audio_frame_t * frame);

void bg_gtk_vumeter_reset_overflow(bg_gtk_vumeter_t *);

void bg_gtk_vumeter_draw(bg_gtk_vumeter_t * m);

void bg_gtk_vumeter_destroy(bg_gtk_vumeter_t * m);

