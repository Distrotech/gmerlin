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

typedef struct bg_gtk_textview_s bg_gtk_textview_t;

bg_gtk_textview_t * bg_gtk_textview_create(void);
void bg_gtk_textview_destroy(bg_gtk_textview_t *);

void bg_gtk_textview_update(bg_gtk_textview_t *, const char * text);

GtkWidget * bg_gtk_textview_get_widget(bg_gtk_textview_t * t);

typedef struct bg_gtk_textwindow_s bg_gtk_textwindow_t;

bg_gtk_textwindow_t *
bg_gtk_textwindow_create(const char * text, const char * title);
void bg_gtk_textwindow_show(bg_gtk_textwindow_t * w, int modal, GtkWidget * parent);
