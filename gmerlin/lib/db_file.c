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


#define LOG_DOMAIN "db.file"

static int file_add(bg_db_t * db, bg_db_file_t * f)
  {
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
  sql = sqlite3_mprintf("INSERT INTO FILES ( ID, PATH, SIZE, MTIME, MIMETYPE, TYPE, PARENT_ID, SCAN_DIR_ID ) VALUES ( %"PRId64", %Q, %"PRId64", %Q, %"PRId64", %d, %"PRId64", %"PRId64" );",
                        f->id, bg_db_filename_to_rel(db, f->path), f->size, mtime_str, f->mimetype_id, f->type, f->parent_id, f->scan_dir_id);

  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
#endif
  return result;
  }


/* Open the file, load metadata and so on */
static void create_file(bg_db_t * db, bg_db_file_t * file, int scan_flags)
  {
  int i;
  bg_track_info_t * ti;
  bg_plugin_handle_t * h = NULL;
  bg_input_plugin_t * plugin = NULL;

  /* Load all infos */  
  if(!bg_input_plugin_load(db->plugin_reg, file->path, NULL, &h, NULL, 0))
    goto fail;
  plugin = (bg_input_plugin_t *)h->plugin;
  
  /* Only one track supported */
  if(plugin->get_num_tracks && (plugin->get_num_tracks(h->priv) < 1))
    goto fail;

  ti = plugin->get_track_info(h->priv, 0);

  /* Detect file type */
  if((ti->num_audio_streams == 1) && !ti->num_video_streams)
    file->type = BG_DB_AUDIO;
  else if(ti->num_video_streams == 1)
    {
    if(!ti->num_audio_streams && (ti->num_video_streams == 1) &&
       (ti->video_streams[0].format.framerate_mode = GAVL_FRAMERATE_STILL))
      file->type = BG_DB_PHOTO;
    else
      file->type = BG_DB_VIDEO;
    }

  file->mimetype =
    gavl_strdup(gavl_metadata_get(&ti->metadata, GAVL_META_MIMETYPE));

  /* Add to DB */
  if(!file_add(db, file))
    goto fail;

  if(!(file->type & scan_flags))
    goto fail;

  if(plugin->set_track && !plugin->set_track(h->priv, 0))
    goto fail;
  
  
  /* Start everything */
  if(plugin->set_audio_stream)
    {
    for(i = 0; i < ti->num_audio_streams; i++)
      plugin->set_audio_stream(h->priv, i, BG_STREAM_ACTION_DECODE);
    }
  if(plugin->set_video_stream)
    {
    for(i = 0; i < ti->num_video_streams; i++)
      plugin->set_video_stream(h->priv, i, BG_STREAM_ACTION_DECODE);
    }
  if(plugin->set_text_stream)
    {
    for(i = 0; i < ti->num_text_streams; i++)
      plugin->set_text_stream(h->priv, i, BG_STREAM_ACTION_DECODE);
    }
  if(plugin->set_overlay_stream)
    {
    for(i = 0; i < ti->num_overlay_streams; i++)
      plugin->set_overlay_stream(h->priv, i, BG_STREAM_ACTION_DECODE);
    }
  if(plugin->start && !plugin->start(h->priv))
    goto fail;
  
  /* Create derived type */
  switch(file->type)
    {
    case BG_DB_AUDIO:
      {
      bg_db_audio_file_t song;
      bg_db_audio_file_init(&song);
      bg_db_audio_file_get_info(&song, file, ti);
      bg_db_audio_file_add(db, &song);
      bg_db_audio_file_free(&song);
      }
      break;
    case BG_DB_VIDEO:
    case BG_DB_PHOTO:
     break;
    }
  fail:

  if(plugin)
    plugin->close(h->priv);
  
  if(h)
    bg_plugin_unref(h);
  }

void bg_db_file_init(bg_db_file_t * file)
  {
  memset(file, 0, sizeof(*file));
  file->id = -1;
  }

void bg_db_file_free(bg_db_file_t * file)
  {
  if(file->path)
    free(file->path);
  }

static int file_query_callback(void * data, int argc, char **argv, char **azColName)
  {
  int i;
  bg_db_file_t * ret = data;
  
  for(i = 0; i < argc; i++)
    {
    BG_DB_SET_QUERY_INT("ID",          id);
    BG_DB_SET_QUERY_STRING("PATH",     path);
    BG_DB_SET_QUERY_INT("SIZE",        size);
    BG_DB_SET_QUERY_MTIME("MTIME",     mtime);
    BG_DB_SET_QUERY_INT("MIMETYPE",    mimetype_id);
    BG_DB_SET_QUERY_INT("TYPE",        type);
    BG_DB_SET_QUERY_INT("PARENT_ID",   parent_id);
    BG_DB_SET_QUERY_INT("SCAN_DIR_ID", scan_dir_id);
    }
  ret->found = 1;
  return 0;
  }


