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
#include <string.h>
#include <gmerlin/http.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "mediahandler"

#include <unistd.h> // access
#include <errno.h> 

#define THREAD_THRESHOLD (1024*1024) // 1 M

typedef struct
  {
  char * path;
  int64_t offset;
  int64_t length;
  } send_file_t;

static void send_file_func(client_t * c)
  {
  send_file_t * s = c->data;
  bg_socket_send_file(c->fd, s->path, s->offset, s->length);
  }

static void cleanup_func(void * data)
  {
  send_file_t * s = data;
  free(s->path);
  free(s);
  }

int server_handle_media(server_t * s, int * fd,
                        const char * method,
                        const char * path_orig,
                        const gavl_metadata_t * req)
  {
  int result = 0;
  bg_db_object_t * o = NULL;
  bg_db_file_t * f;
  int64_t id = 0;
  char * path = NULL;
  gavl_metadata_t res;
  int type;
  int launch_thread = 0;
  char * pos;
  const char * range;
  int64_t start, end;
  id3v2_t * id3;
  int id3_offset = 0;
  int64_t file_size;
  
  gavl_metadata_init(&res);
  
  if(strncmp(path_orig, "/media/", 7))
    return 0;

  path_orig += 7;
  
  if(strcmp(method, "GET") && strcmp(method, "HEAD"))
    {
    /* Method not allowed */
    bg_http_response_init(&res, "HTTP/1.1", 
                          405, "Method Not Allowed");
    goto go_on;
    }
  
  path = gavl_strdup(path_orig);

  /* Some clients append bullshit to the path */
  if((pos = strchr(path, '#')))
    *pos = '\0';

  id = strtoll(path, &pos, 10);
  
  if(*pos != '\0')
    {
    fprintf(stderr, "Bad request: %s\n", path);

    bg_http_response_init(&res, "HTTP/1.1", 
                          400, "Bad Request");
    goto go_on;
    }

  /* Get the object from the db */
  if(!s->db)
    {
    bg_http_response_init(&res, "HTTP/1.1", 
                          404, "Not Found");
    goto go_on;
    }
  
  o = bg_db_object_query(s->db, id);

  type = bg_db_object_get_type(o);
  
  if(!(bg_db_object_get_type(o) & DB_DB_FLAG_FILE))
    {
    bg_http_response_init(&res, "HTTP/1.1", 
                          404, "Not Found");
    goto go_on;
    }

  f = (bg_db_file_t*)o;
  
  if(access(f->path, R_OK))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Could not access file %s (database out of date?)", f->path);

    if(errno == EACCES)
      {
      bg_http_response_init(&res, "HTTP/1.1", 
                            401, "Forbidden");
      goto go_on;
      }
    else
      {
      bg_http_response_init(&res, "HTTP/1.1", 
                            404, "Not Found");
      goto go_on;
      }
    }

  /* Decide whether to send an ID3 tag */
  
  id3 = server_get_id3_for_file(s, f, &id3_offset);

  if(id3)
    {
    fprintf(stderr, "Size: %"PRId64", id3_offset: %d, id3_len: %d\n",
            o->size, id3_offset, id3->len);
    file_size = o->size - id3_offset + id3->len;
    }
  else
    file_size = o->size;
  
  /* Figure out byte range */
  
  range = gavl_metadata_get_i(req, "Range");
  if(range)
    {
    if(gavl_string_starts_with(range, "bytes="))
      {
      range += 6;

      if(sscanf(range, "%"PRId64"-%"PRId64, &start, &end) < 2)
        {
        if(sscanf(range, "%"PRId64"-", &start) == 1)
          end = file_size - 1;
        else
          {
          bg_http_response_init(&res, "HTTP/1.1", 
                                400, "Bad Request");
          goto go_on;
          }
        }
      else
        {
        if(end >= file_size)
          {
          bg_http_response_init(&res, "HTTP/1.1", 
                                416, "Requested range not satisfiable");
          goto go_on;
          }
        }
      }
    else
      {
      bg_http_response_init(&res, "HTTP/1.1", 
                            400, "Bad Request");
      goto go_on;
      }
    // fprintf(stderr, "Got range: %"PRId64"-%"PRId64"\n", start, end);
    }
  else
    {
    start = 0;
    end = file_size - 1;
    }
  
  /* Decide whether to launch a thread */

  if((type == BG_DB_OBJECT_AUDIO_FILE) ||
     (type == BG_DB_OBJECT_VIDEO_FILE) ||
     (o->size > THREAD_THRESHOLD))
    {
    launch_thread = 1;
    }
  
  result = 1;
  
  /* Set up header for transmission */

  if(range)
    bg_http_response_init(&res, "HTTP/1.1", 206, "Partial Content");
  else
    bg_http_response_init(&res, "HTTP/1.1", 200, "OK");
  
  if(s->server_string)
    gavl_metadata_set(&res, "Server", s->server_string);
  bg_http_header_set_date(&res, "Date");
  gavl_metadata_set(&res, "Content-Type", f->mimetype);
  
  if(range)
    {
    char * tmp_string;
    gavl_metadata_set_long(&res, "Content-Length", end - start + 1);
    tmp_string = bg_sprintf("bytes %"PRId64"-%"PRId64"/%"PRId64, start, end, file_size);
    gavl_metadata_set(&res, "Content-Range", tmp_string);
    free(tmp_string);
    }
  else
    gavl_metadata_set_long(&res, "Content-Length", file_size);
  
  
  gavl_metadata_set(&res, "Connection", "close");
  gavl_metadata_set(&res, "Cache-control", "no-cache");
    
  if(gavl_metadata_get(req, "transferMode.dlna.org"))
    {
    if(launch_thread)
      gavl_metadata_set(&res, "transferMode.dlna.org", "Streaming");
    else
      gavl_metadata_set(&res, "transferMode.dlna.org", "Interactive");
      
    }
  
  fprintf(stderr, "Sending file:\n");
  gavl_metadata_dump(&res, 0);

  if(!strcmp(method, "HEAD"))
    result = 0;
  
  go_on:

  if(!bg_http_response_write(*fd, &res) ||
     !result)
    goto cleanup;

  /* Send ID3 tag */

  end++; // http -> C
  
  if(id3)
    {
    int delta = id3->len - start;
    end -= (id3->len - id3_offset);
    if(delta > 0) // | ID3   |<-delta->||                MP3    |
      {
      fprintf(stderr, "Sending data\n");
      // gavl_hexdump(id3->data + start, 176, 16);
      
      if(!bg_socket_write_data(*fd, id3->data + start, delta))
        goto cleanup;
      start = id3_offset;
      }
    else          // | ID3             ||<- -delta-> |   MP3    |
      {
      start -= delta;
      start += id3_offset;
      }
    }

  
  
  if(!launch_thread)
    bg_socket_send_file(*fd, f->path, start, end);
  else
    {
    client_t * c;
    send_file_t * sf = calloc(1, sizeof(*s));
    sf->path = gavl_strdup(f->path);
    sf->offset = start;
    sf->length = end - start;

    c = client_create(CLIENT_TYPE_MEDIA, *fd, sf, cleanup_func, send_file_func);
    server_attach_client(s, c);
    *fd = -1;
    }
  
  cleanup:

  gavl_metadata_free(&res);
  if(path)
    free(path);
  if(o)
    bg_db_object_unref(o);
  
  return 1;
  }
