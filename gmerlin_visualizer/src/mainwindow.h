/*****************************************************************

  mainwindow.h

  Copyright (c) 2001 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://gmerlin.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

/* This file is part of vizualizer4esd */

typedef struct
  {
  char * name;
  char * filename;
  int enabled;

  VisPlugin * plugin;
  
  } xesd_plugin_info;

typedef struct
  {
  xesd_plugin_info * current_plugin_info;

  int no_enable_callback;
  
  GList * plugins;
    
  GtkWidget * window;
  GtkWidget * plugin_list;
  
  GtkWidget * quit_button;
  GtkWidget * about_button;
  GtkWidget * configure_button;
  GtkWidget * enable_button;
  } xesd_main_window;

xesd_main_window * xesd_create_main_window();

/* Load a visualization plugin from file, */
/* Retrun 0 if something failed */

VisPlugin * load_vis_plugin(char * filename);

void load_plugin(xesd_plugin_info *);
void unload_plugin(xesd_plugin_info *);
