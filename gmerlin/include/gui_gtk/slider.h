/*****************************************************************
 
  slider.h
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/


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

void bg_gtk_slider_set_skin(bg_gtk_slider_t *,
                            bg_gtk_slider_skin_t*,
                            const char * directory);

void bg_gtk_slider_set_pos(bg_gtk_slider_t *, float position);

/* Get stuff */

void bg_gtk_slider_get_coords(bg_gtk_slider_t *, int * x, int * y);

GtkWidget * bg_gtk_slider_get_widget(bg_gtk_slider_t *);
