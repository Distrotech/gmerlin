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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "kbd.h"

void kbd_table_entry_free(kbd_table_t * kbd)
  {
  if(kbd->command) free(kbd->command);
  if(kbd->modifiers) free(kbd->modifiers);
  memset(kbd, 0, sizeof(*kbd));
  }

void kbd_table_destroy(kbd_table_t * kbd, int len)
  {
  int i;
  for(i = 0; i < len; i++)
    {
    kbd_table_entry_free(kbd + i);
    }
  if(kbd) free(kbd);
  }
