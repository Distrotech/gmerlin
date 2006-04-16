/*****************************************************************
 
  main.c
 
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

/* System includes */
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>

/* Gmerlin includes */

#include <player.h>

#include "gmerlin.h"

#include <utils.h>
#include <cmdline.h>
#include <gui_gtk/gtkutils.h>

int main(int argc, char ** argv)
  {
  gmerlin_t * gmerlin;
  bg_cfg_registry_t * cfg_reg;
  char * tmp_path;
  char ** locations;
    
  /* Initialize random generator (for shuffle) */

  srand(time(NULL));
    
  cfg_reg = bg_cfg_registry_create();
  
  tmp_path = bg_search_file_read("player", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  /* Fire up the actual player */

  bg_gtk_init(&argc, &argv, "player_icon.png");
  
  gmerlin = gmerlin_create(cfg_reg);

  /* Get locations from the commandline */

  locations = bg_cmdline_get_locations_from_args(&argc, &argv);

  if(locations)
    gmerlin_play_locations(gmerlin, locations);
  
  gmerlin_run(gmerlin);
  
  gmerlin_destroy(gmerlin);

  tmp_path =  bg_search_file_write("player", "config.xml");

  bg_cfg_registry_save(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  bg_cfg_registry_destroy(cfg_reg);
  return 0;
  }

