/*****************************************************************
 
  device.c
 
  Copyright (c) 2003-2006 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://gmerlin.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/* Handling of input devices */
#include <avdec_private.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <utils.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

bgav_device_info_t * bgav_device_info_append(bgav_device_info_t * arr,
                                             const char * device,
                                             const char * name)
  {
  int size = 0;
    
  if(arr)
    {
    while(arr[size].device)
      size++;
    }
  
  size++;

  //  fprintf(stderr, "arr 1: %p\n", arr);
  
  arr = realloc(arr, (size+1) * sizeof(*arr));
  
  //  fprintf(stderr, "arr 2: %p\n", arr);

  arr[size-1].device = bgav_strdup(device);
  arr[size-1].name = bgav_strdup(name);

  /* Zero terminate */
  
  memset(&(arr[size]), 0, sizeof(arr[size]));

  return arr;
  }

void bgav_device_info_destroy(bgav_device_info_t * arr)
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

void bgav_device_info_dump(bgav_device_info_t * arr)
  {
  int i = 0;

  if(!arr || !arr->device)
    {
    fprintf(stderr, "No devices\n");
    return;
    }
  while(arr[i].device)
    {
    fprintf(stderr, "Name:   %s\n", (arr[i].name ? arr[i].name : "Unknown"));
    fprintf(stderr, "Device: %s\n", arr[i].device);
    i++;
    }

  }
