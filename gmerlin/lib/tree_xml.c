/*****************************************************************
 
  tree_xml.c
 
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

#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include <tree.h>
#include <treeprivate.h>
#include <utils.h>

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
  bg_album_t * ret;
  bg_album_t * child_album;
  char * tmp_string;
  xmlNodePtr child;

  ret = bg_album_create(&(tree->com), BG_ALBUM_TYPE_REGULAR, parent);
    
  child = node->children;

  while(child)
    {
    if(!child->name)
      {
      child = child->next;
      continue;
      }
    tmp_string = xmlNodeListGetString(xml_doc, child->children, 1);

    if(!strcmp(child->name, "NAME"))
      {
      ret->name = bg_strdup(ret->name, tmp_string);
      }
    else if(!strcmp(child->name, "LOCATION"))
      {
      ret->location = bg_strdup(ret->location, tmp_string);
      }
    else if(!strcmp(child->name, "COORDS"))
      {
      sscanf(tmp_string, "%d %d %d %d", &(ret->x), &(ret->y),
             &(ret->width), &(ret->height));
      }
    else if(!strcmp(child->name, "OPEN_PATH"))
      {
      ret->open_path = bg_strdup(ret->open_path, tmp_string);
      }
    if(tmp_string)
      xmlFree(tmp_string);
    child = child->next;
    }
  
  tmp_string = xmlGetProp(node, "expanded");
  if(tmp_string)
    {
    if(atoi(tmp_string))
      ret->flags |= BG_ALBUM_EXPANDED;
    xmlFree(tmp_string);
    }

  tmp_string = xmlGetProp(node, "open");
  if(tmp_string)
    {
    if(atoi(tmp_string))
      ret->open_count = 1;
    xmlFree(tmp_string);
    }
  
  //  fprintf(stderr, "Found Album %s\n", ret->name);
  
  /* Load the album if necessary */

  if(ret->open_count && ret->location)
    {
    tmp_string = bg_sprintf("%s/%s", tree->com.directory, ret->location);
    bg_album_load(ret, tmp_string);
    free(tmp_string);
    }
  
  /* Read children */

  node = node->children;
  while(node)
    {
    if(node->name && !strcmp(node->name, "ALBUM"))
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
  char * tmp_string;
  
  xml_doc = xmlParseFile(tree->filename);
  
  if(!xml_doc)
    return;
  
  node = xml_doc->children;

  if(strcmp(node->name, "MEDIATREE"))
    {
    fprintf(stderr, "File %s contains no media tree\n", tree->filename);
    xmlFreeDoc(xml_doc);
    return;
    }

  node = node->children;

  while(node)
    {
    if(node->name)
      {
      if(!strcmp(node->name, "ALBUM"))
        {
        new_album = load_album(xml_doc, tree, node, NULL);
        if(new_album)
          tree->children = append_album(tree->children, new_album);
        }
      else if(!strcmp(node->name, "COORDS"))
        {
        tmp_string = xmlNodeListGetString(xml_doc, node->children, 1);
        if(tmp_string)
          {
          sscanf(tmp_string, "%d %d %d %d", &(tree->x), &(tree->y),
                 &(tree->width), &(tree->height));
          xmlFree(tmp_string);
          }
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
  
  if((album->type == BG_ALBUM_TYPE_REMOVABLE) ||
     (album->type == BG_ALBUM_TYPE_PLUGIN))
    return;
  
  /* Create XML album */
  
    
  xml_album = xmlNewTextChild(parent, (xmlNsPtr)0, "ALBUM", NULL);

  if(bg_album_is_open(album))
    xmlSetProp(xml_album, "open", "1");
  
  if(bg_album_get_expanded(album))
    xmlSetProp(xml_album, "expanded", "1");

  node = xmlNewTextChild(xml_album, (xmlNsPtr)0, "NAME", NULL);
  xmlAddChild(node, xmlNewText(album->name));
  
  if(album->location)
    {
    node = xmlNewTextChild(xml_album, (xmlNsPtr)0, "LOCATION", NULL);
    xmlAddChild(node, xmlNewText(album->location));
    }
  xmlAddChild(parent, xmlNewText("\n"));

#if 0
  /* Save coords */
  node = xmlNewTextChild(xml_album, (xmlNsPtr)0, "COORDS", NULL);
  tmp_string = bg_sprintf("%d %d %d %d", album->x, album->y,
                          album->width, album->height);
  xmlAddChild(node, xmlNewText(tmp_string));
  free(tmp_string);
  xmlAddChild(parent, xmlNewText("\n"));

  /* Save open path */
  if(album->open_path)
    {
    node = xmlNewTextChild(xml_album, (xmlNsPtr)0, "OPEN_PATH", NULL);
    xmlAddChild(node, xmlNewText(album->open_path));
    xmlAddChild(parent, xmlNewText("\n"));
    }
#endif
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
  char * tmp_string;
  xmlDocPtr  xml_doc;
  xmlNodePtr xml_mediatree;
  xmlNodePtr node;
  
  xml_doc = xmlNewDoc("1.0");
  xml_mediatree = xmlNewDocRawNode(xml_doc, NULL, "MEDIATREE", NULL);
  xmlDocSetRootElement(xml_doc, xml_mediatree);

  xmlAddChild(xml_mediatree, xmlNewText("\n"));

  /* Save coords */

  node = xmlNewTextChild(xml_mediatree, (xmlNsPtr)0, "COORDS", NULL);
  tmp_string = bg_sprintf("%d %d %d %d", tree->x, tree->y,
                          tree->width, tree->height);
  xmlAddChild(node, xmlNewText(tmp_string));
  free(tmp_string);
  xmlAddChild(xml_mediatree, xmlNewText("\n"));
  
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
