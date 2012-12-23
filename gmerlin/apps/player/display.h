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


#include <libxml/tree.h>
#include <libxml/parser.h>


#define DISPLAY_WIDTH  232
#define DISPLAY_HEIGHT  59

typedef struct display_s display_t;

typedef struct display_skin_s
  {
  int x, y;
  float background[3];
  float foreground_normal[3];
  float foreground_error[3];
  } display_skin_t;

display_t * display_create(gmerlin_t * gmerlin);

const bg_parameter_info_t * display_get_parameters(display_t * display);

void display_set_parameter(void * data, const char * name,
                           const bg_parameter_value_t * v);

int display_get_parameter(void * data, const char * name,
                           bg_parameter_value_t * v);

void display_destroy(display_t *);

void display_set_playlist_times(display_t *,
                                gavl_time_t duration_before,
                                gavl_time_t duration_current,
                                gavl_time_t duration_after);

void display_set_time(display_t *, gavl_time_t time);

void display_set_mute(display_t *, int mute);

GtkWidget * display_get_widget(display_t *);

void display_get_coords(display_t *, int * x, int * y);


/* Set state to something defined in playermsg.h */

void display_set_state(display_t *, int state,
                       const void * arg);

/* Set track name to be displayed */

void display_set_track_name(display_t * d, const char * name);

void display_set_error_msg(display_t * d, char * msg);

void display_set_skin(display_t * d, display_skin_t * s);

void display_skin_load(display_skin_t * s,
                       xmlDocPtr doc, xmlNodePtr node);
