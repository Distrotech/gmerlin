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

#include "nmjedit.h"
#include <string.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "nmjedit.album"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

void bg_nmj_album_init(bg_nmj_album_t * a)
  {
  memset(a, 0, sizeof(*a));
  a->id = -1;
  }

void bg_nmj_album_free(bg_nmj_album_t * a)
  {
  MY_FREE(a->title);
  MY_FREE(a->search_title);
  MY_FREE(a->total_item);
  MY_FREE(a->release_date);
  MY_FREE(a->update_state); // 3??
  }

void bg_nmj_album_dump(bg_nmj_album_t * a)
  {
  bg_dprintf("Album:\n");
  bg_dprintf("  ID:           %"PRId64"\n", a->id);
  bg_dprintf("  title:        %s\n", a->title);
  bg_dprintf("  search_title: %s\n", a->search_title);
  bg_dprintf("  total_item:   %s\n", a->total_item);
  bg_dprintf("  release_date: %s\n", a->release_date);
  bg_dprintf("  update_state: %s\n", a->update_state);
  bg_dprintf("  genre_id:     %"PRId64"\n", a->genre_id);
  bg_dprintf("  artist_id:    %"PRId64"\n", a->artist_id);
  }

static int album_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_nmj_album_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    //    fprintf(stderr, "col: %s, val: %s\n", azColName[i], argv[i]);
    SET_QUERY_INT("ID", id);
    SET_QUERY_STRING("TITLE", title);
    SET_QUERY_STRING("SEARCH_TITLE", search_title);
    SET_QUERY_STRING("TOTAL_ITEM", total_item );
    SET_QUERY_STRING("RELEASE_DATE", release_date);
    SET_QUERY_STRING("UPDATE_STATE", update_state );
    }
  ret->found = 1;
  return 0;
  }

int bg_nmj_album_query(sqlite3 * db, bg_nmj_album_t * a)
  {
  char * sql;
  int result;

  sql = sqlite3_mprintf("select * from SONG_ALBUMS where ID = %"PRId64";", a->id);
  result = bg_sqlite_exec(db, sql, album_query_callback, a);
  sqlite3_free(sql);
  if(!a->found)
    return 0;

  /* Get Genre */
  a->genre_id = bg_nmj_id_to_id(db,
                                "SONG_GENRES_SONG_ALBUMS",
                                "GENRES_ID",
                                "ALBUMS_ID",
                                a->id);

  /* Get Artist */
  a->artist_id = bg_nmj_id_to_id(db,
                                 "SONG_PERSONS_SONG_ALBUMS",
                                 "PERSONS_ID",
                                 "ALBUMS_ID",
                                 a->id);
  return 1;
  }

