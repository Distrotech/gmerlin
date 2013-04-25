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

void bg_db_audio_album_init(bg_db_audio_album_t*a)
  {
  memset(a, 0, sizeof(*a));
  a->id = -1;
  a->artist_id = -1;
  }

void bg_db_audio_album_free(bg_db_audio_album_t*a)
  {
  if(a->artist)
    free(a->artist);
  if(a->title)
    free(a->title);
  }

void bg_db_audio_file_add_to_album(bg_db_t * db, bg_db_audio_file_t * f)
  {
  char * sql;
  int result;
  bg_db_audio_album_t a;
  char date_string[BG_DB_DATE_STRING_LEN];
  
  bg_db_audio_album_init(&a);

  /* Add albumartist */
  a.artist_id =
    bg_sqlite_string_to_id_add(db->db, "AUDIO_ARTISTS",
                               "ID", "NAME", f->albumartist);
  a.title = gavl_strdup(f->album);

  if(!bg_db_audio_album_query(db, &a))
    {
    a.id = bg_sqlite_get_next_id(db->db, "AUDIO_ALBUMS");
    /* Date */
    bg_db_date_copy(&a.date, &f->date);
    bg_db_date_to_string(&a.date, date_string);
    
    /* Album doesn't exit yet, create it */
    a.tracks = 1;
    a.genre_id = f->genre_id;
    
    sql = sqlite3_mprintf("INSERT INTO AUDIO_ALBUMS ( ID, ARTIST, TITLE, TRACKS, GENRE, DATE ) VALUES ( %"PRId64", %"PRId64", %Q, %"PRId64", %"PRId64", %Q);",
                          a.id, a.artist_id, a.title, a.tracks, a.genre_id, date_string);
    result = bg_sqlite_exec(db->db, sql, NULL, NULL);
    sqlite3_free(sql);
    }
  else
    {
    a.tracks++;
    sql = sqlite3_mprintf("UPDATE AUDIO_ALBUMS SET TRACKS = %"PRId64" WHERE ID = %"PRId64";",
                          a.tracks, a.id);
    result = bg_sqlite_exec(db->db, sql, NULL, NULL);
    sqlite3_free(sql);

    if(!result)
      return;
    
    /* If there is a date set, we check if it's the same for the added track */
    if(!bg_db_date_equal(&a.date, &f->date))
      {
      bg_db_date_set_invalid(&a.date);
      bg_db_date_to_string(&a.date, date_string);

      sql = sqlite3_mprintf("UPDATE AUDIO_ALBUMS SET DATE = %Q WHERE ID = %"PRId64";",
                            date_string, a.id);
      result = bg_sqlite_exec(db->db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return;
      }

    /* If there is a genre set, we check if it's the same for the added track */
    if(a.genre_id != f->genre_id)
      {
      a.genre_id = 0;
      sql = sqlite3_mprintf("UPDATE AUDIO_ALBUMS SET GENRE = %"PRId64" WHERE ID = %"PRId64";",
                            a.genre_id, a.id);
      result = bg_sqlite_exec(db->db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return;
      }
    }
  f->album_id = a.id;
  }

void bg_db_audio_file_remove_from_album(bg_db_t * db, bg_db_audio_file_t * t)
  {
  
  }

static int album_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_audio_album_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_INT("ID",          id);
    BG_DB_SET_QUERY_INT("ARTIST"   ,   artist_id);
    BG_DB_SET_QUERY_STRING("TITLE",    title);
    BG_DB_SET_QUERY_INT("TRACKS" ,     tracks);
    BG_DB_SET_QUERY_INT("GENRE",       genre_id);
    }
  ret->found = 1;
  return 0;
  }


int bg_db_audio_album_query(bg_db_t * db, bg_db_audio_album_t*a)
  {
  char * sql;
  int result;

  if((a->artist_id >= 0) && (a->title))
    {
    sql = sqlite3_mprintf("select * from AUDIO_ALBUMS where ((TITLE = %Q) & (ARTIST = %"PRId64"));",
                          a->title, a->artist_id);
    result = bg_sqlite_exec(db->db, sql, album_query_callback, a);
    sqlite3_free(sql);
    if(!result || !a->found)
      return 0;
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
           "Either ID or path must be set in directory");
    return 0;
    }
  a->artist = bg_sqlite_id_to_string(db->db, "AUDIO_ARTISTS", "NAME", "ID", a->artist_id);
  a->genre  = bg_sqlite_id_to_string(db->db, "AUDIO_GENRES",  "NAME", "ID", a->genre_id);
  return 1;
  }
