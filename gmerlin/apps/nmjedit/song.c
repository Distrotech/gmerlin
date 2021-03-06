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

/* Song */

#include "nmjedit.h"
#include <string.h>
#include <unistd.h>
#include <gmerlin/utils.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "nmjedit.song"

#include <gavl/metatags.h>

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
  song->albumartist_id = -1;
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
  song->genre_id = bg_sqlite_id_to_id(db, "SONG_GENRES_SONGS",
                                   "GENRES_ID", "SONGS_ID", song->id);
  if(song->genre_id >= 0)
    song->genre = bg_sqlite_id_to_string(db, "SONG_GENRES",
                                      "NAME", "ID", song->genre_id);
  
  song->album_id = bg_sqlite_id_to_id(db, "SONG_ALBUMS_SONGS",
                                   "ALBUMS_ID", "SONGS_ID", song->id);
  if(song->album_id >= 0)
    song->album = bg_sqlite_id_to_string(db, "SONG_ALBUMS",
                                      "TITLE", "ID", song->album_id);

  song->artist_id = bg_sqlite_id_to_id(db, "SONG_PERSONS_SONGS",
                                    "PERSONS_ID", "SONGS_ID", song->id);
  if(song->artist_id >= 0)
    song->artist = bg_sqlite_id_to_string(db, "SONG_PERSONS",
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
  int tag_i;
  
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

  /* Check if there is enough info for a database entry */

  if(!gavl_metadata_get(&ti->metadata, GAVL_META_TITLE))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot load %s: Title missing in metadata", file->path);
    goto fail;
    }

  if(!gavl_metadata_get(&ti->metadata, GAVL_META_ARTIST))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot load %s: Artist missing in metadata", file->path);
    goto fail;
    }

  if(!gavl_metadata_get(&ti->metadata, GAVL_META_ALBUM))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot load %s: Album missing in metadata", file->path);
    goto fail;
    }
  
  /* Fill in the data structure */
  
  song->title = bg_nmj_escape_string(gavl_metadata_get(&ti->metadata, GAVL_META_TITLE));
  song->search_title = bg_nmj_make_search_string(song->title);
  
  song->path = gavl_strrep(song->path, file->path);
  song->scan_dirs_id = dir->id;
  song->runtime = bg_sprintf("%d", (int)(bg_track_info_get_duration(ti) / GAVL_TIME_SCALE));

  song->format  = gavl_strrep(song->format,
                            gavl_metadata_get(&ti->metadata, GAVL_META_FORMAT));
  song->size    = file->size;
  
  if(gavl_metadata_get_int(&ti->audio_streams[0].m, GAVL_META_BITRATE, &tag_i))
    song->bit_rate = bg_sprintf("%d", (int)(tag_i / 1000));
  else
    song->bit_rate = gavl_strrep(song->bit_rate, "VBR");

  if(gavl_metadata_get_int(&ti->metadata, GAVL_META_TRACKNUMBER, &tag_i))
    song->track_position = tag_i;

  year = bg_metadata_get_year(&ti->metadata);
  if(year)
    song->release_date = bg_sprintf("%d-01-01", year);
  else
    song->release_date = gavl_strdup("9999-01-01");
  song->create_time = malloc(BG_NMJ_TIME_STRING_LEN);
  bg_nmj_time_to_string(file->time, song->create_time);

  song->album        =
    bg_nmj_escape_string(gavl_metadata_get(&ti->metadata, GAVL_META_ALBUM));
  song->artist       =
    bg_nmj_escape_string(gavl_metadata_get(&ti->metadata, GAVL_META_ARTIST));
  song->albumartist  =
    bg_nmj_escape_string(gavl_metadata_get(&ti->metadata, GAVL_META_ALBUMARTIST));
  song->genre        =
    bg_nmj_escape_string(gavl_metadata_get(&ti->metadata, GAVL_META_GENRE));

  /* Unknown stuff */
  song->update_state = bg_sprintf("%d", 2);
  
  /* Get IDs */
  if(song->genre)
    song->genre_id     = bg_sqlite_string_to_id(db, "SONG_GENRES", "ID",
                                             "NAME", song->genre);
  
  if(song->albumartist)
    song->albumartist_id    = bg_sqlite_string_to_id(db, "SONG_PERSONS", "ID",
                                                  "NAME", song->albumartist);
  if(song->artist)
    song->artist_id    = bg_sqlite_string_to_id(db, "SONG_PERSONS", "ID",
                                             "NAME", song->artist);
  
  /* Lookup album (title AND albumartist MUST match) */
  if(song->album && (song->albumartist_id >= 0))
    song->album_id = bg_nmj_album_lookup(db, song->albumartist_id, song->album);
  
  ret = 1;
  fail:

  if(plugin)
    plugin->close(h->priv);
  
  if(h)
    bg_plugin_unref(h);
  return ret;
  }

