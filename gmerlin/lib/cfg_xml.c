/*****************************************************************
 
  cfg_xml.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

#include <stdlib.h>
#include <string.h>
// #include <locale.h>

#include <cfg_registry.h>
#include <registry_priv.h>
#include <utils.h>

static void load_item(xmlDocPtr xml_doc, xmlNodePtr xml_item,
                      bg_cfg_section_t * cfg_section)
  {
  char * tmp_type;
  char * tmp_string;
  char * start;
  
  bg_parameter_info_t info;

  bg_cfg_item_t * item;

  memset(&info, 0, sizeof(info));
  
  tmp_type = xmlGetProp(xml_item, "type");
  info.name = xmlGetProp(xml_item, "name");
  
  if(!tmp_type || !info.name)
    {
    fprintf(stderr, "Invalid item\n");
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
  else if(!strcmp(tmp_type, "color"))
    {
    info.type = BG_PARAMETER_COLOR_RGBA;
    }
  else if(!strcmp(tmp_type, "time"))
    {
    info.type = BG_PARAMETER_TIME;
    }
  else
    {
    fprintf(stderr, "Unknown type: %s\n", tmp_type);
    return;
    }

  /* Find the item */

  item = bg_cfg_section_find_item(cfg_section, &info);
  
  tmp_string = xmlNodeListGetString(xml_doc, xml_item->children, 1);
  
  switch(item->type)
    {
    case BG_CFG_INT:
      sscanf(tmp_string, "%d", &(item->value.val_i));
      break;
    case BG_CFG_TIME:
      sscanf(tmp_string, "%lld", &(item->value.val_time));
      break;
    case BG_CFG_FLOAT:
      sscanf(tmp_string, "%f", &(item->value.val_f));
      break;
    case BG_CFG_STRING:
      item->value.val_str = bg_strdup(item->value.val_str,
                                      tmp_string);
      break;
    case BG_CFG_COLOR:
      start = tmp_string;
      sscanf(tmp_string, "%f %f %f %f",
             &(item->value.val_color[0]),
             &(item->value.val_color[1]),
             &(item->value.val_color[2]),
             &(item->value.val_color[3]));
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
  char * name;
  bg_cfg_section_t * cfg_child_section;
  
  cur = xml_section->children;
  
  while(cur)
    {
    if(!cur->name)
      {
      cur = cur->next;
      continue;
      }
    /* Load items */
    else if(!strcmp(cur->name, "ITEM"))
      {
      load_item(xml_doc, cur, cfg_section);
      }
    /* Load child */
    else if(!strcmp(cur->name, "SECTION"))
      {
      name = xmlGetProp(cur, "name");
      if(name)
        {
        cfg_child_section = bg_cfg_section_find_subsection(cfg_section, name);
        bg_cfg_xml_2_section(xml_doc, cur, cfg_child_section);
        xmlFree(name);
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
  char * section_name;

  if(!filename)
    return;
    
  xml_doc = xmlParseFile(filename);

  if(!xml_doc)
    return;

  node = xml_doc->children;

  if(strcmp(node->name, "REGISTRY"))
    {
    fprintf(stderr, "File %s contains no config registry\n", filename);
    xmlFreeDoc(xml_doc);
    return;
    }

  node = node->children;

  while(node)
    {
    if(node->name && !strcmp(node->name, "SECTION"))
      {
      section_name = xmlGetProp(node, "name");
      if(section_name)
        {
        cfg_section = bg_cfg_registry_find_section(r, section_name);
        bg_cfg_xml_2_section(xml_doc, node, cfg_section);
        xmlFree(section_name);
        }
      }
    node = node->next;
    }
  
  xmlFreeDoc(xml_doc);
  }

void bg_cfg_section_2_xml(bg_cfg_section_t * section, xmlNodePtr xml_section)
  {
  bg_cfg_item_t    * item;
  bg_cfg_section_t * tmp_section;

  char buffer[1024];
  
  xmlNodePtr xml_item, xml_child;
  
  /* Save items */

  item = section->items;

  xmlAddChild(xml_section, xmlNewText("\n"));

  while(item)
    {
    xml_item = xmlNewTextChild(xml_section, (xmlNsPtr)0, "ITEM", NULL);
    xmlSetProp(xml_item, "name", item->name);
    
    switch(item->type)
      {
      case BG_CFG_INT:
        xmlSetProp(xml_item, "type", "int");
        sprintf(buffer, "%d", item->value.val_i);
        xmlAddChild(xml_item, xmlNewText(buffer));
        break;
      case BG_CFG_TIME:
        xmlSetProp(xml_item, "type", "time");
        sprintf(buffer, "%lld", item->value.val_time);
        xmlAddChild(xml_item, xmlNewText(buffer));
        break;
      case BG_CFG_FLOAT:
        xmlSetProp(xml_item, "type", "float");
        sprintf(buffer, "%f", item->value.val_f);
        xmlAddChild(xml_item, xmlNewText(buffer));
        break;
      case BG_CFG_STRING:
        xmlSetProp(xml_item, "type", "string");
        /* Yes, that's stupid */
        if(item->value.val_str)
          xmlAddChild(xml_item, xmlNewText(item->value.val_str));
        break;
      case BG_CFG_COLOR:
        xmlSetProp(xml_item, "type", "color");
        sprintf(buffer, "%f %f %f %f",
                item->value.val_color[0],
                item->value.val_color[1],
                item->value.val_color[2],
                item->value.val_color[3]);
        xmlAddChild(xml_item, xmlNewText(buffer));
        break;
      }
    xmlAddChild(xml_section, xmlNewText("\n"));
    
    item = item->next;
    }
  
  /* Save child sections */

  tmp_section = section->children;
  while(tmp_section)
    {
    xml_child = xmlNewTextChild(xml_section, (xmlNsPtr)0, "SECTION", NULL);
    xmlSetProp(xml_child, "name", tmp_section->name);

    bg_cfg_section_2_xml(tmp_section, xml_child);
    xmlAddChild(xml_section, xmlNewText("\n"));
    
    tmp_section = tmp_section->next;
    }
  }

void bg_cfg_registry_save(bg_cfg_registry_t * r, const char * filename)
  {
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_registry, xml_section;
  
  bg_cfg_section_t * tmp_section;
  //  char * old_locale;

  if(!filename)
    return;
  
  //  old_locale = setlocale(LC_NUMERIC, "C");
  
  xml_doc = xmlNewDoc("1.0");
  xml_registry = xmlNewDocRawNode(xml_doc, NULL, "REGISTRY", NULL);
  xmlDocSetRootElement(xml_doc, xml_registry);

  xmlAddChild(xml_registry, xmlNewText("\n"));
  
  tmp_section = r->sections;
  while(tmp_section)
    {
    xml_section = xmlNewTextChild(xml_registry, (xmlNsPtr)0, "SECTION", NULL);
    xmlSetProp(xml_section, "name", tmp_section->name);
    
    bg_cfg_section_2_xml(tmp_section, xml_section);
    tmp_section = tmp_section->next;

    xmlAddChild(xml_registry, xmlNewText("\n"));
    
    }
  xmlSaveFile(filename, xml_doc);
  xmlFreeDoc(xml_doc);
  //  setlocale(LC_NUMERIC, old_locale);
  }
