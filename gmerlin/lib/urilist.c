/*****************************************************************
 
  urilist.c
 
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <utils.h>

#define HOSTNAME_MAX_LEN 512

static int do_substitute(uint8_t c)
  {
  if(!isalnum(c) &&
     (c != '-') && 
     (c != '_') && 
     (c != '~') && 
     (c != ':') && 
     (c != '/') && 
     (c != '.'))
    return 1;
  if((c <= 32) || (c >= 127))
    return 1;
  return 0;
  }

char * bg_string_to_uri(const char * _pos, int len)
  {
  char * ret;
  char * dst;
  int i;
  int num_substitutions;

  const uint8_t * pos; 
  
  if(!_pos)
    return NULL;

  pos = (const uint8_t *)_pos;
  num_substitutions = 0;

  if(len < 0)
    len = strlen((char*)pos);

  for(i = 0; i < len; i++)
    {
    if(do_substitute(pos[i]))
      num_substitutions++;
    }
  ret = calloc(1, len + num_substitutions * 2 + 1);
  dst = ret;

  for(i = 0; i < len; i++)
    {
    if(do_substitute(pos[i]))
      {
      sprintf(dst, "%%%02X", pos[i]);
      dst += 3;
      }
    else
      {
      *dst = pos[i];
      dst++;
      }
    }
  return ret;
  }

char * bg_uri_to_string(const char * pos1, int len)
  {
  const char * start;
  int real_char;
  char * ret;
  char * ret_pos;
  char hostname[HOSTNAME_MAX_LEN];
  int hostname_len;

  if(!pos1)
    return NULL;
  
  if(len < 0)
    len = strlen(pos1);
  
  if(!strncmp(pos1, "file:/", 6))
    {
    if(pos1[6] != '/')
      {
      /* KDE Case */
      start = &(pos1[5]);
      }
    else if(pos1[7] != '/') /* RFC .... (text/uri-list) */
      {
      gethostname(hostname, HOSTNAME_MAX_LEN);
      hostname_len = strlen(hostname);

      if((len - 7) < hostname_len)
        return (char*)0;
      
      if(strncmp(&(pos1[7]), hostname, strlen(hostname)))
        return (char *)0;
      start = &(pos1[7+hostname_len]);
      }
    else /* Gnome Case */
      start = &(pos1[7]);
    }
  else
    start = pos1;
  
  /* Allocate return value and decode */
  
  ret = calloc(len - (start - pos1) + 1, sizeof(char));
  ret_pos = ret;
  while(start - pos1 < len)
    {
    if(*start == '%')
      {
      if((len - (start - pos1) < 3) ||
         (!sscanf(&(start[1]), "%02x", &real_char)))
        {
        free(ret);
        return (char*)0;
        }
      start += 3;
      *ret_pos = real_char;
      }
    else
      {
      *ret_pos = *start;
      start++;
      }
    ret_pos++;
    }
  *ret_pos = '\0';

  return ret;
  }

char ** bg_urilist_decode(const char * str, int len)
  {
  char ** ret;
  const char * pos1;
  const char * pos2;
  int end;
  int num_uris;
  int num_added;

  
  pos1 = str;

  /* Count the URIs */
  
  end = 0;
  num_uris = 0;
  while(1)
    {
    /* Skip spaces */
    while(((pos1 - str) < len) && isspace(*pos1))
      pos1++;
    
    if(isspace(*pos1) || (*pos1 == '\0'))
      break;
    
    num_uris++;
    
    /* Skip non-spaces */
    while(((pos1 - str) < len) && !isspace(*pos1))
      pos1++;

    if(!isspace(*pos1))
      break;
    }

  /* Set up the array and decode URLS */

  num_added = 0;
  end = 0;
  pos1 = str;

  ret = calloc(num_uris+1, sizeof(char*));
    
  while(1)
    {
    while(((pos1 - str) < len) && isspace(*pos1))
      pos1++;
    
    pos2 = pos1;
    
    while(((pos2 - str) < len) && !isspace(*pos2))
      pos2++;

    if(!isspace(*pos2) || (pos1 == pos2))
      {
      end = 1;
      
      if(*pos2 != '\0')
        pos2++;
      }
    
    if(pos2 == pos1)
      break;
        
    if((ret[num_added] = bg_uri_to_string(pos1, pos2-pos1)))
      {
      num_added++;
      }

    pos1 = pos2;
    }
  return ret;
  }

void bg_urilist_free(char ** uri_list)
  {
  int i;
  
  i = 0;
  
  while(uri_list[i])
    {
    free(uri_list[i]);
    i++;
    }
  free(uri_list);
  }
