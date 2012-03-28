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

#include "nmjedit.h"

#include <gmerlin/log.h>
#include <gmerlin/translation.h>
#include <gmerlin/utils.h>
#define LOG_DOMAIN "nmjedit"

#include <string.h>
#include <ctype.h>

int
bg_sqlite_exec(sqlite3 * db,                              /* An open database */
               const char *sql,                           /* SQL to be evaluated */
               int (*callback)(void*,int,char**,char**),  /* Callback function */
               void * data)                               /* 1st argument to callback */
  {
  char * err_msg;
  int err;

  err = sqlite3_exec(db, sql, callback, data, &err_msg);

  //  fprintf(stderr, "Sending %s\n", sql);

  if(err)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "SQL query: \"%s\" failed: %s",
           sql, err_msg);
    sqlite3_free(err_msg);
    return 0;
    }
  return 1;
  }

/* Callbacks */

static int id_callback(void * data, int argc, char **argv, char **azColName)
  {
  int64_t * ret = data;
  if(argv[0])
    *ret = strtoll(argv[0], NULL, 10);
  return 0;
  }

static int string_callback(void * data, int argc, char **argv, char **azColName)
  {
  char ** ret = data;
  if((argv[0]) && (*(argv[0]) != '\0'))
    *ret = strdup(argv[0]);
  return 0;
  }

typedef struct
  {
  int64_t * val;
  int val_alloc;
  int num_val;
  } append_int_t;

static int
append_int_callback(void * data, int argc, char **argv, char **azColName)
  {
  int64_t ret;
  append_int_t * val = data;
  if(argv[0])
    {
    ret = strtoll(argv[0], NULL, 10);
    if(val->num_val + 1 > val->val_alloc)
      {
      val->val_alloc = val->num_val + 128;
      val->val = realloc(val->val, val->val_alloc * sizeof(*val->val));
      }
    val->val[val->num_val] = ret;
    val->num_val++;
    }
  return 0;
  }

int64_t bg_nmj_string_to_id(sqlite3 * db,
                            const char * table,
                            const char * id_row,
                            const char * string_row,
                            const char * str)
  {
  char * buf;
  int64_t ret = -1;
  int result;
  buf = sqlite3_mprintf("select %s from %s where %s = %Q;",
                        id_row, table, string_row, str);
  result = bg_sqlite_exec(db, buf, id_callback, &ret);
  sqlite3_free(buf);
  return result ? ret : -1 ;
  }

char * bg_nmj_id_to_string(sqlite3 * db,
                           const char * table,
                           const char * string_row,
                           const char * id_row,
                           int64_t id)
  {
  char * buf;
  char * ret = NULL;
  int result;
  buf = sqlite3_mprintf("select %s from %s where %s = %"PRId64";",
                        string_row, table, id_row, id);
  result = bg_sqlite_exec(db, buf, string_callback, &ret);
  return result ? ret : NULL;
  }

int64_t bg_nmj_id_to_id(sqlite3 * db,
                        const char * table,
                        const char * dst_row,
                        const char * src_row,
                        int64_t id)
  {
  char * buf;
  int64_t ret = -1;
  int result;
  buf = sqlite3_mprintf("select %s from %s where %s = %"PRId64";",
                        dst_row, table, src_row, id);
  result = bg_sqlite_exec(db, buf, id_callback, &ret);
  return result ? ret : -1;
  }

int64_t bg_nmj_get_next_id(sqlite3 * db, const char * table)
  {
  int result;
  char * sql;
  int64_t ret = 0;

  sql = sqlite3_mprintf("select max(id) from %s;", table);
  result = bg_sqlite_exec(db, sql, id_callback, &ret);
  sqlite3_free(sql);
  if(!result)
    return -1;

  if(ret < 0)
    return -1;

  return ret + 1;
  }

int64_t bg_nmj_count_id(sqlite3 * db,
                        const char * table,
                        const char * id_row,
                        int64_t id)
  {
  int result;
  char * sql;
  int64_t ret = 0;

  sql = sqlite3_mprintf("select count(*) from %s where %s = %"PRId64";",
                        table, id_row, id);
  result = bg_sqlite_exec(db, sql, id_callback, &ret);
  sqlite3_free(sql);
  if(!result)
    return -1;
  return ret;
  }

static const struct
  {
  char c;
  char * escaped;
  }
escape_rules[] =
  {
    { '\'', "&apos;" },
    { '&',  "&amp;" },
    { /* End */      }
  };
  
