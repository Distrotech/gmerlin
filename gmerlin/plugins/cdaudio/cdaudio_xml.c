/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <string.h>
#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>

#include "cdaudio.h"

int bg_cdaudio_load(bg_track_info_t * tracks, const char * filename)
  {
  int index;
  xmlDocPtr xml_doc;
  xmlNodePtr node;

  xml_doc = xmlParseFile(filename);
  if(!xml_doc)
    return 0;

  node = xml_doc->children;

  if(BG_XML_STRCMP(node->name, "CD"))
    {
    xmlFreeDoc(xml_doc);
    return 0;
    }

  node = node->children;

  index = 0;

  while(node)
    {
    if(node->name && !BG_XML_STRCMP(node->name, "TRACK"))
      {
      bg_xml_2_metadata(xml_doc, node,
                        &(tracks[index].metadata));
      index++;
      }
    node = node->next;
    }

  
  return 1;
  }

void bg_cdaudio_save(bg_track_info_t * tracks, int num_tracks,
                     const char * filename)
  {
  xmlDocPtr  xml_doc;
  int i;

  xmlNodePtr xml_cd, child;
    
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_cd = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"CD", NULL);
  xmlDocSetRootElement(xml_doc, xml_cd);
  xmlAddChild(xml_cd, BG_XML_NEW_TEXT("\n"));

  for(i = 0; i < num_tracks; i++)
    {
    child = xmlNewTextChild(xml_cd, (xmlNsPtr)0, (xmlChar*)"TRACK", NULL);
    xmlAddChild(child, BG_XML_NEW_TEXT("\n"));
    bg_metadata_2_xml(child,
                      &(tracks[i].metadata));
    xmlAddChild(xml_cd, BG_XML_NEW_TEXT("\n"));
    }

  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  }
