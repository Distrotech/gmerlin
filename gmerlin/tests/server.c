#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>

#include <bgsocket.h>
#include <utils.h>

#include <netinet/in.h>

#define MAX_CONNECTIONS 1024

#define INET_PORT 1122
#define UNIX_NAME "blubberplatsch"

typedef struct connection_s
  {
  int fd;
  struct connection_s * next;
  } connection_t;

connection_t * add_connection(connection_t * list, int fd)
  {
  connection_t * new_connection;
  
  new_connection = calloc(1, sizeof(*new_connection));
  new_connection->fd = fd;
  new_connection->next = list;
  return new_connection;
  }

connection_t * remove_connection(connection_t * list, connection_t * c)
  {
  connection_t * before;

  if(c == list)
    list = list->next;
  else
    {
    before = list;

    while(before->next != c)
      before = before->next;

    before->next = c->next;
    }
  free(c);
  return list;
  }

int main(int argc, char ** argv)
  {
  connection_t * connections = (connection_t *)0;
  connection_t * con_ptr;
  connection_t * con_tmp;
  int new_fd;
  int i;
  struct pollfd * pollfds = (struct pollfd *)0;
  int num_pollfds = 0;
  int num_connections = 0;
  int unix_socket;
  int tcp_socket;
  int result;

  char * buffer = (char *)0;
  int buffer_size = 0;
  int keep_going = 1;
  
  /* Create unix listener */

  tcp_socket = bg_listen_socket_create_inet(1122, 10, INADDR_LOOPBACK);
  if(tcp_socket == -1)
    fprintf(stderr, "Cannot create TCP Socket\n");

  unix_socket = bg_listen_socket_create_unix(UNIX_NAME, 10);
  if(unix_socket == -1)
    fprintf(stderr, "Cannot create UNIX Socket\n");

  while(keep_going)
    {
    if((new_fd = bg_listen_socket_accept(tcp_socket)) != -1)
      {
      fprintf(stderr, "TCP Connection coming in: %d\n", new_fd);
      connections = add_connection(connections, new_fd);
      num_connections++;
      }

    if((new_fd = bg_listen_socket_accept(unix_socket)) != -1)
      {
      fprintf(stderr, "Local connection coming in: %d\n", new_fd);
      connections = add_connection(connections, new_fd);
      num_connections++;
      }
    
    /* Now, poll for messages */

    if(num_connections)
      {
      if(num_pollfds < num_connections)
        {
        num_pollfds = num_connections + 10;
        pollfds = realloc(pollfds, num_pollfds * sizeof(*pollfds));
        }
      con_ptr = connections;

      for(i = 0; i < num_connections; i++)
        {
        pollfds[i].fd     = con_ptr->fd;
        pollfds[i].events = POLLIN | POLLPRI;
        pollfds[i].revents = 0;
        con_ptr = con_ptr->next;
        }

      /* Now, do the poll */

      result = poll(pollfds, num_connections, 1000);

      //      fprintf(stderr, "Poll: %d\n", result);
      
      if(result > 0)
        {
        con_ptr = connections;
        
        for(i = 0; i < num_connections; i++)
          {
          if(pollfds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
            {
            /* Connection closed */
            close(pollfds[i].fd);
            con_tmp = con_ptr->next;
            connections = remove_connection(connections, con_ptr);
            num_connections--;
            con_ptr = con_tmp;
            fprintf(stderr, "Connection %d closed (fd: %d)\n", i+1,
                    pollfds[i].fd);
            }
          else if(pollfds[i].revents & (POLLIN | POLLPRI))
            {
            /* Read message */

            if(!bg_read_line_fd(pollfds[i].fd, &buffer, &buffer_size))
              {
              con_tmp = con_ptr->next;
              connections = remove_connection(connections, con_ptr);
              num_connections--;

              fprintf(stderr, "Connection %d closed (fd: %d)\n", i+1,
                      pollfds[i].fd);
              
              con_ptr = con_tmp;
              }
            else
              {
              fprintf(stderr, "Message from connection %d: %s\n",
                      i+1, buffer);
              if(!strcmp(buffer, "quit"))
                {
                fprintf(stderr, "Exiting\n");
                keep_going = 0;
                }
              }
            }
                    else
            con_ptr = con_ptr->next;
          }
        
        }
        

      }
    }

  /* Close all sockets */
  close(unix_socket);
  close(tcp_socket);
  
  for(i = 0; i < num_connections; i++)
    close(pollfds[i].fd);
  
  return 0;
  }

