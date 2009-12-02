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
// #include <locale.h>

#include <gmerlin/cfg_registry.h>
#include <registry_priv.h>
#include <gmerlin/utils.h>
#include <gmerlin/xmlutils.h>

static void load_item(xmlDocPtr xml_doc, xmlNodePtr xml_item,
                      bg_cfg_section_t * cfg_section)
  {
  char * tmp_type;
  char * tmp_string;
  char * start;
  
  bg_parameter_info_t info;

  bg_cfg_item_t * item;

  memset(&info, 0, sizeof(info));
  
  tmp_type = BG_XML_GET_PROP(xml_item, "type");
  info.name = BG_XML_GET_PROP(xml_item, "name");
  
  if(!tmp_type || !info.name)
    {
    if(tmp_type)
      xmlFree(tmp_type);
    if(info.name)
      xmlFree(info.name);
    return;
    }

  /* Check for the type */
  
  if(!strcmp(tmp_type, "int"))
    {
    info.type = BG_PARAMETER_INT;
    }
  else if(!strcmp(tmp_type, "float"))
    {
    info.type = BG_PARAMETER_FLOAT;
    }
  else if(!strcmp(tmp_type, "string"))
    {
    info.type = BG_PARAMETER_STRING;
    }
  else if(!strcmp(tmp_type, "string_hidden"))
    {
    info.type = BG_PARAMETER_STRING_HIDDEN;
    }
  else if(!strcmp(tmp_type, "color"))
    {
    info.type = BG_PARAMETER_COLOR_RGBA;
    }
  else if(!strcmp(tmp_type, "time"))
    {
    info.type = BG_PARAMETER_TIME;
    }
  else if(!strcmp(tmp_type, "pos"))
    {
    info.type = BG_PARAMETER_POSITION;
    }
  else
    {
    return;
    }

  /* Find the item */

  item = bg_cfg_section_find_item(cfg_section, &info);
  
  tmp_string = (char*)xmlNodeListGetString(xml_doc, xml_item->children, 1);
  
  switch(item->type)
    {
    case BG_CFG_INT:
      sscanf(tmp_string, "%d", &(item->value.val_i));
      break;
    case BG_CFG_TIME:
      sscanf(tmp_string, "%" PRId64, &(item->value.val_time));
      break;
    case BG_CFG_FLOAT:
      sscanf(tmp_string, "%lf", &(item->value.val_f));
      break;
    case BG_CFG_STRING:
      item->value.val_str = bg_strdup(item->value.val_str,
                                      tmp_string);
      break;
    case BG_CFG_STRING_HIDDEN:
      if(item->value.val_str)
        {
        free(item->value.val_str);
        item->value.val_str = (char*)0;
        }
      if(tmp_string && (*tmp_string != '\0'))
        item->value.val_str = bg_descramble_string(tmp_string);
      
      break;
    case BG_CFG_COLOR:
      start = tmp_string;
      sscanf(tmp_string, "%f %f %f %f",
             &(item->value.val_color[0]),
             &(item->value.val_color[1]),
             &(item->value.val_color[2]),
             &(item->value.val_color[3]));
      break;
    case BG_CFG_POSITION:
      start = tmp_string;
      sscanf(tmp_string, "%lf %lf",
             &(item->value.val_pos[0]),
             &(item->value.val_pos[1]));
      break;
    }
  if(tmp_string)
    xmlFree(tmp_string);
  if(info.name)
    xmlFree(info.name);
  if(tmp_type)
    xmlFree(tmp_type);
  }

void bg_cfg_xml_2_section(xmlDocPtr xml_doc,
                          xmlNodePtr xml_section,
                          bg_cfg_section_t * cfg_section)
  {
  xmlNodePtr cur;
  char * tmp_string;
  bg_cfg_section_t * cfg_child_section;
  
  cur = xml_section->children;

  tmp_string = BG_XML_GET_PROP(xml_section, "gettext_domain");
  if(tmp_string)
    {
    cfg_section->gettext_domain =
      bg_strdup(cfg_section->gettext_domain, tmp_string);
    xmlFree(tmp_string);
    }
  tmp_string = BG_XML_GET_PROP(xml_section, "gettext_directory");
  if(tmp_string)
    {
    cfg_section->gettext_directory =
      bg_strdup(cfg_section->gettext_directory, tmp_string);
    xmlFree(tmp_string);
    }
  
  while(cur)
    {
    if(!cur->name)
      {
      cur = cur->next;
      continue;
      }
    /* Load items */
    else if(!BG_XML_STRCMP(cur->name, "ITEM"))
      {
      load_item(xml_doc, cur, cfg_section);
      }
    /* Load child */
    else if(!BG_XML_STRCMP(cur->name, "SECTION"))
      {
      tmp_string = BG_XML_GET_PROP(cur, "name");
      if(tmp_string)
        {
        cfg_child_section = bg_cfg_section_find_subsection(cfg_section, tmp_string);
        bg_cfg_xml_2_section(xml_doc, cur, cfg_child_section);
        xmlFree(tmp_string);
        }
      }
    cur = cur->next;
    }
  }

