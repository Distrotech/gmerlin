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
#include <upnp/devicepriv.h>
#include <string.h>
#include <gmerlin/http.h>
#include <gmerlin/utils.h>
#include <gmerlin/upnp/devicedesc.h>
#include <sys/utsname.h>
#include <gmerlin/log.h>
#define LOG_DOMAIN "upnpdevice"

void bg_upnp_device_send_description(bg_upnp_device_t * dev, int fd, const char * desc)
  {
  int len;
  gavl_metadata_t res;
  
  len = strlen(desc);
  bg_http_response_init(&res, "HTTP/1.1", 200, "OK");
  gavl_metadata_set_int(&res, "CONTENT-LENGTH", len);
  gavl_metadata_set(&res, "CONTENT-TYPE", "text/xml; charset=UTF-8");
  gavl_metadata_set(&res, "SERVER", dev->server_string);
  gavl_metadata_set(&res, "CONNECTION", "close");

#if 0  
  fprintf(stderr, "Send description\n");
  gavl_metadata_dump(&res, 2);
  fprintf(stderr, "%s\n", desc);
#endif
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
    bg_upnp_device_send_description(dev, fd, dev->description);
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

      return bg_upnp_service_handle_request(&dev->services[i], fd,
                                            method, path, header);
      }
    }
  return 0;
  }

void
bg_upnp_device_destroy(bg_upnp_device_t * dev)
  {
  int i;
  if(dev->ssdp)
    bg_ssdp_destroy(dev->ssdp);

  for(i = 0; i < dev->num_services; i++)
    bg_upnp_service_free(&dev->services[i]);
  if(dev->services)
    free(dev->services);

  if(dev->url_base)
    free(dev->url_base);
  if(dev->type)
    free(dev->type);
  if(dev->name)
    free(dev->name);
  if(dev->model_name)
    free(dev->model_name);
  if(dev->model_description)
    free(dev->model_description);
  if(dev->server_string)
    free(dev->server_string);
  if(dev->timer)
    gavl_timer_destroy(dev->timer);
  if(dev->description)
    free(dev->description);

  if(dev->destroy)
    dev->destroy(dev->priv);

  bg_ssdp_root_device_free(&dev->ssdp_dev);

  free(dev);
  }

void bg_upnp_device_init(bg_upnp_device_t * ret, bg_socket_address_t * addr,
                         uuid_t uuid, const char * name, const char * type, 
                         int version, int num_services,
                         const char * model_name,
                         const char * model_description,
                         const bg_upnp_icon_t * icons)
  {
  char addr_str[BG_SOCKET_ADDR_STR_LEN];
  struct utsname os_info;
  ret->num_services = num_services;
  ret->services = calloc(ret->num_services, sizeof(*ret->services));
  ret->sa = addr;

  ret->url_base = bg_sprintf("http://%s/",
                             bg_socket_address_to_string(ret->sa, addr_str));
  
  uuid_copy(ret->uuid, uuid);
  ret->name = gavl_strdup(name);
  ret->type = gavl_strdup(type);
  ret->version = version;
  ret->icons = icons;

  ret->model_name        = gavl_strdup(model_name);
  ret->model_description = gavl_strdup(model_description);

  uname(&os_info);
  ret->server_string = bg_sprintf("%s/%s, UPnP/1.0, "PACKAGE"/"VERSION,
                                  os_info.sysname, os_info.release);

  bg_log(BG_LOG_INFO, LOG_DOMAIN, "Using server string: %s", ret->server_string);

  ret->timer = gavl_timer_create();
  gavl_timer_start(ret->timer);
  
  }

static void create_description(bg_upnp_device_t * dev)
  {
  int i;
  xmlDocPtr doc;
  doc = bg_upnp_device_description_create(dev->url_base, dev->type, dev->version);
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
  xmlFreeDoc(doc);
  
  //  fprintf(stderr, "Created device description:\n%s\n", dev->description);
  
  }

static void create_ssdp(bg_upnp_device_t * dev)
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
  dev->ssdp = bg_ssdp_create(&dev->ssdp_dev, 0, dev->server_string);
  }

int
bg_upnp_device_create_common(bg_upnp_device_t * dev)
  {
  int i;
  /* Set the device pointers in the servives */
  for(i = 0; i < dev->num_services; i++)
    dev->services[i].dev = dev;
  
  create_ssdp(dev);
  create_description(dev);
  return 1;
  }


int
bg_upnp_device_ping(bg_upnp_device_t * dev)
  {
  return bg_ssdp_update(dev->ssdp);
  }

const char * bg_upnp_device_get_server_string(bg_upnp_device_t * d)
  {
  return d->server_string;
  }
