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
#include <stdlib.h>
#include <string.h>

#define LOG_DOMAIN "db.dir"

void bg_db_dir_init(bg_db_dir_t * dir)
  {
  memset(dir, 0, sizeof(*dir));
  dir->id = -1;
  }

void bg_db_dir_free(bg_db_dir_t * dir)
  {
  if(dir->path)
    free(dir->path);
  }

static int dir_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_dir_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_INT("ID",          id);
    BG_DB_SET_QUERY_STRING("PATH",     path);
    BG_DB_SET_QUERY_INT("SIZE",        size);
    BG_DB_SET_QUERY_INT("SCAN_FLAGS",  scan_flags);
    BG_DB_SET_QUERY_INT("UPDATE_ID",   update_id);
    BG_DB_SET_QUERY_INT("PARENT_ID",   parent_id);
    BG_DB_SET_QUERY_INT("SCAN_DIR_ID", scan_dir_id);
    }
  ret->found = 1;
  return 0;
  }

int bg_db_dir_query(bg_db_t * db, bg_db_dir_t * dir, int full)
  {
  char * sql;
  int result;

  if(dir->path)
    {
    sql = sqlite3_mprintf("select * from DIRECTORIES where PATH = %Q;", 
                          bg_db_filename_to_rel(db, dir->path));
    result = bg_sqlite_exec(db->db, sql, dir_query_callback, dir);
    sqlite3_free(sql);
    return result && dir->found;
    }
  else if(dir->id >= 0)
    {
    sql =
      sqlite3_mprintf("select * from DIRECTORIES where ID = %"PRId64";",
                      dir->id);
    result = bg_sqlite_exec(db->db, sql, dir_query_callback, dir);
    sqlite3_free(sql);
    return result && dir->found;
    }
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Either ID or path must be set in directory");
    return 0;
    }
  dir->path = bg_db_filename_to_abs(db, dir->path);
  return 1;
  }

int bg_db_dir_add(bg_db_t * db, bg_db_dir_t * dir)
  {
  char * sql;
  int result;

  dir->id = bg_sqlite_get_next_id(db->db, "DIRECTORIES");
  dir->update_id = 1;

  sql = sqlite3_mprintf("INSERT INTO DIRECTORIES ( ID, PATH, SIZE, SCAN_FLAGS, UPDATE_ID, PARENT_ID, SCAN_DIR_ID ) VALUES ( %"PRId64", %Q, %"PRId64", %"PRId64", %"PRId64", %"PRId64", %"PRId64" );",
                        dir->id, bg_db_filename_to_rel(db, dir->path), dir->size, dir->scan_flags, dir->update_id, dir->parent_id, dir->scan_dir_id);

  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  return result;
  }

int bg_db_dir_update(bg_db_t * db, bg_db_dir_t * dir)
  {
  char * sql;
  int result;
 
  dir->update_id++;
  
  sql = sqlite3_mprintf("UPDATE DIRECORIES SET UPDATE_ID = %"PRId64" WHERE ID = %"PRId64";",
                        dir->update_id, dir->id);
  
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  return result;
  }
