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

#include <mediadb_private.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>

static void get_children_root(bg_db_t * db, void * obj, bg_sqlite_id_tab_t * tab)
  {
  char * sql;
  int result;
  sql = sqlite3_mprintf("SELECT ID FROM OBJECTS WHERE PARENT_ID = %"PRId64" ORDER BY TYPE DESC, LABEL;",
                        bg_db_object_get_id(obj));
  result = bg_sqlite_exec(db->db, sql, bg_sqlite_append_id_callback, tab);
  sqlite3_free(sql);
  }

const bg_db_object_class_t bg_db_root_class =
  {
    .name = "Root",
//    .del = del_audioalbum,
//    .free = free_audioalbum,
//    .query = query_audioalbum,
//    .update = update_audioalbum,
//    .dump = dump_audioalbum,
    .get_children = get_children_root,
    .parent = NULL, // Object
  };

