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

#define _XOPEN_SOURCE
#define _GNU_SOURCE

#include <config.h>
#include <unistd.h>

#include <mediadb_private.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#include <string.h>

#define LOG_DOMAIN "db"

static const char * create_commands[] =
  {

    "CREATE TABLE OBJECTS ("
    "ID INTEGER PRIMARY KEY, "
    "TYPE INTEGER, "
    "REF_ID INTEGER, "
    "PARENT_ID INTEGER, "
    "CHILDREN INTEGER, "
    "SIZE INTEGER, "
    "DURATION INTEGER, "
    "LABEL INTEGER);"
    
    "CREATE TABLE DIRECTORIES ("
    "ID INTEGER PRIMARY KEY, "
    "PATH TEXT, "
    "SCAN_FLAGS INTEGER, "
    "UPDATE_ID INTEGER, "
    "SCAN_DIR_ID INTEGER"
    ");",

    "CREATE TABLE FILES ("
    "ID INTEGER PRIMARY KEY, "
    "PATH TEXT, "
    "MTIME TEXT, "
    "MIMETYPE INTEGER, "
    "SCAN_DIR_ID INTEGER" 
    ");",

    "CREATE TABLE MIMETYPES("
    "ID INTEGER PRIMARY KEY, "
    "NAME TEXT"
    ");",

    
    "CREATE TABLE AUDIO_GENRES("
    "ID INTEGER PRIMARY KEY, "
    "NAME TEXT"
    ");",

    "CREATE TABLE AUDIO_ARTISTS("
    "ID INTEGER PRIMARY KEY, "
    "NAME TEXT"
    ");",

    "CREATE TABLE AUDIO_FILES("
    "ID INTEGER PRIMARY KEY, "
    "TITLE TEXT, "
    "ARTIST INTEGER, "
    "GENRE INTEGER, "
    "DATE TEXT, "
    "ALBUM INTEGER, "
    "TRACK INTEGER, "
    "BITRATE TEXT"
    ");",

    "CREATE TABLE AUDIO_ALBUMS("
    "ID INTEGER PRIMARY KEY, "
    "ARTIST INTEGER, "
    "TITLE TEXT, "
    "SEARCH_TITLE TEXT, "
    "GENRE INTEGER, "
    "DATE TEXT"
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
  bg_db_create_tables_vfolders(db);
  }

const char * directories[] =
  {
    "gmerlin-db",
    "gmerlin-db/previews",
    "gmerlin-db/thumbnails",
    NULL,
  };

bg_db_t * bg_db_create(const char * path,
                       bg_plugin_registry_t * plugin_reg, int create)
  {
  int result;
  int exists = 0;
  sqlite3 * db;
  bg_db_t * ret;
  int i;
  char * tmp_string;

  tmp_string = bg_sprintf("%s/gmerlin-db", path);
  
  if(!access(tmp_string, R_OK | W_OK))
    exists = 1;
  free(tmp_string);

  if(!exists && !create)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open database in %s (maybe use create option)", path);
    return NULL;
    }

  if(exists && create)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Database in %s already exists", path);
    return NULL;
    }

  if(create && !exists)
    {
    i = 0;
    while(directories[i])
      {
      tmp_string = bg_sprintf("%s/%s", path, directories[i]);
      bg_ensure_directory(tmp_string);
      free(tmp_string);
      i++;
      }
    }
  
  /* Create database */

  tmp_string = bg_sprintf("%s/gmerlin-db/gmerlin.db", path);
  result = sqlite3_open(tmp_string, &db);

  if(result)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open database %s: %s", tmp_string,
           sqlite3_errmsg(db));
    sqlite3_close(db);
    free(tmp_string);
    return NULL;
    }
  free(tmp_string);

  ret = calloc(1, sizeof(*ret));
  ret->db = db;
  ret->plugin_reg = plugin_reg;

  ret->cache_size = 16;
  ret->cache = calloc(ret->cache_size, sizeof(*ret->cache));

  for(i = 0; i < ret->cache_size; i++)
    bg_db_object_init(&ret->cache[i]);
                      
  if(!exists)
    build_database(ret);
  
  /* Base path */
  ret->base_dir = bg_canonical_filename(path);
  ret->base_dir = gavl_strcat(ret->base_dir, "/");
  return ret;
  }

