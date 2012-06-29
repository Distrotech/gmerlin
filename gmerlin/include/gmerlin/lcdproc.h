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

typedef struct bg_lcdproc_s bg_lcdproc_t;

/* Create / destroy */

bg_lcdproc_t * bg_lcdproc_create(bg_player_t * player);
void bg_lcdproc_destroy(bg_lcdproc_t*);

/*
 *  Config stuff. The function set_parameter automatically
 *  starts and stops the thread
 */

const bg_parameter_info_t * bg_lcdproc_get_parameters(bg_lcdproc_t *);
void bg_lcdproc_set_parameter(void * data, const char * name,
                              const bg_parameter_value_t * val);
