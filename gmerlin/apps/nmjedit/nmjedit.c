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

#include "nmjedit.h"

#include <gmerlin/log.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#define LOG_DOMAIN "nmjedit"

#include <string.h>

int
bg_sqlite_exec(sqlite3 * db,                              /* An open database */
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

int64_t bg_nmj_string_to_id(sqlite3 * db,
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
  result = bg_sqlite_exec(db, buf, id_callback, &ret);
  sqlite3_free(buf);
  return result ? ret : -1 ;
  }

char * bg_nmj_id_to_string(sqlite3 * db,
                           const char * table,
                           const char * string_row,
                           const char * id_row,
                           int64_t id)
  {
  char * buf;
  char * ret = NULL;
  int result;
  buf = sqlite3_mprintf("select %s from %s where %s = %"PRId64";",
                        string_row, table, id_row, id);
  result = bg_sqlite_exec(db, buf, string_callback, &ret);
  return result ? ret : NULL;
  }

int64_t bg_nmj_id_to_id(sqlite3 * db,
                        const char * table,
                        const char * dst_row,
                        const char * src_row,
                        int64_t id)
  {
  char * buf;
  int64_t ret = -1;
  int result;
  buf = sqlite3_mprintf("select %s from %s where %s = %"PRId64";",
                        dst_row, table, src_row, id);
  result = bg_sqlite_exec(db, buf, id_callback, &ret);
  return result ? ret : -1;
  }

int64_t bg_nmj_get_next_id(sqlite3 * db, const char * table)
  {
  int result;
  char * sql;
  int64_t ret = -1;

  sql = sqlite3_mprintf("select max(id) from %s;", table);
  result = bg_sqlite_exec(db, sql, id_callback, &ret);
  sqlite3_free(sql);
  if(!result)
    return -1;

  if(ret < 0)
    return -1;

  return ret + 1;
  }

static const struct
  {
  char c;
  char * escaped;
  }
escape_rules[] =
  {
    { '\'', "&apos;" },
    { '&',  "&amp;" },
    { /* End */      }
  };
  
char * bg_nmj_escape_string(const char * str)
  {
  int done;
  int i;
  const char * pos;
  char * ret = NULL;
  char buf[2];

  if(!str)
    return NULL;
  
  buf[1] = '\0';

  pos = str;
  while(*pos != '\0')
    {
    done = 0;
    i = 0;
    while(escape_rules[i].escaped)
      {
      if(escape_rules[i].c == *pos)
        {
        ret = bg_strcat(ret, escape_rules[i].escaped);
        done = 1;
        break;
        }
      i++;
      }
    if(!done)
      {
      buf[0] = *pos;
      ret = bg_strcat(ret, buf);
      }
    pos++;
    }
  return ret;
  }

#if 0
char * bg_nmj_unescape_string(const char * str)
  {
  
  }
#endif

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
    result = bg_sqlite_exec(db, sql, dir_query_callback, dir);
    sqlite3_free(sql);
    return dir->found;
    }
  else if(dir->id >= 0)
    {
    sql = sqlite3_mprintf("select * from SCAN_DIRS where ID = %"PRId64";", dir->id);
    result = bg_sqlite_exec(db, sql, dir_query_callback, dir);
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


/* Album */

/* Main entry points */

static const char * audio_extensions = "mp3 flac ogg";
static const char * video_extensions = "avi mov mkv flv";
static const char * image_extensions = "jpg";

static char * make_extensions(int type)
  {
  char * ret = NULL;
  if(type & BG_NMJ_MEDIA_TYPE_AUDIO)
    {
    if(ret)
      ret = bg_strcat(ret, " ");
    ret = bg_strcat(ret, audio_extensions);
    }
  if(type & BG_NMJ_MEDIA_TYPE_VIDEO)
    {
    if(ret)
      ret = bg_strcat(ret, " ");
    ret = bg_strcat(ret, video_extensions);
    }
  if(type & BG_NMJ_MEDIA_TYPE_IMAGE)
    {
    if(ret)
      ret = bg_strcat(ret, " ");
    ret = bg_strcat(ret, image_extensions);
    }
  return ret;
  }

// YYYY-MM-DD HH:MM:SS
#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

#define BG_NMJ_TIME_STRING_LEN 20
time_t bg_nmj_string_to_time(const char * str)
  {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  strptime(str, TIME_FORMAT, &tm); 
  return mktime(&tm);
  }

void bg_nmj_time_to_string(time_t time, char * str)
  {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  localtime_r(&time, &tm);
  strftime(str, BG_NMJ_TIME_STRING_LEN, TIME_FORMAT, &tm);
  }

static int add_directory(bg_plugin_registry_t * plugin_reg,
                         sqlite3 * db, bg_nmj_dir_t * dir, int type)
  {
  return 0;
  }

static int update_directory(bg_plugin_registry_t * plugin_reg,
                            sqlite3 * db, bg_nmj_dir_t * dir, int type)
  {
  append_int_t tab;
  char * sql;
  int result;
  int i;
  bg_nmj_song_t song;
  bg_nmj_file_t * files;
  int64_t size = 0;
  int ret = 0;
  char * extensions;
  bg_nmj_file_t * file;
  char time_str[BG_NMJ_TIME_STRING_LEN];
  
  memset(&tab, 0, sizeof(tab));

  extensions = make_extensions(type);
  
  files = bg_nmj_file_scan(dir->directory, extensions, &size);
  
  /* 1. Query all files in the database and check if they changed or were deleted */
  sql =
    sqlite3_mprintf("select ID from SONGS where SCAN_DIRS_ID = %"PRId64";",
                    dir->id);
  result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);
  
  if(!result)
    goto fail;
  
  for(i = 0; i < tab.num_val; i++)
    {
    bg_nmj_song_init(&song);
    song.id = tab.val[i];
    
    if(!bg_nmj_song_query(db, &song))
      return 0;
    
    //    fprintf(stderr, "Got song\n");
    //    bg_nmj_song_dump(&song);
    
    /* Check if song is still current */

    file = bg_nmj_file_lookup(files, song.path);
    if(!file)
      {
      /* File disappeared */
      bg_nmj_song_delete(db, &song);
      }
    else if(file->time != bg_nmj_string_to_time(song.create_time))
      {
      bg_nmj_song_t new_song;

      bg_nmj_time_to_string(file->time, time_str);
      fprintf(stderr, "Song %s changed %s -> %s (%ld -> %ld)\n",
              song.title, song.create_time, time_str,
              bg_nmj_string_to_time(song.create_time), file->time);
      
      bg_nmj_song_init(&new_song);
      
      /* File changed */
      if(!bg_nmj_song_get_info(db, plugin_reg, dir, file, 
                              &new_song))
        return 0;
      //      bg_nmj_song_dump(&new_song);
      
      bg_nmj_song_update(db, &song, &new_song);
      
      bg_nmj_song_free(&new_song);
      }

    /* Remove from array */
    if(file)
      bg_nmj_file_remove(files, file);
    
    bg_nmj_song_free(&song);
    }
  
  /* 2. Check for newly added files */
  i = 0;
  while(files[i].path)
    {
    bg_nmj_song_t new_song;
    bg_nmj_song_init(&new_song);
    if(!bg_nmj_song_get_info(db, plugin_reg, dir, file, 
                             &new_song))
      return 0;

    fprintf(stderr, "Got new song\n");
    bg_nmj_song_dump(&new_song);
    
    i++;
    }

  
  ret = 1;
  fail:
  
  if(files)
    bg_nmj_file_destroy(files);
  if(extensions)
    free(extensions);
  
  return ret;
  }


int bg_nmj_add_directory(bg_plugin_registry_t * plugin_reg,
                         sqlite3 * db, const char * directory, int types)
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
    result = update_directory(plugin_reg, db, &dir, types);
    }
  else
    {
    fprintf(stderr, "Directory doesn't exist\n");
    result = add_directory(plugin_reg, db, &dir, types);
    }
  
  bg_nmj_dir_free(&dir);
  
  return result;
  }

int bg_nmj_remove_directory(sqlite3 * db, const char * directory, int types)
  {
  return 0;
  }

