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

#define LOG_DOMAIN "db.audioalbum"

static int album_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_audio_album_t * ret = data;

  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_INT("ARTIST"   ,   artist_id);
    BG_DB_SET_QUERY_STRING("TITLE",    title);
    BG_DB_SET_QUERY_STRING("SEARCH_TITLE", search_title);
    BG_DB_SET_QUERY_INT("GENRE",       genre_id);
    BG_DB_SET_QUERY_DATE("DATE",       date);
    BG_DB_SET_QUERY_INT("COVER",       cover_id);
    }
  ret->obj.found = 1;
  return 0;
  }

static int query_audioalbum(bg_db_t * db, void * a1, int full)
  {
  char * sql;
  int result;
  bg_db_audio_album_t * a = a1;
  
  sql = sqlite3_mprintf("select * from AUDIO_ALBUMS where ID = %"PRId64";", bg_db_object_get_id(a));
  result = bg_sqlite_exec(db->db, sql, album_query_callback, a);
  sqlite3_free(sql);
  if(!result || !a->obj.found)
    return 0;

  a->artist = bg_sqlite_id_to_string(db->db,
                                     "AUDIO_ARTISTS", "NAME", "ID",
                                     a->artist_id);
  a->genre  = bg_sqlite_id_to_string(db->db,
                                     "AUDIO_GENRES",  "NAME", "ID",
                                     a->genre_id);

  return 1;
  }

static void get_children_audioalbum(bg_db_t * db, void * obj, bg_sqlite_id_tab_t * tab)
  {
  char * sql;
  int result;
  sql = sqlite3_mprintf("SELECT ID FROM AUDIO_FILES WHERE ALBUM = %"PRId64" ORDER BY TRACK;",
                        bg_db_object_get_id(obj));
  result = bg_sqlite_exec(db->db, sql, bg_sqlite_append_id_callback, tab);
  sqlite3_free(sql);
  }

static void update_audioalbum(bg_db_t * db, void * obj)
  {
  bg_db_audio_album_t * a = (bg_db_audio_album_t *)obj;
  char * sql;
  int result;
  char date_string[BG_DB_DATE_STRING_LEN];
  
  bg_db_date_to_string(&a->date, date_string);
  
  sql = sqlite3_mprintf("UPDATE AUDIO_ALBUMS SET DATE = %Q, GENRE = %"PRId64", COVER = %"PRId64" WHERE ID = %"PRId64";",
                        date_string, a->genre_id, a->cover_id, bg_db_object_get_id(a));
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  return;
  }


static void del_audioalbum(bg_db_t * db, bg_db_object_t * obj) // Delete from db
  {
  bg_sqlite_delete_by_id(db->db, "AUDIO_ALBUMS", obj->id);
  }

static void free_audioalbum(void * obj)
  {
  bg_db_audio_album_t*a = obj;
  if(a->artist)
    free(a->artist);
  if(a->title)
    free(a->title);
  if(a->search_title)
    free(a->search_title);
  if(a->genre)
    free(a->genre);

  }

static void dump_audioalbum(void * obj)
  {
  bg_db_audio_album_t*a = obj;
  gavl_diprintf(2, "Artist:   %s\n", a->artist);
  gavl_diprintf(2, "Title:    %s (%s)\n", a->title, a->search_title);
  gavl_diprintf(2, "Year:     %d\n", a->date.year);
  gavl_diprintf(2, "Genre:    %s\n", a->genre);
  gavl_diprintf(2, "Cover ID: %"PRId64"\n", a->cover_id);
  }

const bg_db_object_class_t bg_db_audio_album_class =
  {
    .name = "Audio album",
    .del = del_audioalbum,
    .free = free_audioalbum,
    .query = query_audioalbum,
    .update = update_audioalbum,
    .dump = dump_audioalbum,
    .get_children = get_children_audioalbum,
    .parent = NULL, // Object
  };

