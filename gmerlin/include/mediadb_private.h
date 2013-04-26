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

struct bg_db_s
  {
  sqlite3 * db;
  char * base_dir;
  int base_len;
  bg_plugin_registry_t * plugin_reg;
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

bg_db_scan_item_t * bg_db_scan_directory(const char * directory, int * num);
void bg_scan_items_free(bg_db_scan_item_t *, int num);

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

