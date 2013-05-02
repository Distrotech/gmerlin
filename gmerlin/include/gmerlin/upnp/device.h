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

#include <uuid/uuid.h>
#include <gmerlin/xmlutils.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/mediadb.h>

typedef struct bg_upnp_device_s bg_upnp_device_t;

int
bg_upnp_device_handle_request(bg_upnp_device_t * dev, int fd,
                              const char * method,
                              const char * path,
                              const gavl_metadata_t * header);

void
bg_upnp_device_destroy(bg_upnp_device_t * dev);

/* Creation routines for device types */


bg_upnp_device_t *
bg_upnp_create_media_server(bg_socket_address_t * addr,
                            uuid_t uuid,
                            const char * name,
                            bg_db_t * db);

