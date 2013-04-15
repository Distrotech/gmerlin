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

#include <string.h>

#include "nmjedit.h"
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "nmjedit.gmerlinalbum"

#include <gmerlin/tree.h>

int bg_nmj_add_album(sqlite3 * db, const char * album)
  {
  bg_album_entry_t * entries;
  bg_album_entry_t * e;
  char * album_xml;
  int skip_chars = -1;
  char * pos;
  char * dir;
  char * name;
  int64_t pls_id;
  char * sql;
  int result;
  int64_t song_id;
  int64_t item_id;
  int64_t num;
  
  album_xml = bg_read_file(album, NULL);
  if(!album_xml)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s could not be opened", album);
    return 0;
    }

  entries = bg_album_entries_new_from_xml(album_xml);
  free(album_xml);

  if(!entries)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s contains no entries",
           album);
    return 0;
    }

  /* Figure out how many characters must be skipped from the album. This does not
     work for insane directory structures */
  
  dir = bg_nmj_find_dir(db, entries->location);
  if(!dir || !(pos = strstr(entries->location, dir)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No directory found for location %s",
           (char*)entries->location);
    return 0;
    }

  skip_chars = pos - (char*)entries->location;
  
  /* Get the playlist name */
  pos = strrchr(album, '/');
  if(!pos)
    name = gavl_strdup(album);
  else
    name = gavl_strdup(pos+1);
  
  pos = strrchr(name, '.');
  if(pos)
    *pos = '\0';

  pls_id = bg_nmj_string_to_id(db, "SONG_PLS", "ID", "NAME", name);
  if(pls_id >= 0)
    {
    /* Playlist already exists: Clear everything inside */
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Clearing existent playlist %s", name);
    sql = sqlite3_mprintf("DELETE FROM SONG_PLS_ITEM WHERE PLS_ID = %"PRId64";", pls_id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return 0;
    }
  else
    {
    char * pls_path;
    char time_str[BG_NMJ_TIME_STRING_LEN];
    
    pls_path = bg_sprintf("MUSIC/PLAYLISTS/%s.M3U", name);
    bg_nmj_time_to_string(time(NULL), time_str);
    
    /* Playlist doesn't exist: Create a new one */
    pls_id = bg_nmj_get_next_id(db, "SONG_PLS");

    sql = sqlite3_mprintf("INSERT INTO SONG_PLS "
                         "( ID, NAME, PATH, TOTAL_ITEM, "
                         "CREATE_TIME ) VALUES "
                         "( %"PRId64", %Q, %Q, 0, %Q );",
                          pls_id, name, pls_path, time_str);
    
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return result;
    
    free(pls_path);
    }

  /* Add entries */
  e = entries;
  num = 0;
  while(e)
    {
    pos = ((char*)e->location) + skip_chars;
    song_id = bg_nmj_string_to_id(db, "SONGS", "ID", "PATH", pos);

    if(song_id >= 0)
      {
      item_id = bg_nmj_get_next_id(db, "SONG_PLS_ITEM");
      
      sql = sqlite3_mprintf("INSERT INTO SONG_PLS_ITEM "
                            "( ID, PLS_ID, SONGS_ID, SEQUENCE) "
                            " VALUES "
                            "( %"PRId64", %"PRId64", %"PRId64", %"PRId64" );",
                            item_id, pls_id, song_id, num+1);
      result = bg_sqlite_exec(db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return result;
      num++;
      }
    else
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Song %s not found",
             pos);
    
    e = e->next;
    }

  /* Update total entries */
  sql = sqlite3_mprintf("UPDATE SONG_PLS SET TOTAL_ITEM = %"PRId64" WHERE ID = %"PRId64";",
                        num, pls_id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return result;
  
  bg_album_entries_destroy(entries);
  return 1;
  }
