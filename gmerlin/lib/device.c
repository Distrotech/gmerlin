/*****************************************************************
 
  input_device.c
 
  Copyright (c) 2003-2004 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
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

#include <stdlib.h>
#include <string.h>

#include <plugin.h>
#include <utils.h>

bg_device_info_t * bg_device_info_append(bg_device_info_t * arr,
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

  arr = realloc(arr, (size+1) * sizeof(*arr));

  arr[size-1].device = bg_strdup(NULL, device);
  arr[size-1].name = bg_strdup(NULL, name);

  /* Zero terminate */
  
  memset(&(arr[size]), 0, sizeof(arr[size]));
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
