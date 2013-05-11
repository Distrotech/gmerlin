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

#ifndef __MEDIADB_H_
#define __MEDIADB_H_


#include <time.h>
#include <gmerlin/pluginregistry.h>

/*
 *   object.item.imageItem.photo
 *   object.item.audioItem.musicTrack
 *   object.item.audioItem.audioBroadcast
 *   object.item.audioItem.audioBook
 *   object.item.videoItem.movie
 *   object.item.videoItem.videoBroadcast
 *   object.item.videoItem.musicVideoClip
 *   object.item.playlistItem
 *   object.item.textItem
 *   object.container
 *   object.container.storageFolder
 *   object.container.person
 *   object.container.person.musicArtist
 *   object.container.playlistContainer
 *   object.container.album.musicAlbum
 *   object.container.album.photoAlbum
 *   object.container.genre.musicGenre
 *   object.container.genre.movieGenre
 */

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

/*
 *  Date
 */

typedef struct
  {
  int day;   // 1 - 31 (0 = unknown)
  int month; // 1 - 12 (0 = unknown)
  int year;  // 2013
  } bg_db_date_t;

int bg_db_date_equal(const bg_db_date_t * d1,
                     const bg_db_date_t * d2);

void bg_db_date_copy(bg_db_date_t * dst,
                     const bg_db_date_t * src);

#define BG_DB_DATE_STRING_LEN GAVL_METADATA_DATE_STRING_LEN

void bg_db_date_to_string(const bg_db_date_t * d, char * str);

void bg_db_string_to_date(const char * str, bg_db_date_t * d);

void bg_db_date_set_invalid(bg_db_date_t * d);

/*
 *  Object definitions
 */

#define BG_DB_FLAG_CONTAINER  (1<<0)  // Real container
#define BG_DB_FLAG_NO_EMPTY   (1<<1)  // Auto delete when empty
#define BG_DB_FLAG_VCONTAINER (1<<2) // Virtual container (needs browse_children method)
#define DB_DB_FLAG_FILE       (1<<3) // Derived from file
#define DB_DB_FLAG_IMAGE      (1<<4) // Derived from image

typedef enum
  {
  BG_DB_OBJECT_OBJECT       = (0<<16),
  BG_DB_OBJECT_FILE         = (1<<16) | DB_DB_FLAG_FILE,
  // object.item.audioItem.musicTrack
  BG_DB_OBJECT_AUDIO_FILE   = (2<<16) | DB_DB_FLAG_FILE,
  BG_DB_OBJECT_VIDEO_FILE   = (3<<16) | DB_DB_FLAG_FILE,
  BG_DB_OBJECT_IMAGE_FILE   = (4<<16) | DB_DB_FLAG_FILE | DB_DB_FLAG_IMAGE,
  // Several image types follow
  BG_DB_OBJECT_PHOTO         = (5<<16) | DB_DB_FLAG_FILE | DB_DB_FLAG_IMAGE,
  BG_DB_OBJECT_ALBUM_COVER   = (6<<16) | DB_DB_FLAG_FILE | DB_DB_FLAG_IMAGE,
  BG_DB_OBJECT_VIDEO_PREVIEW = (7<<16) | DB_DB_FLAG_FILE | DB_DB_FLAG_IMAGE,
  BG_DB_OBJECT_MOVIE_POSTER  = (8<<16) | DB_DB_FLAG_FILE | DB_DB_FLAG_IMAGE,
  BG_DB_OBJECT_THUMBNAIL     = (9<<16) | DB_DB_FLAG_FILE | DB_DB_FLAG_IMAGE,

  // Root container: Object ID 0, has all toplevel containers as children

  BG_DB_OBJECT_ROOT          = (50<<16) | BG_DB_FLAG_VCONTAINER,

  // object.container.album.musicAlbum
  // Audio albums are *no* containers for the internal database
  // because they are referenced by their children not by the parent_id
  // but by the album
  BG_DB_OBJECT_AUDIO_ALBUM =  (51<<16) | BG_DB_FLAG_VCONTAINER,

  BG_DB_OBJECT_CONTAINER   =  (52<<16) | BG_DB_FLAG_CONTAINER,
  // object.container.storageFolder
  BG_DB_OBJECT_DIRECTORY   =  (53<<16) | BG_DB_FLAG_CONTAINER, 
  BG_DB_OBJECT_PLAYLIST    =  (54<<16) | BG_DB_FLAG_VCONTAINER,

  // Virtual Folder */
  BG_DB_OBJECT_VFOLDER      =  (55<<16)  | BG_DB_FLAG_CONTAINER | BG_DB_FLAG_NO_EMPTY,
  BG_DB_OBJECT_VFOLDER_LEAF =  (56<<16) | BG_DB_FLAG_VCONTAINER | BG_DB_FLAG_NO_EMPTY,
  } bg_db_object_type_t;

