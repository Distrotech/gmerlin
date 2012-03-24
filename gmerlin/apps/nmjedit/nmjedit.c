/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <string.h>

#include "nmjedit.h"
#include <config.h>
#include <gmerlin/log.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#define LOG_DOMAIN "nmjedit"

#define MY_FREE(ptr) \
  if(ptr) \
    free(ptr);

#define SET_QUERY_STRING(col, val)   \
  if(!strcasecmp(azColName[i], col)) \
    ret->val = bg_strdup(ret->val, argv[i]);

#define SET_QUERY_INT(col, val)      \
  if(!strcasecmp(azColName[i], col) && argv[i]) \
    ret->val = strtoll(argv[i], NULL, 10);

static int my_sqlite_exec(sqlite3 * db,                              /* An open database */
                          const char *sql,                           /* SQL to be evaluated */
                          int (*callback)(void*,int,char**,char**),  /* Callback function */
                          void * data)                               /* 1st argument to callback */
  {
  char * err_msg;
  int err;

  err = sqlite3_exec(db, sql, callback, data, &err_msg);

  fprintf(stderr, "Sending %s\n", sql);

  if(err)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "SQL query: \"%s\" failed: %s",
           sql, err_msg);
    sqlite3_free(err_msg);
    return 0;
    }
  return 1;
  }

/* Callbacks */

static int id_callback(void * data, int argc, char **argv, char **azColName)
  {
  int64_t * ret = data;
  *ret = strtoll(argv[0], NULL, 10);
  return 0;
  }

static int string_callback(void * data, int argc, char **argv, char **azColName)
  {
  char ** ret = data;
  if((argv[0]) && (*(argv[0]) != '\0'))
    *ret = strdup(argv[0]);
  return 0;
  }

typedef struct
  {
  int64_t * val;
  int val_alloc;
  int num_val;
  } append_int_t;

static int
append_int_callback(void * data, int argc, char **argv, char **azColName)
  {
  int64_t ret;
  append_int_t * val = data;
  ret = strtoll(argv[0], NULL, 10);
  if(val->num_val + 1 > val->val_alloc)
    {
    val->val_alloc = val->num_val + 128;
    val->val = realloc(val->val, val->val_alloc * sizeof(*val->val));
    }
  val->val[val->num_val] = ret;
  val->num_val++;
  return 0;
  }

static int64_t string_to_id(sqlite3 * db,
                            const char * table,
                            const char * id_row,
                            const char * string_row,
                            const char * str)
  {
  char * buf;
  int64_t ret = -1;
  int result;
  buf = sqlite3_mprintf("select %s from %s where %s = \"%s\";",
                        id_row, table, string_row, str);
  result = my_sqlite_exec(db, buf, id_callback, &ret);
  sqlite3_free(buf);
  return result ? ret : -1 ;
  }

static char * id_to_string(sqlite3 * db,
                           const char * table,
                           const char * string_row,
                           const char * id_row,
                           int64_t id)
  {
  char * buf;
  char * ret = NULL;
  int result;
  buf = sqlite3_mprintf("select %s from %s where %s = %ld;",
                        string_row, table, id_row, id);
  result = my_sqlite_exec(db, buf, string_callback, &ret);
  return result ? ret : NULL;
  }

/* Directory */

#if 0
typedef struct
  {
  int64_t id;
  char * directory; // Relative path
  char * name;      // Absolute path??
  int64_t size;     // Sum of all file sizes in bytes
  int64_t category; // 40 for Music
  char * status;    // 3 (??)
  } bg_nmj_dir_t;
#endif

void bg_nmj_dir_init(bg_nmj_dir_t * dir)
  {
  memset(dir, 0, sizeof(*dir));
  dir->id   = -1;
  dir->size = -1;
  }

void bg_nmj_dir_free(bg_nmj_dir_t *  dir)
  {
  MY_FREE(dir->directory);
  MY_FREE(dir->name);
  MY_FREE(dir->status);
  MY_FREE(dir->scan_time);
  }

