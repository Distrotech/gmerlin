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
#include <stdlib.h>
#include <gavl/gavl.h>
#include <gavl/metadata.h>

#include <gmerlin/upnp/ssdp.h>
#include <gmerlin/bgsocket.h>
#include <gmerlin/translation.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "ssdp"



#if 0
typedef struct bg_ssdp_service_s
  {
  gavl_metadata_t m;
  } bg_ssdp_service_t;

typedef struct bg_ssdp_device_s
  {
  gavl_metadata_t m;

  int num_devices;
  bg_ssdp_device_t * devices;
  
  int num_services;
  bg_ssdp_service_t * services;
  } bg_ssdp_device_t;
#endif

struct bg_ssdp_s
  {
  int mcast_fd;
  int ucast_fd;
  gavl_timer_t * timer;
  };

bg_ssdp_t * bg_ssdp_create()
  {
  bg_ssdp_t * ret = calloc(1, sizeof(*ret));
 
  

  return ret;
  }

void bg_ssdp_update(bg_ssdp_t * s)
  {

  }

void bg_ssdp_destroy(bg_ssdp_t * s)
  {

  }

