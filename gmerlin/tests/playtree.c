#include <stdlib.h>
#include <gtk/gtk.h>
#include <tree.h>
#include <utils.h>
#include <gui_gtk/tree.h>
#include <gui_gtk/gtkutils.h>


int main(int argc, char ** argv)
  {
  char * tmp_path;
  bg_gtk_tree_window_t * win;
  bg_media_tree_t      * tree;
  bg_cfg_registry_t    * cfg_reg;
  bg_cfg_section_t     * cfg_section;
  bg_plugin_registry_t * plugin_reg;
  
  bg_gtk_init(&argc, &argv);
  
  cfg_reg = bg_cfg_registry_create();
    
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);
    
  tree = bg_media_tree_create("./tree.xml", plugin_reg);
  
  win = bg_gtk_tree_window_create(tree, NULL, NULL);
  bg_gtk_tree_window_show(win);
  gtk_main();
  bg_gtk_tree_window_hide(win);
  bg_gtk_tree_window_destroy(win);

  bg_media_tree_destroy(tree);
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);

  return 0;
  }
