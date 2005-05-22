/*****************************************************************
 
  tcp.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

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

#include <tcp.h>
#include <utils.h>

// #include <sys/un.h>

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

static int hostbyname(host_address_t * a, const char * hostname, char ** error_msg)
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

  //  fprintf(stderr, "Resolving host %s\n", hostname);
  
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
    *error_msg = bg_sprintf("Could not resolve address %s", hostname);
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
    *error_msg = bg_sprintf("Could not resolve address %s: No known address space",
                              hostname);
    goto fail;
    }
  //  fprintf(stderr, "Done\n");
  ret = 1;

  fail:
                                                                               
  if(gethostbyname_buffer)
    free(gethostbyname_buffer);
  return ret;
  }

/* Client connection (stream oriented) */
                                                                               
static int
socket_connect_inet(host_address_t * a, int milliseconds, char ** error_msg)
  {
  int ret = -1;
  struct sockaddr_in  addr_in;
  struct sockaddr_in6 addr_in6;
  void * addr;
  int addr_len;
  struct timeval timeout;
  fd_set write_fds;
                                                                               
  /* Create the socket */
                                                                               
  if((ret = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
    fprintf(stderr, "Cannot create socket\n");
    return -1;
    }
                                                                               
  /* Set up address */
                                                                               
  if(a->addr_type == AF_INET)
    {
    addr = &addr_in;
    addr_len = sizeof(addr_in);   
    memset(&addr_in, 0, sizeof(addr_len));
    
    addr_in.sin_family = AF_INET;
    memcpy(&(addr_in.sin_addr),
           &(a->addr.ipv4_addr),
           sizeof(a->addr.ipv4_addr));
    addr_in.sin_port = htons(a->port);
    }
  else if(a->addr_type == AF_INET6)
    {
    addr = &addr_in6;
    addr_len = sizeof(addr_in6);
                                                                               
    memset(&addr_in6, 0, addr_len);
    addr_in6.sin6_family = AF_INET6;
    memcpy(&(addr_in6.sin6_addr),
           &(a->addr.ipv6_addr),
           sizeof(a->addr.ipv6_addr));
    addr_in6.sin6_port = htons(a->port);
    }
  else
    {
    fprintf(stderr, "Unknown Address family!!!\n");
    return -1;
    }
  
  /* Connect the thing */
                                                                               
  //  fprintf(stderr, "Connecting...");

  if(fcntl(ret, F_SETFL, O_NONBLOCK) < 0)
    {
    fprintf(stderr, "Cannot set nonblocking mode\n");
    return -1;
    }
  if(connect(ret,(struct sockaddr*)addr,addr_len)<0)
    {
    if(errno == EINPROGRESS)
      {
      timeout.tv_sec = milliseconds / 1000;
      timeout.tv_usec = 1000 * (milliseconds % 1000);
      FD_ZERO (&write_fds);
      FD_SET (ret, &write_fds);
      if(!select(ret+1, (fd_set*)0, &write_fds,(fd_set*)0,&timeout))
        {
        *error_msg = bg_sprintf("Connection timed out");
        return -1;
        }
      }
    else
      {
      *error_msg = bg_sprintf("Connecting failed: %s", strerror(errno));
      return -1;
      }
    }
  //  fprintf(stderr, "Connected\n");
  
  /* Set back to blocking mode */
  
  if(fcntl(ret, F_SETFL, 0) < 0)
    {
    fprintf(stderr, "Cannot set blocking mode\n");
    return -1;
    }
  return ret;
  }

int bg_tcp_connect(const char * host, int port, int milliseconds, char ** error_msg)
  {
  host_address_t addr;
  if(!hostbyname(&addr, host, error_msg))
    return -1;
  addr.port = port;
  return socket_connect_inet(&addr, milliseconds, error_msg);
  }

int bg_tcp_send(int fd, uint8_t * data, int len, char ** error_msg)
  {
  char error_buffer[1024];
  int result;
  result = send(fd, data, len, MSG_NOSIGNAL);
  if(result != len)
    {
    if(!strerror_r(errno, error_buffer, 1024))
      *error_msg = bg_sprintf("Could not send data: %s", error_buffer);
    else
      *error_msg = bg_sprintf("Could not send data");
    return 0;
    }
  return 1;
  }

int bg_tcp_read_data(int fd, uint8_t * ret, int len, int milliseconds)
  {
  int bytes_read = 0;
  int result;
  fd_set rset;
  struct timeval timeout;

  int flags = 0;
  //  if(milliseconds < 0)
  //    flags = MSG_WAITALL;

  while(bytes_read < len)
    {
    FD_ZERO (&rset);
    FD_SET  (fd, &rset);

    timeout.tv_sec  = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;

    if(select (fd+1, &rset, NULL, NULL, &timeout) <= 0)
      {
      return bytes_read;
      }

    result = recv(fd, ret + bytes_read, len - bytes_read, flags);
    if(result > 0)
      bytes_read += result;
    else if(result <= 0)
      {
      //      fprintf(stderr, "recv returned %d\n", result);
      return 0;
      }
    }
  return
    bytes_read;
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

int bg_tcp_read_line(int fd, char ** ret, int * ret_alloc, int milliseconds)
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
    if(!bg_tcp_read_data(fd, (uint8_t*)(&c), 1, milliseconds))
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
