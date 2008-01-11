/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif

#include <unistd.h>

#include <netdb.h> /* gethostbyname */

#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#include <utils.h>
#include <bgsocket.h>

#include <log.h>
#define LOG_DOMAIN "tcpsocket"

#if !HAVE_DECL_MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* Opaque address structure so we can support IPv6 in the future */

struct bg_host_address_s 
  {
  struct addrinfo * addr;
  };

bg_host_address_t * bg_host_address_create()
  {
  bg_host_address_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_host_address_destroy(bg_host_address_t * a)
  {
  free(a);
  }

static int create_socket(int domain, int type, int protocol)
  {
  int ret;
#if HAVE_DECL_SO_NOSIGPIPE // OSX
  int value = 1;
#endif

  ret = socket(domain, type, protocol);

#if HAVE_DECL_SO_NOSIGPIPE // OSX
  if(ret < 0)
    return ret;
  if(setsockopt(ret, SOL_SOCKET, SO_NOSIGPIPE, &value,
                sizeof(int)) == -1)
    return -1;
#endif
  return ret;
  }


/* gethostbyname */

#if 0
static int hostbyname(bg_host_address_t * a, const char * hostname)
  {
  struct hostent   h_ent;
  struct hostent * h_ent_p;
  int gethostbyname_buffer_size;

  char * gethostbyname_buffer = (char*)0;
  int result, herr;
  int ret = 0;
  
  gethostbyname_buffer_size = 1024;
  gethostbyname_buffer = malloc(gethostbyname_buffer_size);

    
  while((result = gethostbyname_r(hostname,
                                  &h_ent,
                                  gethostbyname_buffer,
                                  gethostbyname_buffer_size,
                                  &h_ent_p, &herr)) == ERANGE)
    {
    gethostbyname_buffer_size *= 2;
    gethostbyname_buffer = realloc(gethostbyname_buffer,
                                  gethostbyname_buffer_size);
    }
  
  /* Fill in return value           */

  if(result || (h_ent_p == NULL))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not resolve address");
    goto fail;
    }
  if(h_ent_p->h_addrtype == AF_INET)
    {
    a->addr_type = AF_INET;
    memcpy(&(a->addr.ipv4_addr),
           h_ent_p->h_addr, sizeof(a->addr.ipv4_addr));
    }
  else if(h_ent_p->h_addrtype == AF_INET6)
    {
    a->addr_type = AF_INET6;
    memcpy(&(a->addr.ipv6_addr),
           h_ent_p->h_addr, sizeof(a->addr.ipv6_addr));
    }
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "No known address space");
    goto fail;
    }
  ret = 1;

  fail:
  
  if(gethostbyname_buffer)
    free(gethostbyname_buffer);
  return ret;
  }
#endif

/* */

static void address_set_port(struct addrinfo * info, int port)
  {
  while(info)
    {
    switch(info->ai_family)
      {
      case AF_INET:
        {
        struct sockaddr_in * addr;
        addr = (struct sockaddr_in*)info->ai_addr;
        addr->sin_port = htons(port);
        }
        break;
      case AF_INET6:
        {
        struct sockaddr_in6 * addr;
        addr = (struct sockaddr_in6*)info->ai_addr;
        addr->sin6_port = htons(port);
        }
        break;
      default:
        break;
      }
    info = info->ai_next;
    }
  }

static struct addrinfo * hostbyname(const char * hostname, int port, int socktype)
  {
  int err;
  struct in_addr ipv4_addr;
  
  struct addrinfo hints;
  struct addrinfo * ret;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = PF_UNSPEC;
  hints.ai_socktype = socktype; // SOCK_STREAM, SOCK_DGRAM
  hints.ai_protocol = 0; // 0
  hints.ai_flags    = 0;

  /* prevent DNS lookup for numeric IP addresses */

  if(inet_aton(hostname, &(ipv4_addr)))
    hints.ai_flags |= AI_NUMERICHOST;
  if((err = getaddrinfo(hostname, (char*)0 /* service */,
                        &hints, &ret)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot resolve address of %s: %s",
           hostname, gai_strerror(err));
    return (struct addrinfo *)0;
    }

  address_set_port(ret, port);
  
  return ret;
  }

int bg_host_address_set(bg_host_address_t * a, const char * hostname,
                        int port, int socktype)
  {
  a->addr = hostbyname(hostname, port, socktype);
  //  if(!hostbyname(a, hostname))
  //    return 0;
  //  a->port = port;
  
  if(!a->addr)
    return 0;
  return 1;
  }

/* Client connection (stream oriented) */

int bg_socket_connect_inet(bg_host_address_t * a, int milliseconds)
  {
  int ret = -1;

  struct timeval timeout;
  fd_set write_fds;
                                                                               
  /* Create the socket */
  if((ret = create_socket(a->addr->ai_family, SOCK_STREAM, 0)) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create socket");
    return -1;
    }
  
  /* Set nonblocking mode */
  if(fcntl(ret, F_SETFL, O_NONBLOCK) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot set nonblocking mode");
    return -1;
    }
  
  /* Connect the thing */
  if(connect(ret, a->addr->ai_addr, a->addr->ai_addrlen)<0)
    {
    if(errno == EINPROGRESS)
      {
      timeout.tv_sec = milliseconds / 1000;
      timeout.tv_usec = 1000 * (milliseconds % 1000);
      FD_ZERO (&write_fds);
      FD_SET (ret, &write_fds);
      if(!select(ret+1, (fd_set*)0, &write_fds,(fd_set*)0,&timeout))
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connection timed out");
        return -1;
        }
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connecting failed: %s", strerror(errno));
      return -1;
      }
    }
  
  /* Set back to blocking mode */
  
  if(fcntl(ret, F_SETFL, 0) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot set blocking mode");
    return -1;
    }
  return ret;
  }

