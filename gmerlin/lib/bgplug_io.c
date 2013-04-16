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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <ctype.h>

#include <gmerlin/bgplug.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/utils.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "plug_io"

// #define TIMEOUT 5000

// #define DUMP_HEADERS

/* stdin / stdout */

static gavf_io_t * open_dash(int method, int * flags)
  {
  struct stat st;
  int fd;
  FILE * f;
    
  /* Stdin/stdout */
  if(method == BG_PLUG_IO_METHOD_WRITE)
    f = stdout;
  else
    f = stdin;
    
  fd = fileno(f);

  if(isatty(fd))
    {
    if(method == BG_PLUG_IO_METHOD_WRITE)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Not writing to a TTY");
    else
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Not reading from a TTY");
    return NULL;
    }
    
  if(fstat(fd, &st))
    {
    if(method == BG_PLUG_IO_METHOD_WRITE)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot stat stdout");
    else
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot stat stdin");
    return 0;
    }
  if(S_ISFIFO(st.st_mode)) /* Pipe: Use local connection */
    *flags |= BG_PLUG_IO_IS_LOCAL;
  else if(S_ISREG(st.st_mode))
    *flags |= BG_PLUG_IO_IS_REGULAR;
  return gavf_io_create_file(f, method == BG_PLUG_IO_METHOD_WRITE, 0, 0);
  }

/* TCP client */

/* Unix domain client */

/* Pipe */

static int read_pipe(void * priv, uint8_t * data, int len)
  {
  bg_subprocess_t * sp = priv;
  return bg_subprocess_read_data(sp->stdout_fd, data, len);
  }

static int write_pipe(void * priv, const uint8_t * data, int len)
  {
  bg_subprocess_t * sp = priv;
  return write(sp->stdin_fd, data, len);
  }

static void close_pipe(void * priv)
  {
  bg_subprocess_t * sp = priv;
  bg_subprocess_close(sp);
  }

static gavf_io_t * open_pipe(const char * location, int wr)
  {
  const char * pos;
  bg_subprocess_t * sp;
  
  pos = location;

  if(!wr && (*pos == '|'))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Read pipes must start with '<'");
    return NULL;
    }
  else if(wr && (*pos == '<'))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Write pipes must start with '|'");
    return NULL;
    }
  
  pos++;
  while(isspace(*pos) && (*pos != '\0'))
    pos++;

  if(pos == '\0')
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid pipe: %s", location);
    return NULL;
    }
  
  sp = bg_subprocess_create(pos,
                            wr ? 1 : 0, // Do stdin
                            wr ? 0 : 1, // Do stdout
                            0);         // Do stderr
  
  if(!sp)
    return NULL;
  
  return gavf_io_create(wr ? NULL : read_pipe,
                        wr ? write_pipe : NULL,
                        NULL, // seek
                        close_pipe,
                        NULL, // flush
                        sp);
  }

/* Socket */

/* Handshake */

/*
 * Protocol
 *
 * Client to server:
 * METHOD location protocol
 *
 * METHOD can be READ or WRITE
 *
 * Server: PROTOCOL STATUS STATUS_STRING
 *
 * If the method was read: Server starts to write
 * If the method was write: Server starts to read
 *
 * Supported status codes:
 *
 * 200 OK
 * 400 Bad Request
 * 405 Method Not Allowed
 * 505 Protocol Version Not Supported
 * 503 Service Unavailable
 */

static const struct
  {
  int status_int;
  const char * status_str;
  }
status_codes[] =
  {
    { BG_PLUG_IO_STATUS_100, "Continue"    },
    { BG_PLUG_IO_STATUS_200, "OK"          },
    { BG_PLUG_IO_STATUS_400, "Bad Request" },
    { BG_PLUG_IO_STATUS_404, "Not Found" },
    { BG_PLUG_IO_STATUS_405, "Method Not Allowed" },

    { BG_PLUG_IO_STATUS_406, "Not Acceptable" },
    { BG_PLUG_IO_STATUS_415, "Unsupported Media Type" },
    { BG_PLUG_IO_STATUS_417, "Expectation Failed" },
    { BG_PLUG_IO_STATUS_423, "Locked" },
    { BG_PLUG_IO_STATUS_505, "Protocol Version Not Supported" },
    { BG_PLUG_IO_STATUS_503, "Service Unavailable" },
    { /* End */ }
  };
  
