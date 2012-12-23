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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <gmerlin/parameter.h>
#include <gmerlin/streaminfo.h>
#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>

#include <gavl/metatags.h>

/*
 *  <metatag key="name" val="val" />
 */

static const char * metatag_key = "metatag";
static const char * key_key     = "key";
static const char * value_key   = "value";

void bg_xml_2_metadata(xmlDocPtr xml_doc, xmlNodePtr xml_metadata,
                       gavl_metadata_t * ret)
  {
  char * val;
  char * key;
  xmlNodePtr node;
  
  node = xml_metadata->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    if(!BG_XML_STRCMP(node->name, metatag_key))
      {
      key = BG_XML_GET_PROP(node, key_key);
      val = BG_XML_GET_PROP(node, value_key);
      if(key && val)
        gavl_metadata_set(ret, key, val);
      
      if(key)
        xmlFree(key);
      if(val)
        xmlFree(val);
      }
    node = node->next;
    }
  }

void bg_metadata_2_xml(xmlNodePtr xml_metadata,
                       gavl_metadata_t * m)
  {
  int i;
  xmlNodePtr child;
  for(i = 0; i < m->num_tags; i++)
    {
    child = xmlNewTextChild(xml_metadata, NULL,
                           (xmlChar*)metatag_key, NULL);
    BG_XML_SET_PROP(child, key_key, m->tags[i].key);
    BG_XML_SET_PROP(child, value_key, m->tags[i].val);
    
    xmlAddChild(xml_metadata, BG_XML_NEW_TEXT("\n"));
    }
  }

