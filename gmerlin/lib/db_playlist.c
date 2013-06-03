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
#include <gmerlin/tree.h>

#include <string.h>

#define LOG_DOMAIN "db.playlist"

static void get_children_playlist(bg_db_t * db, void * obj, bg_sqlite_id_tab_t * tab)
  {
  char * sql;
  int result;
  sql = sqlite3_mprintf("SELECT ITEM_ID FROM PLAYLISTS WHERE PLAYLIST_ID = %"PRId64" ORDER BY IDX;",
                        bg_db_object_get_id(obj));
  result = bg_sqlite_exec(db->db, sql, bg_sqlite_append_id_callback, tab);
  sqlite3_free(sql);
  }

static void del_playlist(bg_db_t * db, bg_db_object_t * obj) // Delete from db
  {
  char * sql;
  int result;
  sql = sqlite3_mprintf("DELETE FROM PLAYLISTS WHERE PLAYLIST_ID = %"PRId64";",
                        bg_db_object_get_id(obj));
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  }

const bg_db_object_class_t bg_db_playlist_class =
  {
    .name = "Playlist",
    .del = del_playlist,
    .get_children = get_children_playlist,
    .parent = NULL, // Object
  };

static int compare_playlist_by_name(const bg_db_object_t * obj, const void * data)
  {
  if((obj->type == BG_DB_OBJECT_PLAYLIST) && !strcmp(obj->label, data))
    return 1;
  return 0;
  } 

int64_t bg_db_playlist_by_name(bg_db_t * db, const char * name)
  {
  int64_t ret = -1;
  char * buf;
  int result;

  ret = bg_db_cache_search(db, compare_playlist_by_name, name);
  if(ret > 0)
    return ret;
  
  buf = sqlite3_mprintf("select ID from OBJECTS where (TYPE = %d) & (LABEL = %Q);",
                        BG_DB_OBJECT_PLAYLIST, name);
  result = bg_sqlite_exec(db->db, buf, bg_sqlite_int_callback, &ret);
  sqlite3_free(buf);
  return ret;
  }

static bg_db_object_t * playlist_by_name(bg_db_t * db, char * name)
  {
  bg_db_object_t * playlist;
  int64_t id;

  if((id = bg_db_playlist_by_name(db, name)) <= 0)
    {
    /* Create playlist */
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Creating playlist %s", name);
    
    playlist = bg_db_object_create(db);
    bg_db_object_set_type(playlist, BG_DB_OBJECT_PLAYLIST);
    bg_db_object_set_label_nocpy(playlist, name);
    bg_db_object_set_parent_id(db, playlist, -1);
    bg_db_create_vfolders(db, playlist);
    }
  else
    {
    playlist = bg_db_object_query(db, id);
    
    /* Clear playlist */
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Clearing existing playlist %s", name);
    del_playlist(db, playlist);
    playlist->children = 0;
    free(name);
    }
  return playlist;  
  }

void bg_db_add_album(bg_db_t * db, const char * album_file)
  {
  bg_album_entry_t * entries;
  bg_album_entry_t * e;
  char * album_xml;
  char * name;
  char * sql;
  int result;
  bg_db_object_t * playlist;
  int64_t file_id;
  int64_t id;
  
  album_xml = bg_read_file(album_file, NULL);
  if(!album_xml)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s could not be opened", album_file);
    return;
    }

  entries = bg_album_entries_new_from_xml(album_xml);
  free(album_xml);

  if(!entries)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s contains no entries",
           album_file);
    return;
    }

  name = bg_db_path_to_label(album_file);

  /* Check if a playlist of that name already exists */

  playlist = playlist_by_name(db, name);
  
  /* Add tracks */
  e = entries;
  
  while(e)
    {
    if(*(char*)(e->location) == '/') // Absolute path
      {
      if(strncmp(e->location, db->base_dir, db->base_len))
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "File %s outside of db tree",
               (char*)e->location);
        e = e->next;
        continue;
        }
      if((file_id = bg_db_file_by_path(db, e->location)) < 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "File %s not found in database",
               (char*)e->location);
        e = e->next;
        continue;
        }
      /* Add file */
      id = bg_sqlite_get_next_id(db->db, "PLAYLISTS");

      playlist->children++;

      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Adding %s to playlist %s",
             bg_db_filename_to_rel(db, e->location), playlist->label);
      
      sql = sqlite3_mprintf("INSERT INTO PLAYLISTS ( ID, PLAYLIST_ID, ITEM_ID, IDX ) VALUES "
                            "( %"PRId64", %"PRId64", %"PRId64", %d );",
                            id, playlist->id, file_id, playlist->children);
      result = bg_sqlite_exec(db->db, sql, NULL, NULL);
      sqlite3_free(sql);
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Unsupported location %s",
             (char*)e->location);
      }
    e = e->next;
    }

  bg_db_object_unref(playlist);
  bg_album_entries_destroy(entries);
  }
