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

#include <bgsqlite.h>

struct bg_db_s
  {
  sqlite3 * db;
  char * base_dir;
  int base_len;
  };

/* Utility functions we might want */

char * bg_db_filename_to_abs(bg_db_t * db, const char * filename);
const char * bg_db_filename_to_rel(bg_db_t * db, const char * filename);

bg_db_file_t * bg_db_file_scan_directory(const char * directory, int * num);

void bg_db_add_files(bg_db_t * db, bg_db_file_t * file, int num, bg_db_dir_t * dir);
void bg_db_update_files(bg_db_t * db, bg_db_file_t * file, int num, bg_db_dir_t * dir);
