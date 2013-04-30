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
      .name = "By Genre - Artist",
      .cats = { BG_DB_CAT_GENRE, BG_DB_CAT_ARTIST },
    },
    {
      .type = BG_DB_OBJECT_AUDIO_FILE,
      .name = "By Genre - Artist - Year",
      .cats = { BG_DB_CAT_GENRE, BG_DB_CAT_ARTIST },
    },
    { /* End */ }
  };

static void dump_vfolder(void * obj)
  {
  int i;
  bg_db_vfolder_t * f = obj;

  gavl_diprintf(2, "Type:    %d\n", f->type);
  gavl_diprintf(2, "Depth:   %d\n", f->depth);
  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    if(!f->path[i].cat)
      break;
    gavl_diprintf(4, "Cat %d: %"PRId64"\n", f->path[i].cat, f->path[i].val);
    }
  }

static void del_vfolder(bg_db_t * db, bg_db_object_t * obj) // Delete from db
  {
  bg_sqlite_delete_by_id(db->db, "VFOLDERS", obj->id);
  }

static int vfolder_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_vfolder_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_INT("TYPE",      type);
    BG_DB_SET_QUERY_INT("DEPTH",     depth);

    if(!strncmp(azColName[i], "CAT_", 4))
      ret->path[atoi(azColName[i] + 4)-1].cat = atoi(argv[i]);
    if(!strncmp(azColName[i], "VAL_", 4))
      ret->path[atoi(azColName[i] + 4)-1].val = strtoll(argv[i], NULL, 10);
    }
  ret->obj.found = 1;
  return 0;
  }

static int query_vfolder(bg_db_t * db, void * obj, int full)
  {
  char * sql;
  int result;
  bg_db_vfolder_t * f = obj;
  
  f->obj.found = 0;
  sql =
    sqlite3_mprintf("select * from VFOLDERS where ID = %"PRId64";", bg_db_object_get_id(f));
  result = bg_sqlite_exec(db->db, sql, vfolder_query_callback, f);
  sqlite3_free(sql);
  if(!result || !f->obj.found)
    return 0;
  return 1;
  }

const bg_db_object_class_t bg_db_vfolder_class =
  {
  .name = "Virtual folder",
  .del = del_vfolder,
  .query = query_vfolder,
  .dump = &dump_vfolder,
  .parent = NULL,
  };

const bg_db_object_class_t bg_db_vfolder_leaf_class =
  {
  .name = "Virtual folder (leaf)",
  .del = del_vfolder,
  .query = query_vfolder,
  .dump = &dump_vfolder,
  .parent = NULL,
  };

