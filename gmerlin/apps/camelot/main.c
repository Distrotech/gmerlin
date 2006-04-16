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

#include <gtk/gtk.h>
#include <utils.h>
#include <stdlib.h>
#include <plugin.h>
#include <pluginregistry.h>
#include <gui_gtk/gtkutils.h>


#include "webcam.h"
#include "webcam_window.h"

int main(int argc, char ** argv)
  {
  gmerlin_webcam_t * w;
  gmerlin_webcam_window_t * ww;
  bg_cfg_registry_t    * cfg_reg;
  bg_plugin_registry_t * plugin_reg;
  char * tmp_path;
  bg_cfg_section_t     * section;
  
  /* Create config registry */
  
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("camelot", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  /* Create plugin registry */
  
  section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(section);
  
  bg_gtk_init(&argc, &argv, "camelot_icon.png");
  
  w = gmerlin_webcam_create();

  section = bg_cfg_registry_find_section(cfg_reg, "general");
  
  ww = gmerlin_webcam_window_create(w, plugin_reg, section);

  /* Get config values */
  
  gmerlin_webcam_run(w);
  
  gmerlin_webcam_window_show(ww);
  gtk_main();
  
  gmerlin_webcam_quit(w);

  gmerlin_webcam_window_destroy(ww);
  gmerlin_webcam_destroy(w);

  tmp_path =  bg_search_file_write("camelot", "config.xml");
  
  if(tmp_path)
    {
    bg_cfg_registry_save(cfg_reg, tmp_path);
    free(tmp_path);
    }
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  

  return 0;
  }

