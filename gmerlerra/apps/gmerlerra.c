#include <gtk/gtk.h>
#include <gui_gtk/projectwindow.h>
#include <gui_gtk/utils.h>
#include <gmerlin/pluginregistry.h>
#include <gmerlin/utils.h>

#include <gmerlin/gui_gtk/gtkutils.h>

static bg_cfg_registry_t    * cfg_reg;
static bg_plugin_registry_t * plugin_reg;

static void create_registries()
  {
  char * tmp_path;
  bg_cfg_section_t * cfg_section;
  /* Create registries */
  cfg_reg = bg_cfg_registry_create();

  tmp_path = bg_search_file_read("gmerlerra", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  
  if(tmp_path)
    free(tmp_path);

  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  
  plugin_reg = bg_plugin_registry_create(cfg_section);

  bg_nle_project_window_init_global(cfg_reg, plugin_reg);
  }

static void destroy_registries()
  {
  char * tmp_path;
  tmp_path = bg_search_file_write("gmerlerra", "config.xml");

  if(tmp_path)
    {
    bg_cfg_registry_save(cfg_reg, tmp_path);
    free(tmp_path);
    }
  
  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  }

int main(int argc, char ** argv)
  {
  bg_nle_project_window_t * w;

  bg_gtk_init(&argc, &argv,
              (const char *)0, // default_window_icon,
              (const char *)0, //  default_name,
              (const char *)0); //  default_class);
  
  bg_nle_init_cursors();
  
  create_registries();
  
  w = bg_nle_project_window_create((const char *)0 /* project */, plugin_reg);
  bg_nle_project_window_show(w);
  gtk_main();

  destroy_registries();
  return 0;
  }
