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

typedef struct bg_gtk_scrolltext_s bg_gtk_scrolltext_t;

bg_gtk_scrolltext_t * bg_gtk_scrolltext_create(int width, int height);
GtkWidget * bg_gtk_scrolltext_get_widget(bg_gtk_scrolltext_t *);

void bg_gtk_scrolltext_set_text(bg_gtk_scrolltext_t *, const char * text,
                                float * fg_color, float * bg_color);

void bg_gtk_scrolltext_set_colors(bg_gtk_scrolltext_t *,
                                  float * fg_color, float * bg_color);


void bg_gtk_scrolltext_set_font(bg_gtk_scrolltext_t *, const char * font);

void bg_gtk_scrolltext_destroy(bg_gtk_scrolltext_t *);

