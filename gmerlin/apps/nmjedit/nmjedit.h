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

#include <sqlite3.h>
#include <inttypes.h>

/* Directory */

typedef struct
  {
  int64_t id;
  char * directory; // Relative path
  char * name;      // Absolute path??
  char * scan_time; // NULL?
  int64_t size;     // Sum of all file sizes in bytes
  int64_t category; // 40 for Music
  char * status;    // 3 (??)
  } bg_nmj_dir_t;

void bg_nmj_dir_init(bg_nmj_dir_t*);
void bg_nmj_dir_free(bg_nmj_dir_t*);
void bg_nmj_dir_dump(bg_nmj_dir_t*);
int bg_nmj_dir_query(sqlite3*, bg_nmj_dir_t*);
int bg_nmj_dir_save(sqlite3*, bg_nmj_dir_t*);


/* Song structure */

typedef struct
  {
  /* SONGS */
  int64_t id;
  char * title;
  char * search_title;
  char * path;
  int64_t scan_dirs_id;
  int64_t folders_id;   // Unused? */
  char * runtime;
  char * format;
  char * lyric;
  int64_t rating;   // default: 0
  char * hash;      // Unused?
  int64_t size;
  char * bit_rate;
  int64_t track_position; // 1..
  char * release_date;    // YYYY-01-01
  char * create_time;     // 2012-03-21 23:43:16
  char * update_state;    // "2" or "5"
  char * filestatus;      // unused

  /* Stuff for other databases */
  int64_t album_id;
  int64_t artist_id;
  int64_t genre_id;
  
  } bg_nmj_song_t;

// void bg_nmj_song_load_from_file(sqlite3 * db, bg_nmj_song_t * song, const char * file);
// void bg_nmj_song_load_from_db(sqlite3 * db, bg_nmj_song_t * song, int64_t id);
// void bg_nmj_song_save_to_db(sqlite3 * db, bg_nmj_song_t * song);
// void bg_nmj_song_delete_from_db(sqlite3 * db, bg_nmj_song_t * song);
void bg_nmj_song_free(bg_nmj_song_t * song);
void bg_nmj_song_init(bg_nmj_song_t * song);
void bg_nmj_song_dump(bg_nmj_song_t * song);
int bg_nmj_song_query(sqlite3 * db, bg_nmj_song_t * song);
int bg_nmj_song_save(sqlite3 * db, bg_nmj_song_t * song);

/* Album */

typedef struct
  {
  int64_t id;
  char * title;
  char * search_title;
  char * total_item;
  char * release_date;
  char * update_state; // 3??
  } bg_nmj_album_t;

void bg_nmj_album_init(bg_nmj_album_t *);
void bg_nmj_album_free(bg_nmj_album_t *);
void bg_nmj_album_dump(bg_nmj_album_t *);
int bg_nmj_album_query(bg_nmj_album_t *);
int bg_nmj_album_save(bg_nmj_album_t *);

/* Master functions */

#define BG_NMJ_MEDIA_TYPE_AUDIO (1<<0)
#define BG_NMJ_MEDIA_TYPE_VIDEO (1<<1)
#define BG_NMJ_MEDIA_TYPE_IMAGE (1<<2)

int bg_nmj_add_directory(sqlite3 * db, const char * directory, int types);
int bg_nmj_remove_directory(sqlite3 * db, const char * directory, int types);

