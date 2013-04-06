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

static int socket_is_local(int fd);

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
    { BG_PLUG_IO_STATUS_200, "OK"          },
    { BG_PLUG_IO_STATUS_400, "Bad Request" },
    { BG_PLUG_IO_STATUS_404, "Not Found" },
    { BG_PLUG_IO_STATUS_405, "Method Not Allowed" },
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

#define PROTOCOL "bgplug/"VERSION



static int read_vars(int fd, char ** line, int * line_alloc,
                     gavl_metadata_t * m)
  {
  char * pos;
  while(1)
    {
    if(!bg_socket_read_line(fd, line, line_alloc, TIMEOUT))
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

int bg_plug_request_read(int fd, gavl_metadata_t * req)
  {
  int result;
  char * line = NULL;
  int line_alloc = 0;
  char * pos1, *pos2;
  const char * val;
  int status = 0;
  
  
  if(!bg_socket_read_line(fd, &line, &line_alloc, TIMEOUT))
    return 0;

  //  fprintf(stderr, "Got line: %s\n", line);
  
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

  if(!result)
    return result;

  //  fprintf(stderr, "Got request:\n");
  //  gavl_metadata_dump(req, 0);
  
  /* We check the protocol version right here */
  val = gavl_metadata_get(req, META_PROTOCOL);
  if(!val)
    status = BG_PLUG_IO_STATUS_400; // 400 Bad Request
  else if(strcmp(val, PROTOCOL))
    status = BG_PLUG_IO_STATUS_505; // 505 Protocol Version Not Supported
  
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
  
  line = bg_sprintf("%s %s %s\n", method, location, PROTOCOL);

  line = write_vars(line, req);
  //  fprintf(stderr, "Writing request:\n%s", line);
  
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

  //  fprintf(stderr, "Got line: %s\n", line);
  
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
  
  pos2++;
  gavl_metadata_set(req, META_STATUS_STR, pos2);
  
  result = read_vars(fd, &line, &line_alloc, req);
  
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

  line = bg_sprintf("%s %d %s\n", PROTOCOL, status,
                    status_codes[i].status_str);

  line = write_vars(line, res);

  //  fprintf(stderr, "Writing response:\n%s", line);
  
  result = bg_socket_write_data(fd, (const uint8_t*)line, strlen(line));
  free(line);
  return result;
  }

void
bg_plug_request_set_method(gavl_metadata_t * req, int method)
  {
  if(method == BG_PLUG_IO_METHOD_READ)
    gavl_metadata_set(req, META_METHOD, "READ");
  else
    gavl_metadata_set(req, META_METHOD, "WRITE");
  }

int 
bg_plug_request_get_method(gavl_metadata_t * req, int * method)
  {
  const char * val;

  val = gavl_metadata_get(req, META_METHOD);
  if(!val)
    return 0;
  
  if(!strcmp(val, "WRITE"))
    {
    *method = BG_PLUG_IO_METHOD_WRITE;
    return 1;
    }
  else if(!strcmp(val, "READ"))
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

static int server_handshake(int fd, int method)
  {
  int ret = 0;
  int status = 0;
  gavl_metadata_t req;
  gavl_metadata_t res;
  int request_method;
  const char * location;
  
  gavl_metadata_init(&req);
  gavl_metadata_init(&res);

  if(!bg_plug_request_read(fd, &req))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Reading request failed");
    goto fail;
    }
  
  if(!bg_plug_request_get_method(&req, &request_method))
    {
    // 400 Bad Request
    status = BG_PLUG_IO_STATUS_400;
    }

  else if(method == request_method)
    status = BG_PLUG_IO_STATUS_405;

  else
    {
    location = bg_plug_request_get_location(&req);

    /* We support no locations in our simple servers */
    if(strcmp(location, "/"))
      status = BG_PLUG_IO_STATUS_404;
    }
  
  if(!status)
    {
    status = BG_PLUG_IO_STATUS_200;
    ret = 1;
    }

  bg_plug_response_set_status(&res, status);
  

  //  fprintf(stderr, "Sending response:\n");
  //  gavl_metadata_dump(&res, 0);
  
  if(!bg_plug_response_write(fd, &res))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Writing response failed");
    goto fail;
    }

  fail:
  gavl_metadata_free(&req);
  gavl_metadata_free(&res);
  return ret;
  
  }


static int client_handshake(int fd, int method, const char * path)
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
  
  if(status != BG_PLUG_IO_STATUS_200)
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
                            int method, int * flags)
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
  
  fd = bg_socket_connect_inet(addr, TIMEOUT);

  if(fd < 0)
    goto fail;

  /* Handshake */

  if(!client_handshake(fd, method, path))
    goto fail;
  
  /* Return */
  s = calloc(1, sizeof(*s));
  s->fd = fd;
  
  if(socket_is_local(s->fd))
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

