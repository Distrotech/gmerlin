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

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <avdec_private.h>
#define LOG_DOMAIN "in_ftp"
typedef struct
  {
  int control_fd;
  int data_fd;
  int64_t bytes_read;
  } ftp_priv_t;

static int get_server_answer(int fd, char ** server_msg,
                             int * server_msg_alloc,int connect_timeout)
  {
  char status[5];
  char status_neu[5];
  
  status[4] = '\0';
  status_neu[4] = '\0';
    
  if(!bgav_read_line_fd(fd, server_msg, server_msg_alloc, connect_timeout))
    {
    return 0;
    }

  strncpy(status, *server_msg, 4);

  if(status[3]=='-')
    {
    status[3] = ' ';
    }
  else
    {
    return atoi(*server_msg);
    }

  status_neu[0]='\0';
  
  while(strncmp(status, status_neu, 4)!= 0)
    {
    if(!bgav_read_line_fd(fd, server_msg, server_msg_alloc, connect_timeout))
      return 0;
    strncpy(status_neu, *server_msg, 4);
    }
  return atoi(*server_msg);
  }

static char * parse_address(const char * server_msg, int * port)
  {
  int ip[4];
  int po[2];
  const char * pos;
  char * rest;
  pos = strrchr(server_msg, ')');
  while((*pos != '(') && (pos != server_msg))
    pos--;

  if(*pos != '(')
    return (char*)0;

  pos++;

  ip[0] = strtol(pos, &rest, 10);
  if((pos == rest) || (*rest != ','))
    return (char*)0;
  pos = rest+1;

  ip[1] = strtol(pos, &rest, 10);
  if((pos == rest) || (*rest != ','))
    return (char*)0;
  pos = rest+1;

  ip[2] = strtol(pos, &rest, 10);
  if((pos == rest) || (*rest != ','))
    return (char*)0;
  pos = rest+1;

  ip[3] = strtol(pos, &rest, 10);
  if((pos == rest) || (*rest != ','))
    return (char*)0;
  pos = rest+1;

  po[0] = strtol(pos, &rest, 10);
  if((pos == rest) || (*rest != ','))
    return (char*)0;
  pos = rest+1;

  po[1] = strtol(pos, &rest, 10);
  if((pos == rest) || (*rest != ')'))
    return (char*)0;
  pos = rest+1;

  *port = (po[0] << 8) | po[1];
  return bgav_sprintf("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  }

/* Open Funktion: Gibt 1 bei Erfolg zurueck */
/*
 *  150   About to open data connection
 *  200   Binaer mode 
 *  213   Size of file
 *  220   Connection ready
 *  221   Quit connection
 *  226   File send ok
 *  227   Pasv mode
 *  230   Login successful
 *  250   Directory successfully changed
 *  331   Login password
 */

#define FREE(ptr) if(ptr) { free(ptr); ptr = (char*)0; }

static int open_ftp(bgav_input_context_t * ctx, const char * url, char ** r)
  {
  int port = -1;
  int server_msg_alloc = 0;
  int data_port = -1;
  char * data_ip = (char*)0;
  char * server_msg = (char*)0;
  char * server_cmd = (char*)0;
  char * host = (char*)0;
  char * path = (char*)0;
  char *file_name;
  char * pos;
  char * user = (char*)0;
  char * pass = (char*)0;
  
  ftp_priv_t * p;
  int ret = 0;
  
  if(!bgav_url_split(url,
                     (char**)0, /* Protocol */
                     &user, /* User */
                     &pass, /* Pass */
                     &host,
                     &port,
                     &path))
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Unvalid URL");
    goto fail;
    }
  if(port == -1)
    {
    port = 21;
    }
  
  p = calloc(1, sizeof(*p));
  ctx->priv = p;
  
  /* Connect */
  if((p->control_fd = bgav_tcp_connect(ctx->opt, host, port))== -1)
    goto fail;
  if(get_server_answer(p->control_fd, &server_msg, &server_msg_alloc,
                       ctx->opt->connect_timeout) != 220)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not read answer");
    goto fail;
    }
  /* done */

  /* Check out how to log in */

  if(!user || !pass)
    {
    if(ctx->opt->ftp_anonymous)
      {
      user = bgav_strdup("ftp");

      if(ctx->opt->ftp_anonymous_password)
        pass = bgav_strdup(ctx->opt->ftp_anonymous_password);
      else
        pass = bgav_strdup("gates@nanosoft.com");
      }
    else /* Get user/pass with callback */
      {
      if(!ctx->opt->user_pass_callback ||
         !ctx->opt->user_pass_callback(ctx->opt->user_pass_callback_data,
                                       host, &user, &pass))
        goto fail;
      }
    }
  
  /* Server login */
  server_cmd = bgav_sprintf("USER %s\r\n", user);
  if(!bgav_tcp_send(ctx->opt, p->control_fd, (uint8_t*)server_cmd, strlen(server_cmd)))
    goto fail;
  FREE(server_cmd);
  if(get_server_answer(p->control_fd, &server_msg, &server_msg_alloc,
                       ctx->opt->connect_timeout) != 331)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Could not read answer");
    goto fail;
    }
  server_cmd = bgav_sprintf("PASS %s\r\n", pass);
  
  if(!bgav_tcp_send(ctx->opt, p->control_fd, (uint8_t*)server_cmd, strlen(server_cmd)))
    goto fail;
  FREE(server_cmd);
  if(get_server_answer(p->control_fd, &server_msg, &server_msg_alloc,
                       ctx->opt->connect_timeout) != 230)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not read answer");
    goto fail;
    }
  /* done */

  /* parse file_name and directory */
  file_name = strrchr(path, '/');
  if(!file_name)
    goto fail;

  *file_name = '\0';
  file_name ++;
  /* done */


  
  /* Change Directory */
  server_cmd = bgav_sprintf("CWD %s\r\n",path);
  if(!bgav_tcp_send(ctx->opt, p->control_fd, (uint8_t*)server_cmd, strlen(server_cmd)))
    goto fail;
  FREE(server_cmd);
  if(get_server_answer(p->control_fd, &server_msg, &server_msg_alloc,
                       ctx->opt->connect_timeout) != 250)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not read answer");
    goto fail;
    }
  /* done */


  
  /* Find size of File */
  server_cmd = bgav_sprintf("SIZE %s\r\n",file_name);
  if(!bgav_tcp_send(ctx->opt,
                    p->control_fd, (uint8_t*)server_cmd, strlen(server_cmd)))
    goto fail;
  FREE(server_cmd);
  
  if(get_server_answer(p->control_fd, &server_msg, &server_msg_alloc,
                       ctx->opt->connect_timeout) != 213)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not read answer");
    goto fail;
    }

  pos = server_msg;
  while(!isspace(*pos) && (pos != '\0'))
    pos++;

  if(*pos == '\0')
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Invalid server answer");
    goto fail;
    }
  ctx->total_bytes = strtoll(pos, NULL, 10);

  /* done */
  
  /* Set Binaer */
  server_cmd = bgav_sprintf("TYPE I\r\n");

  if(!bgav_tcp_send(ctx->opt,
                    p->control_fd, (uint8_t*)server_cmd, strlen(server_cmd)))
    goto fail;
  FREE(server_cmd);
  if(get_server_answer(p->control_fd, &server_msg, &server_msg_alloc,
                       ctx->opt->connect_timeout) != 200)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not read answer");
    goto fail;
    }
  /* done */



  
  /* Set PASV */
  server_cmd = bgav_sprintf("PASV\r\n");

  if(!bgav_tcp_send(ctx->opt, p->control_fd, (uint8_t*)server_cmd, strlen(server_cmd)))
    goto fail;
  FREE(server_cmd);
  if(get_server_answer(p->control_fd, &server_msg, &server_msg_alloc,
                       ctx->opt->connect_timeout) != 227)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not read answer");
    goto fail;
    }
  data_ip = parse_address(server_msg, &data_port);
  /* done */

  /* Connect */
  if((p->data_fd = bgav_tcp_connect(ctx->opt, data_ip, data_port))== -1)
    goto fail;
  /* done */

  /* open data connection */ 
  server_cmd = bgav_sprintf("RETR %s\r\n", file_name);

  if(!bgav_tcp_send(ctx->opt, p->control_fd, (uint8_t*)server_cmd, strlen(server_cmd)))
    goto fail;
  FREE(server_cmd);
  
  if(get_server_answer(p->control_fd, &server_msg, &server_msg_alloc,
                       ctx->opt->connect_timeout) != 150)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Could not read answer");
    goto fail;
    }
  /* done */
  
  ctx->do_buffer = 1;
  ctx->url = bgav_strdup(url);
  
  ret = 1;

  fail:

  FREE(server_cmd);
  FREE(host);
  FREE(path);
  FREE(user);
  FREE(pass);
  
  FREE(data_ip);
  FREE(server_msg);
  return ret;
  }


