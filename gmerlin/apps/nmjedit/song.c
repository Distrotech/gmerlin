/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

/* Song */

#include "nmjedit.h"
#include <string.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "nmjedit.song"


void bg_nmj_song_free(bg_nmj_song_t * song)
  {
  MY_FREE(song->title);
  MY_FREE(song->search_title);
  MY_FREE(song->path);
  MY_FREE(song->runtime);
  MY_FREE(song->format);
  MY_FREE(song->lyric);
  MY_FREE(song->hash);
  MY_FREE(song->bit_rate);
  MY_FREE(song->release_date);
  MY_FREE(song->create_time);
  MY_FREE(song->update_state);
  MY_FREE(song->filestatus);
  MY_FREE(song->artist);
  MY_FREE(song->genre);
  MY_FREE(song->albumartist);
  MY_FREE(song->album);
  }

void bg_nmj_song_init(bg_nmj_song_t * song)
  {
  memset(song, 0, sizeof(*song));
  song->id = -1;
  song->album_id = -1;
  song->artist_id = -1;
  song->genre_id = -1;
  }

void bg_nmj_song_dump(bg_nmj_song_t * song)
  {
  bg_dprintf("Song:\n");
  bg_dprintf("  ID:             %"PRId64"\n", song->id);
  bg_dprintf("  title:          %s\n",        song->title);
  bg_dprintf("  search_title:   %s\n",        song->search_title);
  bg_dprintf("  path:           %s\n",        song->path);
  bg_dprintf("  scan_dirs_id:   %"PRId64"\n", song->scan_dirs_id);
  bg_dprintf("  folders_id:     %"PRId64"\n", song->folders_id);
  bg_dprintf("  runtime:        %s\n",        song->runtime);
  bg_dprintf("  format:         %s\n",        song->format);
  bg_dprintf("  lyric:          %s\n",        song->lyric);
  bg_dprintf("  rating:         %"PRId64"\n", song->rating);
  bg_dprintf("  hash:           %s\n",        song->hash);
  bg_dprintf("  size:           %"PRId64"\n",        song->size);
  bg_dprintf("  bit_rate:       %s\n",        song->bit_rate);
  bg_dprintf("  track_position: %"PRId64"\n",        song->track_position);
  bg_dprintf("  release_date:   %s\n",        song->release_date);
  bg_dprintf("  create_time:    %s\n",        song->create_time);
  bg_dprintf("  update_state:   %s\n",        song->update_state);
  bg_dprintf("  filestatus:     %s\n",        song->filestatus);
  bg_dprintf("  album:          %s\n",        song->album);
  bg_dprintf("  genre:          %s\n",        song->genre);
  bg_dprintf("  artist:         %s\n",        song->artist);
  bg_dprintf("  albumartist:    %s\n",        song->albumartist);
  bg_dprintf("  genre_id:       %"PRId64"\n", song->genre_id);
  bg_dprintf("  artist_id:      %"PRId64"\n", song->artist_id);
  bg_dprintf("  albumartist_id: %"PRId64"\n", song->albumartist_id);
  bg_dprintf("  album_id:       %"PRId64"\n",  song->album_id);
  }

static int song_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_nmj_song_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    //    fprintf(stderr, "col: %s, val: %s\n", azColName[i], argv[i]);
    SET_QUERY_INT("ID", id);
    SET_QUERY_STRING("TITLE", title);
    SET_QUERY_STRING("SEARCH_TITLE", search_title);
    SET_QUERY_STRING("PATH", path);
    SET_QUERY_INT("SCAN_DIRS_ID", scan_dirs_id);
    SET_QUERY_INT("FOLDERS_ID", folders_id);
    SET_QUERY_STRING("RUNTIME", runtime);
    SET_QUERY_STRING("FORMAT", format);
    SET_QUERY_STRING("LYRIC", lyric);
    SET_QUERY_INT("RATING", rating);
    SET_QUERY_STRING("HASH", hash);
    SET_QUERY_INT("SIZE", size);
    SET_QUERY_STRING("BIT_RATE", bit_rate);
    SET_QUERY_INT("TRACK_POSITION", track_position);
    SET_QUERY_STRING("RELEASE_DATE", release_date);
    SET_QUERY_STRING("CREATE_TIME", create_time);
    SET_QUERY_STRING("UPDATE_STATE", update_state );
    SET_QUERY_STRING("FILESTATUS", filestatus );
    }
  ret->found = 1;
  return 0;
  }


