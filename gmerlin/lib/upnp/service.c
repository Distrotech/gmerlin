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

//#include <upnp_service.h>
#include <upnp_device.h>

#include <gmerlin/utils.h>
#include <string.h>

int
bg_upnp_service_start(bg_upnp_service_t * s)
  {
  int i, idx;
  
  /* Get the related state variables of all action arguments */
  if(!bg_upnp_service_desc_resolve_refs(&s->desc))
    return 0;
  
  /* Create XML Description */
  s->description = bg_upnp_service_desc_2_xml(&s->desc);

  /* Get evented variables */
  for(i = 0; i < s->desc.num_sv; i++)
    {
    if(s->desc.sv[i].flags & BG_UPNP_SV_EVENT)
      s->num_event_vars++;
    }
  s->event_vars = calloc(s->num_event_vars, sizeof(*s->event_vars));

  idx = 0;
  for(i = 0; i < s->desc.num_sv; i++)
    {
    if(s->desc.sv[i].flags & BG_UPNP_SV_EVENT)
      {
      s->event_vars[idx].sv = &s->desc.sv[i];
      idx++;
      }
    }

  /* Get the maximum number of input- and output args so we can
     allocate our request structure */

  for(i = 0; i < s->desc.num_sa; i++)
    {
    if(s->req.args_in_alloc < s->desc.sa[i].num_args_in)
      s->req.args_in_alloc = s->desc.sa[i].num_args_in;
    
    if(s->req.args_out_alloc < s->desc.sa[i].num_args_out)
      s->req.args_out_alloc = s->desc.sa[i].num_args_out;
    }
  
  s->req.args_in  = calloc(s->req.args_in_alloc, sizeof(*s->req.args_in));
  s->req.args_out = calloc(s->req.args_out_alloc, sizeof(*s->req.args_out));
  
  return 1;
  }

void bg_upnp_service_init(bg_upnp_service_t * ret,
                          const char * name, const char * type, int version)
  {
  ret->name = gavl_strdup(name);
  ret->type = gavl_strdup(type);
  ret->version = version;
  }

int
bg_upnp_service_handle_request(bg_upnp_service_t * s, int fd,
                               const char * method,
                               const char * path,
                               const gavl_metadata_t * header)
  {
  if(!strcmp(path, "desc.xml"))
    {
    /* Send service description */
    bg_upnp_device_send_description(s->dev, fd, s->description);
    return 1;
    }
  else if(!strcmp(path, "control"))
    {
    /* Service control */
    bg_upnp_service_handle_action_request(s, fd, method, header);
    }
  else if(!strcmp(path, "event"))
    {
    /* Service events */
    bg_upnp_service_handle_event_request(s, fd, method, header);
    }
  return 0; // 404
  }
