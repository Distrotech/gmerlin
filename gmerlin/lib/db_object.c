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

static void object_delete(bg_db_t * db, void * obj1, int64_t child_id);


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
  obj->obj.id = -1;
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
  bg_db_obj_cache_t * s = (bg_db_obj_cache_t *)obj;
  s->refcount--;
  }

void
bg_db_object_ref(void * obj)
  {
  bg_db_obj_cache_t * s = (bg_db_obj_cache_t *)obj;
  s->refcount++;
  }

void bg_db_object_dump(void * obj1)
  {
  const bg_db_object_class_t * c;
  bg_db_object_t * obj = obj1;
  gavl_diprintf(0, "Object\n");
  gavl_diprintf(2, "Type:     %d (%s)\n", obj->type, (obj->klass ? obj->klass->name : "Unknown"));
  gavl_diprintf(2, "ID:       %"PRId64"\n", obj->id);
  gavl_diprintf(2, "REF_ID:   %"PRId64"\n", obj->ref_id);
  gavl_diprintf(2, "Parent:   %"PRId64"\n", obj->parent_id);
  gavl_diprintf(2, "Children: %d\n", obj->children);
  gavl_diprintf(2, "Size:     %"PRId64"\n", obj->size);
  gavl_diprintf(2, "Duration: %"PRId64"\n", obj->duration);
  gavl_diprintf(2, "Label:    %s\n", obj->label);
  
  /* Children */
  c = obj->klass;
  while(c)
    {
    if(c->dump)
      c->dump(obj);
    c = c->parent;
    }
  }

const bg_db_object_class_t * bg_db_object_get_class(bg_db_object_type_t t)
  {
  switch(t)
    {
    case BG_DB_OBJECT_OBJECT:
      return NULL;
      break;
    case BG_DB_OBJECT_ROOT:
      return &bg_db_root_class;
      break;
    case BG_DB_OBJECT_FILE:
      return &bg_db_file_class;
      break;
    case BG_DB_OBJECT_AUDIO_FILE:
      return &bg_db_audio_file_class;
      break;
    case BG_DB_OBJECT_VIDEO_FILE:
      break;
    case BG_DB_OBJECT_IMAGE_FILE:
      return &bg_db_image_file_class;
      break;
    case BG_DB_OBJECT_PHOTO:
      break;
    case BG_DB_OBJECT_THUMBNAIL:
      return &bg_db_thumbnail_class;
      //      return &bg_db_photo_file_class;
      break;
  
    case BG_DB_OBJECT_ALBUM_COVER:
      return &bg_db_album_cover_class;
      break;
    case BG_DB_OBJECT_VIDEO_PREVIEW:
      break;
    case BG_DB_OBJECT_MOVIE_POSTER:
      break;
    case BG_DB_OBJECT_CONTAINER:
      break;
    case BG_DB_OBJECT_DIRECTORY: 
      return &bg_db_dir_class;
      break;
    case BG_DB_OBJECT_PLAYLIST:
      return &bg_db_playlist_class;
      break;
    case BG_DB_OBJECT_VFOLDER:
      return &bg_db_vfolder_class;
      break;
    case BG_DB_OBJECT_VFOLDER_LEAF:
      return &bg_db_vfolder_leaf_class;
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
  if(o->parent_id >= 0)
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
  if(o->parent_id >= 0)
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
  return (o->id >= 0);
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
  if((c->parent != o->klass) && (o->klass->parent != c))
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "bg_object_set_type failed");
  o->klass = c;
  o->type = t;
  }

static void * object_create(bg_db_t * db, int root)
  {
  int result;
  char * sql;

  bg_db_object_t * obj = get_cache_item(db);

  if(root)
    obj->id = 0;
  else
    obj->id = bg_sqlite_get_next_id(db->db, "OBJECTS");

  obj->parent_id = -1;
  
  sql = sqlite3_mprintf("INSERT INTO OBJECTS "
                        "( ID ) VALUES ( %"PRId64" );",
                        obj->id);
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  db->num_added++;
  return obj;
  }

void * bg_db_object_create(bg_db_t * db)
  {
  return object_create(db, 0);
  }

void * bg_db_object_create_root(bg_db_t * db)
  {
  return object_create(db, 1);
  }

#if 0
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
#endif

#define TYPE_COL      1
#define REF_ID_COL    2
#define PARENT_ID_COL 3
#define CHILDREN_COL  4
#define SIZE_COL      5
#define DURATION_COL  6
#define LABEL_COL     7

