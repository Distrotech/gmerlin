#include <stdio.h>
#include <string.h>

#include "gmerlin.h"

#include <utils.h>

void gmerlin_skin_load(gmerlin_skin_t * s, const char * filename)
  {
  xmlDocPtr doc;
  xmlNodePtr node;
  xmlNodePtr child;
  const char * path_end;
  
  doc = xmlParseFile(filename);

  if(!doc)
    goto fail;

  path_end = strrchr(filename, '/');
  s->directory = bg_strndup(s->directory, filename, path_end);
  
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
