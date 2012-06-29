/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include <shout/shout.h>

typedef struct bg_shout_s bg_shout_t;

bg_shout_t * bg_shout_create(int format);

const bg_parameter_info_t * bg_shout_get_parameters(void);

void bg_shout_set_parameter(void * data, const char * name,
                            const bg_parameter_value_t * val);

void bg_shout_set_metadata(bg_shout_t * s, const gavl_metadata_t * m);

int bg_shout_open(bg_shout_t *);

void bg_shout_update_metadata(bg_shout_t *, const char * name,
                              const gavl_metadata_t * m);

void bg_shout_destroy(bg_shout_t *);

int bg_shout_write(bg_shout_t *, const uint8_t * data, int len);

/* Also closes */
void bg_shout_destroy(bg_shout_t *);

