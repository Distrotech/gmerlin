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

#ifndef __BG_UPNP_DEVICE_H_
#define __BG_UPNP_DEVICE_H_

#include <uuid/uuid.h>
#include <gmerlin/xmlutils.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/mediadb.h>

typedef struct
  {
  const char * mimetype;
  int width;
  int height;
  int depth;
  const char * location;
  } 
bg_upnp_icon_t;

typedef struct bg_upnp_device_s bg_upnp_device_t;

int
bg_upnp_device_handle_request(bg_upnp_device_t * dev, int fd,
                              const char * method,
                              const char * path,
                              const gavl_metadata_t * header);

void
bg_upnp_device_destroy(bg_upnp_device_t * dev);

/* Call this regularly to keep things up to date */

int
bg_upnp_device_ping(bg_upnp_device_t * dev);

/* Creation routines for device types */

typedef struct
  {
  const char * name;
  const char * in_mimetype;
  const char * out_mimetype;
  int supported;

  int (*is_supported)(bg_plugin_registry_t * plugin_reg);

  char * (*make_command)(const char * source_file);
  char * (*make_protocol_info)(bg_db_object_t * obj);
  void (*set_header)(bg_db_object_t * obj,
                     const gavl_metadata_t * req,
                     gavl_metadata_t * res);
  
  } bg_upnp_transcoder_t;

typedef struct bg_upnp_transcoder_ctx_s bg_upnp_transcoder_ctx_t;

const bg_upnp_transcoder_t *
bg_upnp_transcoder_find(const char ** mimetypes_supp, const char * in_mimetype);

const bg_upnp_transcoder_t *
bg_upnp_transcoder_by_name(const char * name);


bg_upnp_device_t *
bg_upnp_create_media_server(bg_socket_address_t * addr,
                            uuid_t uuid,
                            const char * name,
                            const bg_upnp_icon_t * icons,
                            bg_db_t * db);

#endif // __BG_UPNP_DEVICE_H_
