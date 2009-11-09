/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include "recorder.h"
#include "recorder_window.h"

#include <gmerlin/utils.h>

#include <gtk/gtk.h>
#include <gmerlin/gui_gtk/gtkutils.h>

#define WINDOW_ICON NULL
#define WINDOW_NAME "gmerlin-recorder"
#define WINDOW_CLASS "gmerlin-recorder"

int main(int argc, char ** argv)
  {
  bg_cfg_registry_t    * cfg_reg;
  bg_plugin_registry_t * plugin_reg;
  char * tmp_path;
  bg_cfg_section_t     * section;
  
  bg_recorder_window_t * win;
  
  /* Create config registry */
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("recorder", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  /* Create plugin registry */
  section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(section);

  /* Initialize gtk */
  bg_gtk_init(&argc, &argv, WINDOW_ICON, WINDOW_NAME, WINDOW_CLASS);

  win = bg_recorder_window_create(cfg_reg, plugin_reg);

  bg_recorder_window_run(win);
  
  /* Cleanup */

  tmp_path =  bg_search_file_write("recorder", "config.xml");
  
  if(tmp_path)
    {
    bg_cfg_registry_save(cfg_reg, tmp_path);
    free(tmp_path);
    }
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);

  
  return 0;
  }
