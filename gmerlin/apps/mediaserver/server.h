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

#define CLIENT_TYPE_MEDIA           0 // Media file for upnp devices
#define CLIENT_TYPE_SOURCE_ONDEMAND 1 // Ondemand upstream source (bgplug)
#define CLIENT_TYPE_SOURCE_STREAM   2 // Upstream source (bgplug)
#define CLIENT_TYPE_SINK            3 // Sink (bgplug or filtered)

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

  char * name;
  
  void *data;
  void (*free_data)(void*);
  void (*func)(struct client_s * client);
  struct client_s * source;
  } client_t;

client_t * client_create(int type, int fd, void * data,
                         void (*free_data)(void*), void (*func)(client_t*));

int client_get_status(client_t *);
void client_destroy(client_t * c);

/* Buffer: Passes data from a program source to listeners in a thread save way */

#define BUFFER_TYPE_PACKET      0
#define BUFFER_TYPE_METADATA    1
#define BUFFER_TYPE_SYNC_HEADER 2

typedef struct buffer_element_s
  {
  int type;
  gavl_packet_t p;
  gavl_metadata_t m;

  int64_t seq; // Sequence number

  struct buffer_element_s * next;
  } buffer_element_t;

void buffer_element_set_sync_header(buffer_element_t * el);

void buffer_element_set_packet(buffer_element_t * el,
                               const gavl_packet_t * p);

void buffer_element_set_metadata(buffer_element_t * el,
                                 const gavl_metadata_t * m);

typedef struct buffer_s buffer_t;

buffer_t * buffer_create(int num_elements);

void buffer_destroy(buffer_t *);

buffer_element_t * buffer_get_write(buffer_t *);
void buffer_done_write(buffer_t *);

int buffer_get_read(buffer_t *, int64_t * seq, buffer_element_t ** el);
void buffer_done_read(buffer_t *);

void buffer_wait(buffer_t * b);

// void buffer_done_read(buffer_t *, buffer_element_t *);

buffer_element_t * buffer_get_first(buffer_t *);

int64_t buffer_get_start_seq(buffer_t *);

void buffer_stop(buffer_t * b);

int buffer_get_free(buffer_t * b);

/* On the fly ID3V2 generation for the more stupid clients */

typedef struct
  {
  int64_t id;
  int len;
  int alloc;
  uint8_t * data;
  gavl_time_t last_used;
  } id3v2_t;

int id3v2_generate(bg_db_t * db, int64_t id, id3v2_t * ret);

/* Filter: Sends data to the socket */

typedef struct
  {
  int is_supported;
  int (*supported)();
  
  int (*probe)(const gavf_program_header_t * ph,
               const gavl_metadata_t * request);
  
  void * (*create)(const gavf_program_header_t * ph,
                   const gavl_metadata_t * req,
                   const gavl_metadata_t * vars,
                   gavl_metadata_t * res, bg_plugin_registry_t * reg);

  int (*start)(void * priv,
               const gavf_program_header_t * ph,
               const gavl_metadata_t * req,
               const gavl_metadata_t * inline_metadata,
               gavf_io_t * io, int flags);
  
  int (*put_buffer)(void * priv, buffer_element_t *);
  void (*destroy)(void * priv);
  } filter_t;

void filter_init();

const filter_t * filter_find(const gavf_program_header_t * ph,
                             const gavl_metadata_t * request);

/* Client */

client_t * source_client_create(int * fd,
                                const char * path, bg_plugin_registry_t * plugin_reg, int type);

client_t * sink_client_create(int * fd, const gavl_metadata_t * req,
                              bg_plugin_registry_t * plugin_reg,
                              const gavf_program_header_t * ph,
                              const gavl_metadata_t * inline_metadata,
                              buffer_t * buf);

/* Server */

typedef struct
  {
  /* Config stuff */
  char * dbpath;
  bg_plugin_registry_t * plugin_reg; // Must be set manually
  bg_cfg_registry_t * cfg_reg;
  bg_socket_address_t * addr;
  int fd;
  int port;

  gavl_timer_t * timer;
  gavl_time_t current_time;

  pthread_mutex_t clients_mutex;
  client_t ** clients;
  int num_clients;
  int clients_alloc;

  char * root_url;

  bg_upnp_device_t * dev;
  bg_db_t * db;

  uuid_t uuid;

  const char * server_string;

  id3v2_t * id3_cache;
  int id3_cache_size;
  } server_t;

void server_attach_client(server_t*, client_t*cl);
void server_detach_client(server_t*, client_t*cl);

void server_init(server_t*);

int server_start(server_t*);

id3v2_t * server_get_id3(server_t * s, int64_t id);
id3v2_t * server_get_id3_for_file(server_t * s,
                                  bg_db_file_t * f,
                                  int * offset);


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

int server_handle_source(server_t * s, int * fd,
                         const char * method,
                         const char * path_orig,
                         const gavl_metadata_t * req);

int server_handle_stream(server_t * s, int * fd,
                         const char * method,
                         const char * path_orig,
                         const gavl_metadata_t * req);
