/*****************************************************************
 
  r_ref.c
 
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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <avdec_private.h>


static int probe_ref(bgav_input_context_t * input)
  {
  char probe_data[11];
  if(bgav_input_get_data(input, (uint8_t*)probe_data, 11) < 11)
    return 0;

  if(!strncasecmp(probe_data, "[Reference]", 11))
    return 1;
  return 0;
  }

static int parse_ref(bgav_redirector_context_t * r)
  {
  char * buffer = (char *)0;
  int buffer_alloc = 0;
  char * pos;
    
  if(!bgav_input_read_line(r->input, &buffer, &buffer_alloc, 0))
    return 0;

  if(strncasecmp(buffer, "[Reference]", 11))
    return 0;
  
  while(1)
    {
    if(!bgav_input_read_line(r->input, &buffer, &buffer_alloc, 0))
      break;

    if(!strncasecmp(buffer, "ref", 3) && isdigit(buffer[3]))
      {
      pos = strchr(buffer, '=');
      if(pos)
        {
        pos++;
        r->num_urls++;
        r->urls = realloc(r->urls, r->num_urls * sizeof(*(r->urls)));
        memset(r->urls + r->num_urls - 1, 0, sizeof(*(r->urls)));
        
        r->urls[r->num_urls-1].name = bgav_sprintf("Stream %d (%s)",
                                                   r->num_urls,
                                                   pos);
        r->urls[r->num_urls-1].url = bgav_strndup(pos, NULL);
        }
      }
    }
  return 1;
  }

bgav_redirector_t bgav_redirector_ref = 
  {
    name:  "[Reference]",
    probe: probe_ref,
    parse: parse_ref
  };
