/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gmerlin/bgsocket.h>
#include <gmerlin/utils.h>

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
