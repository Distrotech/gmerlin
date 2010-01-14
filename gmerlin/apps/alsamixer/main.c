/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <config.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#include "gui.h"
#include <gui_gtk/gtkutils.h>
#include <gui_gtk/message.h>

static void nocards()
  {
  bg_gtk_message(TR("Could not find any Alsa mixers.\nCheck your setup!\n"), BG_GTK_MESSAGE_ERROR, NULL);
  exit(-1);
  }

int main(int argc, char ** argv)
  {
  char * tmp_path;
  alsa_mixer_t * mixer;
  mixer_window_t * mixer_window;
  bg_cfg_registry_t * cfg_reg;

  cfg_reg = bg_cfg_registry_create();
  
  tmp_path = bg_search_file_read("alsamixer", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
    
  //  alsa_mixer_dump(mixer);

  bg_gtk_init(&argc, &argv, "mixer_icon.png", NULL, NULL);

  mixer = alsa_mixer_create();

  if(!mixer)
    {
    nocards();
    }
  
  mixer_window = mixer_window_create(mixer, cfg_reg);
  
  mixer_window_run(mixer_window);

  mixer_window_destroy(mixer_window);
  alsa_mixer_destroy(mixer);
  
  /* Cleanup */

  tmp_path = bg_search_file_write("alsamixer", "config.xml");
  bg_cfg_registry_save(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);

  bg_cfg_registry_destroy(cfg_reg);
  
  return 0;
  }
