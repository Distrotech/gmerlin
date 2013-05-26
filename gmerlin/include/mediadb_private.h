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
#define BG_DB_OBJ_FLAG_CHANGED        (1<<2)

/*
 *  Object creation:
 *
 *  - bg_db_object_init(bg_db_object_storage_t * obj);
 *  - bg_db_object_create(db, void*);   // Set a unique ID
 *  - bg_db_object_set_type();

 *  - bg_db_object_set_size();
 *  - bg_db_object_set_duration();
 *
 *  - bg_db_object_set_parent[_id](); // Set unique ID of parent
 *
 *  - bg_db_object_update();
 */

/*
 *  Object querying
 *  - bg_db_object_init(bg_db_object_storage_t * obj);
 *  - bg_db_object_query(void*, id);
 *
 */

typedef union
  {
  bg_db_object_t obj;
  bg_db_dir_t dir;
  bg_db_file_t file;
  bg_db_audio_file_t audio_file;
  bg_db_audio_album_t audio_album;
  bg_db_vfolder_t vfolder;
  } bg_db_object_storage_t;

struct bg_db_object_class_s
  {
  const char * name;
  void (*del)(bg_db_t * db, bg_db_object_t * obj);   // Delete from db
  void (*free)(void * obj);                          // Free memory
  int (*query)(bg_db_t * db, void * obj, int full);  // Query from database
  void (*update)(bg_db_t * db, void * obj);          // Update database with new settings
  void (*dump)(void * obj);
  void (*get_children)(bg_db_t * db, void * obj, bg_sqlite_id_tab_t * tab);
  const struct bg_db_object_class_s * parent;
  };


#define BG_DB_CACHE_SIZE 128

typedef struct
  {
  bg_db_object_storage_t obj; // Must be first
  bg_db_object_storage_t orig;
  int refcount;
  int sync;
  } bg_db_cache_t;

struct bg_db_s
  {
  sqlite3 * db;
  char * base_dir;
  int base_len;
  bg_plugin_registry_t * plugin_reg;
  
  int cache_size;
  bg_db_cache_t * cache;

  int num_added;
  int num_deleted;
  };

/* File scanning */

typedef enum
  {
  BG_DB_DIRENT_FILE,
  BG_DB_DIRENT_DIRECTORY,
  } bg_db_dirent_type_t;

typedef struct
  {
  bg_db_dirent_type_t type;
  char * path;
  int parent_index;
  
  int64_t size;
  time_t mtime;

  int done;
  } bg_db_scan_item_t;

/* Get the parent album of an audio file */

bg_db_audio_album_t *
bg_db_get_audio_album(bg_db_t * db, bg_db_audio_file_t * file);

bg_db_scan_item_t *
bg_db_scan_directory(const char * directory, int * num);
void bg_db_scan_items_free(bg_db_scan_item_t *, int num);

void bg_db_scan_item_free(bg_db_scan_item_t * item);
int bg_db_scan_item_set(bg_db_scan_item_t * ret, char * filename);

/* Object */
void * bg_db_object_create(bg_db_t * db); /* Create an object */
void * bg_db_object_create_root(bg_db_t * db);

void bg_db_object_add(bg_db_t * db, bg_db_object_t * obj);    /* Add to DB      */
void bg_db_object_update(bg_db_t * db, void * obj, int children, int parent); /* Update in DB   */
void bg_db_object_delete(bg_db_t * db, void * obj); /* Delete from DB */
void bg_db_object_free(void * obj);

int bg_db_object_is_valid(void * obj);

const bg_db_object_class_t * bg_db_object_get_class(bg_db_object_type_t t);
void bg_db_object_set_type(void * obj, bg_db_object_type_t t);

void bg_db_object_set_parent_id(bg_db_t * db, void * obj, int64_t parent_id);
void bg_db_object_set_parent(bg_db_t * db, void * obj, void * parent);

int64_t bg_db_object_get_parent(void * obj);

void bg_db_object_update_size(bg_db_t * db, void * obj, int64_t delta_size);
void bg_db_object_update_duration(bg_db_t * db, void * obj, gavl_time_t delta_d);



void bg_db_object_create_ref(void * obj, void * parent);
void bg_db_object_set_label_nocpy(void * obj, char * label);
void bg_db_object_set_label(void * obj1, const char * label);

