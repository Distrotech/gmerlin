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

#include <gtk/gtk.h> 
#include <stdlib.h> 

#include <gmerlin/cfg_registry.h>
#include <gmerlin/preset.h>
#include <gmerlin/utils.h>

#include <gui_gtk/presetmenu.h>

struct bg_gtk_preset_menu_s
  {
  GtkWidget * menu;

  GtkWidget ** preset_items;
  int num_preset_items;
  int preset_items_alloc;

  bg_preset_t * presets;
  char * path;
  };

bg_gtk_preset_menu_t * bg_gtk_preset_menu_create(const char * preset_path,
                                                 bg_cfg_section_t * s,
                                                 void (*cb)(void *), void * cb_data)
  {
  bg_gtk_preset_menu_t * ret = calloc(1, sizeof(*ret));

  ret->path = bg_strdup(ret->path, preset_path);
  ret->presets = bg_presets_load(ret->path);

  return ret;  
  }

void bg_gtk_preset_menu_destroy(bg_gtk_preset_menu_t * m)
  {
  free(m);
  }

GtkWidget * bg_gtk_preset_menu_get_widget(bg_gtk_preset_menu_t * m)
  {
  return m->menu;
  }
