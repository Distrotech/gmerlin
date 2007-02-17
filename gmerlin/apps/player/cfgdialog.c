/*****************************************************************
 
  cfgdialog.c
 
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

#include <config.h>
#include <translation.h>

#include "gmerlin.h"

void gmerlin_create_dialog(gmerlin_t * g)
  {
  void * parent;
  bg_parameter_info_t * parameters;
  /* Create the dialog */
    
  g->cfg_dialog = bg_dialog_create_multi(TR("Gmerlin confiuration"));
  /* Add sections */

  parameters = gmerlin_get_parameters(g);

  bg_dialog_add(g->cfg_dialog,
                TR("General"),
                g->general_section,
                gmerlin_set_parameter,
                (void*)(g),
                parameters);
  
  parameters = bg_player_get_input_parameters(g->player);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Input"),
                g->input_section,
                bg_player_set_input_parameter,
                (void*)(g->player),
                parameters);
  
  parameters = bg_player_get_audio_parameters(g->player);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Audio"),
                g->audio_section,
                bg_player_set_audio_parameter,
                (void*)(g->player),
                parameters);

  parameters = bg_player_get_video_parameters(g->player);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Video"),
                g->video_section,
                bg_player_set_video_parameter,
                (void*)(g->player),
                parameters);

  parameters = bg_player_get_subtitle_parameters(g->player);
  parent = bg_dialog_add_parent(g->cfg_dialog, (void*)0, TR("Text subtitles"));
  
  bg_dialog_add_child(g->cfg_dialog, parent,
                      TR("Subtitles"),
                      g->subtitle_section,
                      bg_player_set_subtitle_parameter,
                      (void*)(g->player),
                      parameters);

  parameters = bg_player_get_osd_parameters(g->player);
  
  bg_dialog_add(g->cfg_dialog,
                TR("OSD"),
                g->osd_section,
                bg_player_set_osd_parameter,
                (void*)(g->player),
                parameters);

  
  parameters = display_get_parameters(g->player_window->display);

  bg_dialog_add(g->cfg_dialog,
                TR("Display"),
                g->display_section,
                display_set_parameter,
                (void*)(g->player_window->display),
                parameters);

  parameters = bg_media_tree_get_parameters(g->tree);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Media Tree"),
                g->tree_section,
                bg_media_tree_set_parameter,
                (void*)(g->tree),
                parameters);

  parameters = bg_remote_server_get_parameters(g->remote);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Remote control"),
                g->remote_section,
                bg_remote_server_set_parameter,
                (void*)(g->remote),
                parameters);
 
  parameters = bg_lcdproc_get_parameters(g->lcdproc);
  
  bg_dialog_add(g->cfg_dialog,
                TR("LCDproc"),
                g->lcdproc_section,
                bg_lcdproc_set_parameter,
                (void*)(g->lcdproc),
                parameters);
  
  parameters = bg_gtk_log_window_get_parameters(g->log_window);
  
  bg_dialog_add(g->cfg_dialog,
                TR("Log window"),
                g->logwindow_section,
                bg_gtk_log_window_set_parameter,
                (void*)(g->log_window),
                parameters);
  }


void gmerlin_configure(gmerlin_t * g)
  {
  bg_dialog_show(g->cfg_dialog);
  }
