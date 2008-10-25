/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#define XML_2_INT(key)                          \
if(!BG_XML_STRCMP(node->name, #key))                 \
  {                                             \
  ret->key = atoi(tmp_string);                  \
  xmlFree(tmp_string);                          \
  node = node->next;                            \
  continue;                                     \
  }

#define XML_2_STRING(key)                     \
if(!BG_XML_STRCMP(node->name, #key))               \
  {                                           \
  ret->key = bg_strdup(ret->key, tmp_string); \
  xmlFree(tmp_string);                        \
  node = node->next;                          \
  continue;                                   \
  }

#if 0
      
  int track;
  char * artist;
  char * title;
  char * album;
  char * date;
  char * genre;
  char * comment;

  char * author;
  char * copyright;
#endif

void bg_xml_2_metadata(xmlDocPtr xml_doc, xmlNodePtr xml_metadata,
                       bg_metadata_t * ret)
  {
  char * tmp_string;
  xmlNodePtr node;
  
  node = xml_metadata->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    tmp_string = (char*)xmlNodeListGetString(xml_doc, node->children, 1);

    XML_2_INT(track);
        
    XML_2_STRING(artist);
    XML_2_STRING(title);
    XML_2_STRING(album);
    XML_2_STRING(date);
    XML_2_STRING(genre);
    XML_2_STRING(comment);
    
    XML_2_STRING(author);
    XML_2_STRING(copyright);
    
    xmlFree(tmp_string);
    node = node->next;
    }
  }

#define STRING_2_XML(key)                                             \
  if(m->key)                                                          \
    {                                                                 \
    child = xmlNewTextChild(xml_metadata, (xmlNsPtr)0, (xmlChar*)#key, NULL); \
    xmlAddChild(child, BG_XML_NEW_TEXT(m->key));                           \
    xmlAddChild(xml_metadata, BG_XML_NEW_TEXT("\n"));                      \
    }

#define INT_2_XML(key)                         \
  if(m->key)                                   \
    { \
    tmp_string = bg_sprintf("%d", m->key); \
    child = xmlNewTextChild(xml_metadata, (xmlNsPtr)0, (xmlChar*)#key, NULL); \
    xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));                       \
    free(tmp_string);                                                 \
    xmlAddChild(xml_metadata, BG_XML_NEW_TEXT("\n"));                      \
    }

void bg_metadata_2_xml(xmlNodePtr xml_metadata,
                       bg_metadata_t * m)
  {
  char * tmp_string;
  xmlNodePtr child;

  INT_2_XML(track);
  
  STRING_2_XML(artist);
  STRING_2_XML(title);
  STRING_2_XML(album);
  STRING_2_XML(date);
  STRING_2_XML(genre);
  STRING_2_XML(comment);
  
  STRING_2_XML(author);
  STRING_2_XML(copyright);
  
  }

