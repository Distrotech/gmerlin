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


#include <libxml/tree.h>
#include <libxml/parser.h>

typedef enum
  {
    BG_GTK_SLIDER_ACTIVE,
    BG_GTK_SLIDER_INACTIVE,
    BG_GTK_SLIDER_HIDDEN,
  } bg_gtk_slider_state_t;

typedef struct
  {
  char * name;
  
  char * pixmap_background;
  char * pixmap_background_l;
  char * pixmap_background_r;
  
  char * pixmap_normal;
  char * pixmap_highlight;
  char * pixmap_pressed;
  char * pixmap_inactive;
  int x, y;
  
  } bg_gtk_slider_skin_t;

void bg_gtk_slider_skin_load(bg_gtk_slider_skin_t * s,
                             xmlDocPtr doc, xmlNodePtr node);

void bg_gtk_slider_skin_free(bg_gtk_slider_skin_t * s);


typedef struct bg_gtk_slider_s bg_gtk_slider_t;

bg_gtk_slider_t * bg_gtk_slider_create();

void bg_gtk_slider_set_state(bg_gtk_slider_t *, bg_gtk_slider_state_t);

void bg_gtk_slider_destroy(bg_gtk_slider_t *);

/* Set attributes */

void bg_gtk_slider_set_change_callback(bg_gtk_slider_t *,
                                       void (*func)(bg_gtk_slider_t *,
                                                    float perc,
                                                    void * data),
                                       void * data);

void bg_gtk_slider_set_release_callback(bg_gtk_slider_t *,
                                        void (*func)(bg_gtk_slider_t *,
                                                     float perc,
                                                     void * data),
                                        void * data);

void
bg_gtk_slider_set_scroll_callback(bg_gtk_slider_t * s,
                                  void (*func)(bg_gtk_slider_t*, int up, void*),
                                  void* data);

void bg_gtk_slider_set_skin(bg_gtk_slider_t *,
                            bg_gtk_slider_skin_t*,
                            const char * directory);

void bg_gtk_slider_set_pos(bg_gtk_slider_t *, float position);

/* Get stuff */

void bg_gtk_slider_get_coords(bg_gtk_slider_t *, int * x, int * y);

GtkWidget * bg_gtk_slider_get_widget(bg_gtk_slider_t *);

GtkWidget * bg_gtk_slider_get_slider_widget(bg_gtk_slider_t *);
