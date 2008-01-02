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


#include <translation.h>

GdkPixbuf * bg_gtk_pixbuf_scale_alpha(GdkPixbuf * src,
                                      int dest_width,
                                      int dest_height,
                                      float * foreground,
                                      float * background);

void bg_gtk_init(int * argc, char *** argv, char * default_window_icon);

void bg_gdk_pixbuf_render_pixmap_and_mask(GdkPixbuf *pixbuf,
                                          GdkPixmap **pixmap_return,
                                          GdkBitmap **mask_return);

void bg_gtk_set_widget_bg_pixmap(GtkWidget * w, GdkPixmap *pixmap);

char * bg_gtk_convert_font_name_from_pango(const char * name);
char * bg_gtk_convert_font_name_to_pango(const char * name);

GtkWidget * bg_gtk_window_new(GtkWindowType type);

void bg_gtk_tooltips_set_tip(GtkWidget * w, const char * str,
                             const char * translation_domain);

void bg_gtk_set_tooltips(int enable);
int bg_gtk_get_tooltips();

GtkWidget * bg_gtk_get_toplevel(GtkWidget * w);

