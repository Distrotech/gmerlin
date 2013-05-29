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

#include <mediadb_private.h>
#include <gmerlin/utils.h>

#include <stdlib.h>
#include <string.h>

/*
  typedef struct
  {
  int64_t id;
  char * str;
  } bg_db_string_cache_item_t;

typedef struct
  {
  int size;
  int alloc;
  bg_db_string_cache_item_t * items;
  char * table;
  char * str_col;
  char * id_col;
  } bg_db_string_cache_t;
*/

bg_db_string_cache_t * bg_db_string_cache_create(int size,
                                                 const char * table,
                                                 const char * str_col,
                                                 const char * id_col)
  {
  bg_db_string_cache_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->alloc = size;
  ret->items = calloc(ret->alloc, sizeof(*ret->items));

  ret->table = gavl_strdup(table);
  ret->str_col = gavl_strdup(str_col);
  ret->id_col = gavl_strdup(id_col);
  
  return ret;
  }

void
bg_db_string_cache_destroy(bg_db_string_cache_t * c)
  {
  int i;
  for(i = 0; i < c->size; i++)
    {
    if(c->items[i].str)
      free(c->items[i].str);
    }
  free(c->items);
  free(c->table);
  free(c->str_col);
  free(c->id_col);
  free(c);
  }

char *
bg_db_string_cache_get(bg_db_string_cache_t * c, sqlite3 * db, int64_t id)
  {
  int i;
  for(i = 0; i < c->size; i++)
    {
    if(c->items[i].id == id)
      return gavl_strdup(c->items[i].str);
    }
  /* Kick out the last item */
  if(c->size == c->alloc)
    {
    free(c->items[c->size - 1].str);
    c->size--;
    }
  if(c->size)
    memmove(c->items + 1, c->items, sizeof(*c->items) * c->size);
  
  c->items[0].str = bg_sqlite_id_to_string(db, c->table, c->str_col, c->id_col, id);
  c->items[0].id = id;
  c->size++;
  return gavl_strdup(c->items[0].str);
  }
