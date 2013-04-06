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

typedef struct
  {
  bg_plug_t * plug;
  } sink_client_t;

typedef struct
  {
  bg_plug_t * plug;
  } source_client_t;

#define PROGRAM_STATUS_DONE    0
#define PROGRAM_STATUS_RUNNING 1

/* One program */

typedef struct
  {
  char * name;
  
  source_client_t * src;

  int num_sink_clients;
  sink_client_t * sinks;
  
  } program_t;


typedef struct
  {
  pthread_mutex_t program_mutex;

  program_t * programs;
  int num_programs;

  int * listen_sockets;
  char ** listen_addresses;
  
  int num_listen_sockets;
    
  } server_t;

server_t * server_create(char ** listen_addresses,
                         int num_listen_addresses);

int server_iteration(server_t * s);

void server_destroy(server_t * s);

