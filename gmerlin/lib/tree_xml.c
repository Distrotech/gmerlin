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

#include <string.h>

#include <gmerlin/xmlutils.h>

#include <gmerlin/tree.h>
#include <treeprivate.h>
#include <gmerlin/utils.h>

static bg_album_t * append_album(bg_album_t * list, bg_album_t * new_album)
  {
  bg_album_t * a;
  if(!list)
    return new_album;

  a = list;
    
  while(a->next)
    a = a->next;
  a->next = new_album;
  return list;
  }

/* tree_xml.c */

static bg_album_t * load_album(xmlDocPtr xml_doc,
                               bg_media_tree_t * tree, xmlNodePtr node,
                               bg_album_t * parent)
  {
  bg_album_t * ret = NULL;
  bg_album_t * child_album;
  char * tmp_string;
  xmlNodePtr child;

  int is_open = 0;

  tmp_string = BG_XML_GET_PROP(node, "incoming");
  if(tmp_string)
    {
    ret = bg_album_create(&tree->com, BG_ALBUM_TYPE_INCOMING, parent);
    xmlFree(tmp_string);
    }

  if(!ret)
    {
    tmp_string = BG_XML_GET_PROP(node, "favourites");
    if(tmp_string)
      {
      ret = bg_album_create(&tree->com, BG_ALBUM_TYPE_FAVOURITES, parent);
      xmlFree(tmp_string);
      }
    }
  if(!ret)
    {
    tmp_string = BG_XML_GET_PROP(node, "plugin");
    if(tmp_string)
      {
      ret = bg_album_create(&tree->com, BG_ALBUM_TYPE_PLUGIN, parent);
      xmlFree(tmp_string);
      }
    }
  if(!ret)
    {
    tmp_string = BG_XML_GET_PROP(node, "tuner");
    if(tmp_string)
      {
      ret = bg_album_create(&tree->com, BG_ALBUM_TYPE_TUNER, parent);
      xmlFree(tmp_string);
      }
    }
  
  if(!ret)
    {
    ret = bg_album_create(&tree->com, BG_ALBUM_TYPE_REGULAR, parent);
    }
  
  child = node->children;

  while(child)
    {
    if(!child->name)
      {
      child = child->next;
      continue;
      }
    tmp_string = (char*)xmlNodeListGetString(xml_doc, child->children, 1);

    if(!BG_XML_STRCMP(child->name, "NAME"))
      ret->name = bg_strdup(ret->name, tmp_string);
    else if(!BG_XML_STRCMP(child->name, "LOCATION"))
      ret->xml_file = bg_strdup(ret->xml_file, tmp_string);
    else if(!BG_XML_STRCMP(child->name, "DEVICE"))
      ret->device = bg_strdup(ret->device, tmp_string);
    else if(!BG_XML_STRCMP(child->name, "WATCH_DIR"))
      ret->watch_dir = bg_strdup(ret->watch_dir, tmp_string);
    else if(!BG_XML_STRCMP(child->name, "PLUGIN"))
      ret->plugin_info = bg_plugin_find_by_name(ret->com->plugin_reg, tmp_string);
    if(tmp_string)
      xmlFree(tmp_string);
    child = child->next;
    }
  
  tmp_string = BG_XML_GET_PROP(node, "expanded");
  if(tmp_string)
    {
    if(atoi(tmp_string))
      ret->flags |= BG_ALBUM_EXPANDED;
    xmlFree(tmp_string);
    }

  tmp_string = BG_XML_GET_PROP(node, "open");
  if(tmp_string)
    {
    if(atoi(tmp_string))
      {
      is_open = 1;
      }
    xmlFree(tmp_string);
    }
  
  bg_album_set_default_location(ret);
  
  /* Load the album if necessary */
  
  if(is_open)
    {
    bg_album_open(ret);
    }
  
  /* Read children */

  node = node->children;
  while(node)
    {
    if(node->name && !BG_XML_STRCMP(node->name, "ALBUM"))
      {
      child_album = load_album(xml_doc, tree, node, ret);
      if(child_album)
        ret->children = append_album(ret->children, child_album);
      }
    node = node->next;
    }
  
  return ret;
  }

