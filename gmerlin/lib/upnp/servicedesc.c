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

#include <gmerlin/upnp/servicedesc.h>

xmlDocPtr bg_upnp_service_description_create()
  {
  xmlDocPtr ret;
  xmlNodePtr root;
  xmlNsPtr ns;
  xmlNodePtr node;
  
  
  ret = xmlNewDoc((xmlChar*)"1.0");
  root = xmlNewDocRawNode(ret, NULL, (xmlChar*)"scpd", NULL);
  xmlDocSetRootElement(ret, root);

  ns =
    xmlNewNs(root,
             (xmlChar*)"urn:schemas-upnp-org:service-1-0",
             NULL);
  xmlSetNs(root, ns);

  node = xmlNewTextChild(root, NULL, (xmlChar*)"specVersion", NULL);
  bg_xml_append_child_node(node, "major", "1");
  bg_xml_append_child_node(node, "minor", "0");

  node = xmlNewTextChild(root, NULL, (xmlChar*)"actionList", NULL);
  node = xmlNewTextChild(root, NULL, (xmlChar*)"serviceStateTable", NULL);
  return ret;
  }

xmlNodePtr bg_upnp_service_description_add_action(xmlDocPtr doc, const char * name)
  {
  xmlNodePtr node;

  if(!(node = bg_xml_find_doc_child(doc, "scpd")) ||
     !(node = bg_xml_find_node_child(node, "actionList")))
    return NULL;

  node = xmlNewTextChild(node, NULL, (xmlChar*)"action", NULL);
  bg_xml_append_child_node(node, "name", name);
  xmlNewTextChild(node, NULL, (xmlChar*)"argumentList", NULL);
  return node;
  }

void bg_upnp_service_action_add_argument(xmlNodePtr node,
                                         const char * name, int out, int retval,
                                         const char * related_statevar)
  {
  if(!(node = bg_xml_find_node_child(node, "argumentList")))
    return;

  node = xmlNewTextChild(node, NULL, (xmlChar*)"argument", NULL);
  bg_xml_append_child_node(node, "name", name);

  bg_xml_append_child_node(node, "direction", (out ? "out" : "in"));

  if(retval)
    xmlNewTextChild(node, NULL, (xmlChar*)"retval", NULL);

  bg_xml_append_child_node(node, "relatedStateVariable", related_statevar);
  }

xmlNodePtr
bg_upnp_service_description_add_statevar(xmlDocPtr doc,
                                         const char * name,
                                         int events,
                                         char * data_type)
  {
  xmlNodePtr node;
  
  if(!(node = bg_xml_find_doc_child(doc, "scpd")) ||
     !(node = bg_xml_find_node_child(node, "serviceStateTable")))
    return NULL;

  node = xmlNewTextChild(node, NULL, (xmlChar*)"StateVariable", NULL);

  BG_XML_SET_PROP(node, "sendEvents", (events ? "yes" : "no"));

  bg_xml_append_child_node(node, "name", name);
  bg_xml_append_child_node(node, "dataType", data_type);
  return node;
  }

void
bg_upnp_service_statevar_add_allowed_value(xmlNodePtr node,
                                           const char * name)
  {
  xmlNodePtr allowedlist = bg_xml_find_node_child(node, "allowedValueList");
  
  if(!allowedlist)
    allowedlist = xmlNewTextChild(node, NULL, (xmlChar*)"allowedValueList", NULL);
  bg_xml_append_child_node(allowedlist, "allowedValue", name);
  }
