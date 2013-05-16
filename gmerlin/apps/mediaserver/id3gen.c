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

#include "server.h"
#include <gmerlin/charset.h>
#include <gavl/numptr.h>

#include <string.h>

#if 0
#define CHARSET "UTF-16"
#define BYTES_PER_CHAR 2
#else
#define CHARSET "ISO_8859-1"
#define BYTES_PER_CHAR 1
#endif

static void write_data(id3v2_t * ret, const void * data, int len)
  {
  if(ret->len + len > ret->alloc)
    {
    ret->alloc = ret->len + len + 1024;
    ret->data = realloc(ret->data, ret->alloc);
    }
  memcpy(ret->data + ret->len, data, len);
  ret->len += len;
  }

static void write_32_syncsave(uint8_t * dst, uint32_t num)
  {
  dst[0] = (num >> 21) & 0x7f;
  dst[1] = (num >> 14) & 0x7f;
  dst[2] = (num >> 7) & 0x7f;
  dst[3] = num & 0x7f;
  }

static void add_text_frame(id3v2_t * ret, char * fourcc, const char * string, bg_charset_converter_t * cnv)
  {
  char buf[4] = { 0, 0, 0, 0 };
  char * converted;
  int converted_len;
  int old_len = ret->len;
  int frame_len;
  if(!string)
    return;

  converted = bg_convert_string(cnv, string, -1, &converted_len);

  write_data(ret, fourcc, 4);
  write_data(ret, buf, 4); // Size (not syncsave)
  write_data(ret, buf, 2); // Flags

  buf[0] = 0x00;
  write_data(ret, buf, 1); // Encoding
  write_data(ret, converted, converted_len+1); // text + '\0'
 
  frame_len = ret->len - old_len - 10;
  GAVL_32BE_2_PTR(frame_len, ret->data + old_len + 4);
  free(converted);
  }

static void add_int_frame(id3v2_t * ret, char * fourcc, int64_t val)
  {
  char buf[64];
  int old_len = ret->len;
  int frame_len;
  if(!val || (val == 9999))
    return;

  write_data(ret, fourcc, 4);
  write_data(ret, buf, 4); // Size (not syncsave)
  write_data(ret, buf, 2); // Flags
  buf[0] = 0x00;
  write_data(ret, buf, 1); // Encoding

  sprintf(buf, "%"PRId64, val);
  write_data(ret, buf, strlen(buf)+1); // text + '\0'
  frame_len = ret->len - old_len - 10;
  GAVL_32BE_2_PTR(frame_len, ret->data + old_len + 4);
  }

static void add_cover(id3v2_t * ret, const char * image_file)
  {
  char buf[4];
  int file_len;
  int frame_len;
  int old_len = ret->len;
  void * file_buf = bg_read_file(image_file, &file_len);
  
  if(!file_buf)
    return;

  write_data(ret, "APIC", 4);
  write_data(ret, buf, 4); // Size (not syncsave)
  write_data(ret, buf, 2); // Flags
  buf[0] = 0x00;
  write_data(ret, buf, 1); // Encoding
  
  write_data(ret, "image/jpeg", 11);
  buf[0] = 0x03; /* front cover */
  write_data(ret, buf, 1);
  buf[0] = 0;
  write_data(ret, buf, 1); // Description
  write_data(ret, file_buf, file_len); // Image

  frame_len = ret->len - old_len - 10;
  GAVL_32BE_2_PTR(frame_len, ret->data + old_len + 4);
  free(file_buf);
  }

#define SIZE_OFFSET 6

int id3v2_generate(bg_db_t * db, int64_t id, id3v2_t * ret)
  {
  // Sig (3) + version (2) + flags (1) + size(4)
  uint8_t header[10] = { 'I', 'D', '3', 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  

  bg_db_audio_file_t * song = NULL;
  bg_db_audio_album_t * album = NULL;
  bg_db_file_t * cover = NULL;
  bg_charset_converter_t * cnv;
  
  /* Query objects */
  song = bg_db_object_query(db, id);
  if(!song)
    return 0;

  if(bg_db_object_get_type(song) != BG_DB_OBJECT_AUDIO_FILE)
    return 0;

  if(song->album_id > 0)
    {
    album = bg_db_object_query(db, song->album_id);
    if(album->cover_id > 0)
      cover = bg_db_get_thumbnail(db, album->cover_id,
                                  320, 320, 0, "image/jpeg");
    }
  /* Create tag */
  ret->len = 0;
  write_data(ret, header, 10);

  /* Add text Frames */
  cnv = bg_charset_converter_create("UTF-8", CHARSET);

  add_text_frame(ret, "TPE1", song->artist, cnv);
  add_text_frame(ret, "TIT2", song->title, cnv);
  add_text_frame(ret, "TCON", song->genre, cnv);
  add_int_frame(ret, "TYER", song->date.year);
  add_int_frame(ret, "TRCK", song->track);
  add_int_frame(ret, "TLEN", song->file.obj.duration / 1000);

  if(album)
    {
    add_text_frame(ret, "TALB", album->title, cnv);
    add_text_frame(ret, "TPE2", album->artist, cnv);
    }

  if(cover)
    {
    /* add cover */
    add_cover(ret, cover->path);
    }

  bg_charset_converter_destroy(cnv);

  /* Write size */
  write_32_syncsave(ret->data + SIZE_OFFSET, ret->len - 10);
  ret->id = id;

  /* Unref objects */
  if(song)
    bg_db_object_unref(song);
  if(album)
    bg_db_object_unref(album);
  if(cover)
    bg_db_object_unref(cover);
  return 1;
  }


id3v2_t * server_get_id3(server_t * s, int64_t id)
  { 
  int i;
  gavl_time_t min_time = GAVL_TIME_UNDEFINED;
  int min_index;

  /* Check for cached value */
  for(i = 0; i < s->id3_cache_size; i++)
    {
    if(s->id3_cache[i].id == id)
      {
      s->id3_cache[i].last_used = s->current_time;
      return &s->id3_cache[i];
      }
    }

  /* Check for empty slot */
  for(i = 0; i < s->id3_cache_size; i++)
    {
    if(s->id3_cache[i].last_used == GAVL_TIME_UNDEFINED)
      {
      min_index = i;
      break;
      }
    else if((min_time == GAVL_TIME_UNDEFINED) ||
            (s->id3_cache[i].last_used < min_time))
      {
      min_index = i;
      min_time = s->id3_cache[i].last_used;
      }
    }

  if(!id3v2_generate(s->db, id, &s->id3_cache[min_index]))
    return NULL;
  
  s->id3_cache[min_index].last_used = s->current_time;
  return &s->id3_cache[min_index];
  }

