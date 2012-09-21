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

static const struct
  {
  const char * xml_name;
  const char * gavl_name;
  }
tags[] =
  {
    { "track",     GAVL_META_TRACKNUMBER },
    { "artist",    GAVL_META_ARTIST      },
    { "title",     GAVL_META_TITLE       },
    { "album",     GAVL_META_ALBUM       },
    { "date",      GAVL_META_DATE        },
    { "genre",     GAVL_META_GENRE       },
    { "comment",   GAVL_META_COMMENT     },
    { "author",    GAVL_META_AUTHOR      },
    { "copyright", GAVL_META_COPYRIGHT   },
    { "software",  GAVL_META_SOFTWARE    },
    { "format",    GAVL_META_FORMAT      },
    { "creator",   GAVL_META_CREATOR     },
    { "label",     GAVL_META_LABEL       },
    { "bitrate",   GAVL_META_BITRATE     },
    { "device",    GAVL_META_DEVICE      },
    { /* End */ }
  };

void bg_xml_2_metadata(xmlDocPtr xml_doc, xmlNodePtr xml_metadata,
                       gavl_metadata_t * ret)
  {
  int i;
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

    i = 0;
    while(tags[i].xml_name)
      {
      if(!BG_XML_STRCMP(node->name, tags[i].xml_name))
        {
        gavl_metadata_set(ret, tags[i].gavl_name, tmp_string);
        break;
        }
      i++;
      }
    xmlFree(tmp_string);
    node = node->next;
    }
  }

#define STRING_2_XML(key)                                             \
  if(m->key)                                                          \
    {                                                                 \
    child = xmlNewTextChild(xml_metadata, NULL, (xmlChar*)#key, NULL); \
    xmlAddChild(child, BG_XML_NEW_TEXT(m->key));                           \
    xmlAddChild(xml_metadata, BG_XML_NEW_TEXT("\n"));                      \
    }

#define INT_2_XML(key)                         \
  if(m->key)                                   \
    { \
    tmp_string = bg_sprintf("%d", m->key); \
    child = xmlNewTextChild(xml_metadata, NULL, (xmlChar*)#key, NULL); \
    xmlAddChild(child, BG_XML_NEW_TEXT(tmp_string));                       \
    free(tmp_string);                                                 \
    xmlAddChild(xml_metadata, BG_XML_NEW_TEXT("\n"));                      \
    }

void bg_metadata_2_xml(xmlNodePtr xml_metadata,
                       gavl_metadata_t * m)
  {
  int i, j;
  xmlNodePtr child;
  for(i = 0; i < m->num_tags; i++)
    {
    j = 0;
    while(tags[j].gavl_name)
      {
      if(!strcmp(m->tags[i].key, tags[j].gavl_name))
        {
        child = xmlNewTextChild(xml_metadata, NULL,
                                (xmlChar*)tags[j].xml_name, NULL);
        xmlAddChild(child, BG_XML_NEW_TEXT(m->tags[i].val));
        xmlAddChild(xml_metadata, BG_XML_NEW_TEXT("\n"));
        break;
        }
      j++;
      }
    }
  
  }

