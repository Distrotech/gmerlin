/*****************************************************************
 
  r_ram.c
 
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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <avdec_private.h>

#define PROBE_BYTES 10

int probe_ram(bgav_input_context_t * input)
  {
  uint8_t probe_buffer[PROBE_BYTES];
  /* Most likely, we get this via http, so we can check the mimetype */
  if(input->mimetype)
    {
    if(!strcmp(input->mimetype, "audio/x-pn-realaudio-plugin") ||
       !strcmp(input->mimetype, "video/x-pn-realvideo-plugin") ||
       !strcmp(input->mimetype, "audio/x-pn-realaudio") ||
       !strcmp(input->mimetype, "video/x-pn-realvideo"))
      return 1;
    else
      return 0;
    }
  if(bgav_input_get_data(input, probe_buffer, PROBE_BYTES) < PROBE_BYTES)
    return 0;
  
  return 0;
  }

int parse_ram(bgav_redirector_context_t * r)
  {
  return 0;
  }

bgav_redirector_t bgav_redirector_ram = 
  {
    probe: probe_ram,
    parse: parse_ram
  };
