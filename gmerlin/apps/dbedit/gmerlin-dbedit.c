/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#include <gmerlin/translation.h>
#include <gmerlin/mediadb.h>
#include <gmerlin/cmdline.h>

#define LOG_DOMAIN "gmerlin-dbedit"

static int num_add_dirs = 0;
static char ** add_dirs = NULL;

static int num_del_dirs = 0;
static char ** del_dirs = NULL;

static int num_add_albums = 0;
static char ** add_albums = NULL;

static int do_create = 0;

static int scan_type_opt = 0;

static char * path = ".";

static char * dump_object = NULL;
static char * dump_children = NULL;

static void
opt_a(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -add requires an argument\n");
    exit(-1);
    }
  
  add_dirs = realloc(add_dirs,
                     sizeof(*add_dirs) * (num_add_dirs+1));
  add_dirs[num_add_dirs] = (*_argv)[arg];
  num_add_dirs++;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void
opt_d(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -del requires an argument\n");
    exit(-1);
    }
  
  del_dirs = realloc(del_dirs,
                     sizeof(*del_dirs) * (num_del_dirs+1));
  del_dirs[num_del_dirs] = (*_argv)[arg];
  num_del_dirs++;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void
opt_add_album(void * data, int * argc, char *** _argv, int arg)
  {
  if(arg >= *argc)
    {
    fprintf(stderr, "Option -add-album requires an argument\n");
    exit(-1);
    }
  
  add_albums = realloc(add_albums,
                     sizeof(*add_albums) * (num_add_albums+1));
  add_albums[num_add_albums] = (*_argv)[arg];
  num_add_albums++;
  bg_cmdline_remove_arg(argc, _argv, arg);
  }

static void
opt_create(void * data, int * argc, char *** _argv, int arg)
  {
  do_create = 1;
  }

static void
opt_audio(void * data, int * argc, char *** _argv, int arg)
  {
  scan_type_opt |= BG_DB_SCAN_AUDIO;
  }

static void
opt_video(void * data, int * argc, char *** _argv, int arg)
  {
  scan_type_opt |= BG_DB_SCAN_VIDEO;
  }

static void
opt_photo(void * data, int * argc, char *** _argv, int arg)
  {
  scan_type_opt |= BG_DB_SCAN_PHOTO;
  }

static bg_cmdline_arg_t global_options[] =
  {
    {
      .arg =         "-create",
      .help_string = TRS("Create a new database in the current directory"),
      .callback =    opt_create,
    },
    {
      .arg =         "-root",
      .help_string = "Root directory of the database",
      .argv         = &path,
    },
    {
      .arg =         "-add",
      .help_arg =    "<directory>",
      .help_string = TRS("Add this directory"),
      .callback =    opt_a,
    },
    {
      .arg =         "-del",
      .help_arg =    "<directory>",
      .help_string = TRS("Delete this directory"),
      .callback =    opt_d,
    },
    {
      .arg =         "-add-album",
      .help_arg =    "<album>",
      .help_string = TRS("Add an album as a playlist"),
      .callback =    opt_add_album,
    },
    {
      .arg =         "-audio",
      .help_string = TRS("Add audio files"),
      .callback =    opt_audio,
    },
    {
      .arg =         "-video",
      .help_string = TRS("Add video files"),
      .callback =    opt_video,
    },
    {
      .arg =         "-photo",
      .help_string = TRS("Add photo files"),
      .callback =    opt_photo,
    },
    {
      .arg =         "-do",
      .help_arg =    "<ID>",
      .help_string = TRS("Dump database object"),
      .argv = &dump_object,
    },
    {
      .arg =         "-dc",
      .help_arg =    "<ID>",
      .help_string = TRS("Dump children of the specified object"),
      .argv = &dump_children,
    },
    { /* End */ }
  };

const bg_cmdline_app_data_t app_data =
  {
    .package =  PACKAGE,
    .version =  VERSION,
    .name =     "gmerlin-dbedit",
    .long_name = TRS("Editor for the gmerlin media database"),
    .synopsis = TRS("[options]\n"),
    .help_before = TRS("gmerlin db editor\n"),
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

static void callback(void * priv, void * object)
  {
  bg_db_object_dump(object);
  }

int main(int argc, char ** argv)
  {
  bg_cfg_section_t * cfg_section;
  char * tmp_path;
  bg_plugin_registry_t * plugin_reg;
  bg_cfg_registry_t * cfg_reg;
  bg_db_t * db;
  int scan_type;
  int i;
  
  bg_cmdline_init(&app_data);
  bg_cmdline_parse(global_options, &argc, &argv, NULL);

  if(!bg_cmdline_check_unsupported(argc, argv))
    return -1;

  /* Create plugin registry */
  cfg_reg = bg_cfg_registry_create();
  tmp_path =  bg_search_file_read("generic", "config.xml");
  bg_cfg_registry_load(cfg_reg, tmp_path);
  if(tmp_path)
    free(tmp_path);
  
  cfg_section = bg_cfg_registry_find_section(cfg_reg, "plugins");
  plugin_reg = bg_plugin_registry_create(cfg_section);

  db = bg_db_create(path, plugin_reg, do_create);
  if(!db)
    return -1;

  if(scan_type_opt)
    scan_type = scan_type_opt;
  else
    scan_type = BG_DB_SCAN_AUDIO | BG_DB_SCAN_VIDEO | BG_DB_SCAN_PHOTO;

  for(i = 0; i < num_add_dirs; i++)
    bg_db_add_directory(db, add_dirs[i], scan_type);
  for(i = 0; i < num_del_dirs; i++)
    bg_db_del_directory(db, add_dirs[i]);
  for(i = 0; i < num_add_albums; i++)
    bg_db_add_album(db, add_albums[i]);
  
  if(dump_object)
    {
    bg_db_object_t * obj;
    int64_t id = strtoll(dump_object, NULL, 10);
    obj = bg_db_object_query(db, id);
    if(!obj)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No such object");
    else
      {
      bg_db_object_dump(obj);
      bg_db_object_unref(obj);
      }
    }
  if(dump_children)
    {
    int64_t id = strtoll(dump_children, NULL, 10);
    bg_db_query_children(db, id, callback, NULL, 0, 0, NULL);
    }
  
  bg_db_destroy(db);

  bg_plugin_registry_destroy(plugin_reg);
  bg_cfg_registry_destroy(cfg_reg);
  

  return 0;
  }
