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

typedef struct bg_gtk_file_entry_s bg_gtk_file_entry_t;

bg_gtk_file_entry_t *
bg_gtk_file_entry_create(int dir,
                         void (*name_changed_callback)(bg_gtk_file_entry_t *,
                                                       void * data),
                         void * name_changed_callback_data,
                         const char * help_string,
                         const char * translation_domain);

void bg_gtk_file_entry_destroy(bg_gtk_file_entry_t *);

const char * bg_gtk_file_entry_get_filename(bg_gtk_file_entry_t *);
void bg_gtk_file_entry_set_filename(bg_gtk_file_entry_t *, const char *);

GtkWidget * bg_gtk_file_entry_get_entry(bg_gtk_file_entry_t *);
GtkWidget * bg_gtk_file_entry_get_button(bg_gtk_file_entry_t *);

