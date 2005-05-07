/*****************************************************************
 
  skin.c
 
  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/


#include <stdio.h>
#include <string.h>
#include <config.h>

#include "gmerlin.h"

#include <utils.h>

static const char * default_skin_directory = GMERLIN_DATA_DIR"skins/Default";

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
  
  if(strcmp(node->name, "SKIN"))
    {
    fprintf(stderr, "File %s contains no skin\n", filename);
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
        
    if(!strcmp(node->name, "PLAYERWIN"))
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
