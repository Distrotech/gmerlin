/*****************************************************************
 
  bgsocket.h
 
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

#ifndef __BG_SOCKET_H_
#define __BG_SOCKET_H_

#include <inttypes.h>

/* Opaque address structure so we can support IPv6 in the future */

typedef struct bg_host_address_s bg_host_address_t;

bg_host_address_t * bg_host_address_create();
void bg_host_address_destroy(bg_host_address_t *);

/* Set the address from an URL, return FALSE on error */

int bg_host_address_set_from_url(bg_host_address_t *, const char * url,
                                 const char ** rest);

/* Get address from hostname and port */

int bg_host_address_set(bg_host_address_t *, const char * host,
                        int port);

/*
 *  Client connection (stream oriented)
 *  timeout is in milliseconds
 */

int bg_socket_connect_inet(bg_host_address_t*, int timeout);
int bg_socket_connect_unix(const char *);

void bg_socket_disconnect(int);

/* Server socket (stream oriented) */

int bg_listen_socket_create_inet(int port,
                                 int queue_size);

int bg_listen_socket_create_unix(const char * name,
                                 int queue_size);

/* Accept a new client connection, return -1 if there is none */

int bg_listen_socket_accept(int);

void bg_listen_socket_destroy(int);

int bg_socket_read_data(int fd, uint8_t * data, int len, int block);
int bg_socket_write_data(int fd, uint8_t * data, int len);

int bg_socket_read_line(int fd, char ** ret,
                        int * ret_alloc, int block);

#endif // __BG_SOCKET_H_
