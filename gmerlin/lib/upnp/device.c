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

#include <upnp_device.h>
#include <string.h>

static void send_description(int fd, const char * desc)
  {
  gavl_metadata_t resp;
  gavl_metadata_init(&resp);
  }

int
bg_upnp_device_handle_request(bg_upnp_device_t * dev, int fd,
                              const char * method,
                              const char * path,
                              const gavl_metadata_t * header)
  {
  int i;
  const char * pos;
  
  if(strncmp(path, "/upnp/", 6))
    return 0;

  path += 6;

  /* Check for description */

  if(!strcmp(path, "desc.xml"))
    {
    send_description(fd, dev->description);
    return 1;
    }

  pos = strchr(path, '/');
  if(!pos)
    return 0;
  
  
  for(i = 0; i < dev->num_services; i++)
    {
    if((strlen(dev->services[i].name) == (pos - path)) &&
       (!strncmp(dev->services[i].name, path, pos - path)))
      {
      /* Found service */
      path = pos + 1;

      if(!strcmp(path, "desc.xml"))
        {
        /* Send service description */
        
        }
      else if(!strcmp(path, "control"))
        {
        /* Service control */
        }
      else if(!strcmp(path, "event"))
        {
        /* Service events */
        }
      else
        return 0; // 404
      }
    }
  return 0;
  }

void
bg_upnp_device_destroy(bg_upnp_device_t * dev);
