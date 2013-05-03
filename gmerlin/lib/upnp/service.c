/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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

#include <upnp_service.h>
#include <gmerlin/utils.h>

int
bg_upnp_service_start(bg_upnp_service_t * s)
  {

  /* Get the related state variables of all action arguments */
  if(!bg_upnp_service_desc_resolve_refs(&s->desc))
    return 0;
  
  /* Create XML Description */
  s->description = bg_upnp_service_desc_2_xml(&s->desc);
  return 1;
  }

void bg_upnp_service_init(bg_upnp_service_t * ret,
                          const char * name, const char * type, int version)
  {
  ret->name = gavl_strdup(name);
  ret->type = gavl_strdup(type);
  ret->version = version;
  }
