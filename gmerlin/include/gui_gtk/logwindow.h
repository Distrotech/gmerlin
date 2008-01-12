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


typedef struct bg_gtk_log_window_s bg_gtk_log_window_t;

bg_gtk_log_window_t *
bg_gtk_log_window_create(void (*close_callback)(bg_gtk_log_window_t*, void*),
                         void * data,
                         const char * app_name);

void bg_gtk_log_window_destroy(bg_gtk_log_window_t *);

void
bg_gtk_log_window_show(bg_gtk_log_window_t *);

void bg_gtk_log_window_hide(bg_gtk_log_window_t *);

const bg_parameter_info_t *
bg_gtk_log_window_get_parameters(bg_gtk_log_window_t *);

void bg_gtk_log_window_set_parameter(void * data, const char * name,
                                     const bg_parameter_value_t * v);

int bg_gtk_log_window_get_parameter(void * data, const char * name,
                                    bg_parameter_value_t * val);

void bg_gtk_log_window_flush(bg_gtk_log_window_t *);

const char * bg_gtk_log_window_last_error(bg_gtk_log_window_t *);
