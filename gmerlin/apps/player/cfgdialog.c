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

#include <config.h>
#include <gmerlin/translation.h>

#include "gmerlin.h"

void gmerlin_create_dialog(gmerlin_t * g)
  {
  const bg_parameter_info_t * parameters;
  /* Create the dialogs */

  /* Audio options */
  parameters = bg_player_get_audio_parameters(g->player);

  g->audio_dialog = bg_dialog_create(g->audio_section,
                                     bg_player_set_audio_parameter,
                                     NULL,
                                     (void*)(g->player),
                                     parameters, TR("Audio options"));

  /* Audio filters */
  parameters = bg_player_get_audio_filter_parameters(g->player);
  g->audio_filter_dialog = bg_dialog_create(g->audio_filter_section,
                                            bg_player_set_audio_filter_parameter,
                                            NULL,
                                            (void*)(g->player),
                                            parameters, TR("Audio filters"));

  /* Video options */
  parameters = bg_player_get_video_parameters(g->player);

  g->video_dialog = bg_dialog_create(g->video_section,
                                     bg_player_set_video_parameter,
                                     NULL,
                                     (void*)(g->player),
                                     parameters, TR("Video options"));

  /* Video filters */
  parameters = bg_player_get_video_filter_parameters(g->player);
  g->video_filter_dialog = bg_dialog_create(g->video_filter_section,
                                            bg_player_set_video_filter_parameter,
                                            NULL,
                                            (void*)(g->player),
                                            parameters, TR("Video filters"));

  /* Subtitles */

  parameters = bg_player_get_subtitle_parameters(g->player);
  
  g->subtitle_dialog = bg_dialog_create(g->subtitle_section,
                                        bg_player_set_subtitle_parameter,
                                        NULL,
                                        (void*)(g->player),
                                        parameters, TR("Subtitle options"));

  parameters = bg_player_get_visualization_parameters(g->player);
  
  g->visualization_dialog = bg_dialog_create(g->visualization_section,
                                             bg_player_set_visualization_parameter,
                                             NULL,
                                             (void*)(g->player),
                                             parameters,
                                             TR("Visualization"));
  
#if 0
  parent = bg_dialog_add_parent(g->subtitle_dialog, (void*)0, TR("Text subtitles"));
  
  bg_dialog_add_child(g->cfg_dialog, parent,
                      TR("Subtitles"),
                      g->subtitle_section,
                      bg_player_set_subtitle_parameter,
                      (void*)(g->player),
                      parameters);
#endif

  
  g->cfg_dialog = bg_dialog_create_multi(TR("Gmerlin confiuration"));
    
  /* Add sections */

  parameters = gmerlin_get_parameters(g);

  bg_dialog_add(g->cfg_dialog,
                TR("General"),
                g->general_section,
                gmerlin_set_parameter,
                NULL,
                (void*)(g),
                parameters);
  
  parameters = bg_player_get_input_parameters(g->player);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Input"),
                g->input_section,
                bg_player_set_input_parameter,
                NULL,
                (void*)(g->player),
                parameters);

  bg_dialog_add(g->cfg_dialog,
                TR("Input plugins"),
                NULL,
                bg_plugin_registry_set_parameter_input,
                bg_plugin_registry_get_parameter_input,
                g->plugin_reg,
                g->input_plugin_parameters);

  bg_dialog_add(g->cfg_dialog,
                TR("Image readers"),
                NULL,
                bg_plugin_registry_set_parameter_input,
                bg_plugin_registry_get_parameter_input,
                g->plugin_reg,
                g->image_reader_parameters);
  
  parameters = bg_player_get_osd_parameters(g->player);
  
  bg_dialog_add(g->cfg_dialog,
                TR("OSD"),
                g->osd_section,
                bg_player_set_osd_parameter,
                NULL,
                (void*)(g->player),
                parameters);

  
  parameters = display_get_parameters(g->player_window->display);

  bg_dialog_add(g->cfg_dialog,
                TR("Display"),
                g->display_section,
                display_set_parameter,
                NULL,
                (void*)(g->player_window->display),
                parameters);

  parameters = bg_media_tree_get_parameters(g->tree);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Media Tree"),
                g->tree_section,
                bg_media_tree_set_parameter,
                NULL,
                (void*)(g->tree),
                parameters);

  parameters = bg_remote_server_get_parameters(g->remote);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Remote control"),
                g->remote_section,
                bg_remote_server_set_parameter,
                NULL,
                (void*)(g->remote),
                parameters);
  
  parameters = bg_gtk_log_window_get_parameters(g->log_window);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Log window"),
                g->logwindow_section,
                bg_gtk_log_window_set_parameter,
                NULL,
                (void*)(g->log_window),
                parameters);
  
  parameters = bg_lcdproc_get_parameters(g->lcdproc);
  
  bg_dialog_add(g->cfg_dialog,
                TR("LCDproc"),
                g->lcdproc_section,
                bg_lcdproc_set_parameter,
                NULL,
                (void*)(g->lcdproc),
                parameters);

  
  }


void gmerlin_configure(gmerlin_t * g)
  {
  bg_dialog_show(g->cfg_dialog, g->player_window->window);
  }
