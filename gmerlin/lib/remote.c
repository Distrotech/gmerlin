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
#include <gmerlin/translation.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif

#include <unistd.h>
#include <string.h>

#include <gmerlin/remote.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/utils.h>
#include <gmerlin/log.h>

/* For INADDR_ Macros */

#include <netinet/in.h>

#define LOG_DOMAIN_SERVER "remote.server"
#define LOG_DOMAIN_CLIENT "remote.client"

/*
 *  Server
 */

typedef struct server_connection_s 
  {
  int fd;
  int do_put_msg;
  struct server_connection_s * next;
  } server_connection_t;

struct bg_remote_server_s
  {
  int fd;
  char * protocol_id;
  
  /* Configuration stuff */

  int allow_remote;
  
  int listen_port;
  int max_connections;

  server_connection_t * connections;

  int do_reopen;
  bg_msg_t * msg;
  int last_read_fd;
  };

bg_remote_server_t * bg_remote_server_create(int listen_port,
                                             char * protocol_id)
  {
  bg_remote_server_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->listen_port = listen_port;
  ret->protocol_id = bg_strdup(ret->protocol_id, protocol_id);
  ret->fd = -1;
  ret->msg = bg_msg_create();
  return ret;
  }

int bg_remote_server_init(bg_remote_server_t * s)
  {
  int flags = 0;
  if(!s->allow_remote)
    flags |= BG_SOCKET_LOOPBACK;
  
  s->fd = bg_listen_socket_create_inet(s->listen_port,
                                       s->max_connections,
                                       flags);
  if(s->fd < 0)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN_SERVER,
           "Setting up socket failed, this instance won't be reachable via remote");
    return 0;
    }

  bg_log(BG_LOG_INFO, LOG_DOMAIN_SERVER,
         "Remote socket listening at port %d", s->listen_port);
  return 1;
  }

static server_connection_t * add_connection(bg_remote_server_t * s,
                                            int fd)
  {
  int len;
  char ** strings = NULL;
  
  char * welcome_msg = NULL;

  char * buffer = NULL;
  int buffer_alloc = 0;

  server_connection_t * ret = NULL;
  
  if(!bg_socket_read_line(fd, &buffer,
                          &buffer_alloc, 1))
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN_SERVER, "Reading hello line failed");
    goto fail;
    }

  strings = bg_strbreak(buffer, ' ');
  if(!strings[0] || strcmp(strings[0], s->protocol_id) ||
     !strings[1] || strcmp(strings[1], VERSION) ||
     !strings[2])
    {
    bg_log(BG_LOG_INFO, LOG_DOMAIN_SERVER, "Protocol mismatch");
    goto fail;
    }
  
  /* Write the answer message */

  welcome_msg = bg_sprintf("%s %s\r\n", s->protocol_id, VERSION);

  len = strlen(welcome_msg);

    
  if(bg_socket_write_data(fd, (uint8_t*)welcome_msg, len) < len)
    goto fail;
  

  ret = calloc(1, sizeof(*ret));
  ret->fd = fd;
  ret->do_put_msg = atoi(strings[2]);
  fail:
  if(buffer)
    free(buffer);
  if(welcome_msg)
    free(welcome_msg);
  if(strings)
    bg_strbreak_free(strings);
  if(!ret)
    close(fd);
  return ret;
  }

static server_connection_t *
remove_connection(server_connection_t * connections,
                  server_connection_t * conn)
  {
  server_connection_t * before;

  
  if(conn == connections)
    {
    connections = connections->next;
    }
  else
    {
    before = connections;
    while(before->next != conn)
      before = before->next;
    before->next = conn->next;
    }
  
  close(conn->fd);
  free(conn);
  bg_log(BG_LOG_DEBUG, LOG_DOMAIN_SERVER, "Client connection closed");
  
  return connections;
  }

static void bg_remote_server_cleanup(bg_remote_server_t * s)
  {
  close(s->fd);
  s->fd = -1;
  while(s->connections) s->connections =
                          remove_connection(s->connections, s->connections);
  }

