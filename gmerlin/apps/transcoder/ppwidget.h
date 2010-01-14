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


/* Encoder window */

typedef struct encoder_pp_window_s encoder_pp_window_t;

encoder_pp_window_t * encoder_pp_window_create(bg_plugin_registry_t * plugin_reg);
void encoder_pp_window_run(encoder_pp_window_t * win, bg_transcoder_track_global_t * global);
void encoder_pp_window_destroy(encoder_pp_window_t * win);

const bg_plugin_info_t * encoder_pp_window_get_plugin(encoder_pp_window_t * win);

