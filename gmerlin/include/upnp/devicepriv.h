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

#include <gmerlin/upnp/device.h>
#include <gmerlin/upnp/ssdp.h>

#include <upnp/servicepriv.h>

struct bg_upnp_device_s
  {
  char * description;
  char * type;
  int version;

  int num_services;
  bg_upnp_service_t * services;
  
  void (*destroy)(void*priv);
  void * priv;

  bg_ssdp_root_device_t ssdp_dev;
  bg_ssdp_t * ssdp;

  bg_socket_address_t * sa;
  uuid_t uuid;
  char * name;

  char * model_name;
  char * model_description;
  const bg_upnp_icon_t * icons;

  char * server_string;

  gavl_timer_t * timer;
  
  };

void bg_upnp_device_init(bg_upnp_device_t * dev,
                         bg_socket_address_t * addr,
                         uuid_t uuid, const char * name,
                         const char * type, int version, int num_services,
                         const char * model_name,
                         const char * model_description,
                         const bg_upnp_icon_t * icons);

int
bg_upnp_device_create_common(bg_upnp_device_t * dev);

/* Send a description */
void bg_upnp_device_send_description(bg_upnp_device_t * dev,
                                     int fd, const char * desc);