#define META_METHOD     "$METHOD"
#define META_LOCATION   "$LOCATION"
#define META_PROTOCOL   "$PROTOCOL"
#define META_STATUS     "$STATUS"
#define META_STATUS_STR "$STATUS_STR"

// #define PROTOCOL "bgplug/"VERSION

#define PROTOCOL "HTTP/1.1"

#define BGPLUG_MIMETYPE "application/bgplug-"VERSION

static int read_vars(int fd, char ** line, int * line_alloc,
                     gavl_metadata_t * m, int timeout)
  {
  char * pos;
  while(1)
    {
    if(!bg_socket_read_line(fd, line, line_alloc, timeout))
      return 0;

    //    fprintf(stderr, "Got line: %s\n", *line);
    
    if(**line == '\0')
      return 1;
    
    pos = strchr(*line, ':');
    if(!pos)
      return 0;
    *pos = '\0';
    pos++;
    while(isspace(*pos) && (*pos != '\0'))
      pos++;
    if(*pos == '\0')
      return 0;
    
    gavl_metadata_set(m, *line, pos);
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
      line = bg_sprintf("%s: %s\r\n",
                        m->tags[i].key,
                        m->tags[i].val);

      str = bg_strcat(str, line);
      free(line);
      }
    }
  str = bg_strcat(str, "\r\n");
  return str;
  }

int bg_plug_request_read(int fd, gavl_metadata_t * req, int timeout)
  {
  int result;
  char * line = NULL;
  int line_alloc = 0;
  char * pos1, *pos2;
  const char * val;
  int status = 0;
  
  if(!bg_socket_read_line(fd, &line, &line_alloc, timeout))
    return 0;

  //  fprintf(stderr, "Got line: %s\n", line);
  
  pos1 = strchr(line, ' ');
  pos2 = strrchr(line, ' ');

  if(!pos1 || !pos2 || (pos1 == pos2))
    return 0;
  
  gavl_metadata_set_nocpy(req, META_METHOD,
                          gavl_strndup( line, pos1));
  gavl_metadata_set_nocpy(req, META_LOCATION,
                           gavl_strndup( pos1+1, pos2));
  gavl_metadata_set_nocpy(req, META_PROTOCOL,
                          gavl_strdup(pos2+1));
  
  result = read_vars(fd, &line, &line_alloc, req, timeout);
  
  if(line)
    free(line);

  if(!result)
    return result;

#ifdef DUMP_HEADERS
  gavl_dprintf("Got request:\n");
  gavl_metadata_dump(req, 0);
#endif
  
  /* We check the protocol version right here */
  val = gavl_metadata_get(req, META_PROTOCOL);
  if(!val)
    status = BG_PLUG_IO_STATUS_400; // 400 Bad Request
  else if(strncmp(val, "HTTP/", 5))
    status = BG_PLUG_IO_STATUS_400; // 400 Bad Request
  
  if(status)
    {
    gavl_metadata_t res;
    gavl_metadata_init(&res);

    gavl_metadata_set_int(&res, META_STATUS, status);
    
    if(!bg_plug_response_write(fd, &res))
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing response failed");
    gavl_metadata_free(&res);
    result = 0;
    }
  
  return result;
  }

static int socket_request_write(int fd, gavl_metadata_t * req)
  {
  const char * method;
  const char * location;
  int result;
  char * line;
  
  method = gavl_metadata_get(req, META_METHOD);
  location = gavl_metadata_get(req, META_LOCATION);

  if(!method || !location)
    return 0;
  
  line = bg_sprintf("%s %s %s\r\n", method, location, PROTOCOL);

  line = write_vars(line, req);
  //  fprintf(stderr, "Writing request:\n%s", line);
  
  result = bg_socket_write_data(fd, (const uint8_t*)line, strlen(line));
  free(line);

  return result;
  }

