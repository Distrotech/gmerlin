/*****************************************************************

  main.c

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

/* This file is part of gmerlin_vizualizer */

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkrgb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <xmms/plugin.h>
#include "fft.h"

#include "config.h"
#include "mainwindow.h"
#include "input.h"

int main(int argc, char ** argv)
  {
  xesd_main_window * main_window;
    
  gtk_init(&argc, &argv);
  gdk_rgb_init();

  if(!input_create())
    {
    fprintf(stderr, "Cannot load audio recording plugin. Call gmerlin_plugincfg to check settings\n");
    return -1;
    }
  
  main_window = xesd_create_main_window();

  gtk_widget_show(main_window->window);
  //  gtk_timeout_add(10, input_iteration, the_input);
  gtk_idle_add(input_iteration, the_input);
  gtk_main();
  return 0;
  
  }