void bg_db_object_add_child(bg_db_t * db, void * obj1, void * child1);
void bg_db_object_remove_child(bg_db_t * db, void * obj1, void * child1);

void bg_db_object_init(void * obj1);

extern const bg_db_object_class_t bg_db_root_class;

/* Directory */

extern const bg_db_object_class_t bg_db_dir_class;

int bg_db_dir_del(bg_db_t * db, bg_db_dir_t * dir);

int64_t bg_dir_by_path(bg_db_t * db, const char * path);
int64_t bg_parent_dir_by_path(bg_db_t * db, const char * path1);

bg_db_dir_t *
bg_db_dir_ensure_parent(bg_db_t * db,
                        bg_db_dir_t * dir, const char * path);

void bg_db_dir_create(bg_db_t * db, int scan_flags,
                      bg_db_scan_item_t * item,
                      bg_db_dir_t ** parent, int64_t * scan_dir_id);

/* File */

int bg_db_file_add(bg_db_t * db, bg_db_file_t * f);
extern const bg_db_object_class_t bg_db_file_class;

void bg_db_file_create(bg_db_t * db, int scan_flags,
                       bg_db_scan_item_t * item,
                       bg_db_dir_t ** parent, int64_t scan_dir_id);

bg_db_file_t *
bg_db_file_create_from_object(bg_db_t * db, bg_db_object_t * obj, int scan_flags,
                              bg_db_scan_item_t * item,
                              int64_t scan_dir_id);


/* Create an internally generated files (e.g. a thumbnail) */
bg_db_file_t * bg_db_file_create_internal(bg_db_t * db, const char * path_rel);

int64_t bg_db_file_by_path(bg_db_t * db, const char * path);

/* Audio file */
void bg_db_audio_file_create(bg_db_t * db, void * obj, bg_track_info_t * t);

int bg_db_audio_file_add(bg_db_t * db, bg_db_audio_file_t * t);
int bg_db_audio_file_query(bg_db_t * db, bg_db_audio_file_t * t, int full);
extern const bg_db_object_class_t bg_db_audio_file_class;

void bg_db_audio_file_create_refs(bg_db_t * db, void * obj);

/* Remove empty audio genres and artists */
void bg_db_cleanup_audio(bg_db_t * db);

/* Audio album */

void bg_db_audio_file_add_to_album(bg_db_t * db, bg_db_audio_file_t * t);
void bg_db_audio_file_remove_from_album(bg_db_t * db, bg_db_audio_file_t * t);
extern const bg_db_object_class_t bg_db_audio_album_class;

/* Image file */

void bg_db_image_file_create_from_ti(bg_db_t * db, void * obj, bg_track_info_t * t);
void bg_db_identify_images(bg_db_t * db, int64_t scan_dir_id, int scan_flags);

const bg_db_object_class_t bg_db_image_file_class;
const bg_db_object_class_t bg_db_album_cover_class;
const bg_db_object_class_t bg_db_thumbnail_class;

/* Virtual folder */

void
bg_db_create_vfolders(bg_db_t * db, void * obj);

void bg_db_create_tables_vfolders(bg_db_t * db);

void
bg_db_cleanup_vfolders(bg_db_t * db);


extern const bg_db_object_class_t bg_db_vfolder_leaf_class;
extern const bg_db_object_class_t bg_db_vfolder_class;

/* Playlists */

int64_t bg_db_playlist_by_name(bg_db_t * db, const char * name);
extern const bg_db_object_class_t bg_db_playlist_class;

/* Utility functions we might want */

char * bg_db_filename_to_abs(bg_db_t * db, char * filename);
const char * bg_db_filename_to_rel(bg_db_t * db, const char * filename);

const char * bg_db_get_search_string(const char * str);

const char * bg_db_get_group(const char * str, int * id);
char * bg_db_get_group_condition(const char * row, int group_id);

void bg_db_add_files(bg_db_t * db, bg_db_scan_item_t * files,
                     int num, int scan_flags, int64_t scan_dir_id);

void bg_db_update_files(bg_db_t * db, bg_db_scan_item_t * files,
                        int num, int scan_flags,
                        int64_t scan_dir_id, const char * scan_dir);

char * bg_db_path_to_label(const char * path);

int64_t bg_db_cache_search(bg_db_t * db,
                           int (*compare)(const bg_db_object_t * ,const void*),
                           const void * data);

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