int bg_db_file_query(bg_db_t * db, bg_db_file_t * f, int full)
  {
  char * sql;
  int result;
  f->found = 0;
  if(f->id >= 0)
    {
    sql = sqlite3_mprintf("select * from FILES where ID = %"PRId64";",
                          f->id);
    result = bg_sqlite_exec(db->db, sql, file_query_callback, f);
    sqlite3_free(sql);
    if(!result || !f->found)
      return 0;
    }
  else if(f->path)
    {
    sql = sqlite3_mprintf("select * from FILES where PATH = %Q;",
                          f->path);
    result = bg_sqlite_exec(db->db, sql, file_query_callback, f);
    sqlite3_free(sql);
    if(!result || !f->found)
      return 0;
    }   
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Either ID or path must be set in path");
    }
  f->path = bg_db_filename_to_abs(db, f->path);

  if(full)
    f->mimetype = bg_sqlite_id_to_string(db->db, "MIMETYPES", "NAME", "ID", f->mimetype_id);
  }

int bg_db_file_del(bg_db_t * db, bg_db_file_t * f)
  {
  char * sql;
  int result;

  switch(f->type)
    {
    case BG_DB_AUDIO:
      {
      bg_db_audio_file_t song;
      bg_db_audio_file_init(&song);
      song.id = f->id;
      if(bg_db_audio_file_query(db, &song, 0))
        bg_db_audio_file_del(db, &song);
      bg_db_audio_file_free(&song);
      }
      break;
    case BG_DB_VIDEO:
    case BG_DB_PHOTO:
     break;
    }
  sql = sqlite3_mprintf("DELETE FROM FILES WHERE ID = %"PRId64";", f->id);
  result = bg_sqlite_exec(db->db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;
  return 1;
  }


void bg_db_add_files(bg_db_t * db, bg_db_scan_item_t * files, int num, int scan_flags)
  {
  int i;

  for(i = 0; i < num; i++)
    {
    switch(files[i].type)
      {
      case BG_SCAN_TYPE_DIRECTORY:
        {
        bg_db_dir_t dir;
        bg_db_dir_init(&dir);
        dir.scan_flags = scan_flags;
        dir.path = files[i].path;
        dir.size = files[i].size;
        dir.parent_id = files[files[i].parent_index].id;
        dir.scan_dir_id = files[0].id;
        files[i].path = NULL;
        bg_db_dir_add(db, &dir);
        files[i].id = dir.id;
        bg_db_dir_free(&dir);
        }
        break;
      case BG_SCAN_TYPE_FILE:
        {
        bg_db_file_t file;
        bg_db_file_init(&file);
        file.path = files[i].path;
        file.size = files[i].size;
        file.mtime = files[i].mtime;
        files[i].path = NULL;
        file.parent_id = files[files[i].parent_index].id;
        file.scan_dir_id = files[0].id;
        create_file(db, &file, scan_flags);
        bg_db_file_free(&file);
        }
        break;
      }
    }
  }

static bg_db_scan_item_t * find_by_path(bg_db_scan_item_t * files, int num, char * path)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    if(!strcmp(files[i].path, path))
      return &files[i];
    }
  return NULL;
  }

void bg_db_update_files(bg_db_t * db, bg_db_scan_item_t * files, int num, int scan_flags,
                        int64_t scan_dir_id)
  {
  char * sql;
  int result;
  bg_db_file_t file;
  int i;
  bg_db_scan_item_t * si;
  
  bg_sqlite_id_tab_t tab;
  bg_sqlite_id_tab_init(&tab);

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Getting songs from database");
  
  sql =
    sqlite3_mprintf("select ID from FILES where SCAN_DIR_ID = %"PRId64";", scan_dir_id);
  result = bg_sqlite_exec(db->db, sql, bg_sqlite_append_id_callback, &tab);
  sqlite3_free(sql);
  
  if(!result)
    goto fail;
  
  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Found %d files in database", tab.num_val);

  for(i = 0; i < tab.num_val; i++)
    {
    bg_db_file_init(&file);
    file.id = tab.val[i];

    if(!bg_db_file_query(db, &file, 1))
      goto fail;

    si = find_by_path(files, num, file.path);

    if(!si || (si->mtime != file.mtime))
      bg_db_file_del(db, &file);
    }
  
  fail:
  bg_sqlite_id_tab_free(&tab);
  
  }
