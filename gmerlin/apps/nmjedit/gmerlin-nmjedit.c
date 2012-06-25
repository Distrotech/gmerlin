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

char ** add_dirs = NULL;
int num_add_dirs = 0;

char ** del_dirs = NULL;
int num_del_dirs = 0;

char ** add_albums = NULL;
int num_add_albums = 0;

int list_dirs = 0;
int do_create = 0;

static void opt_add(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -add requires an argument\n");
    exit(-1);
    }
  
  add_dirs = realloc(add_dirs, (num_add_dirs+1)*sizeof(*add_dirs));
  add_dirs[num_add_dirs] = bg_strdup(NULL, (*_argv)[arg]);
  num_add_dirs++;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_add_album(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -add-album requires an argument\n");
    exit(-1);
    }
  
  add_albums = realloc(add_albums, (num_add_albums+1)*sizeof(*add_albums));
  add_albums[num_add_albums] = bg_strdup(NULL, (*_argv)[arg]);
  num_add_albums++;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }


static void opt_del(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -del requires an argument\n");
    exit(-1);
    }
  
  del_dirs = realloc(del_dirs, (num_del_dirs+1)*sizeof(*del_dirs));
  del_dirs[num_del_dirs] = bg_strdup(NULL, (*_argv)[arg]);
  num_del_dirs++;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void opt_list_dirs(void * data, int * argc, char *** _argv, int arg)
  {
  list_dirs = 1;
  }

static void opt_create(void * data, int * argc, char *** _argv, int arg)
  {
  do_create = 1;
  }

static void opt_v(void * data, int * argc, char *** _argv, int arg)
  {
  int val, verbose = 0;

  if(arg >= *argc)
    {
    fprintf(stderr, "Option -v requires an argument\n");
    exit(-1);
    }
  val = atoi((*_argv)[arg]);  
  
  if(val > 0)
    verbose |= BG_LOG_ERROR;
  if(val > 1)
    verbose |= BG_LOG_WARNING;
  if(val > 2)
    verbose |= BG_LOG_INFO;
  if(val > 3)
    verbose |= BG_LOG_DEBUG;
  bg_log_set_verbose(verbose);
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-add",
      .help_arg =    "<directory>",
      .help_string = "Add directory",
      .callback =    opt_add,
    },
    {
      .arg =         "-del",
      .help_arg =    "<directory>",
      .help_string = "Delete directory",
      .callback =    opt_del,
    },
    {
      .arg =         "-add-album",
      .help_arg =    "<album>",
      .help_string = "Add album as a playlist",
      .callback =    opt_add_album,
    },
    {
      .arg =         "-v",
      .help_arg =    "level",
      .help_string = "Set verbosity level (0..4)",
      .callback =    opt_v,
    },
    {
      .arg =         "-list-dirs",
      .help_string = "List directories",
      .callback =    opt_list_dirs,
    },
    {
      .arg =         "-create",
      .help_string = "Create new and empty database",
      .callback =    opt_create,
    },
    { /* End */ },
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin-nmjedit",
    .long_name = "Editor for nmj databases",
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

#define TYPES BG_NMJ_MEDIA_TYPE_AUDIO

int main(int argc, char ** argv)
  {
  bg_cfg_section_t * cfg_section;
  char * tmp_path;
  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;
  sqlite3 * db;
  int result;
  char * dir;
  int i;

  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  if(do_create)
    {
    bg_nmj_create_new();
    return 0;
    }
  
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

  if(list_dirs)
    {
    bg_nmj_list_dirs(db);
    }
  
  /* Add directories */
  for(i = 0; i < num_add_dirs; i++)
    {
    dir = add_dirs[i];
    
    if(dir[strlen(dir)-1] == '/')
      dir[strlen(dir)-1] = '\0';
    bg_nmj_add_directory(plugin_reg, db, dir, TYPES);
    }

  /* Delete directories */
  for(i = 0; i < num_del_dirs; i++)
    {
    dir = del_dirs[i];
    
    if(dir[strlen(dir)-1] == '/')
      dir[strlen(dir)-1] = '\0';
    bg_nmj_remove_directory(db, dir);
    }

  /* Add albums */
  for(i = 0; i < num_add_albums; i++)
    {
    dir = add_albums[i];
    bg_nmj_add_album(db, dir);
    }
  
  bg_nmj_cleanup(db);
  
  /* Close sql connection */
  sqlite3_close(db);
  
  return 0;
  }