static int compare_vfolder_by_path(const bg_db_object_t * obj, const void * data)
  {
  const bg_db_vfolder_t * f1 = data;
  const bg_db_vfolder_t * f2;

  if((obj->type == BG_DB_OBJECT_VFOLDER) ||
     (obj->type == BG_DB_OBJECT_VFOLDER_LEAF))
    {
    f2 = (bg_db_vfolder_t*)obj;

    // fprintf(stderr, "Compare vfolder\n");
    // dump_vfolder(f1);
    // dump_vfolder(f2);
    
    if((f1->depth == f2->depth) &&
       (f1->type == f2->type) &&
       !memcmp(f1->path, f2->path, BG_DB_VFOLDER_MAX_DEPTH * sizeof(f1->path[0])))
      return 1;
    //    else
    //      fprintf(stderr, "Result: 0\n");
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
  sql = gavl_strcat(sql, ";");
  bg_sqlite_exec(db->db, sql, bg_sqlite_int_callback, &id);
  free(sql);
  return id;  
  }

static void 
vfolder_set(bg_db_t * db,
            bg_db_vfolder_t * f, int type, int depth,
            bg_db_vfolder_path_t * path)
  {
  char * sql;
  int i;
  char tmp_string[256];
  f->type = type;
  f->depth = depth;
  memcpy(&f->path[0], path, BG_DB_VFOLDER_MAX_DEPTH * sizeof(path[0]));

  sql = gavl_strdup("INSERT INTO VFOLDERS ( ID, TYPE, DEPTH");
  
  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    snprintf(tmp_string, 256, ", CAT_%d, VAL_%d", i+1, i+1);
    sql = gavl_strcat(sql, tmp_string);
    }
  snprintf(tmp_string, 256, ") VALUES ( %"PRId64", %d, %d", 
           bg_db_object_get_id(f), f->type, f->depth);
  sql = gavl_strcat(sql, tmp_string);

  for(i = 0; i < BG_DB_VFOLDER_MAX_DEPTH; i++)
    {
    snprintf(tmp_string, 256, ", %d, %"PRId64"", f->path[i].cat, f->path[i].val);
    sql = gavl_strcat(sql, tmp_string);
    }
  sql = gavl_strcat(sql, ");");
  bg_sqlite_exec(db->db, sql, NULL, NULL);
  free(sql);

  fprintf(stderr, "Created vfolder\n");
  bg_db_object_dump(f);
  
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

  /* Root Folder  */
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

  /* Type folder */
  
  type = vfolder_types[idx].type;
 
  id = vfolder_by_path(db, type, depth, path);
  if(id < 0)
    {
    child = bg_db_object_create(db);
    bg_db_object_set_type(child, BG_DB_OBJECT_VFOLDER);

    switch(type)
      {
      case BG_DB_OBJECT_AUDIO_FILE:
        bg_db_object_set_label(child, "Songs");
        break;
      case BG_DB_OBJECT_AUDIO_ALBUM:
        bg_db_object_set_label(child, "Music albums");
        break;
      }
    bg_db_object_set_parent(db, child, parent);
    vfolder_set(db, child, type, depth, path);
    }
  else
    child = bg_db_object_query(db, id);

  bg_db_object_unref(parent);
  parent = child;
  child = NULL;

  /* Folder for this specific path */
  
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
    bg_db_object_set_label(child, vfolder_types[idx].name);
    bg_db_object_set_parent(db, child, parent);
    vfolder_set(db, child, type, depth, path);
    }
  else
    child = bg_db_object_query(db, id);

  bg_db_object_unref(parent);
  return child;
  }

static const char * get_artist(void * obj, int64_t * id)
  {
  switch(bg_db_object_get_type(obj))
    {
    case BG_DB_OBJECT_AUDIO_FILE:
      {
      bg_db_audio_file_t * f = obj;
      *id = f->artist_id;
      return f->artist;
      }
      break;
    case BG_DB_OBJECT_AUDIO_ALBUM:
      {
      bg_db_audio_album_t * f = obj;
      *id = f->artist_id;
      return f->artist;
      }
    case BG_DB_OBJECT_OBJECT:
    case BG_DB_OBJECT_FILE:
    case BG_DB_OBJECT_VIDEO_FILE:
    case BG_DB_OBJECT_PHOTO_FILE:
    case BG_DB_OBJECT_CONTAINER:
    case BG_DB_OBJECT_DIRECTORY: 
    case BG_DB_OBJECT_PLAYLIST:
    case BG_DB_OBJECT_VFOLDER:
    case BG_DB_OBJECT_VFOLDER_LEAF:
      break;
    }
  *id = -1;
  return NULL;
  }

static const char * get_genre(void * obj, int64_t * id)
  {
  switch(bg_db_object_get_type(obj))
    {
    case BG_DB_OBJECT_AUDIO_FILE:
      {
      bg_db_audio_file_t * f = obj;
      *id = f->genre_id;
      return f->genre;
      }
      break;
    case BG_DB_OBJECT_AUDIO_ALBUM:
      {
      bg_db_audio_album_t * f = obj;
      *id = f->genre_id;
      return f->genre;
      }
    case BG_DB_OBJECT_OBJECT:
    case BG_DB_OBJECT_FILE:
    case BG_DB_OBJECT_VIDEO_FILE:
    case BG_DB_OBJECT_PHOTO_FILE:
    case BG_DB_OBJECT_CONTAINER:
    case BG_DB_OBJECT_DIRECTORY: 
    case BG_DB_OBJECT_PLAYLIST:
    case BG_DB_OBJECT_VFOLDER:
    case BG_DB_OBJECT_VFOLDER_LEAF:
      break;
    }
  *id = -1;
  return NULL;
  }

