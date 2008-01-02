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


#include <libxml/tree.h>
#include <libxml/parser.h>

typedef struct bg_gtk_button_s bg_gtk_button_t;

typedef struct bg_gtk_button_skin_s
  {
  /* This is specific for the button */

  int x, y;
  char * pixmap_normal;
  char * pixmap_highlight;
  char * pixmap_pressed;
  } bg_gtk_button_skin_t;

bg_gtk_button_t * bg_gtk_button_create();

void bg_gtk_button_destroy(bg_gtk_button_t *);

/* Set Attributes */

void bg_gtk_button_set_skin(bg_gtk_button_t *,
                            bg_gtk_button_skin_t *,
                            const char * directory);

/* A button can have either a callback or a menu */

void bg_gtk_button_set_callback(bg_gtk_button_t *,
                                void (*callback)(bg_gtk_button_t *,
                                                 void *),
                                void * callback_data);

void bg_gtk_button_set_callback_2(bg_gtk_button_t *,
                                  void (*callback_2)(bg_gtk_button_t *,
                                                     void *),
                                  void * callback_2_data);

void bg_gtk_button_set_menu(bg_gtk_button_t *, GtkWidget * menu);

/* Get Stuff */

void bg_gtk_button_get_coords(bg_gtk_button_t *, int * x, int * y);

GtkWidget * bg_gtk_button_get_widget(bg_gtk_button_t *);

void bg_gtk_button_skin_load(bg_gtk_button_skin_t * s,
                             xmlDocPtr doc, xmlNodePtr node);

void bg_gtk_button_skin_free(bg_gtk_button_skin_t * s);
