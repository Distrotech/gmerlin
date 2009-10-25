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

/* System includes */
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>

/* Gmerlin includes */

#include <config.h>

#include <gmerlin/player.h>

#include "gmerlin.h"
#include "player_remote.h"

#include <gmerlin/utils.h>
#include <gmerlin/cmdline.h>
#include <gui_gtk/gtkutils.h>

// #define MTRACE

#ifdef MTRACE
#include <mcheck.h>
#endif

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin",
    .synopsis = TRS("[options] [gmls ...]\n"),
    .help_before = TRS("Gmerlin GUI Player"),
    .args = (bg_cmdline_arg_array_t[]) { {  } },

    .env = (bg_cmdline_ext_doc_t[])
    { { PLAYER_REMOTE_ENV,
        TRS("Default port for the remote control") },
      { /* End */ }
    },
    .files = (bg_cmdline_ext_doc_t[])
    { { "~/.gmerlin/plugins.xml",
        TRS("Cache of the plugin registry (shared by all applicatons)") },
      { "~/.gmerlin/player/config.xml",
        TRS("Used for configuration data. Delete this file if you think you goofed something up.") },
      { "~/.gmerlin/player/tree/tree.xml",
        TRS("Media tree is saved here. The albums are saved as separate files in the same directory.") },
      { /* End */ }
    },
  };

int main(int argc, char ** argv)
  {
  gmerlin_t * gmerlin;
  bg_cfg_registry_t * cfg_reg;
  char * tmp_path;
  char ** locations;

#ifdef MTRACE
  mtrace();
#endif
 
  /* Initialize random generator (for shuffle) */

  srand(time(NULL));
  
  bg_translation_init();
  bg_gtk_init(&argc, &argv, "player_icon.png", WINDOW_NAME, WINDOW_CLASS);
  
  cfg_reg = bg_cfg_registry_create();
  
  tmp_path = bg_search_file_read("player", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  /* Fire up the actual player */
  
  gmerlin = gmerlin_create(cfg_reg);

  /* Get locations from the commandline */

  bg_cmdline_init(&app_data);
  
  bg_cmdline_parse(NULL, &argc, &argv, NULL);
  
  locations = bg_cmdline_get_locations_from_args(&argc, &argv);
  
  gmerlin_run(gmerlin, locations);
  
  gmerlin_destroy(gmerlin);

  tmp_path =  bg_search_file_write("player", "config.xml");

  bg_cfg_registry_save(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  bg_cfg_registry_destroy(cfg_reg);
  return 0;
  }