static int socket_response_read(int fd, gavl_metadata_t * req, int timeout)
  {
  int result;
  char * line = NULL;
  int line_alloc = 0;
  char * pos1;
  char * pos2;
  
  if(!bg_socket_read_line(fd, &line, &line_alloc, timeout))
    return 0;

  //  fprintf(stderr, "Got line: %s\n", line);
  
  pos1 = strchr(line, ' ');
  if(!pos1)
    return 0;

  pos2 = strchr(pos1+1, ' ');
  
  if(!pos2)
    return 0;
  
  gavl_metadata_set_nocpy(req, META_PROTOCOL,
                          gavl_strndup( line, pos1));
  gavl_metadata_set_nocpy(req, META_STATUS,
                          gavl_strndup( pos1+1, pos2));
  
  pos2++;
  gavl_metadata_set(req, META_STATUS_STR, pos2);
  
  result = read_vars(fd, &line, &line_alloc, req, timeout);
  
  if(line)
    free(line);
  
  return result;
  }

int bg_plug_response_write(int fd, gavl_metadata_t * res)
  {
  int result;
  char * line;
  int status, i;
  
  i = 0;
  if(!gavl_metadata_get_int(res, META_STATUS, &status))
    return 0;

  while(status_codes[i].status_str)
    {
    if(status_codes[i].status_int == status)
      break;
    i++;
    }
  if(!status_codes[i].status_str)
    return 0;

  line = bg_sprintf("%s %d %s\r\n", PROTOCOL, status,
                    status_codes[i].status_str);

  line = write_vars(line, res);

#ifdef DUMP_HEADERS
  gavl_dprintf("Writing response:\n%s", line);
#endif
  
  result = bg_socket_write_data(fd, (const uint8_t*)line, strlen(line));
  free(line);
  return result;
  }

void
bg_plug_request_set_method(gavl_metadata_t * req, int method)
  {
  if(method == BG_PLUG_IO_METHOD_READ)
    gavl_metadata_set(req, META_METHOD, "GET");
  else
    gavl_metadata_set(req, META_METHOD, "PUT");
  }

int 
bg_plug_request_get_method(gavl_metadata_t * req, int * method)
  {
  const char * val;

  val = gavl_metadata_get(req, META_METHOD);
  if(!val)
    return 0;
  
  if(!strcmp(val, "PUT"))
    {
    *method = BG_PLUG_IO_METHOD_WRITE;
    return 1;
    }
  else if(!strcmp(val, "GET"))
    {
    *method = BG_PLUG_IO_METHOD_READ;
    return 1;
    }
  else
    return 0;
  }

const char * bg_plug_request_get_location(gavl_metadata_t * req)
  {
  return gavl_metadata_get(req, META_LOCATION);
  }

void
bg_plug_response_set_status(gavl_metadata_t * res, int status)
  {
  gavl_metadata_set_int(res, META_STATUS, status);
  }

