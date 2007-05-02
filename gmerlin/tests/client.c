#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bgsocket.h>
#include <utils.h>

#define INET_PORT 1122
#define UNIX_NAME "blubberplatsch"
#define BUFFER_SIZE 1024

int main(int argc, char ** argv)
  {
  int  fd;
  char buffer[BUFFER_SIZE];
  bg_host_address_t * addr;
  int result;
  
  if(argc != 2)
    {
    fprintf(stderr, "Usage: %s [-t|-u]\n", argv[0]);
    return -1;
    }

  /* Open socket */

  if(argv[1][1] == 't') /* TCP/IP */
    {
    addr = bg_host_address_create();
    bg_host_address_set(addr, "localhost", INET_PORT, SOCK_STREAM);
    fd = bg_socket_connect_inet(addr, 500);
    if(fd == -1)
      {
      fprintf(stderr, "Cannot connect to TCP server\n");
      return -1;
      }
    }
  else
    {
    fd = bg_socket_connect_unix(UNIX_NAME);
    if(fd == -1)
      {
      fprintf(stderr, "Cannot connect to UNIX server\n");
      return -1;
      }
    }
  while(1)
    {
    fgets(buffer, BUFFER_SIZE-1, stdin);
    result = write(fd, buffer, strlen(buffer));
    fprintf(stderr, "Wrote %d bytes\n", result);
    }
  }
