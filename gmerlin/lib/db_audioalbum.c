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

#define LOG_DOMAIN "db.audiofile"

static int album_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_audio_album_t * ret = data;

  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_INT("ARTIST"   ,   artist_id);
    BG_DB_SET_QUERY_STRING("TITLE",    title);
    BG_DB_SET_QUERY_INT("GENRE",       genre_id);
    }
  ret->obj.found = 1;
  return 0;
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
  
  }

const bg_db_object_class_t bg_db_audio_album_class =
  {
    .del = del_audioalbum,
    .parent = NULL, // Object
  };

void bg_db_audio_file_add_to_album(bg_db_t * db, bg_db_audio_file_t * f)
  {
#if 0
  char * sql;
  int result;
  bg_db_audio_album_t a;
  char date_string[BG_DB_DATE_STRING_LEN];
  
  bg_db_audio_album_init(&a, NULL);

  /* Add albumartist */
  a.artist_id =
    bg_sqlite_string_to_id_add(db->db, "AUDIO_ARTISTS",
                               "ID", "NAME", f->albumartist);
  a.title = gavl_strdup(f->album);

  /* Check if the album already exists */
  sql = sqlite3_mprintf("select ID from AUDIO_ALBUMS where ((TITLE = %Q) & (ARTIST = %"PRId64"));",
                        a.title, a.artist_id);
  result = bg_sqlite_exec(db->db, sql, bg_sqlite_int_callback, &a.obj.id);
  sqlite3_free(sql);

  if(!result)
    return; // Should never happen
  
  if(a.obj.id < 0) /* Album doesn't exit yet, create it */
    {
    /* Date */
    bg_db_date_copy(&a.date, &f->date);
    bg_db_date_to_string(&a.date, date_string);
    
    a.genre_id = f->genre_id;
    
    sql = sqlite3_mprintf("INSERT INTO AUDIO_ALBUMS ( ID, ARTIST, TITLE, GENRE, DATE ) VALUES ( %"PRId64", %"PRId64", %Q, %"PRId64", %Q);",
                          a.id, a.artist_id, a.title, a.genre_id, date_string);
    result = bg_sqlite_exec(db->db, sql, NULL, NULL);
    sqlite3_free(sql);
    }
  else
    {
    /* Update tracks and duration */
    a.tracks++;
    a.duration += f->duration;
    
    /* If there is a date set, we check if it's the same for the added track */
    if(!bg_db_date_equal(&a.date, &f->date))
      bg_db_date_set_invalid(&a.date);

    bg_db_date_to_string(&a.date, date_string);

    /* If there is a genre set, we check if it's the same for the added track */
    if(a.genre_id != f->genre_id)
      a.genre_id = 0;

    sql = sqlite3_mprintf("UPDATE AUDIO_ALBUMS SET TRACKS = %d, DATE = %Q, DURATION = %"PRId64", GENRE = %"PRId64" WHERE ID = %"PRId64";",
                          a.tracks, date_string, a.duration, a.genre_id, a.id);
    result = bg_sqlite_exec(db->db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return;
    
    }
#endif
  }

void bg_db_audio_file_remove_from_album(bg_db_t * db, bg_db_audio_file_t * f)
  {
#if 0
  char * sql;
  int result;
  bg_db_audio_album_t a;

  if(f->album_id <= 0)
    return;

  bg_db_audio_album_init(&a);
  a.id = f->album_id;

  sql = sqlite3_mprintf("select TRACKS, DURATION from AUDIO_ALBUMS where ID = %"PRId64";", a.id);
  result = bg_sqlite_exec(db->db, sql, album_query_callback, &a);
  sqlite3_free(sql);
  if(!result || !a.found)
    return;

  if(a.tracks == 1) // Delete entire album
    {
    sql = sqlite3_mprintf("DELETE FROM AUDIO_ALBUMS WHERE ID = %"PRId64";", f->album_id);
    result = bg_sqlite_exec(db->db, sql, NULL, NULL);
    sqlite3_free(sql);
    }
  else
    {
    a.tracks--;
    a.duration -= f->duration;
    sql = sqlite3_mprintf("UPDATE AUDIO_ALBUMS SET TRACKS = %d, DURATION = %"PRId64" WHERE ID = %"PRId64";",
                          a.tracks, a.duration, a.id);
    result = bg_sqlite_exec(db->db, sql, NULL, NULL);
    sqlite3_free(sql);
    }
#endif
  }

int bg_db_audio_album_query(bg_db_t * db, bg_db_audio_album_t*a, int full)
  {
#if 0
  char * sql;
  int result;

  if(a->id < 0)
    {
    if((a->artist_id >= 0) && (a->title))
      {
      sql = sqlite3_mprintf("select * from AUDIO_ALBUMS where ((TITLE = %Q) & (ARTIST = %"PRId64"));",
                            a->title, a->artist_id);
      result = bg_sqlite_exec(db->db, sql, album_query_callback, a);
      sqlite3_free(sql);
      if(!result || !a->found)
        return 0;
      }
    }
  else if(a->id >= 0)
    {
    sql =
      sqlite3_mprintf("select * from AUDIO_ALBUMS where ID = %"PRId64";", a->id);
    result = bg_sqlite_exec(db->db, sql, album_query_callback, a);
    sqlite3_free(sql);
    if(!result || !a->found)
      return 0;
    }
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Either ID or artist and title must be set in audio album");
    return 0;
    }
  if(!full)
    return 1;
  a->artist = bg_sqlite_id_to_string(db->db, "AUDIO_ARTISTS", "NAME", "ID", a->artist_id);
  a->genre  = bg_sqlite_id_to_string(db->db, "AUDIO_GENRES",  "NAME", "ID", a->genre_id);
#endif
  return 1;
  }
