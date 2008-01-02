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

#ifndef __ACCELERATOR_H_
#define __ACCELERATOR_H_

typedef struct
  {
  int key;   /* See keycodes.h */
  int mask;  /* See keycodes.h */
  int id;    /* Choosen by the application */
  } bg_accelerator_t;

typedef struct bg_accelerator_map_s bg_accelerator_map_t;

void
bg_accelerator_map_append(bg_accelerator_map_t * m,
                          int key, int mask, int id);

/* Array is terminated with BG_KEY_NONE */

void
bg_accelerator_map_append_array(bg_accelerator_map_t * m,
                                const bg_accelerator_t * tail);

void
bg_accelerator_map_destroy(bg_accelerator_map_t * m);

bg_accelerator_map_t *
bg_accelerator_map_create();

void
bg_accelerator_map_remove(bg_accelerator_map_t * m, int id);

void
bg_accelerator_map_clear(bg_accelerator_map_t * m);


int
bg_accelerator_map_has_accel(const bg_accelerator_map_t * m,
                             int key, int mask, int * id);

int
bg_accelerator_map_has_accel_with_id(const bg_accelerator_map_t * m,
                                     int id);


const bg_accelerator_t * 
bg_accelerator_map_get_accels(const bg_accelerator_map_t * m);

#endif // __ACCELERATOR_H_
