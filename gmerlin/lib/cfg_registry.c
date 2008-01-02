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
#include <stdio.h>
#include <string.h>

#include <cfg_registry.h>
#include <registry_priv.h>
#include <utils.h>

bg_cfg_registry_t * bg_cfg_registry_create()
  {
  bg_cfg_registry_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void bg_cfg_registry_destroy(bg_cfg_registry_t * r)
  {
  bg_cfg_section_t * next_section;
  
  while(r->sections)
    {
    next_section = r->sections->next;
    bg_cfg_section_destroy(r->sections);
    r->sections = next_section;
    }
  free(r);
  }

/*
 *  Path looks like "section:subsection:subsubsection"
 */

bg_cfg_section_t * bg_cfg_registry_find_section(bg_cfg_registry_t * r,
                                                const char * path)
  {
  int depth;
  int i, len;
  char * tmp_path;
  char ** tmp_sections;
  char * pos;
  
  bg_cfg_section_t * section;
  bg_cfg_section_t * prev_section;

  depth = 1;
  len = strlen(path);
  for(i = 0; i < len; i++)
    {
    if(path[i] == ':')
      depth++;
    }
  tmp_path = bg_strdup((char*)0, path);
  
  tmp_sections = malloc(depth * sizeof(char*));

  tmp_sections[0] = tmp_path;

  if(depth > 1)
    {
    pos = strchr(tmp_path, ':');
    
    for(i = 1; i < depth; i++)
      {
      *pos = '\0';
      pos++;
      tmp_sections[i] = pos;
      pos = strchr(pos, ':');
      }
    }
  
  /* Find the root section */

  section = r->sections;

  prev_section = (bg_cfg_section_t*)0;
  
  while(section)
    {
    if(!strcmp(tmp_sections[0], section->name))
      break;
    prev_section = section;
    section = section->next;
    }
  
  /* Add new section here, if not present */

  if(!section)
    {
    if(prev_section)
      {
      prev_section->next = bg_cfg_section_create(tmp_sections[0]);
      section = prev_section->next;
      }
    else
      {
      r->sections = bg_cfg_section_create(tmp_sections[0]);
      section = r->sections;
      }
    
    }

  /* Get the subsections */
  
  for(i = 1; i < depth; i++)
    {
    section = bg_cfg_section_find_subsection(section, tmp_sections[i]);
    }

  free(tmp_path);
  free(tmp_sections);
  
  return section;
  }