static int server_handshake(int fd, int method, int timeout)
  {
  int ret = 0;
  int status = 0;
  gavl_metadata_t req;
  gavl_metadata_t res;
  int request_method;
  const char * var;
  int write_response_now = 0;
  
  gavl_metadata_init(&req);
  gavl_metadata_init(&res);

  if(!bg_plug_request_read(fd, &req, timeout))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Reading request failed");
    goto fail;
    }
  
  if(!bg_plug_request_get_method(&req, &request_method))
    {
    // 400 Bad Request
    status = BG_PLUG_IO_STATUS_400;
    goto fail;
    }

  else if(method == request_method)
    {
    status = BG_PLUG_IO_STATUS_405;
    goto fail;
    }
  
  if(!status)
    {
    switch(request_method)
      {
      case BG_PLUG_IO_METHOD_WRITE:  // PUT
        var = gavl_metadata_get(&req, "Expect");
        if(var)
          {
          if(!strcmp(var, "100-continue"))
            write_response_now = 1;
          else
            {
            status = BG_PLUG_IO_STATUS_417;
            goto fail;
            }
          }

        /* We support no locations in our simple servers */
        var = bg_plug_request_get_location(&req);
        if(strcmp(var, "/"))
          {
          status = BG_PLUG_IO_STATUS_404;
          goto fail;
          }
        
        var = gavl_metadata_get(&req, "Content-Type");
        if(!var)
          {
          status = BG_PLUG_IO_STATUS_400;
          goto fail;
          }
        if(strcmp(var, BGPLUG_MIMETYPE))
          {
          status = BG_PLUG_IO_STATUS_415;
          goto fail;
          }
        if(write_response_now)
          {
          status = BG_PLUG_IO_STATUS_100;
          ret = 1;
          }
        else
          {
          status = BG_PLUG_IO_STATUS_200;
          ret = 1;
          }
        break;
      case BG_PLUG_IO_METHOD_READ: // GET

        /* We support no locations in our simple servers */
        var = bg_plug_request_get_location(&req);
        if(strcmp(var, "/"))
          {
          status = BG_PLUG_IO_STATUS_404;
          goto fail;
          }
        
        status = BG_PLUG_IO_STATUS_200;
        write_response_now = 1;
        gavl_metadata_set(&res, "Content-Type", BGPLUG_MIMETYPE);
        ret = 1;
        break;
      default:
        status = BG_PLUG_IO_STATUS_405;
      }
    }

  fail:
  bg_plug_response_set_status(&res, status);
  
  if(write_response_now)
    {
    fprintf(stderr, "Sending response:\n");
    gavl_metadata_dump(&res, 0);
    
    if(!bg_plug_response_write(fd, &res))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing response failed");
      goto fail;
      }
    }
  
  gavl_metadata_free(&req);
  gavl_metadata_free(&res);
  return ret;
  }


static int client_handshake(int fd, int method, const char * path, int timeout)
  {
  int ret = 0;
  int status = 0;
  gavl_metadata_t req;
  gavl_metadata_t res;

  gavl_metadata_init(&req);
  gavl_metadata_init(&res);

  if(!path)
    path = "/";

  bg_plug_request_set_method(&req, method);
  gavl_metadata_set(&req, META_LOCATION, path);
  gavl_metadata_set(&req, META_PROTOCOL, PROTOCOL);
  
  if(method == BG_PLUG_IO_METHOD_WRITE)
    {
    gavl_metadata_set(&req, "Expect", "100-continue");
    gavl_metadata_set(&req, "Content-Type", BGPLUG_MIMETYPE);
    }
  else if(method == BG_PLUG_IO_METHOD_READ)
    {
    gavl_metadata_set(&req, "Accept", BGPLUG_MIMETYPE);
    }
#ifdef DUMP_HEADERS
  fprintf(stderr, "Sending request\n");
  gavl_metadata_dump(&req, 0);
#endif
  
  if(!socket_request_write(fd, &req))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing request failed");
    goto fail;
    }
  if(!socket_response_read(fd, &res, timeout))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Reading response failed");
    goto fail;
    }

  if(!gavl_metadata_get_int(&res, META_STATUS, &status))
    goto fail;
  
  if(((method == BG_PLUG_IO_METHOD_READ) && (status != BG_PLUG_IO_STATUS_200)) ||
     ((method == BG_PLUG_IO_METHOD_WRITE) && (status != BG_PLUG_IO_STATUS_100)))
    {
    const char * status_str;
    status_str = gavl_metadata_get(&res, META_STATUS_STR);
    if(!status_str)
      status_str = "??";
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Got status: %d %s", status, status_str);
    goto fail;
    }
  ret = 1;
  
  fail:
  gavl_metadata_free(&req);
  gavl_metadata_free(&res);
  return ret;
  }

typedef struct
  {
  int fd;
  int timeout;
  } socket_t;

static int read_socket(void * priv, uint8_t * data, int len)
  {
  socket_t * s = priv;
  return bg_socket_read_data(s->fd, data, len, s->timeout);
  }

static int write_socket(void * priv, const uint8_t * data, int len)
  {
  socket_t * s = priv;
  return bg_socket_write_data(s->fd, data, len);
  }

