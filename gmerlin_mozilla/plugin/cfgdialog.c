#include <gmerlin/cfg_dialog.h>
#include <gmerlin/translation.h>

#include <gmerlin_mozilla.h>

/* This creates the dialog and applies config data at once */

void gmerlin_mozilla_create_dialog(bg_mozilla_t * g)
  {
  const bg_parameter_info_t * parameters;
  g->cfg_dialog = bg_dialog_create_multi(TR("Gmerlin confiuration"));

  parameters = bg_mozilla_widget_get_parameters(g->widget);

  bg_dialog_add(g->cfg_dialog,
                TR("GUI"),
                g->gui_section,
                bg_mozilla_widget_set_parameter,
                (void*)(g->widget),
                parameters);
  
  bg_cfg_section_apply(g->gui_section, parameters,
                       bg_mozilla_widget_set_parameter,
                       (void*)(g->widget));
  
  parameters = bg_player_get_visualization_parameters(g->player);
  bg_dialog_add(g->cfg_dialog,
                TR("Visualization"),
                g->visualization_section,
                bg_player_set_visualization_parameter,
                (void*)(g->player),
                parameters);

  bg_cfg_section_apply(g->visualization_section, parameters,
                       bg_player_set_visualization_parameter,
                       (void*)(g->player));

  parameters = bg_player_get_osd_parameters(g->player);
  bg_dialog_add(g->cfg_dialog,
                TR("OSD"),
                g->osd_section,
                bg_player_set_osd_parameter,
                (void*)(g->player),
                parameters);

  bg_cfg_section_apply(g->osd_section, parameters,
                       bg_player_set_osd_parameter,
                       (void*)(g->player));
  
  /* This gets no widget */
  parameters = bg_gtk_info_window_get_parameters(g->info_window);
  bg_cfg_section_apply(g->infowindow_section, parameters,
                       bg_gtk_info_window_set_parameter, (void*)(g->info_window));

  parameters = gmerlin_mozilla_get_parameters(g);
  bg_cfg_section_apply(g->general_section, parameters,
                       gmerlin_mozilla_set_parameter,
                       (void*)(g));
  
  }
