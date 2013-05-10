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
#include <gmerlin/subprocess.h>
#include <gmerlin/http.h>

#include <string.h>
#include <signal.h>


struct
  {
  const char * name;
  const char * program;
  const char * mimetype;
  }
transcode_programs[] =
  {
    { .name = "mp3-320",
      .program = "gavf-decode -vs m -ts m -os m -as d -i \"%s\" | "
      "gavf-encode -enc \"a2v=0:ae=e_lame{do_id3v1=0:do_id3v2=0}\" -ac cbr_bitrate=320 -o -",
      .mimetype = "audio/mpeg",
    },

#if 0
    {
      .name = "wav",
      .program = "gavf-decode -i %s | gavf-encode -enc a2v=0:ae=e_wav -o out.wav"
    },
#endif
    { /* End */ },
  };

typedef struct
  {
  bg_subprocess_t * proc;
  } transcode_t;

static void transcode_func(client_t * c)
  {
  int len;
  uint8_t buf[1024];
  
  transcode_t * s = c->data;

  while(1)
    {
    len = bg_subprocess_read_data(s->proc->stdout_fd, buf, 1024);
    if(len)
      {
      if(!bg_socket_write_data(c->fd, buf, len))
        break;
      }
    if(len < 1024)
      break;
    }
  }

static void cleanup_func(void * data)
  {
  transcode_t * s = data;
  bg_subprocess_kill(s->proc, SIGTERM);
  fprintf(stderr, "Closing subprocess....\n");
  bg_subprocess_close(s->proc);
  fprintf(stderr, "Subprocess closed\n");
  free(s);
  }


int server_handle_transcode(server_t * s, int * fd,
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
  int idx;
  client_t * c;
  char * command;
  bg_subprocess_t * sp;
  transcode_t * t;
  char * pos;
  
  gavl_metadata_init(&res);
  
  if(strncmp(path_orig, "/transcode/", 11))
    return 0;

  path_orig += 11; 
  
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
  
  if(*pos != '/')
    {
    bg_http_response_init(&res, "HTTP/1.1", 
                          400, "Bad Request");
    goto go_on;
    }
  pos++;
  
  idx = 0;
  while(transcode_programs[idx].name)
    {
    if(!strcmp(transcode_programs[idx].name, pos))
      break;
    idx++;
    }
  
  if(!transcode_programs[idx].name)
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

  command = bg_sprintf(transcode_programs[idx].program, f->path);
  fprintf(stderr, "Command: %s\n", command);
  sp = bg_subprocess_create(command, 0, 1, 0);
  
  if(!sp)
    {
    bg_http_response_init(&res, "HTTP/1.1", 500, "Internal Server Error");
    goto go_on;
    }
  
  result = 1;

  /* Set up header for transmission */
  bg_http_response_init(&res, "HTTP/1.1", 
                        200, "OK");
  gavl_metadata_set(&res, "Content-Type", transcode_programs[idx].mimetype);
  gavl_metadata_set(&res, "Connection", "close");
  gavl_metadata_set(&res, "Cache-control", "no-cache");
  gavl_metadata_set(&res, "Accept-Ranges", "none");
    
  fprintf(stderr, "Sending transcoded stream:\n");
  gavl_metadata_dump(&res, 0);

  if(!strcmp(method, "HEAD"))
    result = 0;
  
  go_on:

  if(!bg_http_response_write(*fd, &res) ||
     !result)
    goto cleanup;

  t = calloc(1, sizeof(*t));
  t->proc = sp;
    
  c = client_create(CLIENT_TYPE_MEDIA, *fd, t, cleanup_func, transcode_func);
  server_attach_client(s, c);
  *fd = -1;
  
  cleanup:

  gavl_metadata_free(&res);
  if(path)
    free(path);
  if(o)
    bg_db_object_unref(o);
  
  return 1;
  }