void bg_cfg_registry_load(bg_cfg_registry_t * r, const char * filename)
  {
  xmlDocPtr xml_doc;
  xmlNodePtr node;
  bg_cfg_section_t * cfg_section;
  char * tmp_string;

  if(!filename)
    return;
    
  xml_doc = bg_xml_parse_file(filename);

  if(!xml_doc)
    return;

  node = xml_doc->children;

  if(BG_XML_STRCMP(node->name, "REGISTRY"))
    {
    xmlFreeDoc(xml_doc);
    return;
    }

  node = node->children;

  while(node)
    {
    if(node->name && !BG_XML_STRCMP(node->name, "SECTION"))
      {
      tmp_string = BG_XML_GET_PROP(node, "name");
      if(tmp_string)
        {
        cfg_section = bg_cfg_registry_find_section(r, tmp_string);
        bg_cfg_xml_2_section(xml_doc, node, cfg_section);
        xmlFree(tmp_string);
        }
      }
    node = node->next;
    }
  
  xmlFreeDoc(xml_doc);
  }

void bg_cfg_section_2_xml(bg_cfg_section_t * section, xmlNodePtr xml_section)
  {
  char * tmp_string;
  bg_cfg_item_t    * item;
  bg_cfg_section_t * tmp_section;

  char buffer[1024];
  
  xmlNodePtr xml_item, xml_child;
  
  /* Save items */

  item = section->items;
  
  if(section->gettext_domain)
    BG_XML_SET_PROP(xml_section, "gettext_domain", section->gettext_domain);
  if(section->gettext_directory)
    BG_XML_SET_PROP(xml_section, "gettext_directory", section->gettext_directory);
  
  xmlAddChild(xml_section, BG_XML_NEW_TEXT("\n"));

  while(item)
    {
    xml_item = xmlNewTextChild(xml_section, (xmlNsPtr)0, (xmlChar*)"ITEM", NULL);
    BG_XML_SET_PROP(xml_item, "name", item->name);
    
    switch(item->type)
      {
      case BG_CFG_INT:
        BG_XML_SET_PROP(xml_item, "type", "int");
        sprintf(buffer, "%d", item->value.val_i);
        xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
        break;
      case BG_CFG_TIME:
        BG_XML_SET_PROP(xml_item, "type", "time");
        sprintf(buffer, "%" PRId64, item->value.val_time);
        xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
        break;
      case BG_CFG_FLOAT:
        BG_XML_SET_PROP(xml_item, "type", "float");
        sprintf(buffer, "%.15e", item->value.val_f);
        xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
        break;
      case BG_CFG_STRING:
        BG_XML_SET_PROP(xml_item, "type", "string");
        /* Yes, that's stupid */
        if(item->value.val_str)
          xmlAddChild(xml_item, BG_XML_NEW_TEXT(item->value.val_str));
        break;
      case BG_CFG_STRING_HIDDEN:
        BG_XML_SET_PROP(xml_item, "type", "string_hidden");
        /* Yes, that's stupid */
        if(item->value.val_str)
          {
          tmp_string = bg_scramble_string(item->value.val_str);
          xmlAddChild(xml_item, BG_XML_NEW_TEXT(tmp_string));
          free(tmp_string);
          }
        break;
      case BG_CFG_COLOR:
        BG_XML_SET_PROP(xml_item, "type", "color");
        sprintf(buffer, "%f %f %f %f",
                item->value.val_color[0],
                item->value.val_color[1],
                item->value.val_color[2],
                item->value.val_color[3]);
        xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
        break;
      case BG_CFG_POSITION:
        BG_XML_SET_PROP(xml_item, "type", "pos");
        sprintf(buffer, "%f %f",
                item->value.val_pos[0],
                item->value.val_pos[1]);
        xmlAddChild(xml_item, BG_XML_NEW_TEXT(buffer));
        break;
      }
    xmlAddChild(xml_section, BG_XML_NEW_TEXT("\n"));
    
    item = item->next;
    }
  
  /* Save child sections */

  tmp_section = section->children;
  while(tmp_section)
    {
    xml_child = xmlNewTextChild(xml_section, (xmlNsPtr)0, (xmlChar*)"SECTION", NULL);
    BG_XML_SET_PROP(xml_child, "name", tmp_section->name);

    bg_cfg_section_2_xml(tmp_section, xml_child);
    xmlAddChild(xml_section, BG_XML_NEW_TEXT("\n"));
    
    tmp_section = tmp_section->next;
    }
  }

void bg_cfg_registry_save(bg_cfg_registry_t * r, const char * filename)
  {
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_registry, xml_section;
  
  bg_cfg_section_t * tmp_section;
  if(!filename)
    return;
  
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_registry = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"REGISTRY", NULL);
  xmlDocSetRootElement(xml_doc, xml_registry);

  xmlAddChild(xml_registry, BG_XML_NEW_TEXT("\n"));
  
  tmp_section = r->sections;
  while(tmp_section)
    {
    xml_section = xmlNewTextChild(xml_registry, (xmlNsPtr)0, (xmlChar*)"SECTION", NULL);
    BG_XML_SET_PROP(xml_section, "name", tmp_section->name);
    
    bg_cfg_section_2_xml(tmp_section, xml_section);
    tmp_section = tmp_section->next;

    xmlAddChild(xml_registry, BG_XML_NEW_TEXT("\n"));
    
    }
  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  }

void bg_cfg_section_dump(bg_cfg_section_t * section, const char * filename)
  {
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_section;
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_section = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"SECTION", NULL);
  xmlDocSetRootElement(xml_doc, xml_section);
  BG_XML_SET_PROP(xml_section, "name", section->name);
  bg_cfg_section_2_xml(section, xml_section);
  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  }
