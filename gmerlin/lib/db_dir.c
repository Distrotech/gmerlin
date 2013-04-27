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

static int query_dir(bg_db_t * db, void * dir1, int full)
  {
  char * sql;
  int result;
  bg_db_dir_t * dir = dir1;
  
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

static void update_dir(bg_db_t * db, void * obj)
  {
  bg_db_dir_t * dir = (bg_db_dir_t *)obj;
  char * sql;
  int result;
  
  sql = sqlite3_mprintf("UPDATE DIRECORIES SET UPDATE_ID = %"PRId64" WHERE ID = %"PRId64";",
                        dir->update_id, bg_db_object_get_id(obj));
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  return;
  }

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
  .query = query_dir,
  .update = update_dir,
  .parent = NULL,
  };

int64_t bg_dir_by_path(bg_db_t * db, const char * path)
  {
  return bg_sqlite_string_to_id(db->db,
                                "DIRECTORIES", "ID", "PATH",
                                bg_db_filename_to_rel(db, path));
  }

int64_t bg_parent_dir_by_path(bg_db_t * db, const char * path1)
  {
  int64_t ret;
  char * path = gavl_strdup(path1);
  char * end = strrchr(path, '/');

  if(!end)
    return -1;
  *end = '\0';
  
  ret = bg_sqlite_string_to_id(db->db, "DIRECTORIES", "ID", "PATH",
                               bg_db_filename_to_rel(db, path));
  free(path);
  return ret;
  }

void bg_db_dir_create(bg_db_t * db, int scan_flags,
                      bg_db_scan_item_t * item,
                      bg_db_dir_t * parent, int64_t * scan_dir_id)
  {
  bg_db_object_storage_t obj;
  bg_db_dir_t * dir;
  char * sql;
  int result;
  
  /* Make sure we have the right parent directoy */  
  if(parent && !bg_db_dir_ensure_parent(db, parent, item->path))
    return;

  bg_db_object_init(&obj);
  bg_db_object_create(db, &obj);
  bg_db_object_set_type(&obj, BG_DB_OBJECT_DIRECTORY);
  dir = (bg_db_dir_t *)&obj;

  if(*scan_dir_id > 0)
    dir->scan_dir_id = *scan_dir_id;
  else
    {
    dir->scan_dir_id = bg_db_object_get_id(dir);
    *scan_dir_id = dir->scan_dir_id;
    }
  dir->path = gavl_strdup(item->path);
  
  sql = sqlite3_mprintf("INSERT INTO DIRECTORIES ( ID, PATH, SCAN_FLAGS, UPDATE_ID, SCAN_DIR_ID ) VALUES ( %"PRId64", %Q, %d, %"PRId64", %"PRId64");",
                        bg_db_object_get_id(&obj), bg_db_filename_to_rel(db, dir->path), dir->scan_flags, dir->update_id, dir->scan_dir_id);
  
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);

  bg_db_object_update(db, &obj, 0);
  
  bg_db_object_free(&obj);
  }

#if 0
int bg_db_dir_add(bg_db_t * db, bg_db_dir_t * dir)
  {
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
#endif

int bg_db_dir_ensure_parent(bg_db_t * db, bg_db_dir_t * dir, const char * path)
  {
  int64_t id;
  const char * end = strrchr(path, '/');
  if(!end)
    return 0;
  
  if(bg_db_object_is_valid(dir) &&
     (strlen(dir->path) == (end - path)) &&
     !strncmp(dir->path, path, end-path))
    return 1;

  /* Get proper parent */
  bg_db_object_update(db, dir, 1);
  bg_db_object_free(dir);
  bg_db_object_init(dir);

  id = bg_parent_dir_by_path(db, path);

  if(id < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot find parent directory of %s",
           path);
    return 0;
    }
    
  bg_db_object_query(db, dir, id, 1, 1);
  return 1;
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

