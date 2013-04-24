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

void bg_db_audio_file_init(bg_db_audio_file_t * file)
  {
  memset(file, 0, sizeof(*file));
  file->id = -1;
  }

void bg_db_audio_file_free(bg_db_audio_file_t * file)
  {

  }

void bg_db_audio_file_get_info(bg_db_audio_file_t * f, bg_db_file_t * file, bg_track_info_t * t)
  {

  }

int bg_db_audio_file_add(bg_db_t * db, bg_db_audio_file_t * f)
  {
#if 0
  char * sql;
  int result;
  char mtime_str[BG_DB_TIME_STRING_LEN];
  f->id = bg_sqlite_get_next_id(db->db, "FILES");

  /* Mimetype */
  if(f->mimetype)
    {
    f->mimetype_id = 
      bg_sqlite_string_to_id_add(db->db, "MIMETYPES",
                                 "ID", "NAME", f->mimetype);
    }
  /* Mtime */
  bg_db_time_to_string(f->mtime, mtime_str);

#if 1
  sql = sqlite3_mprintf("INSERT INTO FILES ( ID, PATH, SIZE, MTIME, MIMETYPE, TYPE, PARENT_ID, SCAN_DIR_ID ) VALUES ( %"PRId64", %Q, %"PRId64", %Q, %"PRId64", %"PRId64", %"PRId64", %"PRId64" );",
                        f->id, bg_db_filename_to_rel(db, f->path), f->size, mtime_str, f->mimetype_id, f->type, f->parent_id, f->scan_dir_id);

  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
#endif
  return result;
#endif
  }

int bg_db_audio_file_query(bg_db_t * db, bg_db_audio_file_t * f)
  {
  
  }

int bg_db_audio_file_del(bg_db_t * db, bg_db_audio_file_t * f)
  {

  }

