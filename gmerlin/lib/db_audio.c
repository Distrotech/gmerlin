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

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define LOG_DOMAIN "db.audiofile"

static void free_audio_file(void * obj)
  {
  bg_db_audio_file_t * file = obj;
  if(file->title)
    free(file->title);
  if(file->bitrate)
    free(file->bitrate);
  if(file->genre)
    free(file->genre);
  if(file->album)
    free(file->album);
  if(file->albumartist)
    free(file->albumartist);
  }

static void del_audio_file(bg_db_t * db, bg_db_object_t * obj) // Delete from db
  {
  bg_sqlite_delete_by_id(db->db, "FILES", obj->id);
  bg_sqlite_delete_by_id(db->db, "AUDIO_FILES", obj->id);
  }

static int audio_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_audio_file_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_STRING("TITLE",    title);
    BG_DB_SET_QUERY_INT("ARTIST",      artist_id);
    BG_DB_SET_QUERY_INT("GENRE",       genre_id);
    BG_DB_SET_QUERY_DATE("DATE",       date);
    BG_DB_SET_QUERY_INT("ALBUM",       album_id);
    BG_DB_SET_QUERY_INT("TRACK",       track);
    BG_DB_SET_QUERY_STRING("BITRATE",  bitrate);
    }
  ret->file.obj.found = 1;
  return 0;
  }

static int query_audio_file(bg_db_t * db, void * obj, int full)
  {
  char * sql;
  int result;
  bg_db_audio_file_t * f = obj;
  
  f->file.obj.found = 0;
  sql =
    sqlite3_mprintf("select * from AUDIO_FILES where ID = %"PRId64";", bg_db_object_get_id(f));
  result = bg_sqlite_exec(db->db, sql, audio_query_callback, f);
  sqlite3_free(sql);
  if(!result || f->file.obj.found)
    return 0;
  if(!full)
    return 1;

  f->artist = bg_sqlite_id_to_string(db->db, "AUDIO_ARTISTS", "NAME", "ID", f->artist_id);
  f->genre  = bg_sqlite_id_to_string(db->db, "AUDIO_GENRES",  "NAME", "ID", f->genre_id);
  f->album  = bg_sqlite_id_to_string(db->db, "AUDIO_ALBUMS",  "NAME", "ID", f->album_id);
  return 1;
  }

const bg_db_object_class_t bg_db_audio_file_class =
  {
  .del = del_audio_file,
  .free = free_audio_file,
  .query = query_audio_file,
  .parent = &bg_db_file_class,
  };

/* Add to db */
static void audio_file_add(bg_db_t * db, bg_db_audio_file_t * f)
  {
  char * sql;
  int result;

  char date_string[BG_DB_DATE_STRING_LEN];

  /* Date */
  bg_db_date_to_string(&f->date, date_string);
  
#if 0
  /* Album */
  if(f->album && f->albumartist)
    {
    /* Sets album ID */
    bg_db_audio_file_add_to_album(db, f);
    }
#endif
  
  sql = sqlite3_mprintf("INSERT INTO AUDIO_FILES ( ID, TITLE, ARTIST, GENRE, DATE, ALBUM, TRACK, BITRATE ) "
                        "VALUES"
                        " ( %"PRId64", %Q, %"PRId64", %"PRId64", %Q, %"PRId64", %d, %Q );",
                        bg_db_object_get_id(f) , f->title, f->artist_id, f->genre_id, date_string, f->album_id, f->track, f->bitrate);

  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  }

void bg_db_audio_file_create(bg_db_t * db, void * obj, bg_track_info_t * t)
  {
  gavl_time_t duration;
  int bitrate = 0;
  const char * var;
  bg_db_audio_file_t * f = obj;
  
  bg_db_object_set_type(obj, BG_DB_OBJECT_AUDIO_FILE);
  
  if((var = gavl_metadata_get(&t->metadata, GAVL_META_TITLE)))
    f->title = gavl_strdup(var);
  if((var = gavl_metadata_get(&t->metadata, GAVL_META_ARTIST)))
    f->artist = gavl_strdup(var);
  if((var = gavl_metadata_get(&t->metadata, GAVL_META_ALBUMARTIST)))
    f->albumartist = gavl_strdup(var);
  if((var = gavl_metadata_get(&t->metadata, GAVL_META_GENRE)))
    f->genre = gavl_strdup(var);
  
  f->date.year = bg_metadata_get_year(&t->metadata);
    
  if((var = gavl_metadata_get(&t->metadata, GAVL_META_ALBUM)))
    f->album = gavl_strdup(var);

  gavl_metadata_get_int(&t->metadata, GAVL_META_TRACKNUMBER, &f->track);

  gavl_metadata_get_int(&t->audio_streams[0].m, GAVL_META_BITRATE, &bitrate);

  if(bitrate == GAVL_BITRATE_VBR)
    f->bitrate = gavl_strdup("VBR");
  else if(bitrate <= 0)
    f->bitrate = gavl_strdup("Unknown");
  else
    f->bitrate = bg_sprintf("%d", bitrate / 1000);

  /* Artist */
  if(f->artist)
    {
    f->artist_id = 
      bg_sqlite_string_to_id_add(db->db, "AUDIO_ARTISTS",
                                 "ID", "NAME", f->artist);
    }

  /* Genre */
  if(f->genre)
    {
    f->genre_id = 
      bg_sqlite_string_to_id_add(db->db, "AUDIO_GENRES",
                                 "ID", "NAME", f->genre);
    }
  bg_db_object_update(db, f, 0);
  
  audio_file_add(db, f);
  }

