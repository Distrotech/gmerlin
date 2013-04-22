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

#ifndef __BG_SOCKET_H_
#define __BG_SOCKET_H_

#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>


/* Opaque address structure so we can support IPv6 in the future */

#define BG_SOCKET_ADDR_STR_LEN (46+3+5+1) // INET6_ADDRSTRLEN + []: + port + \0

typedef struct bg_socket_address_s bg_socket_address_t;

bg_socket_address_t * bg_socket_address_create();
void bg_socket_address_destroy(bg_socket_address_t *);


/* Get address from hostname and port */

int bg_socket_address_set(bg_socket_address_t *, const char * host,
                        int port, int socktype);

void bg_socket_address_set_port(bg_socket_address_t * addr, int port);
int bg_socket_address_get_port(bg_socket_address_t * addr);

// str must be at least BG_SOCKET_ADDR_STR_LEN bytes long
char * bg_socket_address_to_string(bg_socket_address_t * addr, char * str);

int bg_socket_address_set_local(bg_socket_address_t * a, int port);

/*
 *  Client connection (stream oriented)
 *  timeout is in milliseconds
 */

int bg_socket_connect_inet(bg_socket_address_t*, int timeout);
int bg_socket_connect_unix(const char *);

void bg_socket_disconnect(int);

/* Server socket (stream oriented) */

// #define BG_SOCKET_IPV6     (1<<0)
#define BG_SOCKET_LOOPBACK (1<<1)

int bg_listen_socket_create_inet(bg_socket_address_t * addr,
                                 int port,
                                 int queue_size,
                                 int flags);

int bg_listen_socket_create_unix(const char * name,
                                 int queue_size);

/* Accept a new client connection, return -1 if there is none */

int bg_listen_socket_accept(int sock, int milliseconds);

void bg_listen_socket_destroy(int);

int bg_socket_get_address(int sock, bg_socket_address_t * local, bg_socket_address_t * remote);

/* UDP */

int bg_udp_socket_create(bg_socket_address_t * addr);

int bg_udp_socket_create_multicast(bg_socket_address_t * addr);

int bg_udp_socket_receive(int fd, uint8_t * data, int data_size,
                          bg_socket_address_t * addr);

int bg_udp_socket_send(int fd, const uint8_t * data, int data_size,
                       bg_socket_address_t * addr);

/* I/0 functions */

int bg_socket_read_data(int fd, uint8_t * data, int len, int milliseconds);
int bg_socket_write_data(int fd, const uint8_t * data, int len);

int bg_socket_read_line(int fd, char ** ret,
                        int * ret_alloc, int milliseconds);

int bg_socket_is_local(int fd);

int bg_socket_can_read(int fd, int milliseconds);
int bg_socket_can_write(int fd, int milliseconds);

int bg_socket_send_file(int fd, const char * filename,
                        int64_t offset, int64_t len);

#endif // __BG_SOCKET_H_
