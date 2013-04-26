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

#include <gavl/gavl.h>
#include <gmerlin/mediadb.h>
#include <gavl/metatags.h>

#include <bgsqlite.h>

#define BG_DB_OBJ_FLAG_DONT_PROPAGATE (1<<0)
#define BG_DB_OBJ_FLAG_QUERIED        (1<<1)

/*
 *  Object function sequences:
 *  - get_parent_id (explicit or implicit)
 *  
 *
 */

typedef union
  {
  bg_db_object_t obj;
  bg_db_dir_t dir;
  bg_db_file_t file;
  bg_db_audio_file_t audio_file;
  bg_db_audio_album_t audio_album;
  } bg_db_object_storage_t;

struct bg_db_object_class_s
  {
  void (*del)(bg_db_t * db, bg_db_object_t * obj); // Delete from db
  void (*free)(void * obj);                        // Free memory
  const struct bg_db_object_class_s * parent;
  };

#define BG_DB_CACHE_SIZE 16

struct bg_db_s
  {
  sqlite3 * db;
  char * base_dir;
  int base_len;
  bg_plugin_registry_t * plugin_reg;
  
  bg_db_object_storage_t object_cache[BG_DB_CACHE_SIZE];
  };

/* File scanning */

typedef enum
  {
  BG_SCAN_TYPE_FILE,
  BG_SCAN_TYPE_DIRECTORY,
  } bg_db_scan_type_t;

typedef struct
  {
  bg_db_scan_type_t type;
  char * path;
  int parent_index;
  int64_t id;

  int64_t size;
  time_t mtime;

  int done;
  } bg_db_scan_item_t;

/* Get the parent album of an audio file */

bg_db_audio_album_t * bg_db_get_audio_album(bg_db_t * db, bg_db_audio_file_t * file);

bg_db_scan_item_t * bg_db_scan_directory(const char * directory, int * num);
void bg_scan_items_free(bg_db_scan_item_t *, int num);

/* Object */
void bg_db_object_init(bg_db_object_storage_t * obj);

void bg_db_object_create(bg_db_t * db, bg_db_object_t * obj); /* Create an ID */

int bg_db_object_query(bg_db_t * db, bg_db_object_t * obj);   /* Query from DB  */
void bg_db_object_add(bg_db_t * db, bg_db_object_t * obj);    /* Add to DB      */
void bg_db_object_update(bg_db_t * db, bg_db_object_t * obj); /* Update in DB   */
void bg_db_object_delete(bg_db_t * db, void * obj); /* Delete from DB */
void bg_db_object_free(void * obj);

const bg_db_object_class_t * bg_db_object_get_class(bg_db_object_type_t t);
void bg_db_object_set_type(void * obj, bg_db_object_type_t t);

void bg_object_set_size(void * obj, int64_t size);
void bg_object_set_duration(void * obj, gavl_time_t d);

int64_t bg_db_object_get_id(void * obj);


/* Directory */

int  bg_db_dir_query(bg_db_t * db, bg_db_dir_t * dir, int full);
int bg_db_dir_add(bg_db_t * db, bg_db_dir_t * dir);
int bg_db_dir_del(bg_db_t * db, bg_db_dir_t * dir);
bg_db_dir_t * bg_db_get_parent_dir(bg_db_t * db, const char * path);
extern const bg_db_object_class_t bg_db_dir_class;

/* File */

void bg_db_file_free(bg_db_file_t * f);

int bg_db_file_add(bg_db_t * db, bg_db_file_t * f);
int bg_db_file_query(bg_db_t * db, bg_db_file_t * f, int full);
extern const bg_db_object_class_t bg_db_file_class;

/* Audio file */

void bg_db_audio_file_get_info(void * obj, bg_track_info_t * t);

int bg_db_audio_file_add(bg_db_t * db, bg_db_audio_file_t * t);
int bg_db_audio_file_query(bg_db_t * db, bg_db_audio_file_t * t, int full);
extern const bg_db_object_class_t bg_db_audio_file_class;

/* Audio album */

void bg_db_audio_file_add_to_album(bg_db_t * db, bg_db_audio_file_t * t);
void bg_db_audio_file_remove_from_album(bg_db_t * db, bg_db_audio_file_t * t);
int bg_db_audio_album_query(bg_db_t * db, bg_db_audio_album_t * a, int full);
extern const bg_db_object_class_t bg_db_audio_album_class;

/* Utility functions we might want */

char * bg_db_filename_to_abs(bg_db_t * db, char * filename);
const char * bg_db_filename_to_rel(bg_db_t * db, const char * filename);

void bg_db_add_files(bg_db_t * db, bg_db_scan_item_t * files, int num, int scan_flags);
void bg_db_update_files(bg_db_t * db, bg_db_scan_item_t * files, int num, int scan_flags,
                        int64_t scan_dir_id);

#define BG_DB_TIME_STRING_LEN 20
time_t bg_db_string_to_time(const char * str);
void bg_db_time_to_string(time_t time, char * str);

/* For querying multiple columns of a table */

#define BG_DB_SET_QUERY_STRING(col, val)   \
  if(!strcasecmp(azColName[i], col)) \
    ret->val = gavl_strrep(ret->val, argv[i]);

#define BG_DB_SET_QUERY_INT(col, val)      \
  if(!strcasecmp(azColName[i], col) && argv[i]) \
    ret->val = strtoll(argv[i], NULL, 10);

#define BG_DB_SET_QUERY_DATE(col, val)      \
  if(!strcasecmp(azColName[i], col) && argv[i]) \
    bg_db_string_to_date(argv[i], &ret->val);

#define BG_DB_SET_QUERY_MTIME(col, val)      \
  if(!strcasecmp(azColName[i], col) && argv[i]) \
    ret->val = bg_db_string_to_time(argv[i]);

