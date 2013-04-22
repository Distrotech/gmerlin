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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#include <net/if.h>
#endif

#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif

#include <netinet/tcp.h> // IPPROTO_TCP, TCP_MAXSEG

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
#define LOG_DOMAIN "socket"

#if !HAVE_DECL_MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/* Opaque address structure so we can support IPv6 */

struct bg_socket_address_s 
  {
  struct sockaddr_storage addr;
  size_t len;
  //  struct addrinfo * addr;
  };

bg_socket_address_t * bg_socket_address_create()
  {
  bg_socket_address_t * ret;
  ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_socket_address_destroy(bg_socket_address_t * a)
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

/* */

void bg_socket_address_set_port(bg_socket_address_t * addr, int port)
  {
  switch(addr->addr.ss_family)
    {
    case AF_INET:
      {
      struct sockaddr_in * a;
      a = (struct sockaddr_in*)&addr->addr;
      a->sin_port = htons(port);
      }
      break;
    case AF_INET6:
      {
      struct sockaddr_in6 * a;
      a = (struct sockaddr_in6*)&addr->addr;
      a->sin6_port = htons(port);
      }
      break;
    default:
      break;
    }
  }

int bg_socket_address_get_port(bg_socket_address_t * addr)
  {
  switch(addr->addr.ss_family)
    {
    case AF_INET:
      {
      struct sockaddr_in * a;
      a = (struct sockaddr_in*)&addr->addr;
      return ntohs(a->sin_port);
      }
      break;
    case AF_INET6:
      {
      struct sockaddr_in6 * a;
      a = (struct sockaddr_in6*)&addr->addr;
      return ntohs(a->sin6_port);
      }
      break;
    default:
      break;
    }
  return 0;
  }

char * bg_socket_address_to_string(bg_socket_address_t * addr)
  {
  switch(addr->addr.ss_family)
    {
    case AF_INET:
      {
      char buf[INET_ADDRSTRLEN];

      struct sockaddr_in * a;
      a = (struct sockaddr_in*)&addr->addr;

      inet_ntop(AF_INET, &a->sin_addr, buf, INET_ADDRSTRLEN);
      return bg_sprintf("%s:%d", buf, ntohs(a->sin_port));
      }
      break;
    case AF_INET6:
      {
      char buf[INET6_ADDRSTRLEN];
      struct sockaddr_in6 * a;
      
      a = (struct sockaddr_in6*)&addr->addr;

      inet_ntop(AF_INET6, &a->sin6_addr, buf, INET6_ADDRSTRLEN);
      return bg_sprintf("[%s]:%d", buf, ntohs(a->sin6_port));
      }
      break;
    default:
      break;
    }
  return NULL;
  }

static struct addrinfo *
hostbyname(const char * hostname, int socktype)
  {
  int err;
  struct in_addr ipv4_addr;
  struct in6_addr ipv6_addr;
  
  struct addrinfo hints;
  struct addrinfo * ret;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = PF_UNSPEC;
  hints.ai_socktype = socktype; // SOCK_STREAM, SOCK_DGRAM
  hints.ai_protocol = 0; // 0
  hints.ai_flags    = 0;

  /* prevent DNS lookup for numeric IP addresses */

  if(inet_pton(AF_INET, hostname, &ipv4_addr))
    {
    hints.ai_flags |= AI_NUMERICHOST;
    hints.ai_family   = PF_INET;
    }
  else if(inet_pton(AF_INET6, hostname, &ipv6_addr))
    {
    hints.ai_flags |= AI_NUMERICHOST;
    hints.ai_family   = PF_INET6;
    }
  
  if((err = getaddrinfo(hostname, NULL /* service */,
                        &hints, &ret)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot resolve address of %s: %s",
           hostname, gai_strerror(err));
    return NULL;
    }
#if 0
  if(ret[0].ai_addr->sa_family == AF_INET)
    fprintf(stderr, "Got IPV4 address\n");
  else if(ret[0].ai_addr->sa_family == AF_INET6)
    fprintf(stderr, "Got IPV6 address\n");
#endif  
  
  return ret;
  }

int bg_socket_address_set(bg_socket_address_t * a, const char * hostname,
                        int port, int socktype)
  {
  struct addrinfo * addr;
  
  addr = hostbyname(hostname, socktype);
  if(!addr)
    return 0;
  
  memcpy(&a->addr, addr->ai_addr, addr->ai_addrlen);
  a->len = addr->ai_addrlen;
  //  if(!hostbyname(a, hostname))
  //    return 0;
  //  a->port = port;

  freeaddrinfo(addr);

  bg_socket_address_set_port(a, port);
  
  return 1;
  }

int bg_socket_address_set_local(bg_socket_address_t * a, int port, int socktype)
  {
#ifdef HAVE_IFADDRS_H  
  int ret = 0;
  struct ifaddrs * ifap = NULL;
  struct ifaddrs * addr;
  if(getifaddrs(&ifap))
    return 0;

  addr = ifap;
  while(addr)
    {
    if(!(addr->ifa_flags & IFF_LOOPBACK))
      {
      if(addr->ifa_addr->sa_family == AF_INET)
        a->len = sizeof(struct sockaddr_in);
      else if(addr->ifa_addr->sa_family == AF_INET6)
        a->len = sizeof(struct sockaddr_in6);
      else
        {
        addr = addr->ifa_next;
        continue;
        }
      memcpy(&a->addr, addr->ifa_addr, a->len);
      bg_socket_address_set_port(a, port);
      ret = 1;
      break;
      }
    addr = addr->ifa_next;
    }
  
  freeifaddrs(ifap);
  return ret;
#else
  return 0;
#endif
  //  return bg_socket_address_set(a, hostname, port, socktype);
  }

int bg_socket_get_address(int sock, bg_socket_address_t * local, bg_socket_address_t * remote)
  {
  socklen_t len;
  if(local)
    {
    len = sizeof(local->addr); 
    if(getsockname(sock, (struct sockaddr *)&local->addr, &len))
      return 0;
    local->len = len;
    }
  if(remote) 
    { 
    len = sizeof(remote->addr);
    if(getpeername(sock, (struct sockaddr *)&remote->addr, &len))
      return 0;
    remote->len = len;
    }
  return 1;
  }

/* Client connection (stream oriented) */

int bg_socket_connect_inet(bg_socket_address_t * a, int milliseconds)
  {
  int ret = -1;
  int err;
  socklen_t err_len;

                                                                               
  /* Create the socket */
  if((ret = create_socket(a->addr.ss_family, SOCK_STREAM, 0)) < 0)
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
  if(connect(ret, (struct sockaddr*)&a->addr, a->len) < 0)
    {
    if(errno == EINPROGRESS)
      {
      if(!bg_socket_can_write(ret, milliseconds))
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connection timed out");
        return -1;
        }
      }
    else
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connecting failed: %s",
             strerror(errno));
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
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Connecting unix socket failed: %s",
           strerror(errno));
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

  if((err = getaddrinfo("localhost", NULL /* service */,
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

int bg_listen_socket_create_inet(bg_socket_address_t * addr,
                                 int port,
                                 int queue_size,
                                 int flags)
  {
  int ret, err, use_ipv6 = 0;
  struct sockaddr_in  name_ipv4;
  struct sockaddr_in6 name_ipv6;

  if(addr)
    {
    if(addr->addr.ss_family == AF_INET6)
      use_ipv6 = 1;
    }
  else if(flags & BG_SOCKET_LOOPBACK)
    use_ipv6 = have_ipv6();
  
  memset(&name_ipv4, 0, sizeof(name_ipv4));
  memset(&name_ipv6, 0, sizeof(name_ipv6));
  
  if(use_ipv6)
    ret = create_socket(PF_INET6, SOCK_STREAM, 0);
  else
    ret = create_socket(PF_INET, SOCK_STREAM, 0);
  
  /* Create the socket. */
  if(ret < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create inet server socket");
    return -1;
    }
  
  /* Give the socket a name. */

  if(addr)
    {
    err = bind(ret, (struct sockaddr*)&addr->addr, addr->len);
    }
  else if(use_ipv6)
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
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot bind UNIX domain socket: %s",
           strerror(errno));
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

int bg_listen_socket_accept(int sock, int milliseconds)
  {
  int ret;

  if((milliseconds >= 0) && !bg_socket_can_read(sock, milliseconds))
    return -1;
  
  ret = accept(sock, NULL, NULL);
  if(ret < 0)
    return -1;
  return ret;
  }

void bg_listen_socket_destroy(int sock)
  {
  close(sock);
  }

int bg_socket_can_read(int fd, int milliseconds)
  {
  fd_set set;
  struct timeval timeout;
  FD_ZERO (&set);
  FD_SET  (fd, &set);

  timeout.tv_sec  = milliseconds / 1000;
  timeout.tv_usec = (milliseconds % 1000) * 1000;
    
  if(select(fd+1, &set, NULL, NULL, &timeout) <= 0)
    {
    // bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Got read timeout");
    return 0;
    }
  return 1;
  }


int bg_socket_can_write(int fd, int milliseconds)
  {
  fd_set set;
  struct timeval timeout;
  FD_ZERO (&set);
  FD_SET  (fd, &set);

  timeout.tv_sec  = milliseconds / 1000;
  timeout.tv_usec = (milliseconds % 1000) * 1000;
    
  if(select(fd+1, NULL, &set, NULL, &timeout) <= 0)
    {
    // bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Got read timeout");
    return 0;
    }
  return 1;
  }


int bg_socket_read_data(int fd, uint8_t * data, int len, int milliseconds)
  {
  int result;
  
  if((milliseconds >= 0) && !bg_socket_can_read(fd, milliseconds))
    return 0;
  
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

int bg_socket_is_local(int fd)
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

/* Send an entire file */
int bg_socket_send_file(int fd, const char * filename,
                        int64_t offset, int64_t len)
  {
  int ret = 0;
  int buf_size;
  uint8_t * buf = NULL;
  int file_fd;
  socklen_t size_len;
  int64_t result;
  int64_t bytes_written;
  int bytes;
  
  file_fd = open(filename, O_RDONLY);
  if(file_fd < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open local file: %s", strerror(errno));
    return 0;
    }
  
  if(len <= 0)
    len = lseek(file_fd, 0, SEEK_END);
  lseek(file_fd, offset, SEEK_SET);
  
  /* Try sendfile */
#ifdef HAVE_SYS_SENDFILE_H
  result = sendfile(fd, file_fd, &offset, len);
  if(result < 0)
    {
    if((errno != EINVAL) && (errno != ENOSYS))
      goto end;
    }
  else
    {
    ret = 1;
    goto end;
    }
#endif
  
  if(getsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, &buf_size, &size_len))
    buf_size = 576 - 40;

  bytes_written = 0;
  
  buf = malloc(buf_size);

  while(bytes_written < len)
    {
    bytes = len - bytes_written;
    
    if(bytes > buf_size)
      bytes = buf_size;
    
    bytes = read(file_fd, buf, bytes);
    if(bytes < 0)
      goto end;
    
    if(bg_socket_write_data(fd, buf, bytes) < bytes)
      goto end;
    bytes_written += bytes;
    }

  ret = 1;
  end:

  close(file_fd);
  
  if(buf)
    free(buf);
  return ret;
  }

int bg_udp_socket_create(bg_socket_address_t * addr)
  {
  int ret;
  int err;
  if((ret = create_socket(addr->addr.ss_family, SOCK_DGRAM, 0)) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create UDP socket: %s", strerror(errno));
    return -1;
    }
  err = bind(ret, (struct sockaddr*)&addr->addr, addr->len);

  if(err)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot bind UDP socket: %s", strerror(errno));
    return -1;
    }
  return ret;
  }

int bg_udp_socket_create_multicast(bg_socket_address_t * addr)
  {
  int reuse = 1;
  int ret;
  int err;
  uint8_t loop = 1;

  bg_socket_address_t bind_addr;
  
  if((ret = create_socket(addr->addr.ss_family, SOCK_DGRAM, 0)) < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot create UDP multicast socket");
    return -1;
    }
  
  if(setsockopt(ret, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)

    {
    close(ret);
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot set SO_REUSEADDR: %s", strerror(errno));
    return -1;
    }

  /* Bind to proper port */
  
  memset(&bind_addr, 0, sizeof(bind_addr));
  
  bg_socket_address_set(&bind_addr, "0.0.0.0",
                      bg_socket_address_get_port(addr), SOCK_DGRAM);

  err = bind(ret, (struct sockaddr*)&addr->addr, addr->len);

  if(err)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot bind UDP socket: %s", strerror(errno));
    close(ret);
    return -1;
    }
  
  if(addr->addr.ss_family == AF_INET)
    {
    struct ip_mreq req;
    struct sockaddr_in * a = (struct sockaddr_in *)(&addr->addr);
    memcpy(&req.imr_multiaddr, &a->sin_addr, sizeof(req.imr_multiaddr));
    req.imr_interface.s_addr = INADDR_ANY;

    if(setsockopt(ret, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(req)))
      {
      bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot join multicast group: %s",
             strerror(errno));
      close(ret);
      return -1;
      }
    }
  else
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "IPV6 multicast not supported yet");
    close(ret);
    return -1;
    }
  
  setsockopt(ret, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
  return ret;
  }

int bg_udp_socket_receive(int fd, uint8_t * data, int data_size,
                          bg_socket_address_t * addr)
  {
  socklen_t len = sizeof(addr->addr);
  ssize_t result;
  
  result = recvfrom(fd, data, data_size, 0 /* int flags */,
                    (struct sockaddr *)&addr->addr, &len);

  if(result < 0)
    bg_log(BG_LOG_ERROR, LOG_DOMAIN, "UDP Receive failed");
  
  addr->len = len;
  return result;
  }

int bg_udp_socket_send(int fd, const uint8_t * data, int data_size,
                       bg_socket_address_t * addr)
  {
  return (sendto(fd, data, data_size, 0 /* int flags */,
                 (struct sockaddr *)&addr->addr, addr->len) == data_size);
  }