static int compare_album_by_file(const bg_db_object_t * obj, const void * data)
  {
  const bg_db_audio_file_t * file;
  const bg_db_audio_album_t * album;

  if(obj->type == BG_DB_OBJECT_AUDIO_ALBUM)
    {
    file = (const bg_db_audio_file_t*)data;
    album = (const bg_db_audio_album_t*)obj;
    
    if(!strcmp(file->album, album->title) &&
       (album->artist_id == file->albumartist_id))
      return 1;
    }
  return 0;
  } 

  /* Check if the album already exists */

static int64_t find_by_file(bg_db_t * db, bg_db_audio_file_t * f)
  {
  int64_t ret;
  char * sql;
  int result;

  ret = bg_db_cache_search(db, compare_album_by_file, f);
  if(ret > 0)
    return ret;

  ret = -1;
  sql = sqlite3_mprintf("select ID from AUDIO_ALBUMS where ((TITLE = %Q) & (ARTIST = %"PRId64"));",
                        f->album, f->albumartist_id);
  result = bg_sqlite_exec(db->db, sql, bg_sqlite_int_callback, &ret);
  sqlite3_free(sql);

  if(!result)
    return -1; // Should never happen

  return ret;
  
  }

void bg_db_audio_file_add_to_album(bg_db_t * db, bg_db_audio_file_t * f)
  {
  int64_t id;
  bg_db_audio_album_t * a;
  bg_db_object_t * o;
  
  if((id = find_by_file(db, f)) > 0)
    {
    /* Add to album */
    a = bg_db_object_query(db, id);
    o = (bg_db_object_t *)a;
    bg_db_object_add_child(db, a, f);

    /* Clear some fields if they changed */

    /* If there is a date set, we check if it's the same for the added track */
    if(!bg_db_date_equal(&a->date, &f->date))
      bg_db_date_set_invalid(&a->date);

    /* If there is a genre set, we check if it's the same for the added track */
    if(a->genre_id != f->genre_id)
      a->genre_id = 0;
    f->album_id = bg_db_object_get_id(a);
    bg_db_object_unref(a);
    }
  else
    {
    char * sql;
    int result;
    char date_string[BG_DB_DATE_STRING_LEN];
    
    /* Create album */
    a = bg_db_object_create(db);
    bg_db_object_set_type(a, BG_DB_OBJECT_AUDIO_ALBUM);
    bg_db_object_set_parent_id(db, a, -1);
    a->genre_id = f->genre_id;
    
    /* Remove title completely? */
    a->title = gavl_strdup(f->album);
    a->search_title = gavl_strdup(bg_db_get_search_string(a->title));
    
    bg_db_object_set_label(a, f->album);
    bg_db_date_copy(&a->date, &f->date);
    a->artist_id = f->albumartist_id;

    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Adding audio album %s", a->title);

    
    bg_db_date_to_string(&a->date, date_string);
    
    sql = sqlite3_mprintf("INSERT INTO AUDIO_ALBUMS ( ID, ARTIST, TITLE, SEARCH_TITLE, GENRE, DATE ) VALUES ( %"PRId64", %"PRId64", %Q, %Q, %"PRId64", %Q);",
                          bg_db_object_get_id(a), a->artist_id, a->title,
                          a->search_title, a->genre_id, date_string);
    result = bg_sqlite_exec(db->db, sql, NULL, NULL);
    sqlite3_free(sql);

    f->album_id = bg_db_object_get_id(a);
    
    bg_db_object_add_child(db, a, f);
    bg_db_create_vfolders(db, a);
    bg_db_object_unref(a);
    }
  }

void bg_db_audio_file_remove_from_album(bg_db_t * db, bg_db_audio_file_t * f)
  {
  bg_db_audio_album_t * a;
  a = bg_db_object_query(db, f->album_id);

  if(!a)
    return; // Shouldn't happen

  bg_db_object_remove_child(db, a, f);

  if(!a->obj.children)
    bg_db_object_delete(db, a);
  else
    bg_db_object_unref(a);
  
  }

