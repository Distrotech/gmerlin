/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

/* Handling of input devices */

#include <stdlib.h>
#include <string.h>

#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gmerlin/plugin.h>
#include <gmerlin/utils.h>

bg_device_info_t * bg_device_info_append(bg_device_info_t * arr,
                                         const char * device,
                                         const char * name)
  {
  int i, size = 0;
  char * real_device;

  if(arr)
    {
    while(arr[size].device)
      size++;
    }

  real_device = bg_canonical_filename(device);
  
  for(i = 0; i < size; i++)
    {
    if(!strcmp(arr[i].device, real_device))
      {
      free(real_device);
      return arr;
      }
    }
  
  size++;

  arr = realloc(arr, (size+1) * sizeof(*arr));

  arr[size-1].device = real_device;
  arr[size-1].name = bg_strdup(NULL, name);
  
  /* Zero terminate */
  
  memset(&arr[size], 0, sizeof(arr[size]));
  return arr;
  }

void bg_device_info_destroy(bg_device_info_t * arr)
  {
  int i = 0;

  if(!arr)
    return;
  
  while(arr[i].device)
    {
    free(arr[i].device);
    if(arr[i].name)
      free(arr[i].name);
    i++;
    }
  free(arr);
  }
