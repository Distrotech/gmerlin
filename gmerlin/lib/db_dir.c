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
#if 0
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
#endif

#define PATH_COL          1
#define SCAN_FLAGS_COL    2
#define UPDATE_ID_COL     3
#define SCAN_DIR_ID_COL   4

static int query_dir(bg_db_t * db, void * dir1, int full)
  {
  int result;
  int found = 0;
  bg_db_dir_t * dir = dir1;
  sqlite3_stmt * st = db->q_directories;
  sqlite3_bind_int64(st, 1, dir->obj.id);

  if((result = sqlite3_step(st)) == SQLITE_ROW)
    {
    BG_DB_GET_COL_STRING(PATH_COL, dir->path);
    BG_DB_GET_COL_INT(SCAN_FLAGS_COL, dir->scan_flags);
    BG_DB_GET_COL_INT(UPDATE_ID_COL, dir->update_id);
    BG_DB_GET_COL_INT(SCAN_DIR_ID_COL, dir->scan_dir_id);
    found = 1;
    }

  sqlite3_reset(st);
  sqlite3_clear_bindings(st);
  
  if(!found)
    return 0;
  
  dir->path = bg_db_filename_to_abs(db, dir->path);
  return 1;
  }

static void update_dir(bg_db_t * db, void * obj)
  {
  bg_db_dir_t * dir = (bg_db_dir_t *)obj;
  char * sql;
  int result;
  
  sql = sqlite3_mprintf("UPDATE DIRECTORIES SET UPDATE_ID = %"PRId64" WHERE ID = %"PRId64";",
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

static void dump_dir(void * obj)
  {
  bg_db_dir_t*d = obj;
  gavl_diprintf(2, "Path:        %s\n", d->path);
  gavl_diprintf(2, "Scan flags:  %04x\n", d->scan_flags);
  gavl_diprintf(2, "Update ID:   %d\n", d->update_id);
  gavl_diprintf(2, "Scan dir ID: %"PRId64"\n", d->scan_dir_id);
  }

const bg_db_object_class_t bg_db_dir_class =
  {
    .name = "Directory",
    .del = del_dir,
    .free = free_dir,
    .query = query_dir,
    .update = update_dir,
    .dump = dump_dir,
    .parent = NULL,
  };

static int compare_dir_by_path(const bg_db_object_t * obj, const void * data)
  {
  bg_db_dir_t * dir;
  if(obj->type == BG_DB_OBJECT_DIRECTORY)
    {
    dir = (bg_db_dir_t*)obj;
    if(!strcmp(dir->path, data))
      return 1;
    }
  return 0;
  } 

int64_t bg_dir_by_path(bg_db_t * db, const char * path)
  {
  int64_t ret;

  ret = bg_db_cache_search(db, compare_dir_by_path,
                           path);
  if(ret > 0)
    return ret;
  
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

  ret = bg_db_cache_search(db, compare_dir_by_path,
                           path);
  if(ret > 0)
    {
    free(path);
    return ret;
    }
  ret = bg_sqlite_string_to_id(db->db, "DIRECTORIES", "ID", "PATH",
                               bg_db_filename_to_rel(db, path));
  free(path);
  return ret;
  }

void bg_db_dir_create(bg_db_t * db, int scan_flags,
                      bg_db_scan_item_t * item,
                      bg_db_dir_t ** parent, int64_t * scan_dir_id)
  {
  bg_db_object_t * obj;
  bg_db_dir_t * dir;
  char * sql;
  int result;
  
  /* Make sure we have the right parent directoy */  
  if(parent)
    {
    if(!(*parent = bg_db_dir_ensure_parent(db, *parent, item->path)))
      return;
    }

  obj = bg_db_object_create(db);
  bg_db_object_set_type(obj, BG_DB_OBJECT_DIRECTORY);
  
  if(parent)
    bg_db_object_set_parent(db, obj, *parent);
  else
    bg_db_object_set_parent_id(db, obj, 0);    
  
  dir = (bg_db_dir_t *)obj;

  if(*scan_dir_id > 0)
    dir->scan_dir_id = *scan_dir_id;
  else
    {
    dir->scan_dir_id = bg_db_object_get_id(dir);
    *scan_dir_id = dir->scan_dir_id;
    }
  dir->scan_flags = scan_flags;
  dir->path = item->path;
  item->path = NULL;
  bg_db_object_set_label_nocpy(dir, bg_db_path_to_label(dir->path));
  
  
  sql = sqlite3_mprintf("INSERT INTO DIRECTORIES ( ID, PATH, SCAN_FLAGS, UPDATE_ID, SCAN_DIR_ID ) VALUES ( %"PRId64", %Q, %d, %"PRId64", %"PRId64");",
                        bg_db_object_get_id(obj), bg_db_filename_to_rel(db, dir->path), dir->scan_flags, dir->update_id, dir->scan_dir_id);
  
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  
  bg_db_object_unref(obj);
  }

bg_db_dir_t *
bg_db_dir_ensure_parent(bg_db_t * db, bg_db_dir_t * dir, const char * path)
  {
  int64_t id;
  const char * end = strrchr(path, '/');
  if(!end)
    return 0;
  
  if(dir &&
     (strlen(dir->path) == (end - path)) &&
     !strncmp(dir->path, path, end-path))
    return dir;

  /* Get proper parent */
  if(dir)
    bg_db_object_unref(dir);
  
  id = bg_parent_dir_by_path(db, path);

  if(id < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot find parent directory of %s",
           path);
    return 0;
    }
  return bg_db_object_query(db, id);
  }
