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

#include <vis_plugin.h>

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
  vis_plugin_info_t * current_plugin_info;
  vis_plugin_handle_t * current_plugin_handle;
  
  int no_enable_callback;
  
  vis_plugin_info_t * plugins;
    
  GtkWidget * window;
  GtkWidget * plugin_list;
  
  GtkWidget * quit_button;
  GtkWidget * about_button;
  GtkWidget * configure_button;
  GtkWidget * enable_button;
  } main_window_t;

main_window_t * main_window_create();
void main_window_destroy(main_window_t*);
