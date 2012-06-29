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


#include <gmerlin/translation.h>
#include <gavl/gavl.h>

GdkPixbuf * bg_gtk_pixbuf_scale_alpha(GdkPixbuf * src,
                                      int dest_width,
                                      int dest_height,
                                      float * foreground,
                                      float * background);

GdkPixbuf * bg_gtk_pixbuf_from_frame(gavl_video_format_t * format,
                                     gavl_video_frame_t * frame);

void bg_gtk_init(int * argc, char *** argv, 
                 const char * default_window_icon,
                 const char * default_name,
                 const char * default_class);

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

#define bg_gtk_box_pack_start_defaults(b, c) \
  gtk_box_pack_start(b, c, TRUE, TRUE, 0)

int bg_gtk_widget_is_realized(GtkWidget * w);
int bg_gtk_widget_is_toplevel(GtkWidget * w);

void bg_gtk_widget_set_can_default(GtkWidget *w, gboolean can_default);
void bg_gtk_widget_set_can_focus(GtkWidget *w, gboolean can_focus);

GtkWidget * bg_gtk_combo_box_new_text();
void bg_gtk_combo_box_append_text(GtkWidget *combo_box, const gchar *text);
void bg_gtk_combo_box_remove_text(GtkWidget * b, int index);
