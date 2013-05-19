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
#include <upnp/mediaserver.h>
#include <string.h>
#include <ctype.h>

#include <gmerlin/utils.h>
#include <upnp/didl.h>

xmlDocPtr bg_didl_create()
  {
  
  xmlNsPtr upnp_ns;
  xmlNsPtr didl_ns;
  xmlNsPtr dc_ns;
  
  xmlDocPtr doc;  
  xmlNodePtr didl;
  
  doc = xmlNewDoc((xmlChar*)"1.0");
  didl = xmlNewDocRawNode(doc, NULL, (xmlChar*)"DIDL-Lite", NULL);
  xmlDocSetRootElement(doc, didl);

  dc_ns =
    xmlNewNs(didl,
             (xmlChar*)"http://purl.org/dc/elements/1.1/",
             (xmlChar*)"dc");

  upnp_ns =
    xmlNewNs(didl,
             (xmlChar*)"urn:schemas-upnp-org:metadata-1-0/upnp/",
             (xmlChar*)"upnp");
  didl_ns =
    xmlNewNs(didl,
             (xmlChar*)"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/",
             NULL);
  
  return doc;
  }

xmlNodePtr bg_didl_add_item(xmlDocPtr doc)
  {
  xmlNodePtr parent = bg_xml_find_next_doc_child(doc, NULL);
  xmlNodePtr node;
  node = xmlNewTextChild(parent, NULL, (xmlChar*)"item", NULL);
  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
  xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
  return node;
  }

xmlNodePtr bg_didl_add_container(xmlDocPtr doc)
  {
  xmlNodePtr parent = bg_xml_find_next_doc_child(doc, NULL);
  xmlNodePtr node = xmlNewTextChild(parent, NULL, (xmlChar*)"container", NULL);
  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
  xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
  return node;
  }

xmlNodePtr bg_didl_add_element(xmlDocPtr doc,
                                   xmlNodePtr node,
                                   const char * name,
                                   const char * value)
  {
  xmlNodePtr ret;
  char * pos;
  char buf[128];
  strncpy(buf, name, 127);
  buf[127] = '\0';

  pos = strchr(buf, ':');
  if(pos)
    {
    xmlNsPtr ns;
    *pos = '\0';
    pos++;
    ns = xmlSearchNs(doc, node, (const xmlChar *)buf);
    ret= xmlNewTextChild(node, ns, (const xmlChar*)pos,
                           (const xmlChar*)value);
    }
  else  
    ret= xmlNewTextChild(node, NULL, (const xmlChar*)name,
                           (const xmlChar*)value);
  xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
  return ret;
  }

void bg_didl_set_class(xmlDocPtr doc,
                           xmlNodePtr node,
                           const char * klass)
  {
  bg_didl_add_element(doc, node, "upnp:class", klass);
  }

void bg_didl_set_title(xmlDocPtr doc,
                           xmlNodePtr node,
                           const char * title)
  {
  bg_didl_add_element(doc, node, "dc:title", title);
  }

int bg_didl_filter_element(const char * name, char ** filter)
  {
  int i = 0;
  const char * pos;
  if(!filter)
    return 1;

  while(filter[i])
    {
    if(!strcmp(filter[i], name))
      return 1;

    // res@size implies res
    if((pos = strchr(filter[i], '@')) &&
       (pos - filter[i] == strlen(name)) &&
       !strncmp(filter[i], name, pos - filter[i]))
      return 1;
     i++;
    }
  return 0;
  }

int bg_didl_filter_attribute(const char * element, const char * attribute, char ** filter)
  {
  int i = 0;
  int allow_empty = 0;
  int len = strlen(element);
  
  if(!filter)
    return 1;

  if(!strcmp(element, "item") ||
     !strcmp(element, "container"))
    allow_empty = 1;
  
  while(filter[i])
    {
    if(!strncmp(filter[i], element, len) &&
       (*(filter[i] + len) == '@') &&
       !strcmp(filter[i] + len + 1, attribute))
      return 1;

    if(allow_empty &&
       (*(filter[i]) == '@') &&
       !strcmp(filter[i] + 1, attribute))
      return 1;
      
    i++;
    }
  return 0;
  }

xmlNodePtr bg_didl_add_element_string(xmlDocPtr doc,
                                    xmlNodePtr node,
                                    const char * name,
                                    const char * content, char ** filter)
  {
  if(!bg_didl_filter_element(name, filter))
    return NULL;
  return bg_didl_add_element(doc, node, name, content);
  }

xmlNodePtr bg_didl_add_element_int(xmlDocPtr doc,
                                       xmlNodePtr node,
                                       const char * name,
                                       int64_t content, char ** filter)
  {
  char buf[128];
  if(!bg_didl_filter_element(name, filter))
    return NULL;
  snprintf(buf, 127, "%"PRId64, content);
  return bg_didl_add_element(doc, node, name, buf);
  }

/* Filtering must be done by the caller!! */
void bg_didl_set_attribute_int(xmlNodePtr node, const char * name, int64_t val)
  {
  char buf[128];
  snprintf(buf, 127, "%"PRId64, val);
  BG_XML_SET_PROP(node, name, buf);
  }

void bg_didl_set_date(xmlDocPtr didl, xmlNodePtr node,
                             const bg_db_date_t * date_c, char ** filter)
  {
  bg_db_date_t date;
  char date_string[BG_DB_DATE_STRING_LEN];
  
  if(date_c->year == 9999)
    return;
  
  if(!bg_didl_filter_element("dc:date", filter))
    return;
  
  memcpy(&date, date_c, sizeof(date));
  if(!date.day)
    date.day = 1;
  if(!date.month)
    date.month = 1;

  bg_db_date_to_string(&date, date_string);
  
  bg_didl_add_element(didl, node, "dc:date", date_string);
  }