char * bg_nmj_escape_string(const char * str)
  {
  int done;
  int i;
  const char * pos;
  char * ret = NULL;
  char buf[2];

  if(!str)
    return NULL;
  
  buf[1] = '\0';

  pos = str;
  while(*pos != '\0')
    {
    done = 0;
    i = 0;
    while(escape_rules[i].escaped)
      {
      if(escape_rules[i].c == *pos)
        {
        ret = bg_strcat(ret, escape_rules[i].escaped);
        done = 1;
        break;
        }
      i++;
      }
    if(!done)
      {
      buf[0] = *pos;
      ret = bg_strcat(ret, buf);
      }
    pos++;
    }
  return ret;
  }

int64_t bg_nmj_get_group(sqlite3 * db, const char * table, char * str)
  {
  char * group;
  char * sql;
  int ret = -1;
  int i;
  int result;
  int first = toupper(str[0]);
  
  append_int_t tab;
  memset(&tab, 0, sizeof(tab));

  sql =
    sqlite3_mprintf("select ID from %s;", table);
  result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);

  for(i = 0; i < tab.num_val; i++)
    {
    group = bg_nmj_id_to_string(db, table, "NAME", "ID", tab.val[i]);
    if(strlen(group) == 1)
      {
      if(first == group[0])
        ret = tab.val[i];
      }
    else if(strlen(group) == 3)
      {
      if((first >= group[0]) && (first <= group[2]))
        ret = tab.val[i];
      }
    free(group);
    if(ret >= 0)
      break;
    }
  if((ret < 0) && tab.num_val)
    ret = tab.val[tab.num_val-1];
  if(tab.val)
    free(tab.val);
  return ret;
  }

static const char * search_string_skip[] =
  {
    "a ",
    "the ",
    "&apos;", // 'Round Midnight
    NULL
  };

char * bg_nmj_make_search_string(const char * str)
  {
  int i, len;
  const char * pos = str;

  i = 0;
  while(search_string_skip[i])
    {
    len = strlen(search_string_skip[i]);
    if(!strncasecmp(str, search_string_skip[i], len))
      {
      pos = str + len;
      break;
      }
    i++;
    }
  return bg_strdup(NULL, pos);
  }


int64_t bg_nmj_album_lookup(sqlite3 * db,
                            int64_t artist_id, const char * title)
  {
  int i;
  int64_t ret = -1;
  append_int_t tab;
  char * sql;
  int result;
  
  memset(&tab, 0, sizeof(tab));

  sql =
    sqlite3_mprintf("select ID from SONG_ALBUMS where TITLE = %Q;",
                    title);
  result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);
  
  if(!result)
    goto fail;
  
  for(i = 0; i < tab.num_val; i++)
    {
    if(bg_nmj_id_to_id(db, "SONG_PERSONS_SONG_ALBUMS",
                       "PERSONS_ID",
                       "ALBUMS_ID", tab.val[i]) == artist_id)
      {
      ret = tab.val[i];
      break;
      }
    }
  
  fail:
  if(tab.val)
    free(tab.val);
  return ret;
  }


#if 0
char * bg_nmj_unescape_string(const char * str)
  {
  
  }
#endif


/* Album */

/* Main entry points */

static const char * audio_extensions = "mp3 flac ogg";
static const char * video_extensions = "avi mov mkv flv";
static const char * image_extensions = "jpg";

static char * make_extensions(int type)
  {
  char * ret = NULL;
  if(type & BG_NMJ_MEDIA_TYPE_AUDIO)
    {
    if(ret)
      ret = bg_strcat(ret, " ");
    ret = bg_strcat(ret, audio_extensions);
    }
  if(type & BG_NMJ_MEDIA_TYPE_VIDEO)
    {
    if(ret)
      ret = bg_strcat(ret, " ");
    ret = bg_strcat(ret, video_extensions);
    }
  if(type & BG_NMJ_MEDIA_TYPE_PHOTO)
    {
    if(ret)
      ret = bg_strcat(ret, " ");
    ret = bg_strcat(ret, image_extensions);
    }
  return ret;
  }

// YYYY-MM-DD HH:MM:SS
#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

#define BG_NMJ_TIME_STRING_LEN 20
time_t bg_nmj_string_to_time(const char * str)
  {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  strptime(str, TIME_FORMAT, &tm); 
  return timegm(&tm);
  }

