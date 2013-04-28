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
#include <string.h>


#define LOG_DOMAIN "db.object"

    /*
  int64_t id;
  bg_db_object_type_t type;
  
  int64_t ref_id;
  int64_t parent_id;
  int num_children;  // -1 for non-containers

  int64_t size;
  gavl_time_t duration;
  char * label;
    */

/* Caching mechanisms */

void bg_db_object_init(void * obj1)
  {
  bg_db_object_storage_t * obj = obj1;
  memset(obj, 0, sizeof(*obj));
  obj->obj.id   = -1;
  }

static bg_db_object_t * get_cache_item(bg_db_t * db)
  {
  int i;
  int ret_index = -1;
  bg_db_object_t * obj;
  
  /* Check for empty items */
  for(i = 0; i < db->cache_size; i++)
    {
    if(!bg_db_object_is_valid(&db->cache[i].obj))
      {
      ret_index = i;
      break;
      }
    }
  
  /* Check for non-referenced non-container items (containers are more
     likely to be reused) */
  
  if(ret_index < 0)
    {
    for(i = 0; i < db->cache_size; i++)
      {
      if(db->cache[i].refcount)
        continue;

      obj = (bg_db_object_t *)&db->cache[i];
      if(!(obj->type & BG_DB_FLAG_CONTAINER))
        {
        ret_index = i;
        break;
        }
      }
    }
  
  /* Check for non-referenced items */
  if(ret_index < 0)
    {
    for(i = 0; i < db->cache_size; i++)
      {
      if(!db->cache[i].refcount)
        {
        ret_index = i;
        break;
        }
      }
    }

  if(ret_index < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cache full");
    return NULL;
    }

  if(bg_db_object_is_valid(&db->cache[i].obj))
    {
    /* Check if we need to update the database */
    if(memcmp(&db->cache[i].obj, &db->cache[i].orig,
              sizeof(db->cache[i].obj)))
      {
      bg_db_object_update(db, &db->cache[i].obj, 1, 1);
      }
    bg_db_object_free(&db->cache[i].obj);
    bg_db_object_init(&db->cache[i].obj);
    }
  
  db->cache[i].refcount = 1;
  return (bg_db_object_t *)&db->cache[i].obj;
  }

void
bg_db_object_unref(void * obj)
  {
  bg_db_cache_t * s = (bg_db_cache_t *)obj;
  s->refcount--;
  }


const bg_db_object_class_t * bg_db_object_get_class(bg_db_object_type_t t)
  {
  switch(t)
    {
    case BG_DB_OBJECT_OBJECT:
      return NULL;
      break;
    case BG_DB_OBJECT_FILE:
      return &bg_db_file_class;
      break;
    case BG_DB_OBJECT_AUDIO_FILE:
      return &bg_db_audio_file_class;
      break;
    case BG_DB_OBJECT_VIDEO_FILE:
      break;
    case BG_DB_OBJECT_PHOTO_FILE:
      break;
    case BG_DB_OBJECT_CONTAINER:
      break;
    case BG_DB_OBJECT_DIRECTORY: 
      return &bg_db_dir_class;
      break;
    case BG_DB_OBJECT_PLAYLIST:
      break;
    case BG_DB_OBJECT_VFOLDER:
      break;
    case BG_DB_OBJECT_VFOLDER_LEAF:
      break;
    case BG_DB_OBJECT_AUDIO_ALBUM:
      return &bg_db_audio_album_class;
      break;
    }
  return NULL;
  }

void bg_db_object_update_size(bg_db_t * db, void * obj, int64_t delta_size)
  {
  bg_db_object_t * o = obj;
  o->size += delta_size;

  //  fprintf(stderr, "Update size %ld %ld: %ld -> %ld\n", o->id, o->parent_id, delta_size, o->size);
  if(o->parent_id > 0)
    {
    bg_db_object_t * parent = bg_db_object_query(db, o->parent_id);
    bg_db_object_update_size(db, parent, delta_size);
    bg_db_object_unref(parent);
    }
  }