char * bg_db_filename_to_abs(bg_db_t * db, char * filename)
  {
  char * ret;
  ret = bg_sprintf("%s%s", db->base_dir, filename);
  free(filename);
  return ret;
  }

const char * bg_db_filename_to_rel(bg_db_t * db, const char * filename)
  {
  return filename + db->base_len;
  }

// YYYY-MM-DD HH:MM:SS
#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

time_t bg_db_string_to_time(const char * str)
  {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  strptime(str, TIME_FORMAT, &tm); 
  return timegm(&tm);
  }

void bg_db_time_to_string(time_t time, char * str)
  {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  gmtime_r(&time, &tm);
  strftime(str, BG_DB_TIME_STRING_LEN, TIME_FORMAT, &tm);
  }

static int flush_object(bg_db_t * db, int i)
  {
  if(memcmp(&db->cache[i].obj, &db->cache[i].orig,
            sizeof(db->cache[i].obj)))
    {
    //    fprintf(stderr, "Flush %ld\n", db->cache[i].obj.obj.id);
    bg_db_object_update(db, &db->cache[i].obj, 1, 1);
    memcpy(&db->cache[i].orig, &db->cache[i].obj,
           sizeof(db->cache[i].obj));
    return 1;
    }
  return 0;
  }

static int children_in_cache(bg_db_t * db, int i)
  {
  int j;
  int64_t id = bg_db_object_get_id(&db->cache[i]);
  
  for(j = 0; j < db->cache_size; j++)
    {
    if(j != i)
      {
      if(bg_db_object_is_valid(&db->cache[j]) &&
         (db->cache[j].obj.obj.parent_id == id))
        return 1;
      }
    }
  return 0;
  }

static void db_flush(bg_db_t * db, int do_free)
  {
  int i;
  int done = 0;

  /* Pass 1: Flush all leaf objects */
  for(i = 0; i < db->cache_size; i++)
    {
    if(!bg_db_object_is_valid(&db->cache[i]))
      continue;
    
    if(!(bg_db_object_get_type(&db->cache[i]) &
         (BG_DB_FLAG_CONTAINER|BG_DB_FLAG_VCONTAINER)))
      {
      flush_object(db, i);
      if(do_free)
        {
        bg_db_object_free(&db->cache[i].obj);
        bg_db_object_init(&db->cache[i].obj);
        }
      }
    }
  
  while(!done)
    {
    done = 1;
    for(i = 0; i < db->cache_size; i++)
      {
      if(!bg_db_object_is_valid(&db->cache[i]))
        continue;
      
      if(!children_in_cache(db, i))
        {
        if(flush_object(db, i))
          done = 0;

        if(do_free)
          {
          bg_db_object_free(&db->cache[i].obj);
          bg_db_object_init(&db->cache[i].obj);
          }
        }
      }
    }
  
  }

void bg_db_flush(bg_db_t * db)
  {
  db_flush(db, 0);
  }

void bg_db_destroy(bg_db_t * db)
  {
  /* Flush cache */
  db_flush(db, 1);
  free(db->cache);
  
  sqlite3_close(db->db);
  free(db->base_dir);
  free(db);
  }

/* Edit functions */
void bg_db_add_directory(bg_db_t * db, const char * d, int scan_flags)
  {
  bg_db_scan_item_t * files;
  int num_files = 0;
  char * path;
  int64_t id;
  
  path = bg_canonical_filename(d);
  
  if(strncmp(path, db->base_dir, db->base_len))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot add directory %s: No child of %s",
           path, db->base_dir);
    return;
    }
  
  files = bg_db_scan_directory(path, &num_files);

  if((id = bg_dir_by_path(db, path) > 0))
    {
    bg_db_dir_t * dir;
    dir = bg_db_object_query(db, id);
    
    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Directory %s already in database, updating", path);
    bg_db_update_files(db, files, num_files, dir->scan_flags, id);
    bg_db_object_unref(dir);
    }
  else
    {
    /* Create root container */
    bg_db_scan_item_t item;
    int64_t scan_dir_id = -1;
    memset(&item, 0, sizeof(&item));
    item.path = path;
    bg_db_dir_create(db, scan_flags,
                     &item, NULL, &scan_dir_id);
    
    bg_db_add_files(db, files, num_files, scan_flags, scan_dir_id);
    }
  bg_db_scan_items_free(files, num_files);
  }

