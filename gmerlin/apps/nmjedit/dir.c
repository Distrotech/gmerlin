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

/* Dir */

#include "nmjedit.h"
#include <string.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "nmjedit.dir"

#if 0
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
#endif

/* Directory */

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

int bg_nmj_dir_add(sqlite3* db, bg_nmj_dir_t * dir)
  {
  int result;
  char * sql;

  dir->id = bg_nmj_get_next_id(db, "SCAN_DIRS");
  if(!dir->name)
    dir->name = gavl_strrep(dir->name, dir->directory);
 
  sql = sqlite3_mprintf("INSERT INTO SCAN_DIRS ( ID, DIRECTORY, NAME, SIZE, CATEGORY, STATUS ) VALUES "
                       "( %"PRId64", %Q, %Q, %"PRId64", %"PRId64", %Q )",
                       dir->id, dir->directory, dir->name, dir->size, dir->category, dir->status );

  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  return result;
  }