/* Query from DB  */
void * bg_db_object_query(bg_db_t * db, int64_t id) 
  {
  int i;
  int result;
  int found = 0;
  const bg_db_object_class_t * c;
  
  bg_db_obj_cache_t * cache;
  bg_db_object_t * obj;
  sqlite3_stmt * st = db->q_objects;

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
  
  sqlite3_bind_int64(st, 1, id);
  
  if((result = sqlite3_step(st)) == SQLITE_ROW)
    {
    obj->id = id;
    BG_DB_GET_COL_INT(TYPE_COL, obj->type);
    BG_DB_GET_COL_INT(REF_ID_COL, obj->ref_id);
    BG_DB_GET_COL_INT(PARENT_ID_COL, obj->parent_id);
    BG_DB_GET_COL_INT(CHILDREN_COL, obj->children);
    BG_DB_GET_COL_INT(SIZE_COL, obj->size);
    BG_DB_GET_COL_INT(DURATION_COL, obj->duration);
    BG_DB_GET_COL_STRING(LABEL_COL, obj->label);
    found = 1;
    }

  sqlite3_reset(st);
  sqlite3_clear_bindings(st);
  //  sqlite3_free(sql);

  if(!found)
    return NULL;
  
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

  cache = (bg_db_obj_cache_t*)obj;
  memcpy(&cache->orig, &cache->obj, sizeof(cache->obj));
  
  return obj;
  }

void bg_db_object_free(void * obj1)
  {
  bg_db_object_t * obj = obj1;
  const bg_db_object_class_t * c = obj->klass;
  //  fprintf(stderr, "bg_db_object_free\n");
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

  if(obj->id < 0)
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

const char * bg_db_object_get_label(void * obj1)
  {
  bg_db_object_t * obj = obj1;
  return obj->label;
  }

void bg_db_object_set_parent_id(bg_db_t * db, void * obj1, int64_t parent_id)
  {
  if(parent_id < 0)
    {
    bg_db_object_t * obj = obj1;
    obj->parent_id = parent_id;
    }
  else
    {
    bg_db_object_t * parent = bg_db_object_query(db, parent_id);
    bg_db_object_set_parent(db, obj1, parent);
    bg_db_object_unref(parent);
    }
  }

void bg_db_object_add_child(bg_db_t * db, void * obj1, void * child1)
  {
  bg_db_object_t * obj = obj1;
  bg_db_object_t * child = child1;

  obj->children++;

  if(obj->type & BG_DB_FLAG_CONTAINER)
    {
    bg_db_object_update_size(db, obj, child->size);

    if(child->duration != GAVL_TIME_UNDEFINED)
      bg_db_object_update_duration(db, obj, child->duration);
    }
  }

void bg_db_object_remove_child(bg_db_t * db, void * obj1, void * child1)
  {
  bg_db_object_t * obj = obj1;
  bg_db_object_t * child = child1;
  obj->children--;

  if(obj->type & BG_DB_FLAG_CONTAINER)
    {
    bg_db_object_update_size(db, obj, -child->size);
    
    if(child->duration != GAVL_TIME_UNDEFINED)
      bg_db_object_update_duration(db, obj, -child->duration);
    }
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
  
  if(obj->parent_id >= 0)
    {
    parent = bg_db_object_query(db, obj->parent_id);

    if(!parent)
      {
      fprintf(stderr, "BUG: Orphaned object\n");
      bg_db_object_dump(obj);
      return;
      }
    
    bg_db_object_remove_child(db, parent, obj);
      
    if(!parent->children && (parent->type & BG_DB_FLAG_NO_EMPTY))
      {
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Deleting empty container %s",
             parent->label);
      object_delete(db, parent, obj->id);
      }
    else
      bg_db_object_unref(parent);
    }
  }

/* child_id if the ID of the child which triggered the delete */
static void object_delete(bg_db_t * db, void * obj1, int64_t child_id)
  {
  const bg_db_object_class_t * c;
  bg_sqlite_id_tab_t  tab;
  bg_db_object_t * child;

  int i;
  int result;
  char * sql;
  bg_db_object_t * obj = obj1;
  bg_db_obj_cache_t * ci;

  ci = obj1;
  
  if(ci->refcount > 1)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot delete object, refcount = %d", ci->refcount);
    bg_db_object_dump(obj);
    return;
    }
  
  bg_sqlite_id_tab_init(&tab);
  
  /* Delete referenced objects and children */
  sql =
    sqlite3_mprintf("select ID from OBJECTS where (REF_ID = %"PRId64") |"
                    " (PARENT_ID = %"PRId64");", obj->id, obj->id);

  result = bg_sqlite_exec(db->db, sql, bg_sqlite_append_id_callback, &tab);
  sqlite3_free(sql);
  
  for(i = 0; i < tab.num_val; i++)
    {
    if(tab.val[i] == child_id)
      continue;
    
    child = bg_db_object_query(db, tab.val[i]);
    
    if(child->parent_id == obj->id)
      child->flags |= BG_DB_OBJ_FLAG_DONT_PROPAGATE;
    
    bg_db_object_delete(db, child);
    }

  /* Remove from parent container */
  if(!(obj->flags & BG_DB_OBJ_FLAG_DONT_PROPAGATE))
    delete_from_parent(db, obj);
  
  bg_sqlite_delete_by_id(db->db, "OBJECTS", obj->id);

  /* Children */
  c = obj->klass;
  while(c)
    {
    if(c->del)
      c->del(db, obj);
    c = c->parent;
    }
  db->num_deleted++;

  bg_db_object_free(obj);
  bg_db_object_init(obj);
  ci->refcount = 0;
  }


void bg_db_object_delete(bg_db_t * db, void * obj1)
  {
  object_delete(db, obj1, -1);
  }

