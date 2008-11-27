/*****************************************************************
 * gmerlin-mozilla - A gmerlin based mozilla plugin
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

#include <string.h>

#include <gmerlin_mozilla.h>
#include <gmerlin/utils.h>

void bg_mozilla_embed_info_set_parameter(bg_mozilla_embed_info_t * e,
                                         const char * name,
                                         const char * value)
  {
  if(!strcmp(name, "src"))
    e->src = bg_strdup(e->src, value);
  else if(!strcmp(name, "target"))
    e->target = bg_strdup(e->target, value);
  else if(!strcmp(name, "type"))
    e->type = bg_strdup(e->type, value);
  else if(!strcmp(name, "id"))
    e->id = bg_strdup(e->id, value);
  else if(!strcmp(name, "controls"))
    e->controls = bg_strdup(e->controls, value);
  if(!strcmp(name, "href"))
    e->href = bg_strdup(e->href, value);
  }

int bg_mozilla_embed_info_check(bg_mozilla_embed_info_t * e)
  {
  if(e->mode == MODE_REAL)
    {
    if((e->controls &&
        strcasecmp(e->controls, "imagewindow")) ||
       !e->src)
    return 0;
    }
  //  if((e->mode == MODE_VLC) && !e->target)
  //    return 1;
  return 1;
  }

void bg_mozilla_embed_info_free(bg_mozilla_embed_info_t * e)
  {
  if(e->src)    free(e->src);
  if(e->target) free(e->target);
  if(e->type)   free(e->type);
  if(e->id)   free(e->id);
  if(e->controls)   free(e->controls);
  if(e->href)   free(e->href);
  }
