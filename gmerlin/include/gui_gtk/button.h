/*****************************************************************
 
  button.h
 
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

void bg_gtk_button_set_menu(bg_gtk_button_t *, GtkWidget * menu);

/* Get Stuff */

void bg_gtk_button_get_coords(bg_gtk_button_t *, int * x, int * y);

GtkWidget * bg_gtk_button_get_widget(bg_gtk_button_t *);

void bg_gtk_button_skin_load(bg_gtk_button_skin_t * s,
                             xmlDocPtr doc, xmlNodePtr node);

void bg_gtk_button_skin_free(bg_gtk_button_skin_t * s);