void bg_db_del_directory(bg_db_t * db, const char * d)
  {
  bg_db_dir_t * dir;
  
  int64_t id;
  char * path = bg_canonical_filename(d);
  
  if((id = bg_dir_by_path(db, path) <= 0))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Directory %s not in database", path);
    free(path);
    return;
    }

  dir = bg_db_object_query(db, id);
  
  if(bg_db_object_get_parent(dir) > 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Directory %s not a root scan directory", path);
    bg_db_object_unref(dir);
    }
  else
    bg_db_object_delete(db, dir);
  free(path);
  }

int bg_db_date_equal(const bg_db_date_t * d1,
                     const bg_db_date_t * d2)
  {
  return
    (d1->year == d2->year) &&
    (d1->month == d2->month) &&
    (d1->day == d2->day);
  }

void bg_db_date_copy(bg_db_date_t * dst,
                     const bg_db_date_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  }

void bg_db_date_to_string(const bg_db_date_t * d, char * str)
  {
  gavl_metadata_date_to_string(d->year, d->month, d->day, str);
  }

void bg_db_string_to_date(const char * str, bg_db_date_t * d)
  {
  sscanf(str, "%d-%d-%d", &d->year, &d->month, &d->day);
  }

void bg_db_date_set_invalid(bg_db_date_t * d)
  {
  d->year  = 9999;
  d->day   = 01;
  d->month = 01;
  }

char * bg_db_path_to_label(const char * path)
  {
  const char * start;
  const char * end;
  start = strrchr(path, '/');
  if(!start)
    start = path;
  else
    start++;

  end = strrchr(start, '.');
  if(!end)
    end = start + strlen(start);

  return gavl_strndup(start, end);
  }

int64_t bg_db_cache_search(bg_db_t * db,
                           int (*compare)(const bg_db_object_t*,const void*),
                           const void * data)
  {
  int i;
  for(i = 0; i < db->cache_size; i++)
    {
    if(compare(&db->cache[i].obj.obj, data))
      return db->cache[i].obj.obj.id;
    }
  return -1;
  }

static const char * search_string_skip[] =
  {
    "a ",
    "the ",
    "'", // 'Round Midnight
    NULL
  };

const char * bg_db_get_search_string(const char * str)
  {
  int i, len;
  
  i = 0;
  while(search_string_skip[i])
    {
    len = strlen(search_string_skip[i]);
    if(!strncasecmp(str, search_string_skip[i], len))
      return str + len;
    i++;
    }
  return str;
  }

/* Query children */

void
bg_db_query_children(bg_db_t * db, int64_t id, bg_db_query_callback cb, void * priv)
  {
  int i;
  char * sql;
  int result;
  
  bg_sqlite_id_tab_t tab;
  bg_db_object_type_t type;
  void * obj;
  bg_db_object_t * parent = bg_db_object_query(db, id);

  if(!parent)
    {
    goto fail;
    }

  bg_sqlite_id_tab_init(&tab);
  bg_db_flush(db);
  
  type = bg_db_object_get_type(parent);
  
  if(type & BG_DB_FLAG_CONTAINER)
    {
    sql = sqlite3_mprintf("select ID from OBJECTS where PARENT_ID = %"PRId64" ORDER BY label;",
                          bg_db_object_get_id(parent));
    result = bg_sqlite_exec(db->db, sql, bg_sqlite_append_id_callback, &tab);
    sqlite3_free(sql);
    if(!result) // Error
      {
      goto fail;
      }
    }
  else // Virtual folder
    {
    }

  for(i = 0; i < tab.num_val; i++)
    {
    obj = bg_db_object_query(db, tab.val[i]);
    if(!obj)
      {
      // Error
      goto fail;
      }
    cb(priv, obj);
    bg_db_object_unref(obj);
    }

  fail:
  if(parent)
    bg_db_object_unref(parent);
  }
