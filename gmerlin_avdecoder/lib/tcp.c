/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
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


#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
                                                                               
#include <netdb.h> /* gethostbyname */
                                                                               
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <avdec_private.h>
#define LOG_DOMAIN "tcp"

#if !HAVE_DECL_MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#if 0
typedef struct 
  {
  int addr_type; /* AF_INET or AF_INET6 */
                                                                               
  union
    {
    struct in_addr  ipv4_addr;
    struct in6_addr ipv6_addr;
    } addr;
                                                                               
  int port;
  } host_address_t;
#endif

/* Utility functions */

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


#if 0

static int hostbyname(const bgav_options_t * opt,
                      host_address_t * a, const char * hostname)
  {
  struct hostent   h_ent;
  struct hostent * h_ent_p;
  int gethostbyname_buffer_size;
                                                                               
  char * gethostbyname_buffer = (char*)0;
  int result, herr;
  int ret = 0;

  if(inet_aton(hostname, &(a->addr.ipv4_addr)))
    {
    a->addr_type = AF_INET;
    return 1;
    }
  
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
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Could not resolve address %s", hostname);
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
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Could not resolve address %s: No known address space",
             hostname);
    goto fail;
    }
  ret = 1;

  fail:
                                                                               
  if(gethostbyname_buffer)
    free(gethostbyname_buffer);
  return ret;
  }

#endif

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


static struct addrinfo * hostbyname(const bgav_options_t * opt,
                                    const char * hostname, int port, int socktype)
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
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot resolve address of %s: %s",
             hostname, gai_strerror(err));
    return (struct addrinfo *)0;
    }

  address_set_port(ret, port);
  
  return ret;
  }


/* Client connection (stream oriented) */
                                                                               
static int socket_connect_inet(const bgav_options_t * opt, struct addrinfo * addr)
  {
  int ret = -1;

  struct timeval timeout;
  fd_set write_fds;
                                                                               
  /* Create the socket */
  if((ret = create_socket(addr->ai_family, SOCK_STREAM, 0)) < 0)
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Cannot create socket");
    return -1;
    }
  
  /* Set nonblocking mode */
  if(fcntl(ret, F_SETFL, O_NONBLOCK) < 0)
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Cannot set nonblocking mode");
    return -1;
    }
  
  /* Connect the thing */
  if(connect(ret, addr->ai_addr, addr->ai_addrlen)<0)
    {
    if(errno == EINPROGRESS)
      {
      timeout.tv_sec = opt->connect_timeout / 1000;
      timeout.tv_usec = 1000 * (opt->connect_timeout % 1000);
      FD_ZERO (&write_fds);
      FD_SET (ret, &write_fds);
      if(!select(ret+1, (fd_set*)0, &write_fds,(fd_set*)0,&timeout))
        {
        bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Connection timed out");
        return -1;
        }
      }
    else
      {
      bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Connecting failed: %s", strerror(errno));
      return -1;
      }
    }
  
  /* Set back to blocking mode */
  
  if(fcntl(ret, F_SETFL, 0) < 0)
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot set blocking mode");
    return -1;
    }
  return ret;
  }

int bgav_tcp_connect(const bgav_options_t * opt,
                     const char * host, int port)
  {
  struct addrinfo * addr;
  int ret;
  
  addr = hostbyname(opt, host, port, SOCK_STREAM);
  if(!addr)
    return -1;
  ret = socket_connect_inet(opt, addr);
  freeaddrinfo(addr);
  return ret;
  }

int bgav_tcp_send(const bgav_options_t * opt, int fd,
                  uint8_t * data, int len)
  {
  int result;
  result = send(fd, data, len, MSG_NOSIGNAL);
  if(result != len)
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Could not send data: %s", strerror(errno));
    return 0;
    }
  return 1;
  }
