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

int probe_m3u(bgav_input_context_t * input)
  {
  //  uint8_t probe_buffer[PROBE_BYTES];
  /* Most likely, we get this via http, so we can check the mimetype */
  if(input->mimetype)
    {
    if(!strcmp(input->mimetype, "audio/x-pn-realaudio-plugin") ||
       !strcmp(input->mimetype, "video/x-pn-realvideo-plugin") ||
       !strcmp(input->mimetype, "audio/x-pn-realaudio") ||
       !strcmp(input->mimetype, "video/x-pn-realvideo") ||
       !strcmp(input->mimetype, "audio/x-mpegurl") ||
       !strcmp(input->mimetype, "audio/mpegurl") ||
       !strcmp(input->mimetype, "audio/m3u"))
      return 1;
    else
      return 0;
    }
  //  if(bgav_input_get_data(input, probe_buffer, PROBE_BYTES) < PROBE_BYTES)
  //    return 0;
  return 0;
  }

static void add_url(bgav_redirector_context_t * r)
  {
  r->num_urls++;
  r->urls = realloc(r->urls, r->num_urls * sizeof(*(r->urls)));
  memset(r->urls + r->num_urls - 1, 0, sizeof(*(r->urls)));
  }

static char * strip_spaces(char * buffer)
  {
  char * ret;
  char * end;
  
  ret = buffer;
  while(isspace(*ret))
    ret++;
  end = &(ret[strlen(ret)-1]);
  while(isspace(*end) && (end > ret))
    end--;
  end++;
  *end = '\0';
  return ret;
  }

int parse_m3u(bgav_redirector_context_t * r)
  {
  char * buffer = (char*)0;
  int buffer_alloc = 0;
  char * pos;
  
  while(1)
    {
    if(!bgav_input_read_line(r->input, &buffer, &buffer_alloc))
      break;
    pos = strip_spaces(buffer);
    if((*pos == '#') || (*pos == '\0') || !strcmp(pos, "--stop--"))
      continue;
    else
      {
      add_url(r);
      r->urls[r->num_urls-1].url = bgav_strndup(pos, NULL);
      }
    }

  if(buffer)
    free(buffer);
  return !!(r->num_urls);
  }

bgav_redirector_t bgav_redirector_m3u = 
  {
    probe: probe_m3u,
    parse: parse_m3u
  };
