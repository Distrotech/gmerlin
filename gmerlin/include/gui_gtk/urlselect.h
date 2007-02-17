/*****************************************************************
 
  urlselect.h
 
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

typedef struct bg_gtk_urlsel_s bg_gtk_urlsel_t;

/* Create urlselector with callback */

bg_gtk_urlsel_t *
bg_gtk_urlsel_create(const char * title,
                     void (*add_url)(char ** urls, const char * plugin,
                                     void * data),
                     void (*close_notify)(bg_gtk_urlsel_t *,
                                          void * data),
                     char ** plugins,
                     void * user_data,
                     GtkWidget * parent_window,
                     bg_plugin_registry_t * plugin_reg);

/* Destroy urlselector */

void bg_gtk_urlsel_destroy(bg_gtk_urlsel_t * urlsel);

/* Show the window */

/* A non modal window will destroy itself when it's closed */

void bg_gtk_urlsel_run(bg_gtk_urlsel_t * urlsel, int modal);
