/*****************************************************************
 
  utils.c
 
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <avdec_private.h>

void bgav_dump_fourcc(uint32_t fourcc)
  {
  if(fourcc & 0xffff0000)
    fprintf(stderr, "%c%c%c%c (%08x)",
            (fourcc & 0xFF000000) >> 24,
            (fourcc & 0x00FF0000) >> 16,
            (fourcc & 0x0000FF00) >> 8,
            (fourcc & 0x000000FF),
            fourcc);
  else
    fprintf(stderr, "WavID: 0x%04x", fourcc);
    
  }

void bgav_hexdump(uint8_t * data, int len, int linebreak)
  {
  int i;
  int bytes_written = 0;
  int imax;
  
  while(bytes_written < len)
    {
    imax = (bytes_written + linebreak > len) ? len - bytes_written : linebreak;
    for(i = 0; i < imax; i++)
      fprintf(stderr, "%02x ", data[bytes_written + i]);
    for(i = imax; i < linebreak - imax; i++)
      fprintf(stderr, "   ");
    for(i = 0; i < imax; i++)
      {
      if(!(data[bytes_written + i] & 0x80) && (data[bytes_written + i] >= 32))
        fprintf(stderr, "%c", data[bytes_written + i]);
      else
        fprintf(stderr, ".");
      }
    bytes_written += imax;
    fprintf(stderr, "\n");
    }
  }

char * bgav_sprintf(const char * format,...)
  {
  va_list argp; /* arg ptr */
  int len;
  char * ret;
  va_start( argp, format);
  len = vsnprintf((char*)0, 0, format, argp);
  ret = malloc(len+1);
  vsnprintf(ret, len+1, format, argp);
  va_end(argp);
  return ret;
  }

/* Split an URL, returned pointers should be free()d after */

char * bgav_strndup(const char * start, const char * end)
  {
  char * ret;
  int len = end - start;
  ret = malloc(len+1);
  strncpy(ret, start, len);
  ret[len] = '\0';
  return ret;
  }

int bgav_url_split(const char * url,
                   char ** protocol,
                   char ** hostname,
                   int * port,
                   char ** path)
  {
  const char * pos1;
  const char * pos2;
  pos1 = url;

  /* Sanity check */
  
  pos2 = strstr(url, "://");
  if(!pos2)
    return 0;

  /* Protocol */
    
  if(protocol)
    *protocol = bgav_strndup(pos1, pos2);

  pos2 += 3;
  pos1 = pos2;

  /* Hostname */

  while((*pos2 != '\0') && (*pos2 != ':') && (*pos2 != '/'))
    pos2++;

  if(hostname)
    *hostname = bgav_strndup(pos1, pos2);

  switch(*pos2)
    {
    case '\0':
      if(port)
        *port = -1;
      return 1;
      break;
    case ':':
      /* Port */
      pos2++;
      if(port)
        *port = atoi(pos2);
      while(isdigit(*pos2))
        pos2++;
    default:
      if(port)
        *port = -1;
      break;
    }

  if(path)
    {
    pos1 = pos2;
    pos2 = pos1 + strlen(pos1);
    if(pos1 == pos2)
      *path = bgav_sprintf("/");
    else
      *path = bgav_strndup(pos1, pos2);
    }
  return 1;
  }
