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

#include <gmerlin/http.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/utils.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define META_PROTOCOL   "$PROTOCOL"
#define META_PATH       "$PATH"
#define META_METHOD     "$METHOD"
#define META_STATUS_INT "$STATUS_INT"
#define META_STATUS_STR "$STATUS_STR"
#define META_EMPTY      "$EMPTY"

/* Prepare and write a http request (client) */

static int parse_vars_line(gavl_metadata_t * m, char * line)
  {
  char * pos;

  if(*line == '\0') // Final line
    return 1;
  
  pos = strchr(line, ':');
  if(!pos)
    return 0;
  *pos = '\0';
  pos++;
  while(isspace(*pos) && (*pos != '\0'))
    pos++;
  if(*pos == '\0')
    gavl_metadata_set(m, line, META_EMPTY);
  else
    gavl_metadata_set(m, line, pos);
  return 1;
  }

static int read_vars(int fd, char ** line, int * line_alloc,
                     gavl_metadata_t * m, int timeout)
  {
  while(1)
    {
    if(!bg_socket_read_line(fd, line, line_alloc, timeout))
      return 0;
    
    //    fprintf(stderr, "Got line: %s\n", *line);
    
    if(**line == '\0')
      return 1;
    
    if(!parse_vars_line(m, *line))
      return 0;
    }
  return 1;
  }

static char * write_vars(char * str, const gavl_metadata_t * m)
  {
  int i;
  char * line;

  for(i = 0; i < m->num_tags; i++)
    {
    if(*(m->tags[i].key) != '$')
      {
      if(!strcmp(m->tags[i].val, META_EMPTY))
        line = bg_sprintf("%s:\r\n",
                          m->tags[i].key);
      else
        line = bg_sprintf("%s: %s\r\n",
                          m->tags[i].key,
                          m->tags[i].val);
      
      str = gavl_strcat(str, line);
      free(line);
      }
    }
  str = gavl_strcat(str, "\r\n");
  return str;
  }


void bg_http_request_init(gavl_metadata_t * req,
                          const char * method,
                          const char * path,
                          const char * protocol)
  {
  gavl_metadata_init(req);
  gavl_metadata_set(req, META_METHOD, method);
  gavl_metadata_set(req, META_PATH, path);
  gavl_metadata_set(req, META_PROTOCOL, protocol);
  }

int bg_http_request_write(int fd, gavl_metadata_t * req)
  {
  int result;
  char * line = bg_http_request_to_string(req);
  result = bg_socket_write_data(fd, (const uint8_t*)line, strlen(line));
  free(line);
  return result;
  }


char * bg_http_request_to_string(gavl_metadata_t * req)
  {
  const char * method;
  const char * path;
  const char * protocol;
  char * line;
  
  method = gavl_metadata_get(req, META_METHOD);
  path = gavl_metadata_get(req, META_PATH);
  protocol = gavl_metadata_get(req, META_PROTOCOL);

  if(!method || !path || !protocol)
    return NULL;
  
  line = bg_sprintf("%s %s %s\r\n", method, path, protocol);
  line = write_vars(line, req);
  return line;
  }


/* Read a http request (server) */

static int parse_request_line(gavl_metadata_t * req, char * line)
  {
  char * pos1, *pos2;
  pos1 = strchr(line, ' ');
  pos2 = strrchr(line, ' ');

  if(!pos1 || !pos2 || (pos1 == pos2))
    return 0;
  
  gavl_metadata_set_nocpy(req, META_METHOD,
                          gavl_strndup( line, pos1));
  gavl_metadata_set_nocpy(req, META_PATH,
                           gavl_strndup( pos1+1, pos2));
  gavl_metadata_set_nocpy(req, META_PROTOCOL,
                          gavl_strdup(pos2+1));
  return 1;
  }

int bg_http_request_read(int fd, gavl_metadata_t * req, int timeout)
  {
  int result;
  char * line = NULL;
  int line_alloc = 0;
  
  if(!bg_socket_read_line(fd, &line, &line_alloc, timeout))
    return 0;

  //  fprintf(stderr, "Got line: %s\n", line);
  
  if(!parse_request_line(req, line))
    return 0;
  
  result = read_vars(fd, &line, &line_alloc, req, timeout);
  
  if(line)
    free(line);

  return 1;
  }

