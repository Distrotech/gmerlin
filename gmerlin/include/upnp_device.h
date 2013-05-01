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

typedef struct bg_upnp_service_s bg_upnp_service_t; 

typedef struct
  {
  int (*handle_soap_request)(bg_upnp_service_t * s, xmlDocPtr request, xmlDocPtr * response);

  void (*handle_event_request)(bg_upnp_service_t * s, int fd,
                               const char * method,
                               const char * path,
                               const gavl_metadata_t * header);
  
  void (*destroy)(void*priv);
  } bg_upnp_service_funcs_t;

struct bg_upnp_service_s
  {
  char * name; // For finding the service from the http request
  char * description;
  int description_length;
    
  const bg_upnp_service_funcs_t * funcs;
  void * priv;
  };

struct bg_upnp_device_s
  {
  char * description;
  int description_length;

  int num_services;
  bg_upnp_service_t * services;
  
  void (*destroy)(void*priv);
  void * priv;
  };