int bg_nmj_album_add(bg_plugin_registry_t * plugin_reg,
                     sqlite3 * db, bg_nmj_album_t * a, bg_nmj_song_t * song)
  {
  char * sql;
  int result;
  int64_t id;
  int64_t group_id;
  
  if(a->id < 0)
    {
    char * artist;
    
    a->id = bg_nmj_get_next_id(db, "SONG_ALBUMS");
    a->title = bg_strdup(a->title, song->album);
    a->search_title = bg_nmj_make_search_string(a->title);
    a->total_item = bg_sprintf("%d", 1);
    a->release_date = bg_strdup(a->release_date, song->release_date);
    a->update_state = bg_sprintf("%d", 3);
    a->genre_id = song->genre_id;
    a->artist_id = song->albumartist_id;

    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Adding album %s", a->title);
    
    /* Insert */

    sql = sqlite3_mprintf("INSERT INTO SONG_ALBUMS "
                          "( ID, TITLE, SEARCH_TITLE, TOTAL_ITEM, "
                          "RELEASE_DATE, UPDATE_STATE ) VALUES "
                          "( %"PRId64", %Q, %Q, %Q, %Q, %Q );",
                          a->id, a->title, a->search_title, a->total_item,
                          a->release_date, a->update_state);
    
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return result;

    /* Set Genre */
    if(song->genre_id >= 0)
      {
      id = bg_nmj_get_next_id(db, "SONG_GENRES_SONG_ALBUMS");
      sql = sqlite3_mprintf("INSERT INTO SONG_GENRES_SONG_ALBUMS "
                           "( ID, ALBUMS_ID, GENRES_ID ) VALUES "
                           "( %"PRId64", %"PRId64", %"PRId64" );",
                           id, a->id, song->genre_id);
      result = bg_sqlite_exec(db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return result;
      }
    
    /* Set Artist */
    id = bg_nmj_get_next_id(db, "SONG_PERSONS_SONG_ALBUMS");
    sql = sqlite3_mprintf("INSERT INTO SONG_PERSONS_SONG_ALBUMS "
                         "( ID, PERSONS_ID, ALBUMS_ID, PERSON_TYPE ) VALUES "
                         "( %"PRId64", %"PRId64", %"PRId64", %Q );",
                          id, a->artist_id, a->id, "ARTIST");
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return result;

    /* Get group */

    group_id = -1;
    artist = bg_nmj_id_to_string(db, "SONG_PERSONS", "NAME", "ID", a->artist_id);
    
    if(artist)
      {
      group_id = bg_nmj_get_group(db, "SONG_GROUPS", artist);
      free(artist);
      }
    
    if(group_id >= 0)
      {
      id = bg_nmj_get_next_id(db, "SONG_GROUPS_SONG_ALBUMS");
      sql = sqlite3_mprintf("INSERT INTO SONG_GROUPS_SONG_ALBUMS "
                            "( ID, GROUPS_ID, ALBUMS_ID ) VALUES "
                            "( %"PRId64", %"PRId64", %"PRId64" );",
                            id, group_id, a->id);
      result = bg_sqlite_exec(db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return result;
      }
    
    /* Check for Cover */
    bg_nmj_album_update_cover(plugin_reg, db, a);
    }
  else
    {
    int64_t ti;
    bg_nmj_album_query(db, a);

    /* Increment total items */
    ti = strtoll(a->total_item, NULL, 10);
    ti++;
    free(a->total_item);
    a->total_item = bg_sprintf("%"PRId64, ti);

    sql = sqlite3_mprintf("UPDATE SONG_ALBUMS SET TOTAL_ITEM = %Q WHERE ID = %"PRId64";",
                          a->total_item, a->id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return result;

    /* If the song has a different release date, clear the release date of the
       album */
    if(strcmp(a->release_date, "9999-01-01") &&
       strcmp(a->release_date, song->release_date))
      {
      a->release_date = bg_strdup(a->release_date, "9999-01-01");

      sql = sqlite3_mprintf("UPDATE SONG_ALBUMS SET RELEASE_DATE = %Q WHERE ID = %"PRId64";",
                            a->release_date, a->id);
      result = bg_sqlite_exec(db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return result;
      }
    }
  
  /* Add this song */
  id = bg_nmj_get_next_id(db, "SONG_ALBUMS_SONGS");

  sql = sqlite3_mprintf("INSERT INTO SONG_ALBUMS_SONGS "
                       "( ID, ALBUMS_ID, SONGS_ID, SEQUENCE ) VALUES "
                       "( %"PRId64", %"PRId64", %"PRId64", %"PRId64" );",
                       id, a->id, song->id, song->track_position );
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return result;
  return 1;
  }

static void delete_thumbnails(sqlite3 * db, int64_t album_id)
  {
  char * image;
  char * sql;
  int result;
  
  image  = bg_nmj_id_to_string(db, "SONG_ALBUM_POSTERS", "THUMBNAIL", "ID", album_id);
  if(image)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing thumbnail %s", image);
    remove(image);
    free(image);
    }
  image  = bg_nmj_id_to_string(db, "SONG_ALBUM_POSTERS", "POSTER", "ID", album_id);
  if(image)
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing poster %s", image);
    remove(image);
    free(image);
    }
  sql = sqlite3_mprintf("DELETE FROM SONG_ALBUM_POSTERS WHERE ID = %"PRId64";", album_id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  }

static int remove_album(sqlite3 * db, int64_t album_id)
  {
  char * tmp_string;
  char * sql;
  int result;

  tmp_string = bg_nmj_id_to_string(db,
                                   "SONG_ALBUMS",
                                   "TITLE",
                                   "ID",
                                   album_id);
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing empty album %s",
         tmp_string);
  free(tmp_string);
  
  /* Delete from SONG_ALBUMS */
  sql = sqlite3_mprintf("DELETE FROM SONG_ALBUMS WHERE ID = %"PRId64";", album_id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;

  /* Delete thumbnails & posters */

  delete_thumbnails(db, album_id);
  
  /* Delete from persons */
  sql = sqlite3_mprintf("DELETE FROM SONG_PERSONS_SONG_ALBUMS WHERE ALBUMS_ID = %"PRId64";", album_id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;

  /* Delete from genres */
  sql = sqlite3_mprintf("DELETE FROM SONG_GENRES_SONG_ALBUMS WHERE ALBUMS_ID = %"PRId64";", album_id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;

  /* Delete from Groups */
  sql = sqlite3_mprintf("DELETE FROM SONG_GROUPS_SONG_ALBUMS WHERE ALBUMS_ID = %"PRId64";", album_id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;
  
  return 1;
  }

int bg_nmj_album_delete(sqlite3 * db, int64_t album_id, bg_nmj_song_t * song)
  {
  char * total_item;
  int64_t ti;
  char * sql;
  int result;
  
  /* Delete from track list */
  
  sql = sqlite3_mprintf("DELETE FROM SONG_ALBUMS_SONGS WHERE SONGS_ID = %"PRId64";",
                        song->id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;
  
  /* Decretment total count */
  total_item = bg_nmj_id_to_string(db, "SONG_ALBUMS", "TOTAL_ITEM", "ID", album_id);

  if(!total_item)
    return 1;
  
  ti = strtoll(total_item, NULL, 10);
  ti--;

  if(!ti)
    remove_album(db, album_id);
  else
    {
    free(total_item);
    total_item = bg_sprintf("%"PRId64, ti);
    
    sql = sqlite3_mprintf("UPDATE SONG_ALBUMS SET TOTAL_ITEM = %Q WHERE ID = %"PRId64";",
                          total_item, album_id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);

    free(total_item);

    if(!result)
      return result;
    }
  
  return 1;
  }

void bg_nmj_album_update_cover(bg_plugin_registry_t * plugin_reg,
                               sqlite3 * db, bg_nmj_album_t * album)
  {
  bg_nmj_song_t song;
  
  char * sql;
  bg_nmj_append_int_t tab;
  int result;
  char * cover_fs;
  char * create_time_db;
  char create_time_fs[BG_NMJ_TIME_STRING_LEN];
  struct stat st;
  
  memset(&tab, 0, sizeof(tab));
  
  /* Get a song in the album */
  bg_nmj_song_init(&song);

  sql =
    sqlite3_mprintf("select SONGS_ID from SONG_ALBUMS_SONGS where ALBUMS_ID = %"PRId64";",
                    album->id);
  result = bg_sqlite_exec(db, sql, bg_nmj_append_int_callback, &tab);
  sqlite3_free(sql);

  if(!tab.num_val)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN,
           "Album %s contains no tracks", album->title);
    goto fail;
    }
  
  song.id = tab.val[0];
  
  bg_nmj_song_query(db, &song);
  
  /* Get cover file from song */
  
  cover_fs = bg_nmj_song_get_cover(&song);
  
  if(cover_fs)
    {
    if(stat(cover_fs, &st))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN,
             "Cannot stat %s: %s", cover_fs, strerror(errno));
      free(cover_fs);
      cover_fs = NULL;
      }
    else
      bg_nmj_time_to_string(st.st_mtime, create_time_fs);
    }
  else
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN, "No cover for album %s",
           album->title);
    }
  
  /* Get cover file from db */
  
  create_time_db = bg_nmj_id_to_string(db,
                                       "SONG_ALBUM_POSTERS",
                                       "CREATE_TIME",
                                       "ID",
                                       album->id);

  if(create_time_db)
    {
    if(!cover_fs || // Cover disappeared
       strcmp(create_time_db, create_time_fs)) // Cover changed
      {
      if(!cover_fs)
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Cover file for album %s disappeared", album->title);
      else
        bg_log(BG_LOG_INFO, LOG_DOMAIN, "Cover file for album %s changed", album->title);
      
      delete_thumbnails(db, album->id);
      free(create_time_db);
      create_time_db = NULL;
      }
    }

  /* (re)generate cover */
  if(!create_time_db && cover_fs)
    {
    char * poster;
    char * thumb;
    
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using cover %s", cover_fs);
    
    poster =
      bg_sprintf("nmj_database/media/music/poster/poster_a_%"PRId64".jpg",
                 album->id);

    thumb =
      bg_sprintf("nmj_database/media/music/thumbnail/thumbnail_%"PRId64".jpg",
                 album->id);
    
    bg_nmj_make_thumbnail(plugin_reg, cover_fs, poster, 768, 0);
    
    /* Create thumbnail */
    bg_nmj_make_thumbnail(plugin_reg, cover_fs, thumb, 248, 1);
    
    sql = sqlite3_mprintf("INSERT INTO SONG_ALBUM_POSTERS "
                          "( ID, POSTER, THUMBNAIL, CREATE_TIME ) VALUES "
                          "( %"PRId64", %Q, %Q, %Q );",
                          album->id, poster, thumb, create_time_fs);
    
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    free(poster);
    free(thumb);
    }
  
  fail:
  if(tab.val)
    free(tab.val);

  if(cover_fs)
    free(cover_fs);
  
  }