void bg_nmj_dir_dump(bg_nmj_dir_t*  dir)
  {
  bg_dprintf("Directory\n");
  bg_dprintf("  ID:        %"PRId64"\n", dir->id);
  bg_dprintf("  Directory: %s\n",        dir->directory);
  bg_dprintf("  Name:      %s\n",        dir->name);
  bg_dprintf("  ScanTime:  %s\n",        dir->scan_time);
  bg_dprintf("  Size:      %"PRId64"\n", dir->size);
  bg_dprintf("  Category:  %"PRId64"\n", dir->category);
  bg_dprintf("  Status:    %s\n",        dir->status);
  }

static int dir_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_nmj_dir_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    //    fprintf(stderr, "col: %s, val: %s\n", azColName[i], argv[i]);
    SET_QUERY_INT("ID", id);
    SET_QUERY_STRING("DIRECTORY", directory);
    SET_QUERY_STRING("NAME", name);
    SET_QUERY_INT("SIZE", size);
    SET_QUERY_INT("CATEGORY", category);
    SET_QUERY_STRING("STATUS", status);
    }
  ret->found = 1;
  return 0;
  }

int bg_nmj_dir_query(sqlite3 * db, bg_nmj_dir_t * dir)
  {
  char * sql;
  int result;
  
  if(dir->directory)
    {
    sql = sqlite3_mprintf("select * from SCAN_DIRS where DIRECTORY = %Q;", dir->directory);
    result = my_sqlite_exec(db, sql, dir_query_callback, dir);
    sqlite3_free(sql);
    return dir->found;
    }
  else if(dir->id >= 0)
    {
    sql = sqlite3_mprintf("select * from SCAN_DIRS where ID = %"PRId64";", dir->id);
    result = my_sqlite_exec(db, sql, dir_query_callback, dir);
    sqlite3_free(sql);
    return dir->found;
    }
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Either ID or path must be set in directory\n");
    return 0;
    }
  }

int bg_nmj_dir_save(sqlite3* db, bg_nmj_dir_t * dir)
  {
  return 0;
  }

/* Song */

#if 0
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

  /* Stuff for other databases */
  int64_t album_id;
  int64_t artist_id;
  int64_t genre_id;
  
  } bg_nmj_song_t;
#endif

void bg_nmj_song_free(bg_nmj_song_t * song)
  {
  MY_FREE(song->title);
  MY_FREE(song->search_title);
  MY_FREE(song->path);
  MY_FREE(song->runtime);
  MY_FREE(song->format);
  MY_FREE(song->lyric);
  MY_FREE(song->hash);
  MY_FREE(song->bit_rate);
  MY_FREE(song->release_date);
  MY_FREE(song->create_time);
  MY_FREE(song->update_state);
  MY_FREE(song->filestatus);
  }

void bg_nmj_song_init(bg_nmj_song_t * song)
  {
  memset(song, 0, sizeof(*song));
  song->id = -1;
  }

void bg_nmj_song_dump(bg_nmj_song_t * song)
  {
  bg_dprintf("Song:\n");
  bg_dprintf("  ID:             %"PRId64"\n", song->id);
  bg_dprintf("  title:          %s\n",        song->title);
  bg_dprintf("  search_title:   %s\n",        song->search_title);
  bg_dprintf("  path:           %s\n",        song->path);
  bg_dprintf("  scan_dirs_id:   %"PRId64"\n", song->scan_dirs_id);
  bg_dprintf("  folders_id:     %"PRId64"\n", song->folders_id);
  bg_dprintf("  runtime:        %s\n",        song->runtime);
  bg_dprintf("  format:         %s\n",        song->format);
  bg_dprintf("  lyric:          %s\n",        song->lyric);
  bg_dprintf("  rating:         %"PRId64"\n", song->rating);
  bg_dprintf("  hash:           %s\n",        song->hash);
  bg_dprintf("  size:           %"PRId64"\n",        song->size);
  bg_dprintf("  bit_rate:       %s\n",        song->bit_rate);
  bg_dprintf("  track_position: %"PRId64"\n",        song->track_position);
  bg_dprintf("  release_date:   %s\n",        song->release_date);
  bg_dprintf("  create_time:    %s\n",        song->create_time);
  bg_dprintf("  update_state:   %s\n",        song->update_state);
  bg_dprintf("  filestatus:     %s\n",        song->filestatus);
  }

