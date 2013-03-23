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

#define TIMEOUT 5000

/* stdin / stdout */

static gavf_io_t * open_dash(int wr, int * flags)
  {
  struct stat st;
  int fd;
  FILE * f;
    
  /* Stdin/stdout */
  if(wr)
    f = stdout;
  else
    f = stdin;
    
  fd = fileno(f);

  if(isatty(fd))
    {
    if(wr)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Not writing to a TTY");
    else
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Not reading from a TTY");
    return NULL;
    }
    
  if(fstat(fd, &st))
    {
    if(wr)
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot stat stdout");
    else
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot stat stdin");
    return 0;
    }
  if(S_ISFIFO(st.st_mode)) /* Pipe: Use local connection */
    *flags |= BG_PLUG_IO_IS_LOCAL;
  else if(S_ISREG(st.st_mode))
    *flags |= BG_PLUG_IO_IS_REGULAR;
  return gavf_io_create_file(f, wr, 0, 0);
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
    { 200, "OK"          },
    { 400, "Bad Request" },
    { 405, "Method Not Allowed" },
    { 505, "Protocol Version Not Supported" },
    { 503, "Service Unavailable" },
    { /* End */ }
  };
  
#define META_METHOD   "$METHOD"
#define META_LOCATION "$LOCATION"
#define META_PROTOCOL "$PROTOCOL"
#define META_STATUS   "$STATUS"

#define PROTOCOL "bgplug/"VERSION

static int read_vars(int fd, char ** line, int * line_alloc,
                     gavl_metadata_t * m)
  {
  char * pos;
  while(1)
    {
    if(!bg_socket_read_line(fd, line, line_alloc, TIMEOUT))
      return 0;

    fprintf(stderr, "Got line: %s\n", *line);
    
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
      line = bg_sprintf("%s: %s\n",
                        m->tags[i].key,
                        m->tags[i].val);

      str = bg_strcat(str, line);
      free(line);
      }
    }
  str = bg_strcat(str, "\n");
  return str;
  }

static int socket_request_read(int fd, gavl_metadata_t * req)
  {
  int result;
  char * line = NULL;
  int line_alloc = 0;
  char * pos1, *pos2;
  
  if(!bg_socket_read_line(fd, &line, &line_alloc, TIMEOUT))
    return 0;

  fprintf(stderr, "Got line: %s\n", line);
  
  pos1 = strchr(line, ' ');
  pos2 = strrchr(line, ' ');

  if(!pos1 || !pos2 || (pos1 == pos2))
    return 0;
  
  gavl_metadata_set_nocpy(req, META_METHOD,
                          bg_strndup(NULL, line, pos1));
  gavl_metadata_set_nocpy(req, META_LOCATION,
                           bg_strndup(NULL, pos1+1, pos2));
  gavl_metadata_set_nocpy(req, META_PROTOCOL,
                          bg_strdup(NULL, pos2+1));
  
  result = read_vars(fd, &line, &line_alloc, req);
  
  if(line)
    free(line);
  
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
  
  line = bg_sprintf("%s %s %s\n", method, location, PROTOCOL);

  line = write_vars(line, req);
  fprintf(stderr, "Writing request:\n%s", line);
  
  result = bg_socket_write_data(fd, (const uint8_t*)line, strlen(line));
  free(line);

  return result;
  }

static int socket_response_read(int fd, gavl_metadata_t * req)
  {
  int result;
  char * line = NULL;
  int line_alloc = 0;
  char * pos1;
  char * pos2;
  
  if(!bg_socket_read_line(fd, &line, &line_alloc, TIMEOUT))
    return 0;

  fprintf(stderr, "Got line: %s\n", line);
  
  pos1 = strchr(line, ' ');
  if(!pos1)
    return 0;

  pos2 = strchr(pos1+1, ' ');
  
  if(!pos2)
    return 0;
  
  gavl_metadata_set_nocpy(req, META_PROTOCOL,
                          bg_strndup(NULL, line, pos1));
  gavl_metadata_set_nocpy(req, META_STATUS,
                          bg_strndup(NULL, pos1+1, pos2));
  
  result = read_vars(fd, &line, &line_alloc, req);
  
  if(line)
    free(line);
  
  return result;
  }

