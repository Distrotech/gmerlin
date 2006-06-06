/*****************************************************************
 
  remote.c
 
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

#include <string.h>
#include <unistd.h>

#include <config.h>

#include <remote.h>
#include <bgsocket.h>
#include <utils.h>
#include <log.h>

/* For INADDR_ Macros */

#include <netinet/in.h>

#define LOG_DOMAIN "remote_server"

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
  };

bg_remote_server_t * bg_remote_server_create(int listen_port,
                                             char * protocol_id)
  {
  bg_remote_server_t * ret;
  ret = calloc(1, sizeof(*ret));
  ret->listen_port = listen_port;
  ret->protocol_id = bg_strdup(ret->protocol_id, protocol_id);
  //  fprintf(stderr, "Listen port: %d\n", default_listen_port);
  ret->fd = -1;
  ret->msg = bg_msg_create();
  return ret;
  }

int bg_remote_server_init(bg_remote_server_t * s)
  {
  s->fd = bg_listen_socket_create_inet(s->listen_port,
                                       s->max_connections,
                                       s->allow_remote ? INADDR_ANY : INADDR_LOOPBACK);
  if(s->fd < 0)
    {
    bg_log(BG_LOG_WARNING, LOG_DOMAIN,
           "Setting up socket failed, this instance won't be reachable via remote");
    return 0;
    }

  fprintf(stderr, "Remote socket listening at port %d\n", s->listen_port);
  return 1;
  }

static void bg_remote_server_cleanup(bg_remote_server_t * s)
  {
  close(s->fd);
  s->fd = -1;
  }

static server_connection_t * add_connection(bg_remote_server_t * s,
                                            int fd)
  {
  int len;
  char ** strings = (char **)0;
  
  char * welcome_msg = (char *)0;

  char * buffer = (char *)0;
  int buffer_alloc = 0;

  server_connection_t * ret = (server_connection_t *)0;
  
  if(!bg_socket_read_line(fd, &(buffer),
                          &buffer_alloc, 1))
    {
    fprintf(stderr, "Reading hello line failed\n");
    goto fail;
    }
  //  fprintf(stderr, "Got hello line: %s\n", buffer);

  strings = bg_strbreak(buffer, ' ');
  if(!strings[0] || strcmp(strings[0], s->protocol_id) ||
     !strings[1] || strcmp(strings[1], VERSION) ||
     !strings[2])
    {
    fprintf(stderr, "Protocol mismatch");
    goto fail;
    }
  
  /* Write the answer message */

  welcome_msg = bg_sprintf("%s %s\r\n", s->protocol_id, VERSION);

  len = strlen(welcome_msg);

  //  fprintf(stderr, "Sending welcome msg: %s\n", welcome_msg);
    
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
  
  return connections;
  }

static void check_connections(bg_remote_server_t * s)
  {
  int new_fd;
  server_connection_t * conn;
  
  /* Check for new connections */

  new_fd = bg_listen_socket_accept(s->fd);

  if(new_fd >= 0)
    {
    //    fprintf(stderr, "New client connection\n");
    
    conn = add_connection(s, new_fd);

    if(conn)
      {
      conn->next = s->connections;
      s->connections = conn;
      }
    }
  }

bg_msg_t * bg_remote_server_get_msg(bg_remote_server_t * s)
  {
  fd_set rset;
  
  struct timeval timeout;
  
  server_connection_t * conn, * tmp_conn;
  check_connections(s);

  if(!s->connections)
    return (bg_msg_t *)0;

  conn = s->connections;
    
  while(conn)
    {
    FD_ZERO (&rset);
    FD_SET  (conn->fd, &rset);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;
    
    if(select (conn->fd+1, &rset, NULL, NULL, &timeout) > 0)
      {
      bg_msg_free(s->msg);
      if(bg_message_read_socket(s->msg, conn->fd, -1))
        return s->msg;
      else /* Select said reading won't block but reading failed
              -> Client probably disconnected */
        {
        //        fprintf(stderr, "Removing connection\n");
        tmp_conn = conn->next;
        s->connections = remove_connection(s->connections, conn);
        conn = tmp_conn;
        }
      }
    else
      conn = conn->next;
    }
  
  return (bg_msg_t *)0;
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

static bg_parameter_info_t server_parameters[] =
  {
    {
      name:        "allow_remote",
      long_name:   "Allow connections from other machines",
      type:        BG_PARAMETER_CHECKBUTTON,
      val_default: { val_i: 0 },
    },
    {
      name:      "max_connections",
      long_name: "Maximum number of connections",
      type:      BG_PARAMETER_INT,
      val_min:     { val_i: 0 },
      val_max:     { val_i: 100 },
      val_default: { val_i: 5 },
    },
    { /* End of parameters */ }
  };

bg_parameter_info_t * bg_remote_server_get_parameters(bg_remote_server_t * s)
  {
  return server_parameters;
  }

void
bg_remote_server_set_parameter(void * data,
                               char * name,
                               bg_parameter_value_t * v)
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

int bg_remote_client_init(bg_remote_client_t * c,
                          const char * host, int port,
                          int milliseconds)
  {
  int ret = 0;
  char ** strings = (char**)0;
  int buffer_alloc = 0;
  char * buffer = (char*)0;
  int len;
  
  char * answer_message;
    
  bg_host_address_t * addr = bg_host_address_create();
  c->milliseconds = milliseconds;
  if(!bg_host_address_set(addr, host, port))
    goto fail;
  fprintf(stderr, "Connecting remote client...");
  c->fd = bg_socket_connect_inet(addr, c->milliseconds);
  fprintf(stderr, "done\n");
  if(c->fd < 0)
    {
    fprintf(stderr, "Connecting failed\n");
    goto fail;
    }

  /* Send hello line */
  answer_message = bg_sprintf("%s %s %s\r\n", c->protocol_id,
                              VERSION, (c->read_messages ? "1" : "0"));
  len = strlen(answer_message);

  if(bg_socket_write_data(c->fd, (uint8_t*)answer_message, len) < len)
    goto fail;

  fprintf(stderr, "Reading answer message\n");
  /* Read welcome message */
  
  if(!bg_socket_read_line(c->fd, &(buffer),
                          &buffer_alloc, c->milliseconds))
    {
    fprintf(stderr, "Reading welcome line failed\n");
    goto fail;
    }
  fprintf(stderr, "Got welcome line: %s\n", buffer);

  strings = bg_strbreak(buffer, ' ');

  if((!strings[0] || strcmp(strings[0], c->protocol_id)) ||
     (!strings[1] || strcmp(strings[1], VERSION)))
    {
    fprintf(stderr, "Protocol mismatch");
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
  return bg_message_write_socket(c->msg, c->fd);
  }

bg_msg_t * bg_remote_client_get_msg_read(bg_remote_client_t * c)
  {
  return (bg_msg_t*)0;
  }