static int get_year(void * obj)
  {
  switch(bg_db_object_get_type(obj))
    {
    case BG_DB_OBJECT_AUDIO_FILE:
      {
      bg_db_audio_file_t * f = obj;
      return f->date.year;
      }
      break;
    case BG_DB_OBJECT_AUDIO_ALBUM:
      {
      bg_db_audio_album_t * f = obj;
      return f->date.year;
      }
    case BG_DB_OBJECT_OBJECT:
    case BG_DB_OBJECT_FILE:
    case BG_DB_OBJECT_VIDEO_FILE:
    case BG_DB_OBJECT_PHOTO_FILE:
    case BG_DB_OBJECT_CONTAINER:
    case BG_DB_OBJECT_DIRECTORY: 
    case BG_DB_OBJECT_PLAYLIST:
    case BG_DB_OBJECT_VFOLDER:
    case BG_DB_OBJECT_VFOLDER_LEAF:
      break;
    }
  return 0;
  }

void
bg_db_create_vfolders(bg_db_t * db, void * obj)
  {
  int i = 0;
  int j;
  int num_cats;
  int type;
  //  int depth;
  int64_t id;
  char * name;
  bg_db_vfolder_path_t path[BG_DB_VFOLDER_MAX_DEPTH];
  bg_db_object_t * parent, * child;  
  const char * str;
  
  type = bg_db_object_get_type(obj);
  
  while(vfolder_types[i].type)
    {
    if(vfolder_types[i].type != type)
      {
      i++;
      continue;
      }
 
    num_cats = 0;
    while(vfolder_types[i].cats[num_cats])
      num_cats++;

    parent = get_root_vfolder(db, i);

    memset(path, 0, sizeof(path));    
    //    depth = 0;
    
    for(j = 0; j < num_cats; j++)
      path[j].cat = vfolder_types[i].cats[j];

    for(j = 0; j < num_cats; j++)
      {
      switch(vfolder_types[i].cats[j])
        {
        case BG_DB_CAT_YEAR:
          path[j].val = get_year(obj);
          id = vfolder_by_path(db, BG_DB_OBJECT_AUDIO_FILE, j+1, path);
          if(id < 0)
            {
            if(path[j].val == 9999)
              name = gavl_strdup("Unknown");
            else
              name = bg_sprintf("%"PRId64, path[j].val);
            }
          break;
        case BG_DB_CAT_ARTIST:
          str = get_artist(obj, &path[j].val);
          id = vfolder_by_path(db, BG_DB_OBJECT_AUDIO_FILE, j+1, path);
          if(id < 0)
            {
            if(!str)
              name = gavl_strdup("Unknown");
            else
              name = gavl_strdup(str);
            }
          break;
        case BG_DB_CAT_GENRE:
          str = get_genre(obj, &path[j].val);
          id = vfolder_by_path(db, BG_DB_OBJECT_AUDIO_FILE, j+1, path);
          if(id < 0)
            {
            if(!str)
              name = gavl_strdup("Unknown");
            else
              name = gavl_strdup(str);
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
        vfolder_set(db, (bg_db_vfolder_t*)child, type, j+1, path);
        }
      else
        child = bg_db_object_query(db, id);

      bg_db_object_add_child(db, parent, child);
      bg_db_object_unref(parent);
      parent = child;
      child = NULL;
      }

    bg_db_object_add_child(db, parent, obj); 
    bg_db_object_unref(parent);
    i++;
    }
  
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

