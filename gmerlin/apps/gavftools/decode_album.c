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

#include "gavf-decode.h"
#include <string.h>

/* Stat */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define LOG_DOMAIN "gavf-decode.album"

void album_init(album_t * a)
  {
  memset(a, 0, sizeof(*a));
  }

void album_free(album_t * a)
  {
  if(a->entries)
    free(a->entries);
  if(a->first)
    bg_album_entries_destroy(a->first);
  }

static int get_mtime(const char * file, time_t * ret)
  {
  struct stat st;
  if(stat(file, &st))
    return 0;
  *ret = st.st_mtime;
  return 1;
  }

static int album_load(album_t * a)
  {
  int i;
  char * album_xml;
  bg_album_entry_t * e;
  
  if(!get_mtime(album_file, &a->mtime))
    return 0;
  
  album_xml = bg_read_file(album_file, NULL);

  if(!album_xml)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s could not be opened",
           album_file);
    return 0;
    }

  a->first = bg_album_entries_new_from_xml(album_xml);
  free(album_xml);
  
  if(!a->first)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Album file %s contains no entries",
           album_file);
    return 0;
    }

  /* Count entries */
  e = a->first;
  while(e)
    {
    a->num_entries++;
    e = e->next;
    }

  /* Set up array */
  a->entries = calloc(a->num_entries, sizeof(a->entries));
  e = a->first;

  for(i = 0; i < a->num_entries; i++)
    {
    a->entries[i] = e;
    e = e->next;
    }

  /* Shuffle */
  if(shuffle)
    {
    int idx;
    for(i = 0; i < a->num_entries; i++)
      {
      idx = rand() % a->num_entries;
      e = a->entries[i];
      a->entries[i] = a->entries[idx];
      a->entries[idx] = e;
      }
    }
  return 1;
  }

static bg_album_entry_t * album_next(album_t * a)
  {
  time_t mtime;
  bg_album_entry_t * ret;

  if(!get_mtime(album_file, &mtime))
    return NULL;

  if(mtime != a->mtime)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Album file %s changed on disk, reloading",
           album_file);
    album_free(a);
    album_init(a);

    if(!album_load(a))
      return NULL;
    }
  
  if(a->current_entry == a->num_entries)
    {
    if(!loop)
      return NULL;
    else a->current_entry = 0;
    }
  
  ret = a->entries[a->current_entry];
  
  a->current_entry++;
  return ret;
  }

int init_decode_album(album_t * a)
  {
  int ret = 0;
  int i, num;
  bg_album_entry_t * e;

  if(!album_load(a))
    return ret;
  
  e = album_next(a);

  if(!load_album_entry(e, &a->in_conn, &a->h, &a->m))
    return ret;

  /* Set up the conn2 from conn1 */
  a->num_streams = a->in_conn.num_streams;

  num = bg_mediaconnector_get_num_streams(&a->in_conn, GAVF_STREAM_AUDIO);
  for(i = 0; i < num; i++)
    stream_create(bg_mediaconnector_get_stream(&a->in_conn, GAVF_STREAM_AUDIO, i), a);

  num = bg_mediaconnector_get_num_streams(&a->in_conn, GAVF_STREAM_VIDEO);
  for(i = 0; i < num; i++)
    stream_create(bg_mediaconnector_get_stream(&a->in_conn, GAVF_STREAM_VIDEO, i), a);

  num = bg_mediaconnector_get_num_streams(&a->in_conn, GAVF_STREAM_TEXT);
  for(i = 0; i < num; i++)
    stream_create(bg_mediaconnector_get_stream(&a->in_conn, GAVF_STREAM_TEXT, i), a);

  num = bg_mediaconnector_get_num_streams(&a->in_conn, GAVF_STREAM_OVERLAY);
  for(i = 0; i < num; i++)
    stream_create(bg_mediaconnector_get_stream(&a->in_conn, GAVF_STREAM_OVERLAY, i), a);

  ret = 1;
  return ret;
  }

static int match_streams(album_t * a, const char *location,
                         gavf_stream_type_t type)
  {
  if(bg_mediaconnector_get_num_streams(&a->in_conn, type) !=
     bg_mediaconnector_get_num_streams(&a->out_conn, type))
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN,
           "Skipping %s (number of %s stream doesn't match)",
           location, gavf_stream_type_name(type));
    return 0;
    }
  return 1;
  }

int album_set_eof(album_t * a, stream_t * s)
  {
  bg_album_entry_t * e;
  
  a->active_streams--;

  if(a->active_streams)
    return 0;

  
  while(1)
    {
    bg_mediaconnector_free(&a->in_conn);
    bg_mediaconnector_init(&a->in_conn);
    if(a->h)
      {
      bg_plugin_unref(a->h);
      a->h = NULL;
      }
    
    e = album_next(a);
    if(!e)
      {
      a->eof = 1;
      return 1;
      }
    
    /* Load next track */
    
    if(!load_album_entry(e, &a->in_conn,
                         &a->h, &a->m))
      {
      bg_log(BG_LOG_WARNING, LOG_DOMAIN, "Skipping %s", (char*)e->location);
      continue;
      }

    if(!match_streams(a, e->location, GAVF_STREAM_AUDIO) ||
       !match_streams(a, e->location, GAVF_STREAM_VIDEO) ||
       !match_streams(a, e->location, GAVF_STREAM_TEXT) ||
       !match_streams(a, e->location, GAVF_STREAM_OVERLAY))
      continue;
    
    }
  
  
  }
