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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <avdec_private.h>
#include <http.h>

/* Creation/destruction of http header */

bgav_http_header_t * bgav_http_header_create()
  {
  bgav_http_header_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bgav_http_header_reset(bgav_http_header_t*h)
  {
  h->num_lines = 0;
  }

void bgav_http_header_destroy(bgav_http_header_t * h)
  {
  int i;
  for(i = 0; i < h->lines_alloc; i++)
    {
    if(h->lines[i].line)
      free(h->lines[i].line);
    }
  if(h->lines)
    free(h->lines);
  free(h);
  }

void bgav_http_header_add_line(bgav_http_header_t * h, const char * line)
  {
  int len;
  //  fprintf(stderr, "bgav_http_header_add_line: %s\n",
  //          line);
  if(h->lines_alloc < h->num_lines + 1)
    {
    h->lines_alloc += 8;
    h->lines = realloc(h->lines, h->lines_alloc * sizeof(*(h->lines)));
    memset(h->lines + h->num_lines, 0,
           (h->lines_alloc - h->num_lines) * sizeof(*(h->lines)));
    }
  len = strlen(line)+1;
  if(h->lines[h->num_lines].line_alloc < len)
    {
    h->lines[h->num_lines].line_alloc = len + 100;
    h->lines[h->num_lines].line =
      realloc(h->lines[h->num_lines].line,
              h->lines[h->num_lines].line_alloc);
    }
  strcpy(h->lines[h->num_lines].line, line);
  h->num_lines++;
  }

void bgav_http_header_send(bgav_http_header_t * h, int fd)
  {
  int i;
  for(i = 0; i < h->num_lines; i++)
    {
    write(fd, h->lines[i].line, strlen(h->lines[i].line));
    write(fd, "\r\n", 2);
    }
  //  write(fd, "\r\n", 2);
  }

/* Reading of http header */

void bgav_http_header_revc(bgav_http_header_t * h, int fd, int milliseconds)
  {
  char * answer = (char*)0;
  int answer_alloc = 0;

  while(bgav_read_line_fd(fd, &answer, &answer_alloc, milliseconds))
    {
    if(*answer == '\0')
      break;
    bgav_http_header_add_line(h, answer);
    }
  if(answer)
    free(answer);
  }

int bgav_http_header_status_code(bgav_http_header_t * h)
  {
  char * pos;
  if(!h->num_lines)
    return 0;

  pos = h->lines[0].line;
  while(!isspace(*pos) && (*pos != '\0'))
    pos++;

  while(isspace(*pos) && (*pos != '\0'))
    pos++;

  if(isdigit(*pos))
    return atoi(pos);
  return -1;
  }

const char * bgav_http_header_get_var(bgav_http_header_t * h,
                                      const char * name)
  {
  int i;
  int name_len;
  const char * ret;
  name_len = strlen(name);
  for(i = 1; i < h->num_lines; i++)
    {
    //    fprintf(stderr, "bgav_http_header_get_var %s\n",
    //           h->lines[i].line);
    
    if(!strncmp(h->lines[i].line, name, name_len) &&
       h->lines[i].line[name_len] == ':')
      {
      ret = &(h->lines[i].line[name_len+1]);
      while(isspace(*ret))
        ret++;
      return ret;
      }
    }
  return (const char*)0;
  }

void bgav_http_header_dump(bgav_http_header_t*h)
  {
  int i;
  fprintf(stderr, "HTTP Header\n");
  for(i = 0; i < h->num_lines; i++)
    {
    fprintf(stderr, "%s\n", h->lines[i].line);
    }
  fprintf(stderr, "End of HTTP Header\n");
  }

struct bgav_http_s
  {
  bgav_http_header_t * header;
  int fd;
  };

bgav_http_t * bgav_http_open(const char * url, int milliseconds,
                             char ** redirect_url,
                             bgav_http_header_t * extra_header)
  {
  int port;
  int status;
  const char * location;
  
  char * host = (char*)0;
  char * path = (char*)0;
  char * protocol = (char*)0;
  char * line = (char*)0;
  bgav_http_header_t * request_header;
  
  bgav_http_t * ret;
  
  ret = calloc(1, sizeof(*ret));
  port = -1;
  if(!bgav_url_split(url,
                     &protocol,
                     &host,
                     &port,
                     &path))
    goto fail;

  
  if(port == -1)
    port = 80;

  if(strcasecmp(protocol, "http"))
    goto fail;

  ret->fd = bgav_tcp_connect(host, port, milliseconds);

  if(ret->fd == -1)
    goto fail;

  /* Build request */

  request_header = bgav_http_header_create();

  line = bgav_sprintf("GET %s HTTP/1.1", ((path) ? path : "/"));
  bgav_http_header_add_line(request_header, line);
  free(line);

  line = bgav_sprintf("Host: %s", host);
  bgav_http_header_add_line(request_header, line);
  free(line);

  bgav_http_header_add_line(request_header, "User-Agent: gmerlin/0.3.0");
  bgav_http_header_add_line(request_header, "Accept: */*");
  
  bgav_http_header_send(request_header, ret->fd);

  //  fprintf(stderr, "Sent request:\n");
  //  bgav_http_header_send(request_header, fileno(stderr));
  //  bgav_http_header_dump(request_header);
  
  if(extra_header)
    {
    bgav_http_header_dump(extra_header);
    bgav_http_header_send(extra_header, ret->fd);
    }
  write(ret->fd, "\r\n", 2);
  bgav_http_header_destroy(request_header);
        
  ret->header = bgav_http_header_create();
  
  bgav_http_header_revc(ret->header, ret->fd, milliseconds);

  /* Check status code */

  status = bgav_http_header_status_code(ret->header);

  if(status >= 400) /* Error */
    {
    goto fail;
    }
  else if(status >= 300) /* Redirection */
    {
    /* Extract redirect URL */

    if(*redirect_url)
      {
      free(*redirect_url);
      *redirect_url = (char*)0;
      }
    location = bgav_http_header_get_var(ret->header, "Location");

    if(location)
      *redirect_url = bgav_strndup(location, (char*)0);
    
    bgav_http_header_destroy(ret->header);
    free(ret);
    
    return (bgav_http_t*)0;
    }
  else if(status < 200)  /* Error */
    {
    goto fail;
    }
  
  //  bgav_http_header_dump(ret->header);

  if(host)
    free(host);
  if(path)
    free(path);
  if(protocol)
    free(protocol);
  
  return ret;
  
  fail:

  if(*redirect_url)
    {
    free(*redirect_url);
    *redirect_url = (char*)0;
    }
  
  if(host)
    free(host);
  if(path)
    free(path);
  if(protocol)
    free(protocol);
  if(ret->header)
    bgav_http_header_destroy(ret->header);
  
  free(ret);
  return (bgav_http_t*)0;
  }

void bgav_http_close(bgav_http_t * h)
  {
  if(h->fd >= 0)
    close(h->fd);
  bgav_http_header_destroy(h->header);
  free(h);
  }

int bgav_http_get_fd(bgav_http_t * h)
  {
  return h->fd;
  }

bgav_http_header_t* bgav_http_get_header(bgav_http_t * h)
  {
  return h->header;
  }
