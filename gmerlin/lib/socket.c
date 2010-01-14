/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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
#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#include <gmerlin/utils.h>
#include <gmerlin/bgsocket.h>

#include <gmerlin/log.h>
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
  freeaddrinfo(a->addr);
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
#if 0
  if(ret[0].ai_addr->sa_family == AF_INET)
    fprintf(stderr, "Got IPV4 address\n");
  else if(ret[0].ai_addr->sa_family == AF_INET6)
    fprintf(stderr, "Got IPV6 address\n");
#endif  
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
  int err;
  socklen_t err_len;

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

      err = select(ret+1, (fd_set*)0, &write_fds,(fd_set*)0,&timeout);
      
      if(!err)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connection timed out");
        return -1;
        }
      else if(err < 0)
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "select() failed on connect");
        return -1;
        }
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connecting failed: %s", strerror(errno));
      return -1;
      }
    }

  /* Check for error */

  err_len = sizeof(err);
  getsockopt(ret, SOL_SOCKET, SO_ERROR, &err, &err_len);

  if(err)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connecting failed: %s",
           strerror(err));
    return -1;
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

/* Older systems assign an ipv4 address to localhost,
   newer systems (e.g. Ubuntu Karmic) assign an ipv6 address to localhost.

   To test this, we make a name lookup for "localhost" and test if it returns
   an IPV4 or an IPV6 address */

static int have_ipv6()
  {
  struct addrinfo hints;
  struct addrinfo * ret;
  int err, has_ipv6;
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM; // SOCK_DGRAM
  hints.ai_protocol = 0; // 0
  hints.ai_flags    = 0;

  if((err = getaddrinfo("localhost", (char*)0 /* service */,
                        &hints, &ret)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot resolve address of localhost: %s",
           gai_strerror(err));
    return 0;
    }

  if(ret[0].ai_addr->sa_family == AF_INET6)
    has_ipv6 = 1;
  else
    has_ipv6 = 0;

  freeaddrinfo(ret);
  return has_ipv6;
  }

/* Server socket (stream oriented) */

int bg_listen_socket_create_inet(int port,
                                 int queue_size,
                                 int flags)
  {
  int ret, err, use_ipv6;
  struct sockaddr_in  name_ipv4;
  struct sockaddr_in6 name_ipv6;

  if(flags & BG_SOCKET_LOOPBACK)
    use_ipv6 = have_ipv6();
  else
    use_ipv6 = 0;
  
  memset(&name_ipv4, 0, sizeof(name_ipv4));
  memset(&name_ipv6, 0, sizeof(name_ipv6));
  
  if(use_ipv6)
    ret = create_socket(PF_INET6, SOCK_STREAM, 0);
  else
    ret = create_socket(PF_INET, SOCK_STREAM, 0);
  
  /* Create the socket. */
  if (ret < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create inet server socket");
    return -1;
    }
  
  /* Give the socket a name. */

  if(use_ipv6)
    {
    name_ipv6.sin6_family = AF_INET6;
    name_ipv6.sin6_port   = htons(port);
    if(flags & BG_SOCKET_LOOPBACK)
      name_ipv6.sin6_addr = in6addr_loopback;
    else
      name_ipv6.sin6_addr = in6addr_any;

    err = bind(ret, (struct sockaddr *)&name_ipv6, sizeof(name_ipv6));
    }
  else
    {
    name_ipv4.sin_family = AF_INET;
    name_ipv4.sin_port = htons(port);
    if(flags & BG_SOCKET_LOOPBACK)
      name_ipv4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    else
      name_ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
    err = bind(ret, (struct sockaddr *)&name_ipv4, sizeof(name_ipv4));
    }
  
  if(err < 0)
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

int bg_socket_write_data(int fd, const uint8_t * data, int len)
  {
  int result;
  result = send(fd, data, len, MSG_NOSIGNAL);
  if(result != len)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Sending data failed: %s", 
           strerror(errno));    
    return 0;
    }
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

