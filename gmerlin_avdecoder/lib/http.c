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

#include <avdec_private.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

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

int bgav_http_header_send(bgav_http_header_t * h, int fd, char ** error_msg)
  {
  int i;

  /* We don't want a SIGPIPE, because we don't want any
     kind of signal handling for now */
  
  for(i = 0; i < h->num_lines; i++)
    {
    if(!bgav_tcp_send(fd, (uint8_t*)(h->lines[i].line),
                      strlen(h->lines[i].line), error_msg) ||
       !bgav_tcp_send(fd, (uint8_t*)"\r\n", 2, error_msg))
      {
      if(error_msg) *error_msg = bgav_sprintf("Remote end closed connection");
      return 0;
      }
    //    write(fd, h->lines[i].line, strlen(h->lines[i].line));
    //    write(fd, "\r\n", 2);
    }
  return 1;
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

const char * bgav_http_header_status_line(bgav_http_header_t * h)
  {
  return h->lines[0].line;
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
    
    if(!strncasecmp(h->lines[i].line, name, name_len) &&
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
    fprintf(stderr, "  %s\n", h->lines[i].line);
    }
  fprintf(stderr, "End of HTTP Header\n");
  }

struct bgav_http_s
  {
  const bgav_options_t * opt;
  bgav_http_header_t * header;
  int fd;
  };

static bgav_http_t * do_connect(const char * host, int port, const bgav_options_t * opt,
                         bgav_http_header_t * request_header,
                         bgav_http_header_t * extra_header,
                         char ** error_msg)
  {
  bgav_http_t * ret = (bgav_http_t *)0;
  
  ret = calloc(1, sizeof(*ret));
  ret->opt = opt;

  //  fprintf(stderr, "Connecting...");
  ret->fd = bgav_tcp_connect(host, port, ret->opt->connect_timeout, error_msg);

  if(ret->fd == -1)
    goto fail;

  if(!bgav_http_header_send(request_header, ret->fd, error_msg))
    goto fail;
  
  //  bgav_http_header_send(request_header, ret->fd);
  //  fprintf(stderr, "Request sent\n");

  //  bgav_http_header_dump(request_header);
  
  if(extra_header)
    {
    //    bgav_http_header_dump(extra_header);
    if(!bgav_http_header_send(extra_header, ret->fd, error_msg))
      goto fail;
    }
  if(!bgav_tcp_send(ret->fd, (uint8_t*)"\r\n", 2, error_msg))
    goto fail;
  
  ret->header = bgav_http_header_create();
  
  bgav_http_header_revc(ret->header, ret->fd, ret->opt->connect_timeout);

  fprintf(stderr, "Got http header:\n");
  bgav_http_header_dump(ret->header);
  return ret;
  
  fail:

  if(error_msg && *error_msg)
    fprintf(stderr, "Connection failed: %s\n", *error_msg);

  if(ret)
    bgav_http_close(ret);
  return (bgav_http_t*)0;
  }

bgav_http_t * bgav_http_open(const char * url, const bgav_options_t * opt,
                             char ** redirect_url,
                             bgav_http_header_t * extra_header,
                             char ** error_msg)
  {
  int port;
  int status;
  const char * location;

  char * userpass;
  char * userpass_enc;

  int userpass_len;
  int userpass_enc_len;
    
  char * host     = (char*)0;
  char * path     = (char*)0;
  char * protocol = (char*)0;
  char * line     = (char*)0;
  char * user     = (char*)0;
  char * pass     = (char*)0;

  bgav_http_header_t * request_header = (bgav_http_header_t*)0;
  bgav_http_t * ret = (bgav_http_t *)0;
  
  port = -1;
  if(!bgav_url_split(url,
                     &protocol,
                     &user, /* User */
                     &pass, /* Pass */
                     &host,
                     &port,
                     &path))
    {
    *error_msg = bgav_sprintf("Unvalid URL");
    goto fail;
    }
  if(port == -1)
    port = 80;
  if(strcasecmp(protocol, "http"))
    goto fail;
#if 0
  if(user && pass)
    fprintf(stderr, "User: %s, pass: %s\n", user, pass);
#endif

  /* Build request */

  request_header = bgav_http_header_create();

  line = bgav_sprintf("GET %s HTTP/1.1", ((path) ? path : "/"));
  bgav_http_header_add_line(request_header, line);
  free(line);

  line = bgav_sprintf("Host: %s", host);
  bgav_http_header_add_line(request_header, line);
  free(line);

  bgav_http_header_add_line(request_header, "User-Agent: gmerlin/0.3.3");
  bgav_http_header_add_line(request_header, "Accept: */*");

  ret = do_connect(host, port, opt, request_header, extra_header, error_msg);
  if(!ret)
    goto fail;
  /* Check status code */

  status = bgav_http_header_status_code(ret->header);

  if(status == 401)
    {
    /* Ok, they won't let us in, try to get a username and/or password */
    bgav_http_close(ret);
    ret = (bgav_http_t*)0;

    if((!user || !pass) && opt->user_pass_callback)
      {
      if(user) { free(user); user = (char*)0; }
      if(pass) { free(pass); pass = (char*)0; }
      
      if(!opt->user_pass_callback(opt->user_pass_callback_data, host, &user, &pass))
        goto fail;
      }

    if(!user || !pass)
      goto fail;

    /* Now, user and pass should be the authentication data */

    //    fprintf(stderr, "User: %s, pass: %s\n", user, pass);

    userpass = bgav_sprintf("%s:%s", user, pass);

    userpass_len = strlen(userpass);
    userpass_enc_len = (userpass_len * 4)/3+4;
    
    userpass_enc = malloc(userpass_enc_len);
    userpass_enc_len = bgav_base64encode(userpass,
                                         userpass_len,
                                         userpass_enc,
                                         userpass_enc_len);
    userpass_enc[userpass_enc_len] = '\0';

    line = bgav_sprintf("Authorization: Basic %s", userpass_enc);
    bgav_http_header_add_line(request_header, line);
    free(line);
    free(userpass);
    free(userpass_enc);
    
    ret = do_connect(host, port, opt, request_header, extra_header, error_msg);
    if(!ret)
      goto fail;
    /* Check status code */
    status = bgav_http_header_status_code(ret->header);
    
    }
    
  if(status >= 400) /* Error */
    {
    if(error_msg) *error_msg = bgav_sprintf(bgav_http_header_status_line(ret->header));
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
    if(error_msg) *error_msg = bgav_sprintf(bgav_http_header_status_line(ret->header));
    goto fail;
    }
  
  //  bgav_http_header_dump(ret->header);
  if(request_header)
    bgav_http_header_destroy(request_header);
  
  if(host)
    free(host);
  if(path)
    free(path);
  if(protocol)
    free(protocol);
  
  return ret;
  
  fail:

  if(request_header)
    bgav_http_header_destroy(request_header);

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

  if(ret)
    {
    if(ret->header)
      bgav_http_header_destroy(ret->header);
    free(ret);
    }
  return (bgav_http_t*)0;
  }

void bgav_http_close(bgav_http_t * h)
  {
  if(h->fd >= 0)
    close(h->fd);
  if(h->header)
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
