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

#include "gavftools.h"

/* Buffer */

#define BUFFER_TYPE_PACKET      0
#define BUFFER_TYPE_METADATA    1
#define BUFFER_TYPE_SYNC_HEADER 2

typedef struct buffer_element_s
  {
  int type;
  gavl_packet_t p;
  gavl_metadata_t m;
  struct buffer_element_s * next;
  } buffer_element_t;

void buffer_element_set_sync_header(buffer_element_t * el);

void buffer_element_set_packet(buffer_element_t * el,
                               const gavl_packet_t * p);

void buffer_element_set_metadata(buffer_element_t * el,
                                 const gavl_metadata_t * m);

buffer_element_t * buffer_element_create();
void buffer_element_destroy(buffer_element_t *);

typedef struct
  {
  buffer_element_t * first;
  buffer_element_t * last;

  buffer_element_t * pool;
  int pool_size;
  } buffer_t;

buffer_t * buffer_create(int num_elements);
void buffer_destroy(buffer_t *);
buffer_element_t * buffer_get_write(buffer_t *);

void buffer_advance(buffer_t *);


/* Client (= listener) connection */

#define CLIENT_WAIT_SYNC    0
#define CLIENT_PLAYING      1
#define CLIENT_ERROR        2

typedef struct
  {
  bg_plug_t * plug;
  buffer_element_t * cur;
  int status;
  } client_t;

client_t * client_create(int fd, const gavf_program_header_t * ph);
void client_destroy(client_t * cl);

/* One program */

#define PROGRAM_STATUS_DONE    0
#define PROGRAM_STATUS_RUNNING 1
#define PROGRAM_STATUS_STOP    2

#define PROGRAM_HAVE_HEADER    (1<<0)

typedef struct
  {
  pthread_mutex_t client_mutex;
  pthread_mutex_t status_mutex;

  char * name;

  bg_plug_t * in_plug;
  bg_mediaconnector_t conn;
  bg_plug_t * conn_plug;
  
  int clients_alloc;
  int num_clients;
  client_t ** clients;
  
  
  int status;
  int flags;
  
  pthread_t thread;

  gavf_program_header_t ph;

  buffer_t * buf;

  gavl_timer_t * timer;
  
  } program_t;

program_t * program_create_from_socket(const char * name, int fd);
program_t * program_create_from_plug(const char * name, bg_plug_t * plug);

int program_get_status(program_t * p);

void program_destroy(program_t *);
void program_attach_client(program_t *, int fd);

void program_run(program_t * p);


typedef struct
  {
  pthread_mutex_t program_mutex;

  program_t ** programs;
  int num_programs;
  int programs_alloc;
  
  int * listen_sockets;
  char ** listen_addresses;
  
  int num_listen_sockets;
    
  } server_t;

server_t * server_create(char ** listen_addresses,
                         int num_listen_addresses);

int server_iteration(server_t * s);

void server_destroy(server_t * s);

