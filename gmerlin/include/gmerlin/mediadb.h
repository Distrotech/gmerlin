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

#include <time.h>
#include <gmerlin/pluginregistry.h>

typedef struct bg_db_s bg_db_t;

typedef enum
  {
  BG_DB_AUDIO = (1<<0),
  BG_DB_VIDEO = (1<<1),
  BG_DB_PHOTO = (1<<2),
  } bg_db_type_t;

typedef enum
  {
  BG_DB_ACCESS_FILE = 0,
  BG_DB_ACCESS_URL  = 1,
  // BG_DB_ACCESS_DEVICE, ?
  // BG_DB_ACCESS_ONDEMAND ?
  } bg_db_access_t;

typedef struct
  {
  int day;   // 1 - 31 (0 = unknown)
  int month; // 1 - 12 (0 = unknown)
  int year;  // 2013
  } bg_db_date_t;

/* Directory on the file system */
typedef struct
  {
  int64_t id;
  char * path;   // Relative to db file
  int scan_type; // ORed flags if BG_DB_* types

  int found; // Used by sqlite
  } bg_db_dir_t;

void bg_db_dir_init(bg_db_dir_t * dir);
void bg_db_dir_free(bg_db_dir_t * dir);
int  bg_db_dir_query(bg_db_t * db, bg_db_dir_t * dir);
void bg_db_dir_add(bg_db_t * db, bg_db_dir_t * dir);

/* File in the file system */
typedef struct
  {
  int64_t id;  // Global id, will be used for upnp also

  char * path;  // Relative to db file if it's a file
  time_t mtime;
  int64_t size;

  int64_t object_id;
  int64_t dir_id;
  char * mimetype;

  int found; // Used by sqlite
  } bg_db_file_t;

/* Audio track */
typedef struct
  {
  int64_t id;

  int64_t album_id;
  int64_t file_id;
  int64_t artist_id;
  int64_t genre_id;

  char * title;
  gavl_time_t duration;
  char * format;
  char * bitrate;
  bg_db_date_t date;
  int track;

  /* Secondary variables */
  char * genre;
  char * album;
  char * albumartist;

  int found; // Used by sqlite
  } bg_db_audio_track_t;

/* Video */
typedef struct
  {
  int64_t id;

  char * title;
  char * genre;
  int year;
  gavl_time_t duration;
  int64_t file_id;

  int found; // Used by sqlite
  } bg_db_video_t;

/* Music Album */
typedef struct
  {
  int64_t id;
  char * artist;
  char * title;
  int tracks;
  int found; // Used by sqlite
  } bg_db_musicalbum_t;

/* Playlist */
typedef struct
  {
  
  } bg_db_playlist_t;


bg_db_t * bg_db_create(const char * file,
                       bg_plugin_registry_t * plugin_reg, int create);

void bg_db_destroy(bg_db_t *);

/* Edit functions */
void bg_db_add_directory(bg_db_t *, const char * dir, int scan_type);
void bg_db_del_directory(bg_db_t *, const char * dir);
