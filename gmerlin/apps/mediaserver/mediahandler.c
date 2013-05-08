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
  gavl_metadata_t url_vars;
  int type;
  int launch_thread = 0;
  
  gavl_metadata_init(&res);
  gavl_metadata_init(&url_vars);
  
  if(strncmp(path_orig, "/media?", 7))
    return 0;
  
  if(strcmp(method, "GET"))
    {
    /* Method not allowed */
    bg_http_response_init(&res, "HTTP/1.1", 
                          405, "Method Not Allowed");
    goto go_on;
    }
  
  path = gavl_strdup(path_orig);

  bg_url_get_vars(path, &url_vars);

  if(!gavl_metadata_get_long(&url_vars, "id", &id))
    {
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

  /* TODO: Figure out byte range */
  
  /* Decide whether to launch a thread */

  if((type == BG_DB_OBJECT_AUDIO_FILE) ||
     (type == BG_DB_OBJECT_VIDEO_FILE) ||
     (o->size > THREAD_THRESHOLD))
    {
    launch_thread = 1;
    }
  
  result = 1;

  /* Set up header for transmission */
  bg_http_response_init(&res, "HTTP/1.1", 
                        200, "OK");
  gavl_metadata_set(&res, "Content-Type", f->mimetype);
  gavl_metadata_set_long(&res, "Content-Length", o->size);

  fprintf(stderr, "Sending file:\n");
  gavl_metadata_dump(&res, 0);
    
  go_on:

  if(!bg_http_response_write(*fd, &res) ||
     !result)
    goto cleanup;

  if(!launch_thread)
    bg_socket_send_file(*fd, f->path, 0, -1);
  else
    {
    client_t * c;
    send_file_t * sf = calloc(1, sizeof(*s));
    sf->path = gavl_strdup(f->path);
    sf->offset = 0;
    sf->length = -1;

    c = client_create(CLIENT_TYPE_MEDIA, *fd, sf, cleanup_func, send_file_func);
    server_attach_client(s, c);
    *fd = -1;
    }
  
  cleanup:

  gavl_metadata_free(&res);
  gavl_metadata_free(&url_vars);
  if(path)
    free(path);
  if(o)
    bg_db_object_unref(o);
  
  return 1;
  }
