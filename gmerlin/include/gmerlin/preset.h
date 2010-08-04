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

#define BG_PRESET_PRIVATE (1<<0) // Preset is private (can be deleted)

typedef struct bg_preset_s
  {
  char * file;
  char * name;
  struct bg_preset_s * next;
  int flags;
  } bg_preset_t;

bg_preset_t * bg_presets_load(const char * preset_path);

bg_preset_t * bg_preset_add(bg_preset_t * presets,
                            const char * preset_path,
                            const char * name,
                            const bg_cfg_section_t * s);

bg_preset_t * bg_preset_find_by_name(bg_preset_t * presets,
                                     const char * name);

bg_preset_t * bg_preset_find_by_file(bg_preset_t * presets,
                                     const char * file);

bg_preset_t * bg_preset_delete(bg_preset_t * presets,
                               bg_preset_t * preset);


void bg_presets_destroy(bg_preset_t *);

bg_cfg_section_t * bg_preset_load(bg_preset_t * p);

void bg_preset_save(bg_preset_t * p, const bg_cfg_section_t * s);
