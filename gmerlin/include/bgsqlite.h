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

#include <inttypes.h>
#include <sqlite3.h>

int
bg_sqlite_exec(sqlite3 * db,                              /* An open database */
               const char *sql,                           /* SQL to be evaluated */
               int (*callback)(void*,int,char**,char**),  /* Callback function */
               void * data);                              /* 1st argument to callback */


int64_t bg_sqlite_string_to_id(sqlite3 * db,
                               const char * table,
                               const char * id_row,
                               const char * string_row,
                               const char * str);

int64_t bg_sqlite_string_to_id_add(sqlite3 * db,
                                   const char * table,
                                   const char * id_row,
                                   const char * string_row,
                                   const char * str);


char * bg_sqlite_id_to_string(sqlite3 * db,
                              const char * table,
                              const char * string_row,
                              const char * id_row,
                              int64_t id);

int64_t bg_sqlite_id_to_id(sqlite3 * db,
                           const char * table,
                           const char * dst_row,
                           const char * src_row,
                           int64_t id);

void bg_sqlite_delete_by_id(sqlite3 * db,
                            const char * table,
                            int64_t id);

int64_t bg_sqlite_get_next_id(sqlite3 * db, const char * table);

/* Get an array of int's */

typedef struct
  {
  int64_t * val;
  int val_alloc;
  int num_val;
  } bg_sqlite_id_tab_t;

void bg_sqlite_id_tab_init(bg_sqlite_id_tab_t * tab);
void bg_sqlite_id_tab_free(bg_sqlite_id_tab_t * tab);
void bg_sqlite_id_tab_reset(bg_sqlite_id_tab_t * tab);

int bg_sqlite_append_id_callback(void * data, int argc, char **argv, char **azColName);

int bg_sqlite_int_callback(void * data, int argc, char **argv, char **azColName);
int bg_sqlite_string_callback(void * data, int argc, char **argv, char **azColName);

