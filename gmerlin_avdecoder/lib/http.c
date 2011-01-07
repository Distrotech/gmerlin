/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <avdec_private.h>
#define LOG_DOMAIN "http"
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

int bgav_http_header_send(const bgav_options_t * opt, bgav_http_header_t * h, int fd)
  {
  int i;

  /* We don't want a SIGPIPE, because we don't want any
     kind of signal handling for now */
  
  for(i = 0; i < h->num_lines; i++)
    {
    if(!bgav_tcp_send(opt, fd, (uint8_t*)(h->lines[i].line),
                      strlen(h->lines[i].line)) ||
       !bgav_tcp_send(opt, fd, (uint8_t*)"\r\n", 2))
      {
      bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Remote end closed connection");
      return 0;
      }
    //    write(fd, h->lines[i].line, strlen(h->lines[i].line));
    //    write(fd, "\r\n", 2);
    }
  return 1;
  }

/* Reading of http header */

int bgav_http_header_revc(const bgav_options_t * opt,
                          bgav_http_header_t * h, int fd)
  {
  int ret = 0;
  char * answer = (char*)0;
  int answer_alloc = 0;
  
  while(bgav_read_line_fd(fd, &answer, &answer_alloc, opt->connect_timeout))
    {
    if(*answer == '\0')
      break;
    bgav_http_header_add_line(h, answer);
    ret = 1;
    }
  
  if(answer)
    free(answer);
  return ret;
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
  if(!h->num_lines)
    return (const char*)0;
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
    
    if(!strncasecmp(h->lines[i].line, name, name_len) &&
       h->lines[i].line[name_len] == ':')
      {
      ret = &h->lines[i].line[name_len+1];
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
  bgav_dprintf( "HTTP Header\n");
  for(i = 0; i < h->num_lines; i++)
    {
    bgav_dprintf( "  %s\n", h->lines[i].line);
    }
  bgav_dprintf( "End of HTTP Header\n");
  }

struct bgav_http_s
  {
  const bgav_options_t * opt;
  bgav_http_header_t * header;
  int fd;
  };

static bgav_http_t * do_connect(const char * host, int port, const bgav_options_t * opt,
                                bgav_http_header_t * request_header,
                                bgav_http_header_t * extra_header)
  {
  bgav_http_t * ret = (bgav_http_t *)0;
  
  ret = calloc(1, sizeof(*ret));
  ret->opt = opt;

  ret->fd = bgav_tcp_connect(ret->opt, host, port);

  if(ret->fd == -1)
    goto fail;

  if(!bgav_http_header_send(ret->opt, request_header, ret->fd))
    goto fail;
  
  if(extra_header)
    {
    //    bgav_http_header_dump(extra_header);
    if(!bgav_http_header_send(ret->opt, extra_header, ret->fd))
      goto fail;
    }
  if(!bgav_tcp_send(ret->opt, ret->fd, (uint8_t*)"\r\n", 2))
    goto fail;
  
  ret->header = bgav_http_header_create();
  
  bgav_http_header_revc(ret->opt, ret->header, ret->fd);

  return ret;
  
  fail:
  
  if(ret)
    bgav_http_close(ret);
  return (bgav_http_t*)0;
  }

static char * encode_user_pass(const char * user, const char * pass)
  {
  int userpass_len;
  int userpass_enc_len;
  char * userpass;
  char * ret;

  userpass = bgav_sprintf("%s:%s", user, pass);
  
  userpass_len = strlen(userpass);
  userpass_enc_len = (userpass_len * 4)/3+4;
  
  ret = malloc(userpass_enc_len);
  userpass_enc_len = bgav_base64encode((uint8_t*)userpass,
                                       userpass_len,
                                       (uint8_t*)ret,
                                       userpass_enc_len);
  ret[userpass_enc_len] = '\0';
  return ret;
  }

bgav_http_t * bgav_http_open(const char * url, const bgav_options_t * opt,
                             char ** redirect_url,
                             bgav_http_header_t * extra_header)
  {
  int port;
  int status;
  const char * location;

  char * userpass_enc;
    
  char * host     = (char*)0;
  char * path     = (char*)0;
  char * line     = (char*)0;
  char * user     = (char*)0;
  char * pass     = (char*)0;
  char * protocol = (char*)0;
  
  const char * real_host;
  int real_port;
  
  bgav_http_header_t * request_header = (bgav_http_header_t*)0;
  bgav_http_t * ret = (bgav_http_t *)0;
  int mhttp; /* Different header fields for Windows media server :) */
  
  port = -1;
  if(!bgav_url_split(url,
                     &protocol,
                     &user, /* User */
                     &pass, /* Pass */
                     &host,
                     &port,
                     &path))
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Unvalid URL");
    goto fail;
    }
  if(path && !strcmp(path, ";stream.nsv"))
    {
    free(path);
    path = (char*)0;
    }
  if(port == -1)
    port = 80;

  if(protocol && !strcmp(protocol, "mhttp"))
    mhttp = 1;
  
  /* Check for proxy */

  if(opt->http_use_proxy)
    {
    real_host = opt->http_proxy_host;
    real_port = opt->http_proxy_port;
    }
  else
    {
    real_host = host;
    real_port = port;
    }
  
  /* Build request */

  request_header = bgav_http_header_create();

  if(opt->http_use_proxy)
    line = bgav_sprintf("GET %s HTTP/1.1", url);
  else
    line = bgav_sprintf("GET %s HTTP/1.1", ((path) ? path : "/"));
  
  bgav_http_header_add_line(request_header, line);
  free(line);

  /* Proxy authentication */

  if(opt->http_proxy_auth)
    {
    userpass_enc = encode_user_pass(opt->http_proxy_user, opt->http_proxy_pass);
    line = bgav_sprintf("Proxy-Authorization: Basic %s", userpass_enc);
    bgav_http_header_add_line(request_header, line);
    free(line);
    free(userpass_enc);
    }
  
  line = bgav_sprintf("Host: %s", host);
  bgav_http_header_add_line(request_header, line);
  free(line);
  
  ret = do_connect(real_host, real_port, opt, request_header, extra_header);
  if(!ret)
    goto fail;

  /* Check status code */
  status = bgav_http_header_status_code(ret->header);

  //  bgav_http_header_dump(ret->header);
    
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
    
    userpass_enc = encode_user_pass(user, pass);
    line = bgav_sprintf("Authorization: Basic %s", userpass_enc);
    bgav_http_header_add_line(request_header, line);
    free(line);
    free(userpass_enc);
    
    ret = do_connect(real_host, real_port, opt, request_header, extra_header);
    if(!ret)
      goto fail;
    /* Check status code */
    status = bgav_http_header_status_code(ret->header);
    }
    
  if(status >= 400) /* Error */
    {
    if(bgav_http_header_status_line(ret->header))
      bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "%s", bgav_http_header_status_line(ret->header));
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
      {
      *redirect_url = bgav_strdup(location);
      }
    
    if(host)
      free(host);
    if(path)
      free(path);
    if(protocol)
      free(protocol);
    if(request_header)
      bgav_http_header_destroy(request_header);
    bgav_http_close(ret);
    return (bgav_http_t*)0;
    }
  else if(status < 200)  /* Error */
    {
    if(bgav_http_header_status_line(ret->header))
      bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN, "%s", bgav_http_header_status_line(ret->header));
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
  if(request_header)
    bgav_http_header_destroy(request_header);
  
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
    closesocket(h->fd);
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
