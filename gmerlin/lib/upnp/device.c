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
#include <upnp_device.h>
#include <string.h>
#include <gmerlin/http.h>
#include <gmerlin/utils.h>
#include <gmerlin/upnp/devicedesc.h>


static void send_description(int fd, const char * desc)
  {
  int len;
  gavl_metadata_t res;
  fprintf(stderr, "Send desc\n%s\n", desc);
  
  len = strlen(desc);
  bg_http_response_init(&res, "HTTP/1.1", 200, "OK");
  gavl_metadata_set_int(&res, "CONTENT-LENGTH", len);
  gavl_metadata_set(&res, "CONTENT-TYPE", "text/xml; charset=UTF-8");
  gavl_metadata_set(&res, "CONNECTION", "close");

  if(!bg_http_response_write(fd, &res))
    goto fail;
  
  bg_socket_write_data(fd, (const uint8_t*)desc, len);
  fail:
  gavl_metadata_free(&res);
  }

int
bg_upnp_device_handle_request(bg_upnp_device_t * dev, int fd,
                              const char * method,
                              const char * path,
                              const gavl_metadata_t * header)
  {
  int i;
  const char * pos;
  
  if(strncmp(path, "/upnp/", 6))
    return 0;

  path += 6;

  /* Check for description */

  if(!strcmp(path, "desc.xml"))
    {
    send_description(fd, dev->description);
    return 1;
    }

  pos = strchr(path, '/');
  if(!pos)
    return 0;
  
  for(i = 0; i < dev->num_services; i++)
    {
    if((strlen(dev->services[i].name) == (pos - path)) &&
       (!strncmp(dev->services[i].name, path, pos - path)))
      {
      /* Found service */
      path = pos + 1;

      if(!strcmp(path, "desc.xml"))
        {
        /* Send service description */
        send_description(fd, dev->services[i].description);
        return 1;
        }
      else if(!strcmp(path, "control"))
        {
        /* Service control */
        }
      else if(!strcmp(path, "event"))
        {
        /* Service events */
        }
      else
        return 0; // 404
      }
    }
  return 0;
  }

void
bg_upnp_device_destroy(bg_upnp_device_t * dev)
  {
  }

void bg_upnp_device_init(bg_upnp_device_t * ret, bg_socket_address_t * addr,
                         uuid_t uuid, const char * name, const char * type, 
                         int version, int num_services,
                         const char * model_name,
                         const char * model_description,
                         const bg_upnp_icon_t * icons)
  {
  ret->num_services = num_services;
  ret->services = calloc(ret->num_services, sizeof(*ret->services));
  ret->sa = addr;
  uuid_copy(ret->uuid, uuid);
  ret->name = gavl_strdup(name);
  ret->type = gavl_strdup(type);
  ret->version = version;
  ret->icons = icons;

  ret->model_name        = gavl_strdup(model_name);
  ret->model_description = gavl_strdup(model_description);
  }

void bg_upnp_device_create_description(bg_upnp_device_t * dev)
  {
  int i;
  xmlDocPtr doc;
  doc = bg_upnp_device_description_create(dev->sa, dev->type, dev->version);
  bg_upnp_device_description_set_name(doc, dev->name);
  bg_upnp_device_description_set_manufacturer(doc, "Gmerlin Project");
  bg_upnp_device_description_set_manufacturer_url(doc, "http://gmerlin.sourceforge.net");
  bg_upnp_device_description_set_model_description(doc, dev->model_description);
  bg_upnp_device_description_set_model_name(doc, dev->model_name);
  bg_upnp_device_description_set_model_number(doc, VERSION);
  bg_upnp_device_description_set_model_url(doc, "http://gmerlin.sourceforge.net");
  bg_upnp_device_description_set_serial_number(doc, "1");
  bg_upnp_device_description_set_uuid(doc, dev->uuid);

  if(dev->icons)
    {
    i = 0;
    while(dev->icons[i].location)
      {
      bg_upnp_device_description_add_icon(doc, 
                                          dev->icons[i].mimetype, 
                                          dev->icons[i].width, dev->icons[i].height, 
                                          dev->icons[i].depth, dev->icons[i].location);
      i++;
      }
    }

  for(i = 0; i < dev->num_services; i++)
    {
    bg_upnp_device_description_add_service(doc, dev->services[i].type, 
                                                dev->services[i].version, 
                                                dev->services[i].name);
    }
  dev->description = bg_xml_save_to_memory(doc);
  
  }

void bg_upnp_device_create_ssdp(bg_upnp_device_t * dev)
  {
  int i;
  char addr_string[BG_SOCKET_ADDR_STR_LEN];
  /* Create ssdp device description */
  dev->ssdp_dev.uuid = calloc(37, 1);
  uuid_unparse_lower(dev->uuid, dev->ssdp_dev.uuid);
   
  bg_socket_address_to_string(dev->sa, addr_string);
  dev->ssdp_dev.url = bg_sprintf("http://%s/upnp/desc.xml", addr_string);
  dev->ssdp_dev.type = gavl_strdup(dev->type);
  dev->ssdp_dev.version = dev->version;
  
  dev->ssdp_dev.num_services = dev->num_services;
  dev->ssdp_dev.services = calloc(dev->ssdp_dev.num_services, sizeof(*dev->ssdp_dev.services));
  
  for(i = 0; i < dev->ssdp_dev.num_services; i++)
    {
    dev->ssdp_dev.services[i].type = gavl_strdup(dev->services[i].type);
    dev->ssdp_dev.services[i].version = dev->services[i].version;
    }
  dev->ssdp = bg_ssdp_create(&dev->ssdp_dev, 0);
  }

int
bg_upnp_device_create_common(bg_upnp_device_t * dev)
  {
  bg_upnp_device_create_ssdp(dev);
  bg_upnp_device_create_description(dev);
  return 1;
  }


int
bg_upnp_device_ping(bg_upnp_device_t * dev)
  {
  return bg_ssdp_update(dev->ssdp);
  }
