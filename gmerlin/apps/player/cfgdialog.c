#include "gmerlin.h"

void gmerlin_create_dialog(gmerlin_t * g)
  {
  bg_cfg_section_t * cfg_section;
  bg_parameter_info_t * parameters;
  /* Create the dialog */
    
  g->cfg_dialog = bg_dialog_create_multi();

  /* Add sections */

  cfg_section = bg_cfg_registry_find_section(g->cfg_reg, "Display");
  parameters = display_get_parameters(g->player_window->display);

  bg_cfg_section_apply(cfg_section, parameters,
                       display_set_parameter, (void*)(g->player_window->display));
  bg_dialog_add(g->cfg_dialog,
                "Display",
                cfg_section,
                display_set_parameter,
                (void*)(g->player_window->display),
                parameters);

  cfg_section = bg_cfg_registry_find_section(g->cfg_reg, "Tree");
  parameters = bg_media_tree_get_parameters(g->tree);
  bg_cfg_section_apply(cfg_section, parameters,
                       bg_media_tree_set_parameter, (void*)(g->tree));
  
  bg_dialog_add(g->cfg_dialog,
                "Media Tree",
                cfg_section,
                bg_media_tree_set_parameter,
                (void*)(g->tree),
                parameters);

  cfg_section = bg_cfg_registry_find_section(g->cfg_reg, "General");
  parameters = gmerlin_get_parameters(g);

  bg_cfg_section_apply(cfg_section, parameters,
                       gmerlin_set_parameter, (void*)(g));
  bg_dialog_add(g->cfg_dialog,
                (char*)0,
                cfg_section,
                gmerlin_set_parameter,
                (void*)(g),
                parameters);


  }


void gmerlin_configure(gmerlin_t * g)
  {
  bg_dialog_show(g->cfg_dialog);
  }