typedef struct bg_db_object_class_s bg_db_object_class_t;

typedef struct
  {
  int64_t id;
  bg_db_object_type_t type;
  
  int64_t ref_id;    // Original ID
  int64_t parent_id;  // ID of the real parent
  int children;      // For containers only

  int64_t size;
  gavl_time_t duration;
  char * label;

  /* Internal stuff, do not touch */

  int found;    // Used by sqlite
  int flags;    // Used in-memory only
  const bg_db_object_class_t * klass;

  } bg_db_object_t;

/* Directory on the file system */
typedef struct
  {
  bg_db_object_t obj;
  char * path;   // Relative to db file
  int scan_flags; // ORed flags if BG_DB_* types
  int update_id;
  int64_t scan_dir_id;
  } bg_db_dir_t;

/* File in the file system */
typedef struct
  {
  bg_db_object_t obj;
  char * path;  // Relative to db file
  time_t mtime;
  char * mimetype;
  int64_t mimetype_id;
  int64_t scan_dir_id;
  } bg_db_file_t;

/* 
 * Audio file 
 */

typedef struct
  {
  bg_db_file_t file;
  
  char * title;      // TITLE
  char * search_title;

  char * artist;     
  int64_t artist_id; // ARTIST

  char * genre;      // GENRE
  int64_t genre_id;

  bg_db_date_t date;

  char * album;
  int64_t album_id;

  int track;
  
  char * bitrate;    // BITRATE
  int samplerate;
  int channels; 

  char * albumartist;     // Not in db, set only while creating
  int64_t albumartist_id; // Not in db, set only while creating  
  } bg_db_audio_file_t;

/* Audio Album */
typedef struct
  {
  bg_db_object_t obj;

  char * artist;
  int64_t artist_id;
  char * title;
  char * search_title;
  
  char * genre;      // GENRE
  int64_t genre_id;

  bg_db_date_t date;

  int64_t cover_id;
  } bg_db_audio_album_t;

/* Image */

typedef struct
  {
  bg_db_file_t file;
  int width;
  int height;
  int64_t context_id; // Context where this item belongs
  bg_db_date_t date;  // Date from EXIF
  } bg_db_image_file_t;

/* Virtual audio folder */

typedef enum
  {
    BG_DB_CAT_YEAR              = 1,
    BG_DB_CAT_GROUP             = 2,
    BG_DB_CAT_ARTIST            = 3,
    BG_DB_CAT_GENRE             = 4,
  } bg_db_category_t;

#define BG_DB_VFOLDER_MAX_DEPTH 8

typedef struct
  {
  bg_db_category_t cat;
  int64_t val;
  } bg_db_vfolder_path_t;

typedef struct
  {
  bg_db_object_t obj;

  int depth;
  bg_db_object_type_t type;
  
  bg_db_vfolder_path_t path[BG_DB_VFOLDER_MAX_DEPTH];
 
  } bg_db_vfolder_t;

/* Video */
typedef struct
  {
  bg_db_file_t file;
  char * title;
  char * genre;
  int64_t genre_id;
  bg_db_date_t date;
  } bg_db_video_file_t;

/* Playlist */
typedef struct
  {
  
  } bg_db_playlist_t;

/*
 *  Public entry points
 */

bg_db_object_type_t bg_db_object_get_type(void * obj);

bg_db_t * bg_db_create(const char * file,
                       bg_plugin_registry_t * plugin_reg, int create);

void bg_db_flush(bg_db_t * db);

void bg_db_destroy(bg_db_t *);

/* Edit functions */
void bg_db_add_directory(bg_db_t *, const char * dir, int scan_type);
void bg_db_del_directory(bg_db_t *, const char * dir);

/* Query functions */

typedef void (*bg_db_query_callback)(void * priv, void * obj);

/* Query from DB  */
void * bg_db_object_query(bg_db_t * db, int64_t id); 
int64_t bg_db_object_get_id(void * obj);


void bg_db_object_dump(void * obj);
void bg_db_object_unref(void * obj);
void bg_db_object_ref(void * obj);

const char * bg_db_object_get_label(void * obj);

int
bg_db_query_children(bg_db_t *, int64_t id, bg_db_query_callback cb, void * priv,
                     int start, int num, int * total_matches);

/* Get thumbnails */

void bg_db_browse_thumbnails(bg_db_t * db, int64_t id, 
                             bg_db_query_callback cb, void * data);

void * bg_db_get_thumbnail(bg_db_t * db, int64_t id,
                           int max_width, int max_height, int force,
                           const char * mimetype);


#endif //  __MEDIADB_H_
