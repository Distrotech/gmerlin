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
#include <unistd.h>

#include <mediadb_private.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#include <string.h>

#define LOG_DOMAIN "db"

static const char * create_commands[] =
  {
    "CREATE TABLE DIRECTORIES ("
    "ID INTEGER PRIMARY KEY, "
    "PATH TEXT, "
    "TYPE INTEGER"
    ");",

    "CREATE TABLE FILES ("
    "ID INTEGER PRIMARY KEY, "
    "PATH TEXT, "
    "SIZE INTEGER, "
    "MTIME TEXT, "
    "MIMETYPE TEXT, "
    "TYPE INTEGER, "
    "DIR INTEGER "
    ");",
    
    "CREATE TABLE MUSIC_GENRES("
    "ID INTEGER PRIMARY KEY, "
    "NAME TEXT"
    ");",

    "CREATE TABLE MUSIC_ARTISTS("
    "ID INTEGER PRIMARY KEY, "
    "NAME TEXT"
    ");",

    "CREATE TABLE MUSIC_FILES("
    "ID INTEGER PRIMARY KEY, "
    "TITLE TEXT, "
    "ARTIST INTEGER, "
    "GENRE INTEGER, "
    "DATE TEXT, "
    "DURATION INTEGER"
    "FILE INTEGER"
    ");",

    "CREATE TABLE MUSIC_ALBUMS("
    "ID INTEGER PRIMARY KEY, "
    "TITLE TEXT"
    ");",

    "CREATE TABLE MUSIC_ALBUMS_GENRES("
    "ID INTEGER PRIMARY KEY, "
    "GENRE INTEGER"
    ");",
    
    "CREATE TABLE MUSIC_ALBUMS_SONGS("
    "ID INTEGER PRIMARY KEY, "
    "ALBUM INTEGER, "
    "FILE INTEGER, "
    "TRACK INTEGER"
    ");",
    
    NULL,
  };

static void build_database(bg_db_t * db)
  {
  int i = 0;
  while(create_commands[i])
    {
    if(!bg_sqlite_exec(db->db, create_commands[i], NULL, NULL))
      return;
    i++;
    }
  }

bg_db_t * bg_db_create(const char * file,
                       bg_plugin_registry_t * plugin_reg, int create)
  {
  int result;
  int exists = 0;
  sqlite3 * db;
  bg_db_t * ret;
  char * pos;
  
  if(!access(file, R_OK | W_OK))
    exists = 1;

  if(!exists && !create)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open database %s (maybe use create option)", file);
    return NULL;
    }

  if(exists && create)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Database %s already exists", file);
    return NULL;
    }
  
  /* Create database */

  result = sqlite3_open(file, &db);
  if(result)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open database %s: %s", file,
           sqlite3_errmsg(db));
    sqlite3_close(db);
    return NULL;
    }

  ret = calloc(1, sizeof(*ret));
  ret->db = db;
  
  if(!exists)
    build_database(ret);
  
  /* Base path */
  ret->base_dir = bg_canonical_filename(file);
  
  pos = strrchr(ret->base_dir, '/');
  pos++;
  *pos = '\0';

  ret->base_len = strlen(ret->base_dir);
  return ret;
  }

char * bg_db_filename_to_abs(bg_db_t * db, const char * filename)
  {
  return bg_sprintf("%s%s", db->base_dir, filename);
  }

const char * bg_db_filename_to_rel(bg_db_t * db, const char * filename)
  {
  return filename + db->base_len;
  }

void bg_db_destroy(bg_db_t * db)
  {
  sqlite3_close(db->db);
  free(db->base_dir);
  free(db);
  }

/* Edit functions */
void bg_db_add_directory(bg_db_t * db, const char * d, int scan_type)
  {
  bg_db_file_t * files;
  int num_files = 0;
  char * real_dir = bg_canonical_filename(d);

  bg_db_dir_t dir;
  bg_db_dir_init(&dir);
  
  if(strncmp(real_dir, db->base_dir, db->base_len))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot add directory %s: No child of %s",
           real_dir, db->base_dir);
    free(real_dir);
    return;
    }

  files = bg_db_file_scan_directory(real_dir, &num_files);

  /* TODO: Check if a directory is already present */
  if(bg_sqlite_string_to_id(db->db, "DIRECTORIES", "ID", "PATH",
                            bg_db_filename_to_rel(db, real_dir)) > 0)
    {
    bg_db_update_files(db, files, num_files, &dir);
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Directory %s already in database, updating",
           real_dir);
    }
  else
    bg_db_add_files(db, files, num_files, &dir);
  bg_db_dir_free(&dir);
  }

void bg_db_del_directory(bg_db_t * db, const char * dir)
  {
  
  }
