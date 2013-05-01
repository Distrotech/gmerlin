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
#include <gmerlin/upnp/devicedesc.h>

#include <config.h>

int main(int argc, char ** argv)
  {
  uuid_t uuid;
  bg_socket_address_t * addr = bg_socket_address_create();
  xmlDocPtr doc;

  bg_socket_address_set(addr, "127.0.0.1", 5555, SOCK_STREAM);

  doc = bg_upnp_device_description_create(addr, "MediaServer", 1);
  uuid_clear(uuid);
  uuid_generate(uuid);
  
  bg_upnp_device_description_set_name(doc, "Gmerlin Server");
  bg_upnp_device_description_set_manufacturer(doc, "Gmerlin Project");
  bg_upnp_device_description_set_manufacturer_url(doc, "http://gmerlin.sourceforge.net");
  bg_upnp_device_description_set_model_description(doc, "Amazing media server");
  bg_upnp_device_description_set_model_name(doc, "Gmerlin Mediaserver");
  bg_upnp_device_description_set_model_number(doc, VERSION);
  bg_upnp_device_description_set_model_url(doc, "http://gmerlin.sourceforge.net");
  bg_upnp_device_description_set_serial_number(doc, "1");
  bg_upnp_device_description_set_uuid(doc, uuid);

  bg_upnp_device_description_add_icon(doc, "image/png", 48, 48, 24, "/static/icons/server.png");
    
  bg_upnp_device_description_add_service(doc, "ContentDirectory", 1, "cd");
  bg_upnp_device_description_add_service(doc, "ConnectionManager", 1, "cm");


  
  // bg_upnp_device_description_set_upc(doc, "UPC");
  
  bg_xml_save_FILE(doc, stdout);
  xmlFreeDoc(doc);
  bg_socket_address_destroy(addr);
  return 0; 
  }

