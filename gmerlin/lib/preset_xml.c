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

#include <gmerlin/cfg_registry.h>
#include <gmerlin/preset.h>
#include <gmerlin/xmlutils.h>

void bg_preset_load(bg_preset_t * p)
  {
  xmlNodePtr node;
  xmlDocPtr xml_doc;
  
  xml_doc = bg_xml_parse_file(p->file);

  if(!xml_doc)
    return;
  
  node = xml_doc->children;
  
  if(BG_XML_STRCMP(node->name, "PRESET"))
    {
    xmlFreeDoc(xml_doc);
    }
  
  p->section = bg_cfg_section_create((char*)0);
  bg_cfg_xml_2_section(xml_doc, node, p->section);
  xmlFreeDoc(xml_doc);
  }

void bg_preset_save(bg_preset_t * p)
  {
  xmlDocPtr  xml_doc;
  xmlNodePtr node;
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  node = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"PRESET", NULL);
  
  xmlDocSetRootElement(xml_doc, node);

  bg_cfg_section_2_xml(p->section, node);
  xmlSaveFile(p->file, xml_doc);
  xmlFreeDoc(xml_doc);
  
  }

