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

#include <gmerlin/utils.h>

xmlDocPtr bg_upnp_device_description_create(bg_socket_address_t * a,
                                            const char * type, int version)
  {
  xmlDocPtr ret;
  xmlNodePtr root;
  xmlNodePtr node;
  xmlNsPtr ns;
  char * tmp_string;
  char addr_string[BG_SOCKET_ADDR_STR_LEN];

  bg_socket_address_to_string(a, addr_string);
  
  ret = xmlNewDoc((xmlChar*)"1.0");

  root = xmlNewDocRawNode(ret, NULL, (xmlChar*)"root", NULL);
  xmlDocSetRootElement(ret, root);

  ns =
    xmlNewNs(root,
             (xmlChar*)"urn:schemas-upnp-org:device-1-0",
             NULL);
  xmlSetNs(root, ns);
  xmlAddChild(root, BG_XML_NEW_TEXT("\n"));
  
  node = bg_xml_append_child_node(root, "specVersion", NULL);
  bg_xml_append_child_node(node, "major", "1");
  bg_xml_append_child_node(node, "minor", "0");
  
  tmp_string = bg_sprintf("http://%s/", addr_string);
  bg_xml_append_child_node(root, "URLBase", tmp_string);
  free(tmp_string);
  
  node = bg_xml_append_child_node(root, "device", NULL);
  
  tmp_string = bg_sprintf("urn:schemas-upnp-org:device:%s:%d", type, version);
  bg_xml_append_child_node(node, "deviceType", tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("http://%s/", addr_string);
  bg_xml_append_child_node(node, "presentationURL", tmp_string);
  free(tmp_string);


  return ret;
  }

static xmlNodePtr get_device_node(xmlDocPtr ptr)
  {
  xmlNodePtr node;

  if(!(node = bg_xml_find_doc_child(ptr, "root")) ||
     !(node = bg_xml_find_node_child(node, "device")))
    return NULL;
  return node;
  }

void bg_upnp_device_description_set_name(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "friendlyName", name);
  }

void bg_upnp_device_description_set_manufacturer(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "manufacturer", name);
  }

void bg_upnp_device_description_set_manufacturer_url(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "manufacturerURL", name);
  }

void bg_upnp_device_description_set_model_description(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "modelDescription", name);
  }

void bg_upnp_device_description_set_model_name(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "modelName", name);
  }

void bg_upnp_device_description_set_model_number(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "modelNumber", name);
  }

void bg_upnp_device_description_set_model_url(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "modelURL", name);
  }

void bg_upnp_device_description_set_serial_number(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "serialNumber", name);
  }

void bg_upnp_device_description_set_uuid(xmlDocPtr ptr, uuid_t uuid)
  {
  char uuid_str[37];
  char * tmp_string;
  xmlNodePtr node = get_device_node(ptr);

  uuid_unparse_lower(uuid, uuid_str);
  tmp_string = bg_sprintf("uuid:%s", uuid_str);
  bg_xml_append_child_node(node, "UDN", tmp_string);
  free(tmp_string);
  }

void bg_upnp_device_description_set_upc(xmlDocPtr ptr, const char * name)
  {
  xmlNodePtr node = get_device_node(ptr);
  bg_xml_append_child_node(node, "UPC", name);
  }

void bg_upnp_device_description_add_service(xmlDocPtr ptr,
                                            const char * type, int version, const char * name)
  {
  char * tmp_string;
  xmlNodePtr servicelist;
  xmlNodePtr service;
  
  xmlNodePtr node = get_device_node(ptr);

  servicelist = bg_xml_find_node_child(node, "serviceList");
  
  if(!servicelist)
    servicelist = bg_xml_append_child_node(node, "serviceList", NULL);
  
  service = bg_xml_append_child_node(servicelist, "service", NULL);

  tmp_string = bg_sprintf("urn:schemas-upnp-org:service:%s:%d", type, version);
  bg_xml_append_child_node(service, "serviceType", tmp_string);
  free(tmp_string);
  
  tmp_string = bg_sprintf("urn:upnp-org:serviceId:%s", type);
  bg_xml_append_child_node(service, "serviceId", tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("/upnp/%s/desc.xml", name);
  bg_xml_append_child_node(service, "SCPDURL", tmp_string);
  free(tmp_string);
  
  tmp_string = bg_sprintf("/upnp/%s/control", name);
  bg_xml_append_child_node(service, "controlURL", tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("/upnp/%s/event", name);
  bg_xml_append_child_node(service, "eventSubURL", tmp_string);
  free(tmp_string);
  
  }

void bg_upnp_device_description_add_icon(xmlDocPtr ptr,
                                         const char * mimetype,
                                         int width, int height, int depth, const char * url)
  {
  char * tmp_string;
  xmlNodePtr iconlist;
  xmlNodePtr icon;
  xmlNodePtr node = get_device_node(ptr);

  iconlist = bg_xml_find_node_child(node, "iconList");
  if(!iconlist)
    iconlist = xmlNewTextChild(node, NULL, (xmlChar*)"iconList", NULL);

  icon = xmlNewTextChild(iconlist, NULL, (xmlChar*)"icon", NULL);
  bg_xml_append_child_node(icon, "mimetype", mimetype);

  tmp_string = bg_sprintf("%d", width);
  bg_xml_append_child_node(icon, "width", tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("%d", height);
  bg_xml_append_child_node(icon, "height", tmp_string);
  free(tmp_string);

  tmp_string = bg_sprintf("%d", depth);
  bg_xml_append_child_node(icon, "depth", tmp_string);
  free(tmp_string);
  
  bg_xml_append_child_node(icon, "url", url);
  }
