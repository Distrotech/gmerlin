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
#include <bgsqlite.h>

#include <string.h>

#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "sqlite"

int
bg_sqlite_exec(sqlite3 * db,                              /* An open database */
               const char *sql,                           /* SQL to be evaluated */
               int (*callback)(void*,int,char**,char**),  /* Callback function */
               void * data)                               /* 1st argument to callback */
  {
  char * err_msg;
  int err;

  err = sqlite3_exec(db, sql, callback, data, &err_msg);

  //  fprintf(stderr, "Sending %s\n", sql);

  if(err)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "SQL query: \"%s\" failed: %s",
           sql, err_msg);
    sqlite3_free(err_msg);
    return 0;
    }
  return 1;
  }

int bg_sqlite_int_callback(void * data, int argc, char **argv, char **azColName)
  {
  int64_t * ret = data;
  if(argv[0])
    *ret = strtoll(argv[0], NULL, 10);
  return 0;
  }

int bg_sqlite_string_callback(void * data, int argc, char **argv, char **azColName)
  {
  char ** ret = data;
  if((argv[0]) && (*(argv[0]) != '\0'))
    *ret = gavl_strdup(argv[0]);
  return 0;
  }

static int64_t get_max_int(sqlite3 * db, const char * table, const char * row)
  {
  int result;
  char * sql;
  int64_t ret = 0;

  sql = sqlite3_mprintf("select max(%s) from %s;", row, table);
  result = bg_sqlite_exec(db, sql, bg_sqlite_int_callback, &ret);
  sqlite3_free(sql);
  if(!result)
    return -1;

  if(ret < 0)
    return -1;
  return ret;
  }

int64_t bg_sqlite_string_to_id(sqlite3 * db,
                               const char * table,
                               const char * id_row,
                               const char * string_row,
                               const char * str)
  {
  char * buf;
  int64_t ret = -1;
  int result;
  buf = sqlite3_mprintf("select %s from %s where %s = %Q;",
                        id_row, table, string_row, str);
  result = bg_sqlite_exec(db, buf, bg_sqlite_int_callback, &ret);
  sqlite3_free(buf);
  return result ? ret : -1 ;
  }

int64_t bg_sqlite_string_to_id_add(sqlite3 * db,
                                   const char * table,
                                   const char * id_row,
                                   const char * string_row,
                                   const char * str)
  {
  char * sql;
  int result;

  int64_t ret;
  ret = bg_sqlite_string_to_id(db, table, id_row, string_row, str);
  if(ret >= 0)
    return ret;
  ret = get_max_int(db, table, id_row);
  if(ret < 0)
    return ret;

  ret++;
  
  /* Insert into table */
  sql = sqlite3_mprintf("INSERT INTO %s "
                          "( %s, %s ) VALUES "
                          "( %"PRId64", %Q );",
                          table, id_row, string_row, ret, str);

  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return -1;
  return ret;
  }
        

char * bg_sqlite_id_to_string(sqlite3 * db,
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
  result = bg_sqlite_exec(db, buf, bg_sqlite_string_callback, &ret);
  return result ? ret : NULL;
  }

int64_t bg_sqlite_id_to_id(sqlite3 * db,
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
  result = bg_sqlite_exec(db, buf, bg_sqlite_int_callback, &ret);
  return result ? ret : -1;
  }

int64_t bg_sqlite_get_next_id(sqlite3 * db, const char * table)
  {
  int64_t ret = get_max_int(db, table, "ID");
  if(ret < 0)
    return -1;
  return ret + 1;
  }

#if 0
typedef struct
  {
  int64_t * val;
  int val_alloc;
  int num_val;
  } bg_sqlite_id_tab_t;
#endif

void bg_sqlite_id_tab_init(bg_sqlite_id_tab_t * tab)
  {
  memset(tab, 0, sizeof(*tab));
  }

void bg_sqlite_id_tab_free(bg_sqlite_id_tab_t * tab)
  {
  if(tab->val)
    free(tab->val);
  }

void bg_sqlite_id_tab_reset(bg_sqlite_id_tab_t * tab)
  {
  tab->num_val = 0;
  }

int bg_sqlite_append_id_callback(void * data, int argc, char **argv, char **azColName)
  {
  int64_t ret;
  bg_sqlite_id_tab_t * val = data;
  if(argv[0])
    {
    ret = strtoll(argv[0], NULL, 10);
    if(val->num_val + 1 > val->val_alloc)
      {
      val->val_alloc = val->num_val + 1024;
      val->val = realloc(val->val, val->val_alloc * sizeof(*val->val));
      }
    val->val[val->num_val] = ret;
    val->num_val++;
    }
  return 0;
  }

void bg_sqlite_delete_by_id(sqlite3 * db,
                            const char * table,
                            int64_t id)
  {
  char * sql;
  int result;
  sql = sqlite3_mprintf("DELETE FROM %s WHERE ID = %"PRId64";", table, id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  }
