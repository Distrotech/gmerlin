/*****************************************************************
 
  http.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <stdlib.h>


#include <bgsocket.h>
#include <http.h>
#include <utils.h>
#include <config.h>


#define MAX_REDIRECTIONS 5

struct bg_http_connection_s
  {
  int fd;

  char * mimetype;
  char ** header;

  char * proxy_host;
  int proxy_port;
  int connect_timeout; /* In milliseconds */
  int read_timeout;       /* In milliseconds */
  int num_redirections;
  int num_header_lines;
  };

bg_http_connection_t * bg_http_connection_create()
  {
  bg_http_connection_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->connect_timeout = 5000;
  return ret;
  }

/* Destroying the connection doesn't close it!! */

static void free_strings(bg_http_connection_t * c)
  {
  int i;
  if(c->header)
    {
    for(i = 0; i < c->num_header_lines; i++)
      {
      if(c->header[i])
        free(c->header[i]);
      }
    free(c->header);
    }
  if(c->proxy_host)
    free(c->proxy_host);
  }

void bg_http_connection_destroy(bg_http_connection_t * c)
  {
  free_strings(c);
  free(c);
  }
          
/* Set parameters */

void bg_http_connection_set_proxy(bg_http_connection_t * c,
                                  const char * h, int p)
  {
  c->proxy_host = bg_strdup(c->proxy_host, h);
  c->proxy_port = p;
  }

void bg_http_connection_set_timeouts(bg_http_connection_t * c,
                                     int connect_ms, int read_ms)
  {
  c->connect_timeout = connect_ms;
  c->read_timeout    = read_ms;
  }

#if 0

/* Connect, return FALSE on failure */

static int parse_url(bg_http_connection_t * c,
                     const char * url,
                     char **host,
                     int  * port,
                     char ** path,
                     char ** real_host)
  {
  int len;
  const char * pos1;
  const char * pos2;

  if(strncmp(url, "http://", 7))
    {
    fprintf(stderr, "URL must start with http://\n");
    return 0;
    }

  /* Get Hostname */

  pos1 = url + 7;

  pos2 = pos1;
  len = 0;
  while(((*pos2) != ':') && ((*pos2) != '/') && ((*pos2) != '\0'))
    {
    len++;
    pos2++;
    }
  if(!len)
    {
    fprintf(stderr, "Invalid URL\n");
    return 0;
    }
  
  if(c->proxy_host)
    {
    *host = bg_strdup(*host, c->proxy_host);
    *port = c->proxy_port;
    *path = bg_strdup(*path, url);
    
    *real_host = malloc(len+1);
    strncpy(*real_host, pos1, len);
    (*real_host)[len] = '\0';
    return 1;
    }

  *host = malloc(len+1);  
  strncpy(*host, pos1, len);
  (*host)[len] = '\0';
  
  /* Get Port */
  
  if(*pos2 == ':')
    {
    pos2++;
    *port = atoi(pos2);

    pos1 = pos2;

    while(isdigit(*pos1))
      pos1++;
    
    if(pos1 == pos2)
      {
      fprintf(stderr, "Invalid port\n");
      return 0;
      }
    pos2 = pos1;
    }
  else
    *port = 80;
  
  /* Get Path */

  if(*pos2 == '\0')
    *path = bg_strdup(*path, "/");
  else if(*pos2 == '/')
    *path = bg_strdup(*path, pos2);
  else
    {
    fprintf(stderr, "Invalid URL %s\n", url);
    return 0;
    }
  return 1;
  }


/* Connects to a host at port, returns -1 if failure */
/* Timeout is in milliseconds */

