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

  /* This gets no widget */
  parameters = bg_gtk_info_window_get_parameters(g->info_window);
  bg_cfg_section_apply(g->infowindow_section, parameters,
                       bg_gtk_info_window_set_parameter, (void*)(g->info_window));
  
  }