static int
socket_response_write(int fd, gavl_metadata_t * req)
  {
  int result;
  char * line;
  int status, i;
  
  i = 0;
  if(!gavl_metadata_get_int(req, META_STATUS, &status))
    return 0;

  while(status_codes[i].status_str)
    {
    if(status_codes[i].status_int == status)
      break;
    i++;
    }
  if(!status_codes[i].status_str)
    return 0;

  line = bg_sprintf("%s %d %s\n", PROTOCOL, status,
                    status_codes[i].status_str);

  line = write_vars(line, req);

  fprintf(stderr, "Writing response:\n%s", line);
  
  result = bg_socket_write_data(fd, (const uint8_t*)line, strlen(line));
  free(line);
  return result;
  }

static int socket_handshake(int fd, int wr, int server)
  {
  int ret = 0;
  const char * val;
  int status = 0;
  gavl_metadata_t req;
  gavl_metadata_t res;

  gavl_metadata_init(&req);
  gavl_metadata_init(&res);
  
  if(server)
    {
    if(!socket_request_read(fd, &req))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Reading request failed");
      goto fail;
      }
    val = gavl_metadata_get(&req, "$PROTOCOL");
    if(!val)
      {
      // 400 Bad Request
      status = 400;
      }
    else if(strcmp(val, PROTOCOL))
      {
      // 505 Protocol Version Not Supported
      status = 505;
      }
    val = gavl_metadata_get(&req, "$METHOD");
    if(!val)
      {
      // 400 Bad Request
      status = 400;
      }
    if(wr && !strcmp(val, "WRITE"))
      {
      // 405 Method Not Allowed
      status = 405;
      }
    else if(!wr && !strcmp(val, "READ"))
      {
      // 405 Method Not Allowed
      status = 405;
      }
    if(!status)
      {
      status = 200;
      ret = 1;
      }
    
    gavl_metadata_set_int(&res, META_STATUS, status);
    
    if(!socket_response_write(fd, &res))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing response failed");
      goto fail;
      }
    }
  else
    {
    if(wr)
      gavl_metadata_set(&req, "$METHOD", "WRITE");
    else
      gavl_metadata_set(&req, "$METHOD", "READ");
    gavl_metadata_set(&req, "$LOCATION", "*");
    gavl_metadata_set(&req, "$PROTOCOL", PROTOCOL);
    if(!socket_request_write(fd, &req))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing request failed");
      goto fail;
      }
    if(!socket_response_read(fd, &res))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Reading response failed");
      goto fail;
      }

    if(!gavl_metadata_get_int(&res, META_STATUS, &status))
      goto fail;

    if(status != 200)
      goto fail;
    
    ret = 1;
    }

  fail:
  gavl_metadata_free(&req);
  gavl_metadata_free(&res);
  return ret;
  }

typedef struct
  {
  int fd;
  } socket_t;

static int read_socket(void * priv, uint8_t * data, int len)
  {
  socket_t * s = priv;
  return bg_socket_read_data(s->fd, data, len, TIMEOUT);
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

static gavf_io_t * open_tcp(const char * location, int wr)
  {
  /* Remote TCP socket */
  char * host = NULL;
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
                   NULL))
    {
    if(host)
      free(host);
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Invalid TCP address %s", location);
    return NULL;
    }
  
  if(port < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Port missing in address %s", location);
    return NULL;
    }

  addr = bg_host_address_create();
  if(!bg_host_address_set(addr, host, port, SOCK_STREAM))
    goto fail;
  
  fd = bg_socket_connect_inet(addr, TIMEOUT);

  if(fd < 0)
    goto fail;

  /* Handshake */

  if(!socket_handshake(fd, wr, 0))
    goto fail;
  
  /* Return */
  s = calloc(1, sizeof(*s));

  ret = gavf_io_create(wr ? NULL : read_socket,
                       wr ? write_socket : NULL,
                       NULL, // seek
                       close_socket,
                       NULL, // flush
                       s);
  
  fail:
  if(host)
    free(host);
  if(addr)
    bg_host_address_destroy(addr);
  return ret;
  }

