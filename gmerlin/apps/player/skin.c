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


#include <stdio.h>
#include <string.h>
#include <config.h>

#include "gmerlin.h"

#include <gmerlin/utils.h>
#include <xmlutils.h>

static const char * default_skin_directory = GMERLIN_DATA_DIR"/skins/Default";

char * gmerlin_skin_load(gmerlin_skin_t * s, char * directory)
  {
  xmlNodePtr node;
  xmlNodePtr child;
  
  char * filename = (char*)0;
  xmlDocPtr doc = (xmlDocPtr)0;
  
  filename = bg_sprintf("%s/skin.xml", directory);
  doc = xmlParseFile(filename);

  if(!doc)
    {
    free(filename);

    directory = bg_strdup(directory, default_skin_directory);
        
    filename = bg_sprintf("%s/skin.xml", directory);
    doc = xmlParseFile(filename);
    }

  if(!doc)
    {
    goto fail;
    }
    
  s->directory = bg_strdup(s->directory, directory);
  
  node = doc->children;
  
  if(BG_XML_STRCMP(node->name, "SKIN"))
    {
    goto fail;
    }
  node = node->children;

  while(node)
    {
    if(!node->name)
      {
      node = node->next;
      continue;
      }
    child = node->children;

    /* Main window */
        
    if(!BG_XML_STRCMP(node->name, "PLAYERWIN"))
      player_window_skin_load(&(s->playerwindow), doc, node);
    
    node = node->next;
    }

  fail:
  if(doc)
    xmlFreeDoc(doc);
  if(filename)
    free(filename);
  return directory;
  }

void gmerlin_skin_set(gmerlin_t * g)
  {
  player_window_set_skin(g->player_window,
                         &(g->skin.playerwindow),
                         g->skin.directory);
  }

void gmerlin_skin_free(gmerlin_skin_t * s)
  {
  
  }