void bg_db_object_update_duration(bg_db_t * db, void * obj, gavl_time_t delta_d)
  {
  bg_db_object_t * o = obj;
  if(delta_d == GAVL_TIME_UNDEFINED)
    return;
    
  o->duration += delta_d;
  if(o->parent_id > 0)
    {
    bg_db_object_t * parent = bg_db_object_query(db, o->parent_id);
    bg_db_object_update_duration(db, parent, delta_d);
    bg_db_object_unref(parent);
    }
  }

int64_t bg_db_object_get_id(void * obj)
  {
  bg_db_object_t * o = obj;
  return o->id;
  }

int bg_db_object_is_valid(void * obj)
  {
  bg_db_object_t * o = obj;
  return (o->id > 0);
  }

void bg_db_object_create_ref(void * obj1, void * parent1)
  {
  bg_db_object_t * obj = obj1;
  //  bg_db_object_t * parent = parent1;
  
  /* Creating a reference to a reference will reference the original */
  if(!obj->ref_id)
    obj->ref_id = obj->id;
  }


int64_t bg_db_object_get_parent(void * obj)
  {
  bg_db_object_t * o = obj;
  return o->parent_id;
  }

void bg_db_object_set_type(void * obj, bg_db_object_type_t t)
  {
  bg_db_object_t * o = obj;
  const bg_db_object_class_t * c = bg_db_object_get_class(t);
  if(c->parent != o->klass)
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "bg_object_set_type failed");
  o->klass = c;
  o->type = t;
  }

void * bg_db_object_create(bg_db_t * db)
  {
  int result;
  char * sql;
  
  bg_db_object_t * obj = get_cache_item(db);
  
  obj->id = bg_sqlite_get_next_id(db->db, "OBJECTS");
  
  sql = sqlite3_mprintf("INSERT INTO OBJECTS "
                        "( ID ) VALUES ( %"PRId64" );",
                        obj->id);
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  return obj;
  }

static int
object_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_object_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_INT("ID",          id);
    BG_DB_SET_QUERY_INT("TYPE",        type);
    BG_DB_SET_QUERY_INT("REF_ID",      ref_id);
    BG_DB_SET_QUERY_INT("PARENT_ID",   parent_id);
    BG_DB_SET_QUERY_INT("CHILDREN",    children);
    BG_DB_SET_QUERY_INT("SIZE",        size);
    BG_DB_SET_QUERY_INT("DURATION",    duration);
    BG_DB_SET_QUERY_STRING("LABEL",    label);
    }
  ret->found = 1;
  return 0;
  }

/* Query from DB  */
void * bg_db_object_query(bg_db_t * db, int64_t id) 
  {
  int i;
  char * sql;
  int result;
  const bg_db_object_class_t * c;
  
  bg_db_cache_t * cache;
  bg_db_object_t * obj;

  /* Check if the object is in the cache */
  for(i = 0; i < db->cache_size; i++)
    {
    if(bg_db_object_get_id(&db->cache[i]) == id)
      {
      db->cache[i].refcount++;
      return &db->cache[i];
      }
    }

  obj = get_cache_item(db);
  
  obj->found = 0;
  sql = sqlite3_mprintf("select * from OBJECTS where ID = %"PRId64";",
                        id);
  result = bg_sqlite_exec(db->db, sql, object_query_callback, obj);
  sqlite3_free(sql);
  if(!result || !obj->found)
    return 0;
  
  obj->klass = bg_db_object_get_class(obj->type);

  
  /* Children */
  c = obj->klass;
  while(c)
    {
    if(c->query && !c->query(db, obj, 1))
      return 0;
    c = c->parent;
    }

  /* Remember original state */

  cache = (bg_db_cache_t*)obj;
  memcpy(&cache->orig, &cache->obj, sizeof(cache->obj));
  
  return obj;
  }

void bg_db_object_free(void * obj1)
  {
  bg_db_object_t * obj = obj1;
  const bg_db_object_class_t * c = obj->klass;
  
  if(obj->label)
    free(obj->label);

  while(c)
    {
    if(c->free)
      c->free(obj);
    c = c->parent;
    }
  obj->id = -1;
  }

bg_db_object_type_t bg_db_object_get_type(void * obj1)
  {
  bg_db_object_t * obj = obj1;
  return obj->type;
  }


