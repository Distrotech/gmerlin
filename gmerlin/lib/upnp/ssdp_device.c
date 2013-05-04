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
#include <string.h>
#include <ctype.h>

#include <gavl/gavl.h>
#include <gavl/metadata.h>

#include <gmerlin/upnp/ssdp.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "ssdpdevice"

#include <gmerlin/utils.h>

static int match_type_version(const char * type_req, int ver_req,
                              const char * type_avail, int ver_avail)
  {
  return !strcmp(type_req, type_avail) && (ver_avail >= ver_req);
  }

static int match_type_version_str(const char * req,
                                  const char * type_avail, int ver_avail)
  {
  const char * pos;
  pos = strchr(req, ':');
  if(!pos)
    return 0;

  if((strlen(type_avail) != pos - req) ||
     strncmp(req, type_avail, pos - req))
    return 0;

  pos++;
  if(atoi(pos) > ver_avail)
    return 0;
  return 1;
  }

int
bg_ssdp_has_device(const bg_ssdp_root_device_t * dev, const char * type, int version, int * dev_index)
  {
  int i;
  if(match_type_version(type, version, dev->type, dev->version))
    {
    if(dev_index)
      *dev_index = -1;
    return 1;
    }
  
  for(i = 0; i < dev->num_devices; i++)
    {
    if(match_type_version(type, version, dev->devices[i].type, dev->devices[i].version))
      {
      if(dev_index)
        *dev_index = i;
      return 1;
      }
    }
  return 0;
  }

int
bg_ssdp_has_device_str(const bg_ssdp_root_device_t * dev, const char * type_version, int * dev_index)
  {
  int i;
  if(match_type_version_str(type_version, dev->type, dev->version))
    {
    if(dev_index)
      *dev_index = -1;
    return 1;
    }
  
  for(i = 0; i < dev->num_devices; i++)
    {
    if(match_type_version_str(type_version, dev->devices[i].type, dev->devices[i].version))
      {
      if(dev_index)
        *dev_index = i;
      return 1;
      }
    }
  return 0;
  }

static int lookup_service(bg_ssdp_service_t * s, int num, const char * type, int version)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    if(match_type_version(type, version, s[i].type, s[i].version))
      return i;
    }
  return -1;
  }

static int lookup_service_str(bg_ssdp_service_t * s, int num, const char * type_version)
  {
  int i;
  for(i = 0; i < num; i++)
    {
    if(match_type_version_str(type_version, s[i].type, s[i].version))
      return i;
    }
  return -1;
  }

int
bg_ssdp_has_service(const bg_ssdp_root_device_t * dev, const char * type, int version, int * dev_index, int * srv_index)
  {
  int idx, i;

  idx = lookup_service(dev->services, dev->num_services, type, version);

  if(idx >= 0)
    {
    if(dev_index)
      *dev_index = -1;

    if(srv_index)
      *srv_index = idx;
    return 1;
    }

  for(i = 0; i < dev->num_devices; i++)
    {
    idx = lookup_service(dev->devices[i].services, dev->devices[i].num_services, type, version);

    if(idx >= 0)
      {
      if(dev_index)
        *dev_index = i;
      
      if(srv_index)
        *srv_index = idx;
      return 1;
      }
    }
  return 0;
  }

int
bg_ssdp_has_service_str(const bg_ssdp_root_device_t * dev, const char * type_version, int * dev_index, int * srv_index)
  {
  int idx, i;

  idx = lookup_service_str(dev->services, dev->num_services, type_version);

  if(idx >= 0)
    {
    if(dev_index)
      *dev_index = -1;

    if(srv_index)
      *srv_index = idx;
    return 1;
    }

  for(i = 0; i < dev->num_devices; i++)
    {
    idx = lookup_service_str(dev->devices[i].services, dev->devices[i].num_services, type_version);

    if(idx >= 0)
      {
      if(dev_index)
        *dev_index = i;
      
      if(srv_index)
        *srv_index = idx;
      return 1;
      }
    }
  return 0;
  }

void
bg_ssdp_root_device_dump(const bg_ssdp_root_device_t * d)
  {
  int i, j;
  gavl_dprintf("Root device\n");
  gavl_diprintf(2, "Location: %s\n", d->url);
  gavl_diprintf(2, "UUID: %s\n", d->uuid);
  if(d->type)
    gavl_diprintf(2, "Type: %s.%d\n", d->type, d->version);
  gavl_diprintf(2, "Devices: %d\n", d->num_devices);
  for(i = 0; i < d->num_devices; i++)
    {
    gavl_diprintf(4, "UUID: %s\n", d->devices[i].uuid);
    gavl_diprintf(4, "Type: %s.%d\n", d->devices[i].type, d->devices[i].version);
    gavl_diprintf(4, "Services: %d\n", d->devices[i].num_services);
    for(j = 0; j < d->devices[i].num_services; j++)
      {
      gavl_diprintf(6, "Type: %s.%d\n", d->devices[i].services[j].type, d->devices[i].services[j].version);
      }
    }
  gavl_diprintf(2, "Services: %d\n", d->num_services);
  for(j = 0; j < d->num_services; j++)
    {
    gavl_diprintf(4, "Type: %s.%d\n", d->services[j].type,
                  d->services[j].version);
    }

  }