void bg_nmj_time_to_string(time_t time, char * str)
  {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  gmtime_r(&time, &tm);
  strftime(str, BG_NMJ_TIME_STRING_LEN, TIME_FORMAT, &tm);
  }

/*
 *  Categories are summed up from the following:
 *  
 *   3: Video
 *   4: Video (private)
 *  16: Photo
 *  40: Music
 */

static int category_from_types(int types)
  {
  int ret = 0;
  if(types & BG_NMJ_MEDIA_TYPE_AUDIO)
    ret += 40;
  if(types & BG_NMJ_MEDIA_TYPE_VIDEO)
    ret += 3;
  if(types & BG_NMJ_MEDIA_TYPE_VIDEO_PRIVATE)
    ret += 4;
  if(types & BG_NMJ_MEDIA_TYPE_PHOTO)
    ret += 16;
  return ret;
  }

static int add_directory(bg_plugin_registry_t * plugin_reg,
                         sqlite3 * db, bg_nmj_dir_t * dir, int types)
  {
  bg_nmj_file_t * files;
  char * extensions;
  int i;
  bg_nmj_song_t song;
  int64_t size = 0;

  /* Scan */
  extensions = make_extensions(types);
  files = bg_nmj_file_scan(dir->directory, extensions, &size);
  
  /* Add directory */

  dir->size = size;
  dir->category = category_from_types(types);
  dir->status = bg_sprintf("%d", 3); // Whatever that means

  if(!bg_nmj_dir_add(db, dir))
    return 0;
  
  /* Add songs */
  
  i = 0;
  while(files[i].path)
    {
    bg_nmj_song_init(&song);
    bg_nmj_song_get_info(db, plugin_reg, dir, &files[i], &song);
    //    fprintf(stderr, "Got song:\n");
    //    bg_nmj_song_dump(&song);
    //    bg_log(BG_LOG_INFO, LOG_DOMAIN, "Adding song %s", song.title);
    bg_nmj_song_add(plugin_reg, db, &song);
    i++;
    }
  
  return 0;
  }

static int update_directory(bg_plugin_registry_t * plugin_reg,
                            sqlite3 * db, bg_nmj_dir_t * dir, int type)
  {
  append_int_t tab;
  char * sql;
  int result;
  int i;
  bg_nmj_song_t song;
  bg_nmj_file_t * files;
  int64_t size = 0;
  int ret = 0;
  char * extensions;
  bg_nmj_file_t * file;
  char time_str[BG_NMJ_TIME_STRING_LEN];
  
  memset(&tab, 0, sizeof(tab));

  extensions = make_extensions(type);
  
  files = bg_nmj_file_scan(dir->directory, extensions, &size);
  
  /* 1. Query all files in the database and check if they changed or were deleted */
  sql =
    sqlite3_mprintf("select ID from SONGS where SCAN_DIRS_ID = %"PRId64";",
                    dir->id);
  result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);
  
  if(!result)
    goto fail;
  
  for(i = 0; i < tab.num_val; i++)
    {
    bg_nmj_song_init(&song);
    song.id = tab.val[i];
    
    if(!bg_nmj_song_query(db, &song))
      return 0;
    
    //    fprintf(stderr, "Got song\n");
    //    bg_nmj_song_dump(&song);
    
    /* Check if song is still current */

    file = bg_nmj_file_lookup(files, song.path);
    if(!file)
      {
      /* File disappeared */
      bg_nmj_song_delete(db, &song);
      }
    else if(file->time != bg_nmj_string_to_time(song.create_time))
      {
      bg_nmj_song_t new_song;

      bg_nmj_time_to_string(file->time, time_str);
      fprintf(stderr, "Song %s changed %s -> %s (%ld -> %ld)\n",
              song.title, song.create_time, time_str,
              bg_nmj_string_to_time(song.create_time), file->time);
      
      bg_nmj_song_init(&new_song);
      
      /* File changed */
      if(!bg_nmj_song_get_info(db, plugin_reg, dir, file, 
                              &new_song))
        return 0;
      //      bg_nmj_song_dump(&new_song);
      
      bg_nmj_song_update(db, &song, &new_song);
      
      bg_nmj_song_free(&new_song);
      }

    /* Remove from array */
    if(file)
      bg_nmj_file_remove(files, file);
    
    bg_nmj_song_free(&song);
    }
  
  /* 2. Check for newly added files */
  i = 0;
  while(files[i].path)
    {
    bg_nmj_song_t new_song;
    bg_nmj_song_init(&new_song);
    if(!bg_nmj_song_get_info(db, plugin_reg, dir, &files[i], 
                             &new_song))
      return 0;

    //    fprintf(stderr, "Got new song\n");
    //    bg_nmj_song_dump(&new_song);
    bg_nmj_song_add(plugin_reg, db, &new_song);
    i++;
    }

  ret = 1;
  fail:
  
  if(files)
    bg_nmj_file_destroy(files);
  if(extensions)
    free(extensions);
  
  return ret;
  }

