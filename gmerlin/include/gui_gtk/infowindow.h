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


#ifndef __BG_GTK_INFOWINDOW_H_
#define __BG_GTK_INFOWINDOW_H_

typedef struct bg_gtk_info_window_s bg_gtk_info_window_t;

bg_gtk_info_window_t *
bg_gtk_info_window_create(bg_player_t * player,
                          void (*close_callback)(bg_gtk_info_window_t*, void*),
                          void * data);

void bg_gtk_info_window_destroy(bg_gtk_info_window_t *);

/* Configuration (doesn't need a dialog) */

const bg_parameter_info_t *
bg_gtk_info_window_get_parameters(bg_gtk_info_window_t * win);
  
void bg_gtk_info_window_set_parameter(void * data, const char * name,
                                      const bg_parameter_value_t * val);

int bg_gtk_info_window_get_parameter(void * data, const char * name,
                                     bg_parameter_value_t * val);


/* Show/hide the window */

void bg_gtk_info_window_show(bg_gtk_info_window_t *);
void bg_gtk_info_window_hide(bg_gtk_info_window_t *);

#endif // __BG_GTK_INFOWINDOW_H_