static void close_socket(void * priv)
  {
  socket_t * s = priv;
  close(s->fd);
  free(s);
  }

static gavf_io_t * io_create_socket(socket_t * s, int method)
  {
  return gavf_io_create((method == BG_PLUG_IO_METHOD_READ) ? read_socket : NULL,
                        (method == BG_PLUG_IO_METHOD_WRITE) ? write_socket : NULL,
                        NULL, // seek
                        close_socket,
                        NULL, // flush
                        s);
  }

static gavf_io_t * open_tcp(const char * location,
                            int method, int * flags, int timeout)
  {
  /* Remote TCP socket */
  char * host = NULL;
  char * path = NULL;
  gavf_io_t * ret = NULL;
  int port;
  int fd;
  socket_t * s;
  bg_host_address_t * addr = NULL;
  
  if(!bg_url_split(location,
                   NULL,
                   NULL,
                   NULL,
                   &host,
                   &port,
                   &path))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Invalid TCP address %s", location);
    goto fail;
    }
  
  if(port < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Port missing in address %s", location);
    goto fail;
    }

  addr = bg_host_address_create();
  if(!bg_host_address_set(addr, host, port, SOCK_STREAM))
    goto fail;
  
  fd = bg_socket_connect_inet(addr, timeout);

  if(fd < 0)
    goto fail;

  /* Handshake */

  if(!client_handshake(fd, method, path, timeout))
    goto fail;
  
  /* Return */
  s = calloc(1, sizeof(*s));
  s->fd = fd;
  s->timeout = timeout;
  
  if(bg_socket_is_local(s->fd))
    *flags |= BG_PLUG_IO_IS_LOCAL;

  ret = io_create_socket(s, method);
  
  fail:
  if(host)
    free(host);
  if(path)
    free(path);
  if(addr)
    bg_host_address_destroy(addr);
  return ret;
  }

static gavf_io_t * open_unix(const char * addr, int method, int timeout)
  {
  char * name = NULL;
  const char * path;
  
  socket_t * s;
  int fd;
  gavf_io_t * ret = NULL;
  gavl_metadata_t vars;
  gavl_metadata_init(&vars);
  
  name = gavl_strdup(addr+7);
  bg_url_get_vars(name, &vars);
  
  fd = bg_socket_connect_unix(name);
  
  if(fd < 0)
    goto fail;

  path = gavl_metadata_get(&vars, "p");
  
  /* Handshake */
  
  if(!client_handshake(fd, method, path, timeout))
    goto fail;

  /* Return */
  s = calloc(1, sizeof(*s));
  s->fd = fd;
  s->timeout = timeout;
  
  ret = io_create_socket(s, method);
  
  fail:
  if(name)
    free(name);
  
  gavl_metadata_free(&vars);
  
  return ret;
  }

static gavf_io_t * open_tcpserv(const char * addr,
                                int method, int * flags, int timeout)
  {
  socket_t * s;
  
  bg_host_address_t * a = NULL;
  int port;
  int server_fd, fd;

  char * host = NULL;
  gavf_io_t * ret = NULL;

  if(!bg_url_split(addr,
                   NULL,
                   NULL,
                   NULL,
                   &host,
                   &port,
                   NULL))
    {
    if(host)
      free(host);
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Invalid TCP address %s", addr);
    goto fail;
    }

  a = bg_host_address_create();
  if(!bg_host_address_set(a, host,
                          port, SOCK_STREAM))
    goto fail;

  server_fd = bg_listen_socket_create_inet(a, 0, 1, 0);
  
  if(server_fd < 0)
    {
    return NULL;
    }
  while(1)
    {
    fd = bg_listen_socket_accept(server_fd, -1);
    
    if(fd < 0)
      break;

    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Got connection");
    
    if(server_handshake(fd, method, timeout))
      break;

    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Handshake failed");
    close(fd);
    }
  
  bg_listen_socket_destroy(server_fd);

  if(fd < 0)
    goto fail;
  
  s = calloc(1, sizeof(*s));
  s->fd = fd;
  s->timeout = timeout;
  
  if(bg_socket_is_local(s->fd))
    *flags |= BG_PLUG_IO_IS_LOCAL;

  ret = io_create_socket(s, method);
  
  fail:

  if(a)
    bg_host_address_destroy(a);
  
  return ret;
  }