int bg_nmj_add_directory(bg_plugin_registry_t * plugin_reg,
                         sqlite3 * db, const char * directory, int types)
  {
  int result;
  /* Check if the directory already exists */
  bg_nmj_dir_t dir;
  bg_nmj_dir_init(&dir);
  dir.directory = bg_strdup(NULL, directory);

  if(bg_nmj_dir_query(db, &dir))
    {
    fprintf(stderr, "Directory exists\n");
    bg_nmj_dir_dump(&dir);
    result = update_directory(plugin_reg, db, &dir, types);
    }
  else
    {
    fprintf(stderr, "Directory doesn't exist\n");
    result = add_directory(plugin_reg, db, &dir, types);
    }
  
  bg_nmj_dir_free(&dir);
  
  return result;
  }

int bg_nmj_remove_directory(sqlite3 * db, const char * directory)
  {
  bg_nmj_dir_t dir;
  char * sql;
  int result;
  append_int_t tab;
  bg_nmj_song_t song;
  int ret = 0;
  int i;
  
  bg_nmj_dir_init(&dir);
  memset(&tab, 0, sizeof(tab));
  dir.directory = bg_strdup(NULL, directory);

  if(!bg_nmj_dir_query(db, &dir))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Directory %s doesn't exist", directory);
    return 0;
    }

  /* Loop through all songs and remove them */
  sql =
    sqlite3_mprintf("select ID from SONGS where SCAN_DIRS_ID = %"PRId64";", dir.id);
  result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);
  if(!result)
    goto fail;

  for(i = 0; i < tab.num_val; i++)
    {
    bg_nmj_song_init(&song);
    song.id = tab.val[i];
    if(bg_nmj_song_query(db, &song))
      bg_nmj_song_delete(db, &song);
    bg_nmj_song_free(&song);
    }

  /* Remove directory */
  sql = sqlite3_mprintf("DELETE FROM SCAN_DIRS WHERE ID = %"PRId64";",
                        dir.id);
  result = bg_sqlite_exec(db, sql, NULL, NULL);
  sqlite3_free(sql);
  if(!result)
    return 0;

  
  ret = 1;
  fail:

  if(tab.val)
    free(tab.val);
  
  return ret;
  }

int bg_nmj_make_thumbnail(bg_plugin_registry_t * plugin_reg,
                          const char * in_file,
                          const char * out_file,
                          int thumb_size)
  {
  int ret = 0;
  /* Formats */
  
  gavl_video_format_t input_format;
  gavl_video_format_t output_format;
  
  /* Frames */
  
  gavl_video_frame_t * input_frame = NULL;
  gavl_video_frame_t * output_frame = NULL;

  gavl_video_converter_t * cnv = 0;
  int do_convert;
  bg_image_writer_plugin_t * output_plugin;
  bg_plugin_handle_t * output_handle = NULL;
  const bg_plugin_info_t * plugin_info;
  
  input_frame = bg_plugin_registry_load_image(plugin_reg,
                                              in_file,
                                              &input_format, NULL);

  gavl_video_format_copy(&output_format, &input_format);

  /* Scale the image to square pixels */
  output_format.image_width *= output_format.pixel_width;
  output_format.image_height *= output_format.pixel_height;
  
  if(output_format.image_width > input_format.image_height)
    {
    output_format.image_height = (thumb_size * output_format.image_height) /
      output_format.image_width;
    output_format.image_width = thumb_size;
    }
  else
    {
    output_format.image_width      = (thumb_size * output_format.image_width) /
      output_format.image_height;
    output_format.image_height = thumb_size;
    }

  output_format.pixel_width = 1;
  output_format.pixel_height = 1;
  output_format.interlace_mode = GAVL_INTERLACE_NONE;

  output_format.frame_width = output_format.image_width;
  output_format.frame_height = output_format.image_height;

  plugin_info =
    bg_plugin_find_by_name(plugin_reg, "iw_jpeg");

  if(!plugin_info)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No plugin for %s", out_file);
    goto fail;
    }

  output_handle = bg_plugin_load(plugin_reg, plugin_info);

  if(!output_handle)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Loading %s failed", plugin_info->long_name);
    goto fail;
    }
  
  output_plugin = (bg_image_writer_plugin_t*)output_handle->plugin;

  if(!output_plugin->write_header(output_handle->priv,
                                  out_file, &output_format, NULL))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing image header failed");
    return 0;
    }

  /* Initialize video converter */
  cnv = gavl_video_converter_create();
  do_convert = gavl_video_converter_init(cnv, &input_format, &output_format);

  if(do_convert)
    {
    output_frame = gavl_video_frame_create(&output_format);
    gavl_video_convert(cnv, input_frame, output_frame);
    if(!output_plugin->write_image(output_handle->priv,
                                   output_frame))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing image failed");
      goto fail;
      }
    }
  else
    {
    if(!output_plugin->write_image(output_handle->priv,
                                   input_frame))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing image failed");
      goto fail;
      }
    }
  
  ret = 1;
  fail:

  if(input_frame)
    gavl_video_frame_destroy(input_frame);
  if(output_frame)
    gavl_video_frame_destroy(output_frame);
  if(output_handle)
    bg_plugin_unref(output_handle);
  if(cnv)
    gavl_video_converter_destroy(cnv);
  return ret;
  }