int bg_socket_connect_unix(const char * name)
  {
  struct sockaddr_un addr;
  int addr_len;
  int ret;
  ret = create_socket(PF_LOCAL, SOCK_STREAM, 0);
  if(ret < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create unix socket");
    return -1;
    }

  addr.sun_family = AF_LOCAL;
  strncpy (addr.sun_path, name, sizeof(addr.sun_path));
  addr.sun_path[sizeof (addr.sun_path) - 1] = '\0';
  addr_len = SUN_LEN(&addr);
  
  if(connect(ret,(struct sockaddr*)(&addr),addr_len)<0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connecting unix socket failed");
    return -1;
    }
  return ret;
  }

void bg_socket_disconnect(int sock)
  {
  close(sock);
  }

/* Server socket (stream oriented) */

int bg_listen_socket_create_inet(int port,
                                 int queue_size,
                                 uint32_t addr)
  {
  int ret;
  struct sockaddr_in name;
  
  /* Create the socket. */
  ret = create_socket(PF_INET, SOCK_STREAM, 0);
  if (ret < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create inet server socket");
    return -1;
    }
  
  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (ret, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot bind inet socket: %s", strerror(errno));
    return -1;
    }
  if(fcntl(ret, F_SETFL, O_NONBLOCK) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot set nonblocking mode");
    return -1;
    }
  if(listen(ret, queue_size))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot put socket into listening mode");
    return -1;
    }
  return ret;
  }

int bg_listen_socket_create_unix(const char * name,
                                 int queue_size)
  {
  int ret;

  struct sockaddr_un addr;
  int addr_len;
  ret = create_socket(PF_LOCAL, SOCK_STREAM, 0);
  if(ret < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create unix server socket");
    return -1;
    }

  addr.sun_family = AF_LOCAL;
  strncpy (addr.sun_path, name, sizeof(addr.sun_path));
  addr.sun_path[sizeof (addr.sun_path) - 1] = '\0';
  addr_len = SUN_LEN(&addr);
  if(bind(ret,(struct sockaddr*)(&addr),addr_len)<0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Could not bind socket");
    return -1;
    }
  if(fcntl(ret, F_SETFL, O_NONBLOCK) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot set nonblocking mode");
    return -1;
    }
  if(listen(ret, queue_size))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot put socket into listening mode");
    return -1;
    }
  return ret;
  }

/* Accept a new client connection, return -1 if there is none */

int bg_listen_socket_accept(int sock)
  {
  int ret;
  ret = accept(sock, NULL, NULL);
  if(ret < 0)
    return -1;
  return ret;
  }

void bg_listen_socket_destroy(int sock)
  {
  close(sock);
  }

int bg_socket_read_data(int fd, uint8_t * data, int len, int milliseconds)
  {
  int result;
  fd_set rset;
  struct timeval timeout;

  if(milliseconds >= 0)
    {
    FD_ZERO (&rset);
    FD_SET  (fd, &rset);
    
    timeout.tv_sec  = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    
    if(select (fd+1, &rset, NULL, NULL, &timeout) <= 0)
      {
      // bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Got read timeout");
      return 0;
      }
    }

  result = recv(fd, data, len, MSG_WAITALL);
  if(result <= 0)
    {
    return 0;
    }
  return result;
  }

int bg_socket_write_data(int fd, uint8_t * data, int len)
  {
  int result;
  result = send(fd, data, len, MSG_NOSIGNAL);
  if(result != len)
    return 0;
  return result;
  }

/*
 *  Read a single line from a filedescriptor
 *
 *  ret will be reallocated if neccesary and ret_alloc will
 *  be updated then
 *
 *  The string will be 0 terminated, a trailing \r or \n will
 *  be removed
 */

#define BYTES_TO_ALLOC 1024

int bg_socket_read_line(int fd, char ** ret,
                        int * ret_alloc, int milliseconds)
  {
  char * pos;
  char c;
  int bytes_read;
  bytes_read = 0;
  /* Allocate Memory for the case we have none */
  if(!(*ret_alloc))
    {
    *ret_alloc = BYTES_TO_ALLOC;
    *ret = realloc(*ret, *ret_alloc);
    }
  pos = *ret;
  while(1)
    {
    if(!bg_socket_read_data(fd, (uint8_t*)(&c), 1, milliseconds))
      {
      if(!bytes_read)
        return 0;
      break;
      }
    /*
     *  Line break sequence
     *  is starting, remove the rest from the stream
     */
    if(c == '\n')
      {
      break;
      }
    /* Reallocate buffer */
    else if(c != '\r')
      {
      if(bytes_read+2 >= *ret_alloc)
        {
        *ret_alloc += BYTES_TO_ALLOC;
        *ret = realloc(*ret, *ret_alloc);
        pos = &((*ret)[bytes_read]);
        }
      /* Put the byte and advance pointer */
      *pos = c;
      pos++;
      bytes_read++;
      }
    }
  *pos = '\0';
  return 1;
  }