static int connect_tcp(const char * host, int port, int milliseconds)
  {
  int ret;
  struct sockaddr_in sock;
  struct hostent * hp;

  struct timeval timeout;
  fd_set write_fds;
  
  /* Create the socket */

  if((ret = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
    fprintf(stderr, "Cannot create socket\n");
    return -1;
    }
  
  if(fcntl(ret, F_SETFL, O_NONBLOCK) < 0)
    {
    fprintf(stderr, "Cannot set nonblocking mode\n");
    return -1;
    }

  /* Set up the address */

  sock.sin_family = AF_INET;
  sock.sin_port = htons(port);

  fprintf(stderr, "Resolving host...");
  if(!(hp = gethostbyname(host)))
    {
    fprintf(stderr, "Cannot resolve address for %s\n", host);
    return -1;
    }
  fprintf(stderr, "done\n");
    
  memcpy(&sock.sin_addr, hp->h_addr, hp->h_length);
  
  /* Now, connect */

  fprintf(stderr, "Connecting...");
  
  if(connect(ret,(struct sockaddr*)&sock,sizeof(sock))<0)
    {
    if(errno == EINPROGRESS)
      {
      timeout.tv_sec = milliseconds / 1000;
      timeout.tv_usec = 1000 * (milliseconds % 1000);
      FD_ZERO (&write_fds);
      FD_SET (ret, &write_fds);
      if(!select(ret+1, (fd_set*)0, &write_fds,(fd_set*)0,&timeout))
        {
        fprintf(stderr, "Connection timed out\n");
        return -1;
        }
      }
    else
      {
      fprintf(stderr, "Failed\n");
      return -1;
      }
    }

  /* Set back to blocking mode */
  
  if(fcntl(ret, F_SETFL, 0) < 0)
    {
    fprintf(stderr, "Cannot set blocking mode\n");
    return -1;
    }

  fprintf(stderr, "Connected\n");
  return ret;
  }
#endif

/* Tolerant function for getting the status code */

static int get_status_code(char * line)
  {
  char * rest;
  char * pos = line;
  int ret;
  
  while(!isspace(*pos) && (*pos != '\0'))
    pos++;

  if(*pos == '\0')
    return 0;

  while(isspace(*pos) && (*pos != '\0'))
    pos++;

  if(*pos == '\0')
    return 0;

  ret = strtoul(pos, &rest, 10);

  if(pos == rest)
    return 0;
  
  return ret;
  }

#define LINES_TO_ALLOC 10

static char * get_hostname(char * old, const char * url)
  {
  const char * pos1;
  const char * pos2;

  pos1 = url;
  while(isalnum(*pos1))
    pos1++;
  if(strncmp(pos1, "://", 3))
    return (char*)0;

  pos1+=3;
  if(!isalnum(*pos1))
    return (char*)0;
  pos2 = pos1;
  while((*pos2 != ':') && (*pos2 != '/') && (*pos2 != '\0'))
    {
    pos2++;
    }
  return bg_strndup(old, pos1, pos2);
  }

int bg_http_connection_connect(bg_http_connection_t * c,
                               const char * _url,
                               char ** extra_request)
  {
  const char * path;

  char * hostname = (char*)0;
  char * request  = (char*)0;
  
  char * url = (char*)0;
  
  int ret = 0;
  int status_code;

  int buffer_size = 0;
  char * buffer   = (char*)0;

  int lines_alloc;
  int i;
  char * pos;
  int done = 0;
  
  bg_host_address_t * addr;
  int num_redirections = 0;
  free_strings(c);

  addr = bg_host_address_create();
  url = bg_strdup(url, _url);
  while(!done)
    {
    hostname = get_hostname(hostname, url);
    if(c->proxy_host)
      {
      if(!bg_host_address_set(addr, c->proxy_host, c->proxy_port))
        goto fail;
      path = url;
      }
    else
      {
      if(!bg_host_address_set_from_url(addr, url, &path))
        goto fail;
      }
    if((c->fd = bg_socket_connect_inet(addr, c->connect_timeout)) == -1)
      goto fail;
    
    /* Build request */
    if(request)
      free(request);
    
    request =
      bg_sprintf("GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s/%s\r\nAccept: */*\r\n",
                 ((*path != '\0') ? path : "/"), hostname, PACKAGE, VERSION);    
    if(extra_request)
      {
      i = 0;
      while(extra_request[i])
        {
        request = bg_strcat(request, extra_request[i]);
        request = bg_strcat(request, "\r\n");
        i++;
        }
      }
    request = bg_strcat(request, "\r\n");
    
    fprintf(stderr, request);
    write(c->fd, request, strlen(request));

    if(!bg_read_line_fd(c->fd, &buffer, &buffer_size))
      goto fail;

    fprintf(stderr, "Buffer: %s\n", buffer);
    
    status_code = get_status_code(buffer);

    if(status_code >= 400)
      goto fail;

    else if(status_code >= 300)
      {
      if(num_redirections > MAX_REDIRECTIONS)
        goto fail;
      else
        {
        /* Check for location field */

        while(1)
          {
          if(!bg_read_line_fd(c->fd, &buffer, &buffer_size))
            goto fail;
          if(!strncmp(buffer, "Location: ", 10))
            break;
          }

        /* Close old connection */

        close(c->fd);
        num_redirections++;

        /* Recursively call us */
        
        url = bg_strdup(url, &(buffer[10]));
        continue;
        }
      }
    /* Looks like we are done */
    else if(status_code >= 200)
      break;
    }
  
  c->num_header_lines = 0;
  lines_alloc = 0;
  
  c->header = malloc(lines_alloc * sizeof(char*));
  
  while(1)
    {
    if(!bg_read_line_fd(c->fd, &buffer, &buffer_size))
      goto fail;
    if(!strlen(buffer))
      break;
    
    if(c->num_header_lines >= lines_alloc)
      {
      lines_alloc += LINES_TO_ALLOC;
      c->header = realloc(c->header, lines_alloc * sizeof(char*));
      for(i = lines_alloc - LINES_TO_ALLOC; i < lines_alloc; i++)
        c->header[i] = (char*)0;
      }
    
    c->header[c->num_header_lines] = bg_strdup(c->header[c->num_header_lines],
                                               buffer);
    /* Catch mimetype */
    
    if(!strncasecmp(c->header[c->num_header_lines], "Content-Type: ", 14))
      {
      pos = &(c->header[c->num_header_lines][14]);
      while(isspace(*pos))
        pos++;
      c->mimetype = pos;
      //      fprintf(stderr, "Content-Type: %s\n", c->mimetype);
      }
    c->num_header_lines++;
    }
  ret = 1;
  
  fail:

  if(url)
    free(url);
  if(hostname)
    free(hostname);
  if(request)
    free(request);
  return ret;
  }

/* Get Informations about the contents */

const char * bg_http_connection_get_mimetype(bg_http_connection_t * c)
  {
  return c->mimetype;
  }

/* NULL terminated array of header lines */

const char ** bg_http_connection_get_header(bg_http_connection_t * c)
  {
  return (const char**)c->header;
  }

/* Close the connection */

void bg_http_connection_close(bg_http_connection_t * c)
  {
  close(c->fd);
  }

/* Return the file descriptor */

int bg_http_connection_get_fd(bg_http_connection_t * c)
  {
  return c->fd;
  }

#define SAVE_BUFFER_SIZE 512

void bg_http_connection_save(bg_http_connection_t * c, const char * filename)
  {
  FILE * output;
  char buffer[SAVE_BUFFER_SIZE];
  int bytes_read;

  output = fopen(filename, "w");
  
  while(1)
    {
    bytes_read = read(c->fd, buffer, SAVE_BUFFER_SIZE);
    fwrite(buffer, 1, bytes_read, output);
    if(bytes_read < SAVE_BUFFER_SIZE)
      break;
    }
  fclose(output);
  }

int bg_http_connection_read(bg_http_connection_t * c, uint8_t * buf, int len)
  {
  struct timeval timeout;
  fd_set read_fds;
  int result;
  
  if(c->read_timeout)
    {
    FD_ZERO(&read_fds);
    FD_SET(c->fd, &read_fds);

    timeout.tv_sec  = c->read_timeout / 1000;
    timeout.tv_usec = (c->read_timeout % 1000) * 1000;
    
    if(!select(c->fd+1, &read_fds,(fd_set*)0, (fd_set*)0,&timeout))
      {
      //      fprintf(stderr, "Got read timeout\n");
      return 0;
      }
    }
  result = read(c->fd, buf, len);
  //  fprintf(stderr, "Read %d bytes\n", result);
  return result;
  }


int bg_http_connection_get_num_header_lines(bg_http_connection_t * c)
  {
  return c->num_header_lines;
  
  }

const char * bg_http_connection_get_header_line(bg_http_connection_t * c,
                                                int index)
  {
  return c->header[index];
  }