static void check_connections(bg_remote_server_t * s)
  {
  int new_fd;
  server_connection_t * conn;
  
  /* Check for new connections */

  new_fd = bg_listen_socket_accept(s->fd, 0);

  if(new_fd >= 0)
    {
    bg_log(BG_LOG_DEBUG, LOG_DOMAIN_SERVER, "New client connection");
    conn = add_connection(s, new_fd);

    if(conn)
      {
      conn->next = s->connections;
      s->connections = conn;
      }
    }
  }

static int select_socket(int fd, int milliseconds)
  {
  fd_set rset;
  struct timeval timeout;

  FD_ZERO (&rset);
  FD_SET  (fd, &rset);

  timeout.tv_sec  = milliseconds / 1000;
  timeout.tv_usec = (milliseconds % 1000) * 1000;
  
  if(select (fd+1, &rset, NULL, NULL, &timeout) > 0)
    return 1;
  return 0;
  }

bg_msg_t * bg_remote_server_get_msg(bg_remote_server_t * s)
  {
  server_connection_t * conn, * tmp_conn;
  check_connections(s);

  if(!s->connections)
    return NULL;

  conn = s->connections;
    
  while(conn)
    {
    if(select_socket(conn->fd, 0))
      {
      bg_msg_free(s->msg);
      if(bg_msg_read_socket(s->msg, conn->fd, -1))
        {
        s->last_read_fd = conn->fd;
        return s->msg;
        }
      else /* Select said reading won't block but reading failed
              -> Client probably disconnected */
        {
        tmp_conn = conn->next;
        s->connections = remove_connection(s->connections, conn);
        conn = tmp_conn;
        }
      }
    else
      conn = conn->next;
    }
  
  return NULL;
  }

bg_msg_t * bg_remote_server_get_msg_write(bg_remote_server_t * s)
  {
  bg_msg_free(s->msg);
  return s->msg;
  }

int bg_remote_server_done_msg_write(bg_remote_server_t * s)
  {
  /* Write message */
  return bg_msg_write_socket(s->msg, s->last_read_fd);
  }

void bg_remote_server_wait_close(bg_remote_server_t * s)
  {
  gavl_time_t delay_time = GAVL_TIME_SCALE/200;
  while(1)
    {
    bg_remote_server_get_msg(s);
    if(!s->connections)
      break;
    gavl_time_delay(&delay_time);
    }
  }

void bg_remote_server_put_msg(bg_remote_server_t * s, bg_msg_t * m)
  {
  
  }

void bg_remote_server_destroy(bg_remote_server_t * s)
  {
  while(s->connections)
    s->connections = remove_connection(s->connections, s->connections);
  
  if(s->protocol_id)
    free(s->protocol_id);

  if(s->fd >= 0)
    close(s->fd);
  if(s->msg)
    bg_msg_destroy(s->msg);
  free(s);
  }

static const bg_parameter_info_t server_parameters[] =
  {
    {
      .name =        "allow_remote",
      .long_name =   TRS("Allow connections from other machines"),
      .type =        BG_PARAMETER_CHECKBUTTON,
      .val_default = { .val_i = 0 },
    },
    {
      .name =      "max_connections",
      .long_name = TRS("Maximum number of connections"),
      .type =      BG_PARAMETER_INT,
      .val_min =     { .val_i = 0 },
      .val_max =     { .val_i = 100 },
      .val_default = { .val_i = 5 },
    },
    { /* End of parameters */ }
  };

const bg_parameter_info_t * bg_remote_server_get_parameters(bg_remote_server_t * s)
  {
  return server_parameters;
  }

