/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <gmerlin/tree.h>
#include <gmerlin/utils.h>
#include <gui_gtk/tree.h>
#include <gui_gtk/gtkutils.h>

static void close_callback(bg_gtk_tree_window_t * w,void * data)
  {
  gtk_main_quit();
  }

int main(int argc, char ** argv)
  {
  char * tmp_path;
  bg_gtk_tree_window_t * win;
  bg_media_tree_t      * tree;
  bg_cfg_registry_t    * cfg_reg;
  bg_cfg_section_t     * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  
  bg_gtk_init(&argc, &argv, NULL, NULL, NULL);  

  cfg_reg = bg_cfg_registry_create();
    
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);
  
  tree = bg_media_tree_create("./tree.xml", plugin_reg);
  
  win = bg_gtk_tree_window_create(tree, close_callback, NULL, NULL);
  bg_gtk_tree_window_show(win);
  gtk_main();
  bg_gtk_tree_window_hide(win);
  bg_gtk_tree_window_destroy(win);

  bg_media_tree_destroy(tree);
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);

  return 0;
  }
