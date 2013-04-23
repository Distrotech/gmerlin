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

typedef struct
  {
  char * type;
  int version;
  } bg_ssdp_service_t;

void
bg_ssdp_service_free(bg_ssdp_service_t * s);

typedef struct
  {
  char * type;
  char * uuid;
  int version;

  int num_services;
  bg_ssdp_service_t * services;
  } bg_ssdp_device_t;

void
bg_ssdp_device_free(bg_ssdp_device_t * dev);

typedef struct
  {
  char * uuid;
  char * url;

  char * type;
  int version;
  
  int num_devices;
  bg_ssdp_device_t * devices;

  int num_services;
  bg_ssdp_service_t * services;

  gavl_time_t expire_time;
  } bg_ssdp_root_device_t;

void
bg_ssdp_root_device_free(bg_ssdp_root_device_t*);

void
bg_ssdp_root_device_dump(const bg_ssdp_root_device_t*);

bg_ssdp_device_t *
bg_ssdp_device_add_device(bg_ssdp_root_device_t*, const char * uuid);

typedef struct bg_ssdp_s bg_ssdp_t;

bg_ssdp_t *
bg_ssdp_create(bg_ssdp_root_device_t * local_dev, int discover_remote);

void bg_ssdp_update(bg_ssdp_t *);
void bg_ssdp_destroy(bg_ssdp_t *);

