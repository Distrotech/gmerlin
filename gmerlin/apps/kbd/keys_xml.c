/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

#include <inttypes.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <gmerlin/utils.h>

#include <gmerlin/xmlutils.h>

#include "kbd.h"

#define scancode_name  "code"
#define modifiers_name "modifiers"
#define command_name   "command"
#define key_name       "key"
#define root_name      "kbd_keys"

kbd_table_t * kbd_table_load(const char * filename, int * len)
  {
  char * tmp_string;
  
  kbd_table_t * ret = NULL;
  xmlDocPtr xml_doc;
  xmlNodePtr node;
  xmlNodePtr child;
  
  *len = 0;
  
  xml_doc = xmlParseFile(filename);
  if(!xml_doc)
    return ret;
  node = xml_doc->children;

  if(BG_XML_STRCMP(node->name, "kbd_keys"))
    {
    xmlFreeDoc(xml_doc);
    return ret;
    }
  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    else if(!BG_XML_STRCMP(node->name, key_name))
      {
      ret = realloc(ret, (*len +1) * sizeof(*ret));
      memset(ret + (*len), 0, sizeof(*ret));
      
      child = node->children;

      while(child)
        {
        if(!child->name)
          {
          child = child->next;
          continue;
          }

        tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);
        
        if(!BG_XML_STRCMP(child->name, scancode_name))
          {
          ret[*len].scancode = atoi(tmp_string);
          }
        else if(!BG_XML_STRCMP(child->name, modifiers_name))
          {
          ret[*len].modifiers =
            bg_strdup(ret[*len].modifiers, tmp_string);
          }
        else if(!BG_XML_STRCMP(child->name, command_name))
          {
          ret[*len].command =
            bg_strdup(ret[*len].command, tmp_string);
          }
        xmlFree(tmp_string);
        child = child->next;
        }
      (*len)++;
      }
    node = node->next;
    }
  
  return ret;
  }

void kbd_table_save(const char * filename, kbd_table_t * keys, int len)
  {
  char * tmp_string;
  xmlDocPtr  xml_doc;
  xmlNodePtr node;
  xmlNodePtr xml_child;
  
  xmlNodePtr xml_key;
  int i;

  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  node = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)root_name, NULL);
  xmlDocSetRootElement(xml_doc, node);
  
  xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
  
  for(i = 0; i < len; i++)
    {
    xml_key = xmlNewTextChild(node, NULL, (xmlChar*)key_name, NULL);
    
    xml_child =
      xmlNewTextChild(xml_key, NULL, (xmlChar*)scancode_name, NULL);
    tmp_string = bg_sprintf("%d", keys[i].scancode);
    xmlAddChild(xml_child, BG_XML_NEW_TEXT(tmp_string));
    free(tmp_string);
    
    if(keys[i].modifiers)
      {
      xml_child =
        xmlNewTextChild(xml_key, NULL, (xmlChar*)modifiers_name, NULL);
      xmlAddChild(xml_child, BG_XML_NEW_TEXT(keys[i].modifiers));
      }
    if(keys[i].command)
      {
      xml_child =
        xmlNewTextChild(xml_key, NULL, (xmlChar*)command_name, NULL);
      xmlAddChild(xml_child, BG_XML_NEW_TEXT(keys[i].command));
      }
    xmlAddChild(node, BG_XML_NEW_TEXT("\n"));
    }
  
  xmlSaveFile(filename, xml_doc);
  
  xmlFreeDoc(xml_doc);
  }
