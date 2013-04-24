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

#define _GNU_SOURCE
#include <time.h>
#include <sqlite3.h>
#include <inttypes.h>

#include <gmerlin/pluginregistry.h>
#include <bgsqlite.h>

#define DATABASE_FILE "nmj_database/media.db"
#define DATABASE_VERSION "2.0.0"

/* Utilits functions */



#define MY_FREE(ptr) \
  if(ptr) \
    free(ptr);

#define SET_QUERY_STRING(col, val)   \
  if(!strcasecmp(azColName[i], col)) \
    ret->val = gavl_strrep(ret->val, argv[i]);

#define SET_QUERY_INT(col, val)      \
  if(!strcasecmp(azColName[i], col) && argv[i]) \
    ret->val = strtoll(argv[i], NULL, 10);

typedef struct
  {
  int64_t * val;
  int val_alloc;
  int num_val;
  } bg_nmj_append_int_t;

int bg_nmj_append_int_callback(void * data, int argc, char **argv, char **azColName);

#define BG_NMJ_TIME_STRING_LEN 20
time_t bg_nmj_string_to_time(const char * str);
void bg_nmj_time_to_string(time_t time, char * str);

char * bg_nmj_escape_string(const char * str);
// char * bg_nmj_unescape_string(const char * str);

char * bg_nmj_make_search_string(const char * str);

int64_t bg_nmj_string_to_id(sqlite3 * db,
                            const char * table,
                            const char * id_row,
                            const char * string_row,
                            const char * str);

char * bg_nmj_id_to_string(sqlite3 * db,
                           const char * table,
                           const char * string_row,
                           const char * id_row,
                           int64_t id);

int64_t bg_nmj_count_id(sqlite3 * db,
                        const char * table,
                        const char * id_row,
                        int64_t id);

int bg_nmj_make_thumbnail(bg_plugin_registry_t * plugin_reg,
                          const char * in_file,
                          const char * out_file,
                          int thumb_size, int force_scale);

int64_t bg_nmj_get_group(sqlite3 * db, const char * table, char * str);

char * bg_nmj_find_dir(sqlite3 * db, const char * path);


/* Directory scanning utility */

typedef struct
  {
  int checked;
  char * path;
  time_t time;
  int64_t size;
  } bg_nmj_file_t;

bg_nmj_file_t * bg_nmj_file_scan(const char * directory,
                                 const char * extensions, int64_t * size, int * num);

bg_nmj_file_t * bg_nmj_file_lookup(bg_nmj_file_t * files,
                                   const char * path);

void bg_nmj_file_destroy(bg_nmj_file_t * files);

void bg_nmj_file_remove(bg_nmj_file_t * files,
                        bg_nmj_file_t * file);


/* Directory */

typedef struct
  {
  int64_t id;
  char * directory; // Relative path
  char * name;      // Absolute path??
  char * scan_time; // NULL?
  int64_t size;     // Sum of all file sizes in bytes
  int64_t category; // 40 for Music
  char * status;    // 3 (??)

  int found;
  } bg_nmj_dir_t;

void bg_nmj_dir_init(bg_nmj_dir_t*);
void bg_nmj_dir_free(bg_nmj_dir_t*);
void bg_nmj_dir_dump(bg_nmj_dir_t*);
int bg_nmj_dir_query(sqlite3*, bg_nmj_dir_t*);
int bg_nmj_dir_add(sqlite3*, bg_nmj_dir_t*);
int bg_nmj_dir_update(sqlite3*, bg_nmj_dir_t*);

/* Song structure */

typedef struct
  {
  /* SONGS */
  int64_t id;
  char * title;
  char * search_title;
  char * path;
  int64_t scan_dirs_id;
  int64_t folders_id;   // Unused? */
  char * runtime;
  char * format;
  char * lyric;
  int64_t rating;   // default: 0
  char * hash;      // Unused?
  int64_t size;
  char * bit_rate;
  int64_t track_position; // 1..
  char * release_date;    // YYYY-01-01
  char * create_time;     // 2012-03-21 23:43:16
  char * update_state;    // "2" or "5"
  char * filestatus;      // unused

  /* Secondary info */
  char * album;
  char * genre;
  char * artist;
  char * albumartist;

  int64_t album_id;
  int64_t artist_id;
  int64_t albumartist_id;
  int64_t genre_id;
  
  int found;
  
  } bg_nmj_song_t;

void bg_nmj_song_free(bg_nmj_song_t * song);
void bg_nmj_song_init(bg_nmj_song_t * song);
void bg_nmj_song_dump(bg_nmj_song_t * song);

int bg_nmj_song_get_info(sqlite3 * db,
                         bg_plugin_registry_t * plugin_reg,
                         bg_nmj_dir_t * dir,
                         bg_nmj_file_t * file,
                         bg_nmj_song_t * song);

int bg_nmj_song_query(sqlite3 * db, bg_nmj_song_t * song);

int bg_nmj_song_add(bg_plugin_registry_t * plugin_reg,
                    sqlite3 * db, bg_nmj_song_t * song);

int bg_nmj_song_delete(sqlite3 * db, bg_nmj_song_t * song);

char * bg_nmj_song_get_cover(bg_nmj_song_t * song);

/* Album */

extern const char * bg_nmj_album_groups[];

typedef struct
  {
  int64_t id;
  char * title;
  char * search_title;
  char * total_item;
  char * release_date;
  char * update_state; // 3??
  
  int64_t genre_id;
  int64_t artist_id;
  
  int found;
  } bg_nmj_album_t;

void bg_nmj_album_init(bg_nmj_album_t *);
void bg_nmj_album_free(bg_nmj_album_t *);
void bg_nmj_album_dump(bg_nmj_album_t *);
int bg_nmj_album_query(sqlite3 * db, bg_nmj_album_t *);
int bg_nmj_album_add(bg_plugin_registry_t * plugin_reg,
                     sqlite3 * db, bg_nmj_album_t *, bg_nmj_song_t * song);
int bg_nmj_album_delete(sqlite3 * db, int64_t album_id, bg_nmj_song_t * song);

void bg_nmj_album_update_cover(bg_plugin_registry_t * plugin_reg,
                               sqlite3 * db, bg_nmj_album_t * album);


int64_t bg_nmj_album_lookup(sqlite3 * db,
                            int64_t artist, const char * title);


/* Master functions */

#define BG_NMJ_MEDIA_TYPE_AUDIO         (1<<0)
#define BG_NMJ_MEDIA_TYPE_VIDEO         (1<<1)
#define BG_NMJ_MEDIA_TYPE_VIDEO_PRIVATE (1<<2)
#define BG_NMJ_MEDIA_TYPE_PHOTO         (1<<3)

int bg_nmj_add_directory(bg_plugin_registry_t * plugin_reg,
                         sqlite3 * db, const char * directory, int types);
int bg_nmj_remove_directory(sqlite3 * db, const char * directory);

void bg_nmj_list_dirs(sqlite3 * db);

int bg_nmj_add_album(sqlite3 * db, const char * album);

void bg_nmj_cleanup(sqlite3 * db);
void bg_nmj_create_new();

void bg_nmj_update_album_covers(bg_plugin_registry_t * plugin_reg, sqlite3 * db);