void bg_db_object_update(bg_db_t * db, void * obj1, int children, int parent)
  {
  char * sql;
  int result;
  const bg_db_object_class_t * c;
  bg_db_object_t * obj = obj1;

  if(obj->id <= 0)
    return;

  //  fprintf(stderr, "Update Object %ld\n", obj->id);
  
  sql = sqlite3_mprintf("UPDATE OBJECTS SET TYPE = %d, REF_ID = %"PRId64", PARENT_ID = %"PRId64",  CHILDREN = %d, SIZE = %"PRId64", DURATION = %"PRId64", LABEL = %Q   WHERE ID = %"PRId64";",
                        obj->type, obj->ref_id, obj->parent_id, obj->children, obj->size,
                        obj->duration, obj->label, obj->id);
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  
  c = obj->klass;
  
  while(c)
    {
    if(c->update)
      c->update(db, obj);
    c = c->parent;
    }
  }

void bg_db_object_set_label_nocpy(void * obj1, char * label)
  {
  bg_db_object_t * obj = obj1;
  obj->label = label;
  }

void bg_db_object_set_label(void * obj1, const char * label)
  {
  bg_db_object_t * obj = obj1;
  obj->label = gavl_strdup(label);
  }

void bg_db_object_set_parent_id(bg_db_t * db, void * obj, int64_t parent_id)
  {
  bg_db_object_t * parent = bg_db_object_query(db, parent_id);
  bg_db_object_set_parent(db, obj, &parent);
  bg_db_object_unref(parent);
  }

void bg_db_object_add_child(bg_db_t * db, void * obj1, void * child1)
  {
  bg_db_object_t * obj = obj1;
  bg_db_object_t * child = child1;

  obj->children++;
  bg_db_object_update_size(db, obj, child->size);
  bg_db_object_update_duration(db, obj, child->duration);
  }

void bg_db_object_remove_child(bg_db_t * db, void * obj1, void * child1)
  {
  bg_db_object_t * obj = obj1;
  bg_db_object_t * child = child1;
  obj->children--;
  bg_db_object_update_size(db, obj, -child->size);
  bg_db_object_update_duration(db, obj, -child->duration);
  }

void bg_db_object_set_parent(bg_db_t * db, void * obj1, void * parent1)
  {
  bg_db_object_t * obj = obj1;
  bg_db_object_t * parent = parent1;
  obj->parent_id = parent->id;
  bg_db_object_add_child(db, parent1, obj1);
  }


static void delete_from_parent(bg_db_t * db, bg_db_object_t * obj)
  {
  bg_db_object_t * parent;
  
  if(obj->parent_id > 0)
    {
    parent = bg_db_object_query(db, obj->parent_id);

    bg_db_object_remove_child(db, parent, obj);
      
    if(!parent->children && (parent->type & BG_DB_FLAG_NO_EMPTY))
      bg_db_object_delete(db, parent);
    else
      bg_db_object_unref(parent);
    }
  }

void bg_db_object_delete(bg_db_t * db, void * obj1)
  {
  bg_sqlite_id_tab_t  tab;
  bg_db_object_storage_t child_s;
  bg_db_object_t * child;

  int i;
  char * sql;
  bg_db_object_t * obj = obj1;
  bg_db_cache_t * ci;

  child = (bg_db_object_t *)&child_s;

  ci = obj1;
  
  if(ci->refcount > 1)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot delete object, refcount > 1");
    return;
    }
  
  bg_sqlite_id_tab_init(&tab);
  
  /* Delete referenced objects and children */
  sql =
    sqlite3_mprintf("select ID from OBJECTS where (REF_ID = %"PRId64" | PARENT_ID = %"PRId64");",
                    obj->id, obj->id);
  
  for(i = 0; i < tab.num_val; i++)
    {
    child = bg_db_object_query(db, tab.val[i]);
    
    if(child->parent_id == obj->id)
      child->flags |= BG_DB_OBJ_FLAG_DONT_PROPAGATE;
    
    bg_db_object_delete(db, child);
    }

  /* Remove from parent container */
  if(!(obj->flags & BG_DB_OBJ_FLAG_DONT_PROPAGATE))
    delete_from_parent(db, obj);
  
  bg_sqlite_delete_by_id(db->db, "OBJECTS", obj->id);
  }