void
bg_ssdp_service_free(bg_ssdp_service_t * s)
  {
  if(s->type)
    free(s->type);
  }

void
bg_ssdp_device_free(bg_ssdp_device_t * dev)
  {
  int i;
  for(i = 0; i < dev->num_services; i++)
    bg_ssdp_service_free(&dev->services[i]);
  if(dev->services)
    free(dev->services);
  if(dev->type)
    free(dev->type);
  if(dev->uuid)
    free(dev->uuid);

  }

bg_ssdp_device_t *
bg_ssdp_device_add_device(bg_ssdp_root_device_t * dev, const char * uuid)
  {
  bg_ssdp_device_t * ret;
  dev->devices = realloc(dev->devices, (dev->num_devices+1)*sizeof(*dev->devices));
  ret = dev->devices + dev->num_devices;
  memset(ret, 0, sizeof(*ret));
  ret->uuid = gavl_strdup(uuid);
  return ret;
  }

void
bg_ssdp_root_device_free(bg_ssdp_root_device_t * dev)
  {
  int i;
  for(i = 0; i < dev->num_services; i++)
    bg_ssdp_service_free(&dev->services[i]);
  if(dev->services)
    free(dev->services);

  for(i = 0; i < dev->num_devices; i++)
    bg_ssdp_device_free(&dev->devices[i]);
  if(dev->devices)
    free(dev->devices);

  if(dev->uuid)
    free(dev->uuid);
  if(dev->url)
    free(dev->url);
  if(dev->type)
    free(dev->type);
  }

/* Get USN / NT */

char * bg_ssdp_get_service_type_usn(const bg_ssdp_root_device_t * d, int dev, int serv)
  {
  if(dev < 0)
    {
    return bg_sprintf("uuid:%s::urn:schemas-upnp-org:service:%s:%d",
                      d->uuid,
                      d->services[serv].type,
                      d->services[serv].version);
    }
  else
    {
    return bg_sprintf("uuid:%s::urn:schemas-upnp-org:device:%s:%d",
                      d->devices[dev].uuid,
                      d->devices[dev].services[serv].type,
                      d->devices[dev].services[serv].version);
    }
  }

char * bg_ssdp_get_device_type_usn(const bg_ssdp_root_device_t * d, int dev)
  {
  if(dev < 0)
    {
    return bg_sprintf("uuid:%s::urn:schemas-upnp-org:device:%s:%d",
                      d->uuid,
                      d->type,
                      d->version);
    }
  else
    {
    return bg_sprintf("uuid:%s::urn:schemas-upnp-org:device:%s:%d",
                      d->devices[dev].uuid,
                      d->devices[dev].type,
                      d->devices[dev].version);
    }
  }


char * bg_ssdp_get_device_uuid_usn(const bg_ssdp_root_device_t * d, int dev)
  {
  if(dev < 0)
    return bg_sprintf("uuid:%s", d->uuid);
  else
    return bg_sprintf("uuid:%s", d->devices[dev].uuid);
  }

char * bg_ssdp_get_root_usn(const bg_ssdp_root_device_t * d)
  {
  return bg_sprintf("uuid:%s::upnp:rootdevice",
                    d->uuid);
  }

char * bg_ssdp_get_service_type_nt(const bg_ssdp_root_device_t * d, int dev, int serv)
  {
  if(dev < 0)
    return bg_sprintf("urn:schemas-upnp-org:service:%s:%d",
                     d->services[serv].type,
                     d->services[serv].version);
  else
    return bg_sprintf("urn:schemas-upnp-org:service:%s:%d",
                     d->devices[dev].services[serv].type,
                     d->devices[dev].services[serv].version);
  }

char * bg_ssdp_get_device_type_nt(const bg_ssdp_root_device_t * d, int dev)
  {
  if(dev < 0)
    return bg_sprintf("urn:schemas-upnp-org:device:%s:%d",
                     d->type, d->version);
  else
    return bg_sprintf("urn:schemas-upnp-org:device:%s:%d",
                     d->devices[dev].type,
                     d->devices[dev].version);
  }

char * bg_ssdp_get_device_uuid_nt(const bg_ssdp_root_device_t * d, int dev)
  {
  if(dev < 0)
    return bg_sprintf("uuid:%s", d->uuid);
  else
    return bg_sprintf("uuid:%s", d->devices[dev].uuid);
  }

char * bg_ssdp_get_root_nt(const bg_ssdp_root_device_t * d)
  {
  return gavl_strdup("upnp:rootdevice");
  }