static gavf_io_t * open_unixserv(const char * addr, int method, int timeout)
  {
  socket_t * s;
  int server_fd, fd;
  char * name = NULL;
  gavf_io_t * ret = NULL;

  if(!bg_url_split(addr,
                   NULL,
                   NULL,
                   NULL,
                   &name,
                   NULL,
                   NULL))
    {
    if(name)
      free(name);
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Invalid UNIX domain address address %s", addr);
    return NULL;
    }
  server_fd = bg_listen_socket_create_unix(name, 1);
    
  if(server_fd < 0)
    {
    free(name);
    return NULL;
    }
  while(1)
    {
    fd = bg_listen_socket_accept(server_fd, -1);
    
    if(fd < 0)
      break;

    bg_log(BG_LOG_INFO, LOG_DOMAIN,
           "Got connection");
    
    if(server_handshake(fd, method, timeout))
      break;

    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Handshake failed");
    close(fd);
    }
  
  bg_listen_socket_destroy(server_fd);
  
  if(unlink(name))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Unlinking socket failed: %s",
           strerror(errno));
    }
  
  free(name);
  
  if(fd < 0)
    return NULL;

  s = calloc(1, sizeof(*s));
  s->fd = fd;
  s->timeout = timeout;
  
  ret = io_create_socket(s, method);
  
  return ret;
  }

/* File */

static gavf_io_t * open_file(const char * file, int wr, int * flags)
  {
  FILE * f;
  struct stat st;
  
  if(stat(file, &st))
    {
    if(!wr)
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN,
             "Cannot stat %s: %s", file, strerror(errno));
      return NULL;
      }
    }

  if(S_ISFIFO(st.st_mode)) /* Pipe: Use local connection */
    *flags |= BG_PLUG_IO_IS_LOCAL;
  else if(S_ISREG(st.st_mode))
    *flags |= BG_PLUG_IO_IS_REGULAR;
  
  f = fopen(file, (wr ? "w" : "r"));
  if(!f)
    return NULL;
  return gavf_io_create_file(f, wr, !!(*flags & BG_PLUG_IO_IS_REGULAR), 1);
  }

gavf_io_t * bg_plug_io_open_location(const char * location,
                                     int method, int * flags, int timeout)
  {
  *flags = 0;

  if(!location)
    location = "-";
  
  if(!strcmp(location, "-"))
    return open_dash(method, flags);
  else if(!strncmp(location, "tcp://", 6))
    return open_tcp(location, method, flags, timeout);
  else if(!strncmp(location, "unix://", 7))
    {
    *flags |= BG_PLUG_IO_IS_LOCAL;
    /* Local UNIX domain socket */
    return open_unix(location, method, timeout);
    }
  else if(!strncmp(location, "tcpserv://", 10))
    {
    return open_tcpserv(location, method, flags, timeout);
    }
  else if(!strncmp(location, "unixserv://", 11))
    {
    *flags |= BG_PLUG_IO_IS_LOCAL;
    return open_unixserv(location, method, timeout);
    }
  
  else if((location[0] == '|') ||
          (location[0] == '<'))
    {
    /* Pipe */
    *flags |= BG_PLUG_IO_IS_LOCAL;
    return open_pipe(location, method);
    }
  else
    {
    /* Regular file */
    return open_file(location, method, flags);
    }
  return NULL;
  }


gavf_io_t * bg_plug_io_open_socket(int fd,
                                   int method, int * flags, int timeout)
  {
  socket_t * s;

  if(bg_socket_is_local(fd))
    *flags |= BG_PLUG_IO_IS_LOCAL;
  
  /* Handshake was already done at this point */

  /* Return */
  s = calloc(1, sizeof(*s));
  s->fd = fd;
  s->timeout = timeout;
  
  return io_create_socket(s, method);
  }
