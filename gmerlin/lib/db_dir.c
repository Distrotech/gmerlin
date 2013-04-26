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

/* Delete from database */

static void del_dir(bg_db_t * db, bg_db_object_t * obj) // Delete from db
  {
  bg_sqlite_delete_by_id(db->db, "DIRECTORIES", obj->id);
  }

static void free_dir(void * obj) // Delete from db
  {
  bg_db_dir_t*d = obj;
  if(d->path)
    free(d->path);
  }

const bg_db_object_class_t bg_db_dir_class =
  {
  .del = del_dir,
  .free = free_dir,
  .parent = NULL,
  };

#if 0
void bg_db_dir_init(bg_db_dir_t * dir)
  {
  memset(dir, 0, sizeof(*dir));

  if(obj)
    memcpy(&dir->obj, obj, sizeof(*obj));

  dir->obj.type = BG_DB_OBJECT_DIRECTORY;
  dir->obj.klass = &klass;
  
  bg_db_object_init(&dir->obj);
  
  }
void bg_db_dir_free(bg_db_dir_t * dir)
  {
  if(dir->path)
    free(dir->path);
  }
#endif


static int dir_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_dir_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_STRING("PATH",     path);
    BG_DB_SET_QUERY_INT("SCAN_FLAGS",  scan_flags);
    BG_DB_SET_QUERY_INT("UPDATE_ID",   update_id);
    BG_DB_SET_QUERY_INT("SCAN_DIR_ID", scan_dir_id);
    }
  ret->obj.found = 1;
  return 0;
  }

int bg_db_dir_query(bg_db_t * db, bg_db_dir_t * dir, int full)
  {
  char * sql;
  int result;

  if(dir->obj.id <= 0)
    {
    if(!dir->path)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Either ID or path must be set in directory");
      return 0;
      }
    
    dir->obj.id = bg_sqlite_string_to_id(db->db,
                                         "DIRECTORIES",
                                         "ID",
                                         "PATH",
                                         dir->path);
    }

  if(!bg_db_object_query(db, &dir->obj))
    return 0;
  
  sql =
    sqlite3_mprintf("select * from DIRECTORIES where ID = %"PRId64";",
                    dir->obj.id);
  result = bg_sqlite_exec(db->db, sql, dir_query_callback, dir);
  sqlite3_free(sql);
  if(!result || dir->obj.found)
    return 0;
  
  dir->path = bg_db_filename_to_abs(db, dir->path);
  return 1;
  }

int bg_db_dir_add(bg_db_t * db, bg_db_dir_t * dir)
  {
  char * sql;
  int result;
  bg_db_object_create(db, &dir->obj);
  bg_db_object_add(db, &dir->obj);
  
  dir->obj.id = bg_sqlite_get_next_id(db->db, "DIRECTORIES");
  dir->update_id = 1;

  sql = sqlite3_mprintf("INSERT INTO DIRECTORIES ( ID, PATH, SCAN_FLAGS, UPDATE_ID, SCAN_DIR_ID ) VALUES ( %"PRId64", %Q, %"PRId64", %"PRId64", %"PRId64", %"PRId64", %"PRId64" );",
                        dir->obj.id, bg_db_filename_to_rel(db, dir->path), dir->scan_flags, dir->update_id, dir->scan_dir_id);

  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  return result;
  }

bg_db_dir_t * bg_db_get_parent_dir(bg_db_t * db, const char * path_c)
  {
  int i;
  char * path = gavl_strdup(path_c);
  char * end_pos = strrchr(path, '/');
  if(!end_pos)
    return NULL;

  *end_pos = '\0';

  for(i = 0; i < BG_DB_CACHE_SIZE; i++)
    {
    
    }
  }

#if 0
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
#endif