int bg_nmj_song_query(sqlite3 * db, bg_nmj_song_t * song)
  {
  char * sql;
  int result;
  if(song->path)
    {
    sql = sqlite3_mprintf("select * from SONGS where PATH = %Q;", song->path);
    result = bg_sqlite_exec(db, sql, song_query_callback, song);
    sqlite3_free(sql);
    if(!song->found)
      return 0;
    }
  else if(song->id >= 0)
    {
    sql = sqlite3_mprintf("select * from SONGS where ID = %"PRId64";", song->id);
    result = bg_sqlite_exec(db, sql, song_query_callback, song);
    sqlite3_free(sql);
    if(!song->found)
      return 0;
    }
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Either ID or path must be set in directory");
    return 0;
    }

  /* Get secondary stuff */
  song->genre_id = bg_nmj_id_to_id(db, "SONG_GENRES_SONGS",
                                   "GENRES_ID", "SONGS_ID", song->id);
  if(song->genre_id >= 0)
    song->genre = bg_nmj_id_to_string(db, "SONG_GENRES",
                                      "NAME", "ID", song->genre_id);
  
  song->album_id = bg_nmj_id_to_id(db, "SONG_ALBUMS_SONGS",
                                   "ALBUMS_ID", "SONGS_ID", song->id);
  if(song->album_id >= 0)
    song->album = bg_nmj_id_to_string(db, "SONG_ALBUMS",
                                      "TITLE", "ID", song->album_id);

  song->artist_id = bg_nmj_id_to_id(db, "SONG_PERSONS_SONGS",
                                    "PERSONS_ID", "SONGS_ID", song->id);
  if(song->artist_id >= 0)
    song->artist = bg_nmj_id_to_string(db, "SONG_PERSONS",
                                       "NAME", "ID", song->artist_id);
  return 1;
  }

int bg_nmj_song_get_info(sqlite3 * db,
                         bg_plugin_registry_t * plugin_reg,
                         bg_nmj_dir_t * dir,
                         bg_nmj_file_t * file,
                         bg_nmj_song_t * song)
  {
  bg_plugin_handle_t * h = NULL;
  bg_input_plugin_t * plugin = NULL;
  int ret = 0;
  bg_track_info_t * ti;
  int year;
  
  if(!bg_input_plugin_load(plugin_reg, file->path, NULL, &h, NULL, 0))
    goto fail;

  plugin = (bg_input_plugin_t *)h->plugin;

  /* Only one track supported */
  if(plugin->get_num_tracks && (plugin->get_num_tracks(h->priv) < 1))
    goto fail;

  ti = plugin->get_track_info(h->priv, 0);

  if(!ti->num_audio_streams)
    goto fail;
    
  if(plugin->set_track && !plugin->set_track(h->priv, 0))
    goto fail;

  if(!plugin->set_audio_stream(h->priv, 0, BG_STREAM_ACTION_DECODE))
    goto fail;

  if(!plugin->start(h->priv))
    goto fail;

  /* Fill in the data structure */
  
  song->title = bg_nmj_escape_string(ti->metadata.title);
  song->search_title = bg_nmj_escape_string(ti->metadata.title);

  song->path = bg_strdup(song->path, file->path);
  song->scan_dirs_id = dir->id;
  song->runtime = bg_sprintf("%d", (int)(ti->duration / GAVL_TIME_SCALE));
  song->format  = bg_strdup(song->format, ti->description);
  song->size    = file->size;

  if(ti->audio_streams[0].bitrate)
    song->bit_rate = bg_sprintf("%d", (int)(ti->audio_streams[0].bitrate / 1000));
  else
    song->bit_rate = bg_strdup(song->bit_rate, "VBR");

  song->track_position = ti->metadata.track;

  year = bg_metadata_get_year(&ti->metadata);
  if(year)
    song->release_date = bg_sprintf("%d-01-01", year);
  else
    song->release_date = bg_strdup(NULL, "9999-01-01");
  song->create_time = malloc(BG_NMJ_TIME_STRING_LEN);
  bg_nmj_time_to_string(file->time, song->create_time);

  song->album        = bg_nmj_escape_string(ti->metadata.album);
  song->artist       = bg_nmj_escape_string(ti->metadata.artist);
  song->albumartist  = bg_nmj_escape_string(ti->metadata.albumartist);
  song->genre        = bg_nmj_escape_string(ti->metadata.genre);

  /* Get IDs */
  if(song->genre)
    song->genre_id     = bg_nmj_string_to_id(db, "SONG_GENRES", "ID",
                                             "NAME", song->genre);
  
  if(song->albumartist)
    song->albumartist_id    = bg_nmj_string_to_id(db, "SONG_PERSONS", "ID",
                                                  "NAME", song->albumartist);
  if(song->artist)
    song->artist_id    = bg_nmj_string_to_id(db, "SONG_PERSONS", "ID",
                                             "NAME", song->artist);
  
  /* TODO: Many albums can have the same name, check with PERSONS as well */
  if(song->album)
    song->album_id     = bg_nmj_string_to_id(db, "SONG_ALBUMS", "ID",
                                             "TITLE", song->album);
  
  ret = 1;
  fail:

  if(plugin)
    plugin->close(h->priv);
  
  if(h)
    bg_plugin_unref(h);
  return ret;
  }

