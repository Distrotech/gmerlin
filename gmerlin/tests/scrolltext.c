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
#include <gui_gtk/scrolltext.h>
#include <gui_gtk/gtkutils.h>

float fg_color[] = { 1.0, 0.5, 0.0 };
float bg_color[] = { 0.0, 0.0, 0.0 };


int main(int argc, char ** argv)
  {
  GtkWidget * win;
  bg_gtk_scrolltext_t * scrolltext;
  bg_gtk_init(&argc, &argv, NULL, NULL, NULL);
  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  scrolltext = bg_gtk_scrolltext_create(226, 18);
  bg_gtk_scrolltext_set_font(scrolltext, "Sans Bold 10");
  bg_gtk_scrolltext_set_text(scrolltext,
                             "Beneide nicht das Glueck derer, die in einem Narrenparadies leben, denn nur ein Narr kann das fuer Glueck halten", fg_color, bg_color);

  
  gtk_container_add(GTK_CONTAINER(win),
                    bg_gtk_scrolltext_get_widget(scrolltext));
    
  gtk_widget_show(win);
  gtk_main();
  return 0;
  }
