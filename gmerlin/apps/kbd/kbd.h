/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

/* Common definitions */

typedef struct
  {
  uint32_t scancode;   /* X11 specific */
  uint32_t modifiers_i;   /* X11 specific */
  char     * modifiers;
  char     * command;
  } kbd_table_t;

/* Load/Save (xml) */

kbd_table_t * kbd_table_load(const char * filename, int * len);
void kbd_table_save(const char * filename, kbd_table_t *, int len);

/* Destroy */

void kbd_table_destroy(kbd_table_t *, int len);

void kbd_table_entry_free(kbd_table_t * kbd);

/* Convert modifiers from/to string */

uint32_t kbd_modifiers_from_string(const char * str);
char *  kbd_modifiers_to_string(uint32_t state);

#define kbd_ignore_mask (Mod2Mask)
