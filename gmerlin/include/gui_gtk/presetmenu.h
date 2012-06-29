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

typedef struct bg_gtk_preset_menu_s bg_gtk_preset_menu_t;

bg_gtk_preset_menu_t * bg_gtk_preset_menu_create(const char * preset_path,
                                                 bg_cfg_section_t * s,
                                                 void (*load_cb)(void *),
                                                 void (*save_cb)(void *), 
                                                 void * cb_data);

void bg_gtk_preset_menu_destroy(bg_gtk_preset_menu_t *);

GtkWidget * bg_gtk_preset_menu_get_widget(bg_gtk_preset_menu_t *);