static int song_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_nmj_song_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    //    fprintf(stderr, "col: %s, val: %s\n", azColName[i], argv[i]);
    SET_QUERY_INT("ID", id);
    SET_QUERY_STRING("TITLE", title);
    SET_QUERY_STRING("SEARCH_TITLE", search_title);
    SET_QUERY_STRING("PATH", path);
    SET_QUERY_INT("SCAN_DIRS_ID", scan_dirs_id);
    SET_QUERY_INT("FOLDERS_ID", folders_id);
    SET_QUERY_STRING("RUNTIME", runtime);
    SET_QUERY_STRING("FORMAT", format);
    SET_QUERY_STRING("LYRIC", lyric);
    SET_QUERY_INT("RATING", rating);
    SET_QUERY_STRING("HASH", hash);
    SET_QUERY_INT("SIZE", size);
    SET_QUERY_STRING("BIT_RATE", bit_rate);
    SET_QUERY_INT("TRACK_POSITION", track_position);
    SET_QUERY_STRING("RELEASE_DATE", release_date);
    SET_QUERY_STRING("CREATE_TIME", create_time);
    SET_QUERY_STRING("UPDATE_STATE", update_state );
    SET_QUERY_STRING("FILESTATUS", filestatus );
    }
  ret->found = 1;
  return 0;
  }


int bg_nmj_song_query(sqlite3 * db, bg_nmj_song_t * song)
  {
  char * sql;
  int result;
  if(song->path)
    {
    sql = sqlite3_mprintf("select * from SONGS where PATH = %Q;", song->path);
    result = my_sqlite_exec(db, sql, song_query_callback, song);
    sqlite3_free(sql);
    return song->found;
    }
  else if(song->id >= 0)
    {
    sql = sqlite3_mprintf("select * from SONGS where ID = %"PRId64";", song->id);
    result = my_sqlite_exec(db, sql, song_query_callback, song);
    sqlite3_free(sql);
    return song->found;
    }
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Either ID or path must be set in directory\n");
    return 0;
    }
  }

int bg_nmj_song_save(sqlite3 * db, bg_nmj_song_t * song)
  {
  return 0;
  }

/* Album */

/* Main entry points */

static int add_directory(sqlite3 * db, bg_nmj_dir_t * dir, int type)
  {
  return 0;
  }

static int update_directory(sqlite3 * db, bg_nmj_dir_t * dir, int type)
  {
  append_int_t tab;
  char * sql;
  int result;
  int i;
  bg_nmj_song_t song;
  
  memset(&tab, 0, sizeof(tab));
  
  /* 1. Query all files in the database and check if they changed or were deleted */
  sql = sqlite3_mprintf("select ID from SONGS where SCAN_DIRS_ID = %"PRId64";", dir->id);
  result = my_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);
  
  if(!result)
    return 0;

  for(i = 0; i < tab.num_val; i++)
    {
    bg_nmj_song_init(&song);
    song.id = tab.val[i];

    if(!bg_nmj_song_query(db, &song))
      return 0;
    
    fprintf(stderr, "Got song\n");
    bg_nmj_song_dump(&song);
    
    /* Check if song is still current */
    
    bg_nmj_song_free(&song);
    }
  
  /* 2. Check for newly added files */
  
  return 1;
  }


int bg_nmj_add_directory(sqlite3 * db, const char * directory, int types)
  {
  int result;
  /* Check if the directory already exists */
  bg_nmj_dir_t dir;
  bg_nmj_dir_init(&dir);
  dir.directory = bg_strdup(NULL, directory);

  if(bg_nmj_dir_query(db, &dir))
    {
    fprintf(stderr, "Directory exists\n");
    bg_nmj_dir_dump(&dir);
    result = update_directory(db, &dir, types);
    }
  else
    {
    fprintf(stderr, "Directory doesn't exist\n");
    result = add_directory(db, &dir, types);
    }
  
  bg_nmj_dir_free(&dir);
  
  return result;
  }

int bg_nmj_remove_directory(sqlite3 * db, const char * directory, int types)
  {
  return 0;
  }

