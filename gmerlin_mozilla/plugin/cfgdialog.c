/*****************************************************************
 * gmerlin-mozilla - A gmerlin based mozilla plugin
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

  parameters = bg_player_get_input_parameters(g);
  bg_cfg_section_apply(g->input_section, parameters,
                       bg_player_set_input_parameter,
                       (void*)(g->player));
  
  }