void bg_nmj_list_dirs(sqlite3 * db)
  {
  append_int_t tab;
  bg_nmj_dir_t dir;
  int i;
  char * sql;
  int result;
  
  memset(&tab, 0, sizeof(tab));

  sql =
    sqlite3_mprintf("select ID from SCAN_DIRS;");
  result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);

  if(!result)
    return;

  for(i = 0; i < tab.num_val; i++)
    {
    bg_nmj_dir_init(&dir);
    dir.id = tab.val[i];
    if(!bg_nmj_dir_query(db, &dir))
      return;
    bg_nmj_dir_dump(&dir);
    bg_nmj_dir_free(&dir);
    }
  if(tab.val)
    free(tab.val);
  }


void bg_nmj_cleanup(sqlite3 * db)
  {
  int i;
  int64_t count;
  append_int_t tab;
  char * tmp_string;
  char * sql;
  int result;
  
  memset(&tab, 0, sizeof(tab));
  
  /* Check for empty music genres */
  sql =
    sqlite3_mprintf("select ID from SONG_GENRES;");
  result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);
  if(!result)
    return;

  for(i = 0; i < tab.num_val; i++)
    {
    count = bg_nmj_count_id(db,
                            "SONG_GENRES_SONGS",
                            "GENRES_ID",
                            tab.val[i]);
    count += bg_nmj_count_id(db,
                            "SONG_GENRES_SONG_ALBUMS",
                            "GENRES_ID",
                            tab.val[i]);
    if(!count)
      {
      tmp_string = bg_nmj_id_to_string(db, "SONG_GENRES", "NAME", "ID", tab.val[i]);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing empty genre %s", tmp_string);
      free(tmp_string);

      sql = sqlite3_mprintf("DELETE FROM SONG_GENRES WHERE ID = %"PRId64";",
                            tab.val[i]);
      result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
      sqlite3_free(sql);
      if(!result)
        return;
      }
    }
  
  /* Check for empty music persons */

  tab.num_val = 0;
  sql =
    sqlite3_mprintf("select ID from SONG_PERSONS;");
  result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
  sqlite3_free(sql);
  if(!result)
    return;

  for(i = 0; i < tab.num_val; i++)
    {
    count = bg_nmj_count_id(db,
                            "SONG_PERSONS_SONGS",
                            "PERSONS_ID",
                            tab.val[i]);
    count += bg_nmj_count_id(db,
                            "SONG_PERSONS_SONG_ALBUMS",
                            "PERSONS_ID",
                            tab.val[i]);
    if(!count)
      {
      tmp_string = bg_nmj_id_to_string(db, "SONG_PERSONS", "NAME", "ID", tab.val[i]);
      bg_log(BG_LOG_INFO, LOG_DOMAIN, "Removing empty person %s", tmp_string);
      free(tmp_string);

      sql = sqlite3_mprintf("DELETE FROM SONG_PERSONS WHERE ID = %"PRId64";",
                            tab.val[i]);
      result = bg_sqlite_exec(db, sql, append_int_callback, &tab);
      sqlite3_free(sql);
      if(!result)
        return;

      }
    }
  
  /* Rebuild indices */
  
  
  }
