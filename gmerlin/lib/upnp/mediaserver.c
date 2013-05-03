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

#include <upnp_device.h>
#include <stdlib.h>


bg_upnp_device_t *
bg_upnp_create_media_server(bg_socket_address_t * addr,
                            uuid_t uuid,
                            const char * name,
                            const bg_upnp_icon_t * icons,
                            bg_db_t * db)
  {
  bg_upnp_device_t * ret;
  ret = calloc(1, sizeof(*ret));
  
  bg_upnp_device_init(ret, addr, uuid, name, "MediaServer", 1, 1 /* 2 */,
                      "Gmerlin media server", 
                      "Gmerlin media server",
                      icons);

  bg_upnp_service_init_content_directory(&ret->services[0],
                                         "cd", db);
  
  //  bg_upnp_service_init(&ret->services[1], "cm", "ConnectionManager", 1);
  
  bg_upnp_device_create_common(ret);
  return ret;
  }