static gavf_io_t * open_unix(const char * addr, int wr)
  {
  char * name = NULL;
  socket_t * s;
  int fd;
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
  
  fd = bg_socket_connect_unix(name);
  free(name);
  
  if(fd < 0)
    return NULL;

  /* Handshake */
  
  if(!socket_handshake(fd, wr, 0))
    return NULL;

  /* Return */
  s = calloc(1, sizeof(*s));
  s->fd = fd;

  ret = gavf_io_create(wr ? NULL : read_socket,
                       wr ? write_socket : NULL,
                       NULL, // seek
                       close_socket,
                       NULL, // flush
                       s);
  return ret;
  }

static gavf_io_t * open_tcpserv(const char * addr, int wr)
  {
  int port;
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
    return NULL;
    }

  
  }

static gavf_io_t * open_unixserv(const char * addr, int wr)
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
    
    if(socket_handshake(fd, wr, 1))
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
  ret = gavf_io_create(wr ? NULL : read_socket,
                       wr ? write_socket : NULL,
                       NULL, // seek
                       close_socket,
                       NULL, // flush
                       s);
  
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
                                     int wr, int * flags)
  {
  *flags = 0;

  if(!location)
    location = "-";
  
  if(!strcmp(location, "-"))
    return open_dash(wr, flags);
  else if(!strncmp(location, "tcp://", 6))
    return open_tcp(location, wr);
  else if(!strncmp(location, "unix://", 7))
    {
    *flags |= BG_PLUG_IO_IS_LOCAL;
    /* Local UNIX domain socket */
    return open_unix(location, wr);
    }
  else if(!strncmp(location, "tcpserv://", 10))
    {
    return open_tcpserv(location, wr);
    }
  else if(!strncmp(location, "unixserv://", 11))
    {
    *flags |= BG_PLUG_IO_IS_LOCAL;
    return open_unixserv(location, wr);
    }
  
  else if((location[0] == '|') ||
          (location[0] == '<'))
    {
    /* Pipe */
    *flags |= BG_PLUG_IO_IS_LOCAL;
    return open_pipe(location, wr);
    }
  else
    {
    /* Regular file */
    return open_file(location, wr, flags);
    }
  return NULL;
  }

/* Socket utility function */

static int socket_is_local(int fd)
  {
  struct sockaddr_storage ss;
  socklen_t slen;
  slen = sizeof(ss);
  
  if(getsockname(fd, (struct sockaddr*)&ss, &slen) == -1)
    return 1;

  if(slen == 0 || ss.ss_family == AF_LOCAL)
    return 1;

  if(ss.ss_family == AF_INET)
    {
    struct sockaddr_in * a = (struct sockaddr_in *)&ss;
    if(a->sin_addr.s_addr == INADDR_LOOPBACK)
      return 1;
    }
  return 0;
  }

gavf_io_t * bg_plug_io_open_socket(int fd,
                                   int wr, int * flags)
  {
  socket_t * s;

  if(socket_is_local(fd))
    *flags |= BG_PLUG_IO_IS_LOCAL;
  
  /* TODO: Handshake */

  /* Return */
  s = calloc(1, sizeof(*s));
  s->fd = fd;
  return gavf_io_create(wr ? NULL : read_socket,
                        wr ? write_socket : NULL,
                        NULL, // seek
                        close_socket,
                        NULL, // flush
                        s);
  }