void
bg_remote_server_set_parameter(void * data,
                               const char * name,
                               const bg_parameter_value_t * v)
  {
  bg_remote_server_t * s;
  s = (bg_remote_server_t *)data;

  if(!name)
    {
    if((s->fd < 0) && s->max_connections)
      s->do_reopen = 1;
    
    if(!s->max_connections)
      {
      s->do_reopen = 0;
      if(s->fd >= 0)
        bg_remote_server_cleanup(s); /* Close everything */
      }
    else if(s->do_reopen)
      {
      if(s->fd >= 0)
        bg_remote_server_cleanup(s); /* Close everything */
      bg_remote_server_init(s);
      }
    return;
    }
  else if(!strcmp(name, "allow_remote"))
    {
    if(s->allow_remote != v->val_i)
      s->do_reopen = 1;
    s->allow_remote = v->val_i;
    }
  else if(!strcmp(name, "max_connections"))
    s->max_connections = v->val_i;
  
  }

/* Client */

struct bg_remote_client_s
  {
  int fd;
  char * protocol_id;
  int read_messages;

  bg_msg_t * msg;
  int milliseconds; /* Read and connect timeout */
  };

bg_remote_client_t * bg_remote_client_create(const char * protocol_id,
                                             int read_messages)
  {
  bg_remote_client_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->protocol_id = bg_strdup(ret->protocol_id, protocol_id);
  ret->read_messages = read_messages;
  ret->msg = bg_msg_create();
  return ret;
  }

void bg_remote_client_close(bg_remote_client_t * c)
  {
  close(c->fd);
  c->fd = -1;
  }

int bg_remote_client_init(bg_remote_client_t * c,
                          const char * host, int port,
                          int milliseconds)
  {
  int ret = 0;
  char ** strings = NULL;
  int buffer_alloc = 0;
  char * buffer = NULL;
  int len;
  
  char * answer_message;
    
  bg_host_address_t * addr = bg_host_address_create();
  c->milliseconds = milliseconds;
  if(!bg_host_address_set(addr, host, port, SOCK_STREAM))
    goto fail;
  c->fd = bg_socket_connect_inet(addr, c->milliseconds);
  if(c->fd < 0)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN_CLIENT, "Connecting failed");
    goto fail;
    }

  /* Send hello line */
  answer_message = bg_sprintf("%s %s %s\r\n", c->protocol_id,
                              VERSION, (c->read_messages ? "1" : "0"));
  len = strlen(answer_message);

  if(bg_socket_write_data(c->fd, (uint8_t*)answer_message, len) < len)
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN_CLIENT,
           "Sending initialization string failed");
    goto fail;
    }
  /* Read welcome message */
  
  if(!bg_socket_read_line(c->fd, &buffer,
                          &buffer_alloc, c->milliseconds))
    {
    bg_log(BG_LOG_ERROR,
           LOG_DOMAIN_CLIENT, "Reading welcome line failed");
    goto fail;
    }

  strings = bg_strbreak(buffer, ' ');

  if((!strings[0] || strcmp(strings[0], c->protocol_id)) ||
     (!strings[1] || strcmp(strings[1], VERSION)))
    {
    bg_log(BG_LOG_ERROR, LOG_DOMAIN_CLIENT, "Protocol mismatch");
    goto fail;
    }

  ret = 1;
  fail:
  if(strings)
    bg_strbreak_free(strings);
  bg_host_address_destroy(addr);
  return ret;
  }


void bg_remote_client_destroy(bg_remote_client_t * c)
  {
  bg_msg_destroy(c->msg);
  if(c->protocol_id)
    free(c->protocol_id);
  if(c->fd >= 0)
    close(c->fd);
  free(c);
  }

bg_msg_t * bg_remote_client_get_msg_write(bg_remote_client_t * c)
  {
  bg_msg_free(c->msg);
  return c->msg;
  }

int bg_remote_client_done_msg_write(bg_remote_client_t * c)
  {
  /* Write message */
  return bg_msg_write_socket(c->msg, c->fd);
  }

bg_msg_t * bg_remote_client_get_msg_read(bg_remote_client_t * c)
  {
  if(select_socket(c->fd, 1000))
    {
    bg_msg_free(c->msg);
    if(bg_msg_read_socket(c->msg, c->fd, -1))
      return c->msg;
    }
  return NULL;
  }

