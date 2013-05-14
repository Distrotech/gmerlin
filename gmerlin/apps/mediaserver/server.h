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
#include <gmerlin/utils.h>
#include <gmerlin/log.h>
#include <gmerlin/translation.h>
#include <gmerlin/mediadb.h>
#include <gmerlin/cmdline.h>

#include <gmerlin/upnp/device.h>

#define CLIENT_STATUS_STARTING     0
#define CLIENT_STATUS_WAIT_SYNC    1
#define CLIENT_STATUS_RUNNING      2
#define CLIENT_STATUS_DONE         3
#define CLIENT_STATUS_STOP         4

#define CLIENT_TYPE_MEDIA          0 // Media file for upnp devices
#define CLIENT_TYPE_BGPLUG_SOURCE  1 // Upstream source (bgplug)
#define CLIENT_TYPE_BGPLUG_SINK    2 // Sink (bgplug or filtered)

/*
 *  URL structure:
 *  http://<root>/                      -> http interface
 *  http://<root>/static                -> Static files
 *  http://<root>/upnp/<dev>/desc.xml   -> Device description
 *  http://<root>/upnp/<dev>/<serv>.xml -> Service description
 *  http://<root>/media?id=<id>         -> Media files
 *
 */

typedef struct client_s
  {
  int type;
  pthread_mutex_t status_mutex;
  int status;
  
  pthread_t thread;
  int fd;

  void *data;
  void (*free_data)(void*);
  void (*func)(struct client_s * client);
  struct client_s * source;
  } client_t;

client_t * client_create(int type, int fd, void * data,
                         void (*free_data)(void*), void (*func)(client_t*));

int client_get_status(client_t *);
void client_destroy(client_t * c);

typedef struct
  {
  /* Config stuff */
  char * dbpath;
  bg_plugin_registry_t * plugin_reg; // Must be set manually
  bg_cfg_registry_t * cfg_reg;

  bg_socket_address_t * addr;
  int fd;

  pthread_mutex_t clients_mutex;
  client_t ** clients;
  int num_clients;
  int clients_alloc;

  char * root_url;

  bg_upnp_device_t * dev;
  bg_db_t * db;

  uuid_t uuid;

  const char * server_string;
  } server_t;

void server_attach_client(server_t*, client_t*cl);
void server_detach_client(server_t*, client_t*cl);

void server_init(server_t*);

int server_start(server_t*);

const bg_parameter_info_t * server_get_parameters();
void server_set_parameter(void * server, const char * name,
                          const bg_parameter_value_t * val);

int server_iteration(server_t * s);

void server_cleanup(server_t * s);

/* Media handler (fd will be set to -1 if we launched a
   thread for transferring the file) */

int server_handle_media(server_t * s, int * fd, const char * method, const char * path,
                        const gavl_metadata_t * req);

int server_handle_transcode(server_t * s, int * fd,
                            const char * method,
                            const char * path_orig,
                            const gavl_metadata_t * req);
