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

#include <gmerlin/http.h>
#include <unistd.h> // access
#include <errno.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/stat.h>


#define WEB_ROOT DATA_DIR"/server/"

/* Mimetypes of the statically served files */

static struct
  {
  const char * ext;
  const char * mimetype;
  }
mimetypes[] =
  {
    { "html", "text/html" },
    { "png", "image/png" },
    { "js", "text/javascript" },
    { /* End */ },
  };
  
static void send_file(int fd, const char * method, const char * file)
  {
  gavl_metadata_t res;
  struct stat st;
  char * real_file = NULL;
  int i;
  char * ext;
  int result = 0;
  
  real_file = bg_canonical_filename(file);
  gavl_metadata_init(&res);
  
  if(strcmp(method, "GET") && strcmp(method, "HEAD"))
    {
    /* Method not allowed */
    bg_http_response_init(&res, "HTTP/1.1", 
                          405, "Method Not Allowed");
    goto go_on;
    }

  /* Check if we are outside of the tree */
  if(!real_file)
    {
    bg_http_response_init(&res, "HTTP/1.1", 404, "Not Found");
    goto go_on;
    }
  
  /* Check if we are outside of the tree */
  if(strncmp(real_file, WEB_ROOT, strlen(WEB_ROOT)))
    {
    bg_http_response_init(&res, "HTTP/1.1", 
                          401, "Forbidden");
    goto go_on;
    }
  
  if(stat(real_file, &st))
    {
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

  /* Check if the file is world wide readable */
  if(!(st.st_mode & S_IROTH))
    {
    bg_http_response_init(&res, "HTTP/1.1", 
                          401, "Forbidden");
    goto go_on;
    }
  
  bg_http_response_init(&res, "HTTP/1.1", 200, "OK");
  gavl_metadata_set_long(&res, "Content-Length", st.st_size);

  ext = strrchr(real_file, '.');
  if(ext)
    {
    ext++;

    i = 0;
    while(mimetypes[i].mimetype)
      {
      if(!strcmp(mimetypes[i].ext, ext))
        {
        gavl_metadata_set(&res, "Content-Type", mimetypes[i].mimetype);
        break;
        }
      i++;
      }
    }

  if(!strcmp(method, "GET"))
    result = 1;
  
  go_on:
  if(!bg_http_response_write(fd, &res) ||
     !result)
    goto cleanup;

  bg_socket_send_file(fd, real_file, 0, 0);

  
  cleanup:

  gavl_metadata_free(&res);
  if(real_file)
    free(real_file);
  }

int server_handle_static(server_t * s, int * fd,
                         const char * method,
                         const char * path_orig,
                         const gavl_metadata_t * req)
  {
  if(!strcmp(path_orig, "/"))
    {
    send_file(*fd, method, WEB_ROOT"index.html");
    return 1;
    }
  else if(!strncmp(path_orig, "/static/", 8))
    {
    char * filename = bg_sprintf("%s%s", WEB_ROOT, path_orig + 8);
    send_file(*fd, method, filename);
    free(filename);
    return 1;
    }
  else
    return 0;
  }
