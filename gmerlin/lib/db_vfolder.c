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
#include <unistd.h>

#include <mediadb_private.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#include <string.h>

#define LOG_DOMAIN "db.vfolder"

static const struct
  {
  const char * name;
  bg_db_object_type_t type;
  bg_db_category_t cats[BG_DB_VFOLDER_MAX_DEPTH + 1];
  }
vfolder_types[] =
  {
    {
      .type = BG_DB_OBJECT_AUDIO_ALBUM,
      .name = "By Genre - Artist",
      .cats = { BG_DB_CAT_GENRE,
                BG_DB_CAT_ARTIST,
              },
    },
    {
      .type = BG_DB_OBJECT_AUDIO_FILE,
      .name = "By Artist",
      .cats = { BG_DB_CAT_ARTIST },
    },
    { /* End */ }
  };

static int compare_vfolder_by_path(const bg_db_object_t * obj, const void * data)
  {
  const bg_db_vfolder_t * f1 = data;
  const bg_db_vfolder_t * f2;

  if((obj->type == BG_DB_OBJECT_VFOLDER) ||
     (obj->type == BG_DB_OBJECT_VFOLDER_LEAF))
    {
    f2 = (bg_db_vfolder_t*)obj;
    if(f1->depth != f2->depth)
      return 0;
    if(f1->type != f1->type)
      return 0;
    return !memcmp(f1->path, f2->path, BG_DB_VFOLDER_MAX_DEPTH * sizeof(f1->path[0]));
    }
  return 0;
  }

static int64_t
vfolder_by_path(bg_db_t * db, int type, int depth, 
                bg_db_vfolder_path_t * path)
  {
  int64_t id;
  char * sql;
  char tmp_string[64];
  int i;  

  bg_db_vfolder_t f;
  memset(&f, 0, sizeof(f));
  f.depth = depth;
  f.type = type;
  memcpy(f.path, path, BG_DB_VFOLDER_MAX_DEPTH * sizeof(path[0]));
	
  /* Look in cache */
  id = bg_db_cache_search(db, compare_vfolder_by_path, &f);
  if(id > 0)
    return id;

  /* Do a real lookup */
  id = -1;

  sql = bg_sprintf("SELECT ID FROM VFOLDERS WHERE (TYPE = %d) & (DEPTH = %d)", type, depth);
  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    snprintf(tmp_string, 64, "& (CAT_%d = %d) & (VAL_%d =  %"PRId64")",
             i+1, path[i].cat, i+1, path[i].val);
    sql = gavl_strcat(sql, tmp_string);
    }
  sql = gavl_strcat(sql, ");");
  bg_sqlite_exec(db->db, sql, bg_sqlite_int_callback, &id);
  free(sql);
  return id;  
  }

static void 
vfolder_set(bg_db_t * db,
            bg_db_vfolder_t * f, int type, int depth, bg_db_vfolder_path_t * path)
  {
  char * sql;
  int i;
  char tmp_string[256];
  f->type = type;
  f->depth = depth;
  memcpy(f->path, path, BG_DB_VFOLDER_MAX_DEPTH * sizeof(*path));

  sql = gavl_strdup("INSERT INTO VFOLDERS ( ID, TYPE, DEPTH");
  
  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    snprintf(tmp_string, 256, ", CAT_%d, VAL_%d", i+1, i+1);
    sql = gavl_strcat(sql, tmp_string);
    }
  snprintf(tmp_string, 256, ") VALUES ( %"PRId64", %d, %d", 
           bg_db_object_get_id(f), f->type, f->depth);

  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    snprintf(tmp_string, 256, ", %d, %"PRId64"", f->path[i].cat, f->path[i].val);
    sql = gavl_strcat(sql, tmp_string);
    }
  sql = gavl_strcat(sql, ");");
  bg_sqlite_exec(db->db, sql, NULL, NULL);
  free(sql);
  }

static void *
get_root_vfolder(bg_db_t * db, int idx)
  {
  bg_db_vfolder_t * parent;
  bg_db_vfolder_t * child;
  int64_t id;
  int type;
  int depth;
  int i;
  bg_db_vfolder_path_t path[BG_DB_VFOLDER_MAX_DEPTH];

  /* Check if the root folder is already there */
  memset(path, 0, sizeof(path));
  type = 0;
  depth = 0;
  
  id = vfolder_by_path(db, type, depth, path);
  if(id < 0)
    {
    parent = bg_db_object_create(db);
    bg_db_object_set_type(parent, BG_DB_OBJECT_VFOLDER);
    bg_db_object_set_label(parent, "Media");
    vfolder_set(db, parent, type, depth, path);
    }
  else
    parent = bg_db_object_query(db, id);
  
  type = vfolder_types[idx].type;
 
  id = vfolder_by_path(db, type, depth, path);
  if(id < 0)
    {
    child = bg_db_object_create(db);
    bg_db_object_set_type(child, BG_DB_OBJECT_VFOLDER);

    switch(type)
      {
      case BG_DB_OBJECT_AUDIO_FILE:
        bg_db_object_set_label(parent, "Songs");
        break;
      case BG_DB_OBJECT_AUDIO_ALBUM:
        bg_db_object_set_label(parent, "Music albums");
        break;
      }
    vfolder_set(db, child, type, depth, path);
    bg_db_object_set_parent(db, child, parent);
    }
  else
    child = bg_db_object_query(db, id);

  bg_db_object_unref(parent);
  parent = child;
  child = NULL;

  i = 0;
  while(vfolder_types[idx].cats[i])
    {
    path[i].cat = vfolder_types[idx].cats[i];
    i++;
    }
  
  id = vfolder_by_path(db, type, depth, path);
  if(id < 0)
    {
    child = bg_db_object_create(db);
    bg_db_object_set_type(child, BG_DB_OBJECT_VFOLDER);
    bg_db_object_set_label(parent, vfolder_types[idx].name);
    vfolder_set(db, child, type, depth, path);
    bg_db_object_set_parent(db, child, parent);
    }
  else
    child = bg_db_object_query(db, id);

  bg_db_object_unref(parent);
  return child;
  }

