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

#include <stdlib.h>

#include <gavl/gavl.h>
#include <gavl/metadata.h>
#include <gmerlin/upnp/ssdp.h>

int main(int argc, char ** argv)
  {
  gavl_time_t delay_time = GAVL_TIME_SCALE / 100;
  bg_ssdp_t * s = bg_ssdp_create(NULL, 1, "Linux/2.6.32-46-generic, UPnP/1.0, gmerlin/1.2.0");
  
  while(1)
    {
    bg_ssdp_update(s);
    gavl_time_delay(&delay_time);
    }
  }