void bg_media_tree_load(bg_media_tree_t * tree)
  {
  xmlDocPtr xml_doc;
  xmlNodePtr node;
  bg_album_t * new_album;
  
  xml_doc = bg_xml_parse_file(tree->filename);
  
  if(!xml_doc)
    return;
  
  node = xml_doc->children;

  if(BG_XML_STRCMP(node->name, "MEDIATREE"))
    {
    xmlFreeDoc(xml_doc);
    return;
    }

  node = node->children;

  while(node)
    {
    if(node->name)
      {
      if(!BG_XML_STRCMP(node->name, "ALBUM"))
        {
        new_album = load_album(xml_doc, tree, node, NULL);
        if(new_album)
          tree->children = append_album(tree->children, new_album);
        }
      else if(!BG_XML_STRCMP(node->name, "CFG_SECTION"))
        {
        bg_cfg_xml_2_section(xml_doc, node, tree->cfg_section);
        }
      }
    
    node = node->next;
    }
  
  xmlFreeDoc(xml_doc);
  }

static void save_album(bg_album_t * album, xmlNodePtr parent)
  {
  xmlNodePtr xml_album;
  bg_album_t * child;
  xmlNodePtr node;

  if(album->type == BG_ALBUM_TYPE_PLUGIN)
    {
    if(album->plugin_info->flags & BG_PLUGIN_REMOVABLE)
      return;
    }
  else if(album->type == BG_ALBUM_TYPE_REMOVABLE)
    return;
  
  /* Create XML album */
      
  xml_album = xmlNewTextChild(parent, NULL, (xmlChar*)"ALBUM", NULL);

  if(bg_album_is_open(album))
    BG_XML_SET_PROP(xml_album, "open", "1");
  
  if(bg_album_get_expanded(album))
    BG_XML_SET_PROP(xml_album, "expanded", "1");

  if(album->type == BG_ALBUM_TYPE_INCOMING)
    BG_XML_SET_PROP(xml_album, "incoming", "1");
  else if(album->type == BG_ALBUM_TYPE_FAVOURITES)
    BG_XML_SET_PROP(xml_album, "favourites", "1");
  else if(album->type == BG_ALBUM_TYPE_PLUGIN)
    BG_XML_SET_PROP(xml_album, "plugin", "1");
  else if(album->type == BG_ALBUM_TYPE_TUNER)
    BG_XML_SET_PROP(xml_album, "tuner", "1");
  
  node = xmlNewTextChild(xml_album, NULL, (xmlChar*)"NAME", NULL);
  xmlAddChild(node, BG_XML_NEW_TEXT(album->name));
  
  if(album->xml_file)
    {
    node = xmlNewTextChild(xml_album, NULL, (xmlChar*)"LOCATION", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(album->xml_file));
    }
  if(album->device)
    {
    node = xmlNewTextChild(xml_album, NULL, (xmlChar*)"DEVICE", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(album->device));
    }
  if(album->watch_dir)
    {
    node = xmlNewTextChild(xml_album, NULL, (xmlChar*)"WATCH_DIR", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(album->watch_dir));
    }
  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));

  if(album->plugin_info)
    {
    node = xmlNewTextChild(xml_album, NULL, (xmlChar*)"PLUGIN", NULL);
    xmlAddChild(node, BG_XML_NEW_TEXT(album->plugin_info->name));
    }
  xmlAddChild(parent, BG_XML_NEW_TEXT("\n"));
  child = album->children;

  while(child)
    {
    save_album(child, xml_album);
    child = child->next;
    }
  }

void bg_media_tree_save(bg_media_tree_t * tree)
  {
  bg_album_t * child;
  char * filename;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_mediatree;
  xmlNodePtr node;
  
  xml_doc = xmlNewDoc((xmlChar*)"1.0");
  xml_mediatree = xmlNewDocRawNode(xml_doc, NULL, (xmlChar*)"MEDIATREE", NULL);
  xmlDocSetRootElement(xml_doc, xml_mediatree);

  xmlAddChild(xml_mediatree, BG_XML_NEW_TEXT("\n"));

  /* Config stuff */

  if(tree->cfg_section)
    {
    node = xmlNewTextChild(xml_mediatree, NULL, (xmlChar*)"CFG_SECTION", NULL);
    bg_cfg_section_2_xml(tree->cfg_section, node);
    xmlAddChild(xml_mediatree, BG_XML_NEW_TEXT("\n"));
    }
 
  child = tree->children;
  while(child)
    {
    save_album(child, xml_mediatree);
    child = child->next;
    }

  filename = bg_sprintf("%s/%s", tree->com.directory, "tree.xml");
  xmlSaveFile(filename, xml_doc);
  free(filename);

  xmlFreeDoc(xml_doc);
  }