static void create_vfolders_audio_file(bg_db_t * db, bg_db_object_t * obj)
  {
  int i = 0;
  int j;
  int num_cats;
  int type;
  int depth;
  int64_t id;
  char * name;
  bg_db_vfolder_path_t path[BG_DB_VFOLDER_MAX_DEPTH];
  bg_db_audio_file_t * f = (bg_db_audio_file_t *)obj;

  bg_db_object_t * parent, * child;  

  while(vfolder_types[i].type)
    {
    if(vfolder_types[i].type != BG_DB_OBJECT_AUDIO_FILE)
      {
      i++;
      continue;
      }
 
    num_cats = 0;
    while(vfolder_types[i].cats[num_cats])
      num_cats++;

    parent = get_root_vfolder(db, i);

    type = BG_DB_OBJECT_AUDIO_FILE;
    memset(path, 0, sizeof(path));    
    depth = 0;

    for(j = 0; j < num_cats; j++)
      path[j].cat = vfolder_types[i].cats[j];

    for(j = 0; j < num_cats; j++)
      {
      switch(vfolder_types[i].cats[j])
        {
        case BG_DB_CAT_YEAR:
          path[j].val = f->date.year;
          id = vfolder_by_path(db, BG_DB_OBJECT_AUDIO_FILE, j+1, path);
          if(id < 0)
            {
            if(f->date.year == 9999)
              name = gavl_strdup("Unknown");
            else
              name = bg_sprintf("%d", f->date.year);
            }
          break;
        case BG_DB_CAT_ARTIST:
          path[j].val = f->artist_id;
          id = vfolder_by_path(db, BG_DB_OBJECT_AUDIO_FILE, j+1, path);
          if(id < 0)
            {
            if(f->artist_id < 0)
              name = gavl_strdup("Unknown");
            else
              name = gavl_strdup(f->artist);
            }
          break;
        case BG_DB_CAT_GENRE:
          path[j].val = f->genre_id;
          id = vfolder_by_path(db, BG_DB_OBJECT_AUDIO_FILE, j+1, path);
          if(id < 0)
            {
            if(f->genre_id < 0)
              name = gavl_strdup("Unknown");
            else
              name = gavl_strdup(f->genre);
            }
          break;
        }

      if(id < 0)
        {
        child = bg_db_object_create(db);
        bg_db_object_set_label_nocpy(child, name);
 
        if(j < num_cats - 1)
          bg_db_object_set_type(child, BG_DB_OBJECT_VFOLDER);
        else
          bg_db_object_set_type(child, BG_DB_OBJECT_VFOLDER_LEAF);      
        bg_db_object_set_parent(db, child, parent);
        }
      else
        child = bg_db_object_query(db, id);
      
      }
    i++;
    }
  
  }

static void create_vfolders_audio_album(bg_db_t * db, bg_db_object_t * obj)
  {
  
  }

void
bg_db_create_tables_vfolders(bg_db_t * db)
  {
  int i;
  char * str = gavl_strdup("CREATE TABLE VFOLDERS ( ID INTEGER PRIMARY KEY,"
                           "TYPE INTEGER, "
                           "DEPTH INTEGER" );
  char tmp_string[32];
  
  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    snprintf(tmp_string, 32, ", CAT_%d INTEGER", i+1);
    str = gavl_strcat(str, tmp_string);
    snprintf(tmp_string, 32, ", VAL_%d INTEGER", i+1);
    str = gavl_strcat(str, tmp_string);
    }

  str = gavl_strcat(str, ");");
  bg_sqlite_exec(db->db, str, NULL, NULL);
  free(str);
  }

void
bg_db_create_vfolders(bg_db_t * db, bg_db_object_t * obj)
  {
  bg_db_object_type_t type;

  type = bg_db_object_get_type(obj);

  switch(type)
    {
    case BG_DB_OBJECT_AUDIO_FILE:
      create_vfolders_audio_file(db, obj);
      break;
    case BG_DB_OBJECT_AUDIO_ALBUM:
      create_vfolders_audio_album(db, obj);
      break;
    default:
      break;
    }
  
  }

