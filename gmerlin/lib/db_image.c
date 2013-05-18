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

#include <mediadb_private.h>
#include <gmerlin/log.h>
#include <gmerlin/utils.h>
#include <string.h>

#define LOG_DOMAIN "db.image"

static int image_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_image_file_t * ret = data;

  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_INT("WIDTH",   width);
    BG_DB_SET_QUERY_INT("HEIGHT",  height);
    BG_DB_SET_QUERY_DATE("DATE",   date);
    }
  ret->file.obj.found = 1;
  return 0;
  }

static int query_image(bg_db_t * db, void * a1, int full)
  {
  char * sql;
  int result;
  bg_db_object_t * a = a1;
  
  a->found = 0;
  sql = sqlite3_mprintf("select * from IMAGE_FILES where ID = %"PRId64";", bg_db_object_get_id(a));
  result = bg_sqlite_exec(db->db, sql, image_query_callback, a);
  sqlite3_free(sql);
  if(!result || !a->found)
    return 0;
  return 1;
  }

static void update_image(bg_db_t * db, void * obj)
  {
  bg_db_image_file_t * a = obj;
  char * sql;
  int result;
  char date_string[BG_DB_DATE_STRING_LEN];
  
  bg_db_date_to_string(&a->date, date_string);
  
  sql = sqlite3_mprintf("UPDATE IMAGE_FILES SET DATE = %Q, WIDTH = %d, HEIGHT = %d WHERE ID = %"PRId64";",
                        date_string, a->width, a->height, bg_db_object_get_id(a));
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  return;
  }


static void del_image(bg_db_t * db, bg_db_object_t * obj) // Delete from db
  {
  bg_sqlite_delete_by_id(db->db, "IMAGE_FILES", obj->id);
  }

static void free_image(void * obj)
  {

  }

static void dump_image(void * obj)
  {
  bg_db_image_file_t*a = obj;
  gavl_diprintf(2, "Size:   %dx%d\n", a->width, a->height);
  gavl_diprintf(2, "Date:   %04d-%02d-%02d\n", a->date.year, a->date.month, a->date.day);
  }

const bg_db_object_class_t bg_db_image_file_class =
  {
    .name = "Image file",
    .del = del_image,
    .free = free_image,
    .query = query_image,
    .update = update_image,
    .dump = dump_image,
    .parent = &bg_db_file_class, // Object
  };

const bg_db_object_class_t bg_db_thumbnail_class =
  {
    .name = "Thumbnail",
    .parent = &bg_db_image_file_class, // Object
  };


const bg_db_object_class_t bg_db_album_cover_class =
  {
    .name = "Album cover",
#if 0
    .del = del_image,
    .free = free_image,
    .query = query_image,
    .update = update_image,
    .dump = dump_image,
#endif
    .parent = &bg_db_image_file_class, // Object
  };

void bg_db_image_file_create_from_ti(bg_db_t * db, void * obj, bg_track_info_t * ti)
  {
  char * sql;
  int result;
  char date_string[BG_DB_DATE_STRING_LEN];
  bg_db_image_file_t * f = obj;
  bg_db_object_set_type(obj, BG_DB_OBJECT_IMAGE_FILE);
  
  f->width  = ti->video_streams[0].format.image_width;
  f->height = ti->video_streams[0].format.image_height;

  /* Date */
  bg_db_date_to_string(&f->date, date_string);

  /* Creation date (comes from exit data) */
  if(!gavl_metadata_get_date(&ti->metadata, GAVL_META_DATE_CREATE, &f->date.year, &f->date.month, &f->date.day))
    bg_db_date_set_invalid(&f->date);

  sql = sqlite3_mprintf("INSERT INTO IMAGE_FILES ( ID, WIDTH, HEIGHT, DATE ) VALUES ( %"PRId64", %d, %d, %Q);",
                          bg_db_object_get_id(f), f->width, f->height, date_string);
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  }

static int detect_album_cover(bg_db_t * db, bg_db_image_file_t * f)
  {
  int result = 0;
  int found = 0;
  char * sql;
  int id = -1;
  const char * basename;
  bg_db_audio_file_t * song = NULL;
  bg_db_audio_album_t * album = NULL;

  /* Check if there are music files in the same directory */
  sql = sqlite3_mprintf("SELECT ID FROM OBJECTS WHERE TYPE = %"PRId64" and PARENT_ID =  %"PRId64";",
                        BG_DB_OBJECT_AUDIO_FILE, f->file.obj.parent_id);
  result = bg_sqlite_exec(db->db, sql, bg_sqlite_int_callback, &id);
  sqlite3_free(sql);
  if(!result || (id <= 0))
    goto end;

  /* Check if the file has a valid album */
  song = bg_db_object_query(db, id);
  if(!song || (song->album_id <= 0))
    goto end;

  album = bg_db_object_query(db, song->album_id);
  if(album->cover_id > 0)
    goto end;

  basename = bg_db_object_get_label(f);
  if(!strcasecmp(basename, "cover") ||
     !strcasecmp(basename, "folder") ||
     !strcasecmp(basename, album->title))
    found = 1;

  end:

  if(found)
    {
    bg_db_object_t * thumb;
    
    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using %s as cover for album %s",
           f->file.path, album->title);
    album->cover_id = bg_db_object_get_id(f);
    bg_db_object_set_type(f, BG_DB_OBJECT_ALBUM_COVER);
    
    thumb = bg_db_get_thumbnail(db, album->cover_id, 160, 160, 1, "image/jpeg");
    bg_db_object_unref(thumb);
    
    }
  if(song)
    bg_db_object_unref(song);
  if(album)
    bg_db_object_unref(album);
  return found;
  }

void bg_db_identify_images(bg_db_t * db, int64_t scan_dir_id, int scan_flags)
  {
  int i;

  bg_db_image_file_t * f;

  bg_sqlite_id_tab_t tab;
  bg_sqlite_id_tab_init(&tab);

  /* Get all unidentified images from the scan directory */  

  if(!bg_sqlite_select_join(db->db, &tab,
                            "OBJECTS",
                            "TYPE",
                            BG_DB_OBJECT_IMAGE_FILE,
                            "FILES",
                            "SCAN_DIR_ID",
                            scan_dir_id))
    return;

  for(i = 0; i < tab.num_val; i++)
    {
    f = bg_db_object_query(db, tab.val[i]);
 
    if((scan_flags & BG_DB_SCAN_AUDIO) && detect_album_cover(db, f))
      {
      bg_db_object_unref(f);
      continue;
      }
    bg_db_object_unref(f);
    }
  bg_sqlite_id_tab_free(&tab);
  }