int bg_http_request_from_string(gavl_metadata_t * req, const char * buf)
  {
  int i;
  char * pos;
  int result = 0;
  char ** str = bg_strbreak(buf, '\n');

  i = 0;
  while(str[i])
    {
    if((pos = strchr(str[i], '\r')))
      *pos = '\0';
    i++;
    }

  if(!parse_request_line(req, str[0]))
    goto fail;
  
  i = 1;
  while(str[i])
    {
    if(!parse_vars_line(req, str[i]))
      goto fail;
    i++;
    }
  result = 1;
  fail:
  bg_strbreak_free(str);
  return result;
  }

const char * bg_http_request_get_protocol(gavl_metadata_t * req)
  {
  return gavl_metadata_get(req, META_PROTOCOL);
  }

const char * bg_http_request_get_method(gavl_metadata_t * req)
  {
  return gavl_metadata_get(req, META_METHOD);
  }

const char * bg_http_request_get_path(gavl_metadata_t * req)
  {
  return gavl_metadata_get(req, META_PATH);
  }


/* Prepare and write a response (server) */

void bg_http_response_init(gavl_metadata_t * res,
                           const char * protocol,
                           int status_int, const char * status_str)
  {
  gavl_metadata_init(res);
  gavl_metadata_set(res, META_PROTOCOL, protocol);
  gavl_metadata_set_int(res, META_STATUS_INT, status_int);
  gavl_metadata_set(res, META_STATUS_STR, status_str);
  }

int bg_http_response_write(int fd, gavl_metadata_t * res)
  {
  int result;
  char * line = bg_http_response_to_string(res);
  result = bg_socket_write_data(fd, (const uint8_t*)line, strlen(line));
  free(line);
  return result;
  }

char * bg_http_response_to_string(gavl_metadata_t * res)
  {
  char * line;
  int status_int, i;
  const char * status_str;
  const char * protocol;
  
  i = 0;
  if(!gavl_metadata_get_int(res, META_STATUS_INT, &status_int) ||
     (status_str = gavl_metadata_get(res, META_STATUS_STR)) ||
     (protocol = gavl_metadata_get(res, META_PROTOCOL)))
    return 0;
  
  line = bg_sprintf("%s %d %s\r\n", protocol, status_int, status_str);
  line = write_vars(line, res);
  return line;
  }

/* Read a response (client) */

static int parse_response_line(gavl_metadata_t * res, char * line)
  {
  char * pos1;
  char * pos2;

  pos1 = strchr(line, ' ');
  if(!pos1)
    return 0;

  pos2 = strchr(pos1+1, ' ');
  
  if(!pos2)
    return 0;
  
  gavl_metadata_set_nocpy(res, META_PROTOCOL,
                          gavl_strndup( line, pos1));
  gavl_metadata_set_nocpy(res, META_STATUS_INT,
                          gavl_strndup( pos1+1, pos2));

  pos2++;
  gavl_metadata_set(res, META_STATUS_STR, pos2);
  return 1;
  }

int bg_http_response_read(int fd, gavl_metadata_t * res, int timeout)
  {
  int result = 0;
  char * line = NULL;
  int line_alloc = 0;
  
  if(!bg_socket_read_line(fd, &line, &line_alloc, timeout) ||
     !parse_response_line(res, line) ||
     !read_vars(fd, &line, &line_alloc, res, timeout))
    goto fail;
  
  result = 1;
  fail:
  if(line)
    free(line);
  
  return result;
  }

int bg_http_response_from_string(gavl_metadata_t * res, const char * buf)
  {
  int i;
  char * pos;
  int result = 0;
  char ** str = bg_strbreak(buf, '\n');

  i = 0;
  while(str[i])
    {
    if((pos = strchr(str[i], '\r')))
      *pos = '\0';
    i++;
    }

  if(!parse_response_line(res, str[0]))
    goto fail;
  
  i = 1;
  while(str[i])
    {
    if(!parse_vars_line(res, str[i]))
      goto fail;
    i++;
    }

  result = 1;
  fail:
  bg_strbreak_free(str);
  
  return result;
  }


const char * bg_http_response_get_protocol(gavl_metadata_t * res)
  {
  return gavl_metadata_get(res, META_PROTOCOL);
  }

const int bg_http_response_get_status_int(gavl_metadata_t * res)
  {
  int ret = 0;
  gavl_metadata_get_int(res, META_STATUS_INT, &ret);
  return ret;
  }

const char * bg_http_response_get_status_str(gavl_metadata_t * res)
  {
  return gavl_metadata_get(res, META_STATUS_STR);
  }

void bg_http_header_set_empty_var(gavl_metadata_t * h, const char * name)
  {
  gavl_metadata_set(h, name, META_EMPTY);
  }