int bg_nmj_song_add(bg_plugin_registry_t * plugin_reg,
                    sqlite3 * db, bg_nmj_song_t * song)
  {
  int result;
  char * sql;
  int64_t id;

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Adding song %s", song->title);
  
  /* Get ID */
  song->id = bg_sqlite_get_next_id(db, "SONGS");
  
  /* Add Song */
  sql = sqlite3_mprintf("INSERT INTO SONGS "
                        "( ID, TITLE, SEARCH_TITLE, PATH, SCAN_DIRS_ID, "
                        "RUNTIME, FORMAT, RATING, SIZE, PLAY_COUNT, BIT_RATE, "
                        "TRACK_POSITION, RELEASE_DATE, CREATE_TIME, UPDATE_STATE ) VALUES "
                        "( %"PRId64", %Q, %Q, %Q, %"PRId64","
                        " %Q, %Q, %"PRId64", %"PRId64", %"PRId64", %Q, "
                        " %"PRId64", %Q, %Q, %Q);",
                        song->id, song->title, song->search_title, song->path, song->scan_dirs_id,
                        song->runtime, song->format, song->rating, song->size, "0", song->bit_rate,
                        song->track_position, song->release_date, song->create_time, song->update_state);
  
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return result;
  
  if(song->genre)
    {
    /* Add new genre */
    if(song->genre_id < 0)
      {
      song->genre_id = bg_sqlite_get_next_id(db, "SONG_GENRES");
      sql = sqlite3_mprintf("INSERT INTO SONG_GENRES ( ID, NAME ) VALUES ( %"PRId64", %Q );",
                            song->genre_id, song->genre);
      result = bg_sqlite_exec(db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return result;
      }
    /* Set Genre */
    id = bg_sqlite_get_next_id(db, "SONG_GENRES_SONGS");
    sql = sqlite3_mprintf("INSERT INTO SONG_GENRES_SONGS ( ID, SONGS_ID, GENRES_ID ) VALUES "
                          "( %"PRId64", %"PRId64", %"PRId64" );",
                          id, song->id, song->genre_id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return result;
    }
  
  if(song->artist)
    {
    /* Add new artist */
    if(song->artist_id < 0)
      {
      song->artist_id = bg_sqlite_get_next_id(db, "SONG_PERSONS");
      sql = sqlite3_mprintf("INSERT INTO SONG_PERSONS ( ID, NAME ) VALUES ( %"PRId64", %Q );",
                            song->artist_id, song->artist);
      result = bg_sqlite_exec(db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return result;
      }

    /* Set artist */
    id = bg_sqlite_get_next_id(db, "SONG_PERSONS_SONGS");
    sql = sqlite3_mprintf("INSERT INTO SONG_PERSONS_SONGS ( ID, PERSONS_ID, SONGS_ID, PERSON_TYPE ) VALUES "
                          "( %"PRId64", %"PRId64", %"PRId64", %Q );",
                          id, song->artist_id, song->id, "ARTIST");
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return result;
    }
  
  if(song->albumartist)
    {
    if(!strcmp(song->artist, song->albumartist))
      song->albumartist_id = song->artist_id;

    /* Add new album artist */
    if(song->albumartist_id < 0)
      {
      song->albumartist_id = bg_sqlite_get_next_id(db, "SONG_PERSONS");
      sql =
        sqlite3_mprintf("INSERT INTO SONG_PERSONS ( ID, NAME ) VALUES ( %"PRId64", %Q );",
                            song->albumartist_id, song->albumartist);
      result = bg_sqlite_exec(db, sql, NULL, NULL);
      sqlite3_free(sql);
      if(!result)
        return result;
      }
    }
  
  /* Album */

  if(song->album && song->albumartist && (song->track_position > 0))
    {
    bg_nmj_album_t album;
    bg_nmj_album_init(&album);
    album.id = song->album_id;
    bg_nmj_album_add(plugin_reg, db, &album, song);
    bg_nmj_album_free(&album);
    }
  
  return 1;
  }

char * bg_nmj_song_get_cover(bg_nmj_song_t * song)
  {
  char * pos;
  char * ret;
  char * directory = gavl_strdup(song->path);
  pos = strrchr(directory, '/');
  if(!pos)
    {
    free(directory);
    return NULL;
    }
  *pos = '\0';
  ret = bg_sprintf("%s/cover.jpg", directory);
  if(!access(ret, R_OK | W_OK))
    {
    free(directory);
    return ret;
    }
  free(directory);
  free(ret);
  return NULL;
  }

#define APPEND_STRING(member, col) \
  if(old_song->member && new_song->member && \
     strcmp(old_song->member, new_song->member))        \
    { \
    if(num_cols)                                  \
      sql = gavl_strcat(sql, ","); \
    tmp_string = sqlite3_mprintf(" " col " = %Q", new_song->member); \
    sql = gavl_strcat(sql, tmp_string);                               \
    sqlite3_free(tmp_string); \
    num_cols++; \
    }

#define APPEND_INT(member, col)           \
  if(old_song->member != new_song->member)        \
    { \
    if(num_cols)                                  \
      sql = gavl_strcat(sql, ","); \
    tmp_string = sqlite3_mprintf(" " col " = %"PRId64, new_song->member); \
    sql = gavl_strcat(sql, tmp_string);                               \
    sqlite3_free(tmp_string); \
    num_cols++; \
    }


int bg_nmj_song_delete(sqlite3 * db, bg_nmj_song_t * song)
  {
  char * sql;
  int result;
  /* Delete from SONGS */
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Deleting song %s", song->title);
  
  sql = sqlite3_mprintf("DELETE FROM SONGS WHERE ID = %"PRId64";", song->id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;
  
  if(song->album_id >= 0)
    {
    /* Delete from album */
    if(!bg_nmj_album_delete(db, song->album_id, song))
      return 0;
    }
  
  /* Delete from SONG_GENRES_SONGS */
  if(song->genre_id >= 0)
    {
    sql = sqlite3_mprintf("DELETE FROM SONG_GENRES_SONGS WHERE SONGS_ID = %"PRId64";",
                          song->id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return 0;
    sql = sqlite3_mprintf("DELETE FROM SONG_GENRES_SONGS WHERE SONGS_ID = %"PRId64";",
                          song->id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return 0;
    }

  /* Delete from SONG_PERSONS_SONGS */
  if(song->artist_id >= 0)
    {
    sql = sqlite3_mprintf("DELETE FROM SONG_PERSONS_SONGS WHERE SONGS_ID = %"PRId64";",
                          song->id);
    result = bg_sqlite_exec(db, sql, NULL, NULL);
    sqlite3_free(sql);
    if(!result)
      return 0;
    }
  return 1;
  }