/* Lese funktionen: Geben die Zahl der gelesenen BYTES zurueck */

static int do_read(bgav_input_context_t * ctx,
                   uint8_t * buffer, int len, int timeout)
  {
  int len_read;
  ftp_priv_t * p;
  p = (ftp_priv_t*)(ctx->priv);

  if(len + p->bytes_read > ctx->total_bytes)
    len = ctx->total_bytes - p->bytes_read;

  if(!len)
    {
    return 0;
    }
  len_read = bgav_read_data_fd(p->data_fd, buffer, len, timeout);
  p->bytes_read += len_read;
  return len_read;
  }

static int read_ftp(bgav_input_context_t * ctx,
                    uint8_t * buffer, int len)
  {
  return do_read(ctx, buffer, len, ctx->opt->read_timeout);
  }

static int read_nonblock_ftp(bgav_input_context_t * ctx,
                             uint8_t * buffer, int len)
  {
  return do_read(ctx, buffer, len, 0);
  }


/* Close: Zumachen */

static void close_ftp(bgav_input_context_t * ctx)
  {
  char * server_cmd;
  ftp_priv_t * p;
  p = (ftp_priv_t*)(ctx->priv);
  
  server_cmd = bgav_sprintf("QUIT\r\n");
  bgav_tcp_send(ctx->opt, p->control_fd, (uint8_t*)server_cmd,
                strlen(server_cmd));
  free(server_cmd);
  
  if(p->control_fd >= 0)
    close(p->control_fd);
  if(p->data_fd >= 0)
    close(p->data_fd);
  
  free(p);
  }

bgav_input_t bgav_input_ftp =
  {
    .name =          "ftp",
    .open =          open_ftp,
    .read =          read_ftp,
    .read_nonblock = read_nonblock_ftp,
    .close =         close_ftp
  };