int bg_nmj_song_add(sqlite3 * db, bg_nmj_song_t * song)
  {
  int result;
  char * sql;

  song->id = bg_nmj_get_next_id(db, "SONGS");

  /* Add Song */

  /* Add Genre */

  /* Add Artist */
  
  /* Add Album */
  
  return 0;
  }

#define APPEND_STRING(member, col) \
  if(old_song->member && new_song->member && \
     strcmp(old_song->member, new_song->member))        \
    { \
    if(num_cols)                                  \
      sql = bg_strcat(sql, ","); \
    tmp_string = sqlite3_mprintf(" " col " = %Q", new_song->member); \
    sql = bg_strcat(sql, tmp_string);                               \
    sqlite3_free(tmp_string); \
    num_cols++; \
    }

#define APPEND_INT(member, col)           \
  if(old_song->member != new_song->member)        \
    { \
    if(num_cols)                                  \
      sql = bg_strcat(sql, ","); \
    tmp_string = sqlite3_mprintf(" " col " = %"PRId64, new_song->member); \
    sql = bg_strcat(sql, tmp_string);                               \
    sqlite3_free(tmp_string); \
    num_cols++; \
    }

int bg_nmj_song_update(sqlite3 * db,
                       bg_nmj_song_t * old_song,
                       bg_nmj_song_t * new_song)
  {
  int result;
  char * sql;
  char * tmp_string;
  int num_cols = 0;
  /* Update SONGS */

  sql = bg_strdup(NULL, "UPDATE SONGS SET");

  APPEND_STRING(title,        "TITLE");
  APPEND_STRING(search_title, "SEARCH_TITLE");
  APPEND_STRING(runtime,      "RUNTIME");
  APPEND_STRING(format,       "FORMAT");
  APPEND_INT(rating,          "RATING");
  APPEND_INT(rating,          "SIZE");
  APPEND_INT(rating,          "RATING");
  APPEND_STRING(bit_rate,     "BIT_RATE");
  APPEND_INT(track_position,  "TRACK_POSITION");
  APPEND_STRING(release_date, "RELEASE_DATE");
  APPEND_STRING(create_time,  "CREATE_TIME");
  
  tmp_string = sqlite3_mprintf(" WHERE ID = %"PRId64, old_song->id);
  sql = bg_strcat(sql, tmp_string);
  sqlite3_free(tmp_string);

  if(num_cols)
    result = bg_sqlite_exec(db, sql, NULL, NULL);
  else
    result = 1;
  
  free(sql);

  if(!result)
    return 0;
  
  /* Album changed */
  if(old_song->album_id != new_song->album_id)
    {
    
    }
  
  /* Genre changed */
  if(old_song->genre_id != new_song->genre_id)
    {
    
    }

  /* Artist changed */
  if(old_song->genre_id != new_song->genre_id)
    {
    
    }
  
  return 1;
  }

int bg_nmj_song_delete(sqlite3 * db, bg_nmj_song_t * song)
  {
  char * sql;
  int result;
  /* Delete from SONGS */
  sql = sqlite3_mprintf("DELETE FROM SONGS WHERE ID = %"PRId64";", song->id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;
  
  /* Delete from SONG_ALBUMS_SONGS */
  if(song->album_id >= 0)
    {
    sql = sqlite3_mprintf("DELETE FROM SONGS_ALBUMS_SONGS WHERE SONGS_ID = %"PRId64";",
                          song->id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return 0;

    /* Decrement track count for this album */
    
    
    }

  /* Delete from SONG_GENRES_SONGS */
  if(song->genre_id >= 0)
    {
    sql = sqlite3_mprintf("DELETE FROM SONGS_GENRES_SONGS WHERE SONGS_ID = %"PRId64";",
                          song->id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return 0;
    }

  /* Delete from SONG_PERSONS_SONGS */
  if(song->artist_id >= 0)
    {
    sql = sqlite3_mprintf("DELETE FROM SONGS_PERSONS_SONGS WHERE SONGS_ID = %"PRId64";",
                          song->id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return 0;
    }
  return 1;
  }
