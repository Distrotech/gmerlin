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
#include <gmerlin/upnp/servicedesc.h>

#include <config.h>

static void test_device_desc()
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
  }

#if 0
static void test_service_desc()
  {
  xmlNodePtr node;
  xmlDocPtr doc = bg_upnp_service_description_create();

  node = bg_upnp_service_description_add_action(doc, "Browse");
  bg_upnp_service_action_add_argument(node, "ObjectID", 0, 0, "A_ARG_TYPE_ObjectID");
  bg_upnp_service_action_add_argument(node, "BrowseFlag", 0, 0, "A_ARG_TYPE_BrowseFlag");
  bg_upnp_service_action_add_argument(node, "Filter", 0, 0, "A_ARG_TYPE_Filter");
  bg_upnp_service_action_add_argument(node, "StartingIndex", 0, 0, "A_ARG_TYPE_Index");
  bg_upnp_service_action_add_argument(node, "RequestedCount", 0, 0, "A_ARG_TYPE_Count");
  bg_upnp_service_action_add_argument(node, "SortCriteria", 0, 0, "A_ARG_TYPE_SortCriteria");
  bg_upnp_service_action_add_argument(node, "Result", 1, 0, "A_ARG_TYPE_Result");
  bg_upnp_service_action_add_argument(node, "NumberReturned", 1, 0, "A_ARG_TYPE_Count");
  bg_upnp_service_action_add_argument(node, "TotalMatches", 1, 0, "A_ARG_TYPE_Count");
  bg_upnp_service_action_add_argument(node, "UpdateID", 1, 0, "A_ARG_TYPE_UpdateID");

  node = bg_upnp_service_description_add_statevar(doc, "A_ARG_TYPE_BrowseFlag", 0, "string");
  bg_upnp_service_statevar_add_allowed_value(node, "BrowseMetadata");
  bg_upnp_service_statevar_add_allowed_value(node, "BrowseDirectChildren");
  
  bg_xml_save_FILE(doc, stdout);
  xmlFreeDoc(doc);
  
  }
#endif

int main(int argc, char ** argv)
  {
  test_device_desc();
  
  //  test_service_desc();
  return 0; 
  }

