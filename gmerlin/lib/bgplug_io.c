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

#include <unistd.h>
#include <ctype.h>

#include <gmerlin/bgplug.h>
#include <gmerlin/subprocess.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/utils.h>

#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "plug_io"

#define TIMEOUT 1000

/* stdin / stdout */

static gavf_io_t * open_dash(int wr, int * do_shm)
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
    *do_shm = 1;
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

  /* TODO: Handshake */

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
  socket_t * s;
  int fd;
  gavf_io_t * ret = NULL;
  
  fd = bg_socket_connect_unix(addr);

  if(fd < 0)
    return NULL;

  /* TODO: Handshake */

  /* Return */
  s = calloc(1, sizeof(*s));

  ret = gavf_io_create(wr ? NULL : read_socket,
                       wr ? write_socket : NULL,
                       NULL, // seek
                       close_socket,
                       NULL, // flush
                       s);
  return ret;
  }

/* File */

static gavf_io_t * open_file(const char * file, int wr, int * do_shm)
  {
  FILE * f;
  int can_seek = 0;
  struct stat st;
  
  if(stat(file, &st))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN,
           "Cannot stat %s: %s", file, strerror(errno));
    }

  if(S_ISFIFO(st.st_mode)) /* Pipe: Use local connection */
    *do_shm = 1;
  else if(S_ISREG(st.st_mode))
    can_seek = 1;
  
  f = fopen(file, (wr ? "w" : "r"));
  if(!f)
    return NULL;
  return gavf_io_create_file(f, wr, can_seek, 1);
  }

gavf_io_t * bg_plug_open_location(const char * location,
                                  int wr, int * do_shm)
  {
  *do_shm = 0;
  
  if(!strcmp(location, "-"))
    return open_dash(wr, do_shm);
  else if(!strncmp(location, "tcp://", 6))
    return open_tcp(location, wr);
  else if(!strncmp(location, "unix://", 7))
    {
    *do_shm = 1;
    /* Local UNIX domain socket */
    return open_unix(location, wr);
    }
  else if((location[0] == '|') ||
          (location[0] == '<'))
    {
    /* Pipe */
    *do_shm = 1;
    return open_pipe(location, wr);
    }
  else
    {
    /* Regular file */
    return open_file(location, wr, do_shm);
    }
  return NULL;
  }

/* Socket utility function */

static int socket_is_local(int fd)
  {
  struct sockaddr_storage ss;
  socklen_t slen;
  slen = sizeof(ss);
  
  if(getsockname(fd, (struct sockaddr*)&ss, &slen) == -1 ||
     slen == 0 ||
     ss.ss_family == AF_LOCAL)
    return 1;
  
  return 0;
  }

gavf_io_t * bg_plug_open_socket(int fd,
                                int wr, int * use_shm)
  {
  socket_t * s;
  
  *use_shm = socket_is_local(fd);

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