static gavf_io_t * open_unix(const char * addr, int method)
  {
  char * name = NULL;
  char * path = NULL;
  socket_t * s;
  int fd;
  gavf_io_t * ret = NULL;

  if(!bg_url_split(addr,
                   NULL,
                   NULL,
                   NULL,
                   &name,
                   NULL,
                   &path))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Invalid UNIX domain address address %s", addr);
    goto fail;
    }
  
  fd = bg_socket_connect_unix(name);
  
  if(fd < 0)
    goto fail;

  /* Handshake */
  
  if(!client_handshake(fd, method, path))
    goto fail;

  /* Return */
  s = calloc(1, sizeof(*s));
  s->fd = fd;

  ret = io_create_socket(s, method);
  
  fail:
  if(name)
    free(name);
  if(path)
    free(path);
  
  return ret;
  }

static gavf_io_t * open_tcpserv(const char * addr, int method, int * flags)
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
    
    if(server_handshake(fd, method))
      break;

    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Handshake failed");
    close(fd);
    }
  
  bg_listen_socket_destroy(server_fd);

  if(fd < 0)
    goto fail;
  
  s = calloc(1, sizeof(*s));
  s->fd = fd;

  if(socket_is_local(s->fd))
    *flags |= BG_PLUG_IO_IS_LOCAL;

  ret = io_create_socket(s, method);
  
  fail:

  if(a)
    bg_host_address_destroy(a);
  
  return ret;
  }

static gavf_io_t * open_unixserv(const char * addr, int method)
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
    
    if(server_handshake(fd, method))
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
                                     int method, int * flags)
  {
  *flags = 0;

  if(!location)
    location = "-";
  
  if(!strcmp(location, "-"))
    return open_dash(method, flags);
  else if(!strncmp(location, "tcp://", 6))
    return open_tcp(location, method, flags);
  else if(!strncmp(location, "unix://", 7))
    {
    *flags |= BG_PLUG_IO_IS_LOCAL;
    /* Local UNIX domain socket */
    return open_unix(location, method);
    }
  else if(!strncmp(location, "tcpserv://", 10))
    {
    return open_tcpserv(location, method, flags);
    }
  else if(!strncmp(location, "unixserv://", 11))
    {
    *flags |= BG_PLUG_IO_IS_LOCAL;
    return open_unixserv(location, method);
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

/* Socket utility function */

static int socket_is_local(int fd)
  {
  struct sockaddr_storage us;
  struct sockaddr_storage them;
  socklen_t slen;
  slen = sizeof(us);
  
  if(getsockname(fd, (struct sockaddr*)&us, &slen) == -1)
    return 1;

  if(slen == 0 || us.ss_family == AF_LOCAL)
    return 1;

  if(us.ss_family == AF_INET)
    {
    struct sockaddr_in * a1, *a2;
    a1 = (struct sockaddr_in *)&us;
    if(a1->sin_addr.s_addr == INADDR_LOOPBACK)
      return 1;

    slen = sizeof(them);
    
    if(getpeername(fd, (struct sockaddr*)&them, &slen) == -1)
      return 0;

    a2 = (struct sockaddr_in *)&them;

    if(!memcmp(&a1->sin_addr.s_addr, &a2->sin_addr.s_addr,
               sizeof(a2->sin_addr.s_addr)))
      return 1;
    }
  else if(us.ss_family == AF_INET6)
    {
    struct sockaddr_in6 * a1, *a2;
    a1 = (struct sockaddr_in6 *)&us;

    /* Detect loopback */
    if(!memcmp(&a1->sin6_addr.s6_addr, &in6addr_loopback,
               sizeof(in6addr_loopback)))
      return 1;

    if(getpeername(fd, (struct sockaddr*)&them, &slen) == -1)
      return 0;

    a2 = (struct sockaddr_in6 *)&them;

    if(!memcmp(&a1->sin6_addr.s6_addr, &a2->sin6_addr.s6_addr,
               sizeof(a2->sin6_addr.s6_addr)))
      return 1;

    }
  return 0;
  }

gavf_io_t * bg_plug_io_open_socket(int fd,
                                   int method, int * flags)
  {
  socket_t * s;

  if(socket_is_local(fd))
    *flags |= BG_PLUG_IO_IS_LOCAL;
  
  /* Handshake was already done at this point */

  /* Return */
  s = calloc(1, sizeof(*s));
  s->fd = fd;
  
  return io_create_socket(s, method);
  }
