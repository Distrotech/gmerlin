/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <string.h>


#include <config.h>
#include <gmerlin/recorder.h>
#include <gmerlin/pluginregistry.h>
#include <gavl/gavl.h>
#include <gmerlin/utils.h>
#include <gmerlin/cmdline.h>
#include <gmerlin/log.h>
#include <gmerlin/translation.h>

#include "nmjedit.h"


static bg_cmdline_arg_t global_options[] =
  {
    {},
  };
  
const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin-nmjedit",
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("Editor for NMJ2 databases\n"),
    .args = (bg_cmdline_arg_array_t[]) { { TRS("Options"), global_options },
                                       {  } },
    .files = (bg_cmdline_ext_doc_t[])
    { { "~/.gmerlin/plugins.xml",
        TRS("Cache of the plugin registry (shared by all applicatons)") },
      { "~/.gmerlin/generic/config.xml",
        TRS("Default plugin parameters are read from there. Use gmerlin_plugincfg to change them.") },
      { /* End */ }
    },
    
  };

#define DATABASE_FILE "nmj_database/media.db"

int main(int argc, char ** argv)
  {
  bg_cfg_section_t * cfg_section;
  char * tmp_path;
  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;
  sqlite3 * db;
  int result;
  char ** dirs;
  int index;
  
  /* Make database connection */
  
  result = sqlite3_open(DATABASE_FILE, &db);
  if(result)
    {
    fprintf(stderr, "Can't open database %s: %s\n",
            DATABASE_FILE, sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
    }
  
  /* Create plugin registry */
  
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  dirs = bg_cmdline_get_locations_from_args(&argc, &argv);

  index = 0;
  while(dirs[index])
    {
    char * dir = dirs[index];
    if(dir[strlen(dir)-1] == '/')
      dir[strlen(dir)-1] = '\0';
    
    bg_nmj_add_directory(db, dirs[index], 0);
    index++;
    }
    
  return 0;
  }
