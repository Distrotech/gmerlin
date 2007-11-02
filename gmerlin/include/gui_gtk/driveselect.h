/*****************************************************************
 
  driveselect.h
 
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

typedef struct bg_gtk_drivesel_s bg_gtk_drivesel_t;

/* Create driveselector with callback */

bg_gtk_drivesel_t *
bg_gtk_drivesel_create(const char * title,
                       void (*add_drive)(char ** drives, const char * plugin,
                                         void * data),
                       void (*close_notify)(bg_gtk_drivesel_t *,
                                            void * data),
                       void * user_data,
                       GtkWidget * parent_window,
                       bg_plugin_registry_t * plugin_reg, int type_mask,
                       int flag_mask);

/* Destroy driveselector */

void bg_gtk_drivesel_destroy(bg_gtk_drivesel_t * drivesel);

/* Show the window */

/* A non modal window will destroy itself when it's closed */

void bg_gtk_drivesel_run(bg_gtk_drivesel_t * drivesel, int modal,
                         GtkWidget * w);
