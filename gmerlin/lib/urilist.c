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


static char * parse_uri(const char * pos1, int len)
  {
  const char * start;
  int real_char;
  char * ret;
  char * ret_pos;
  char hostname[HOSTNAME_MAX_LEN];
  int hostname_len;
      
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
  else if(bg_string_is_url(pos1))
    start = pos1;
  else
    return (char*)0;

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
  //  bg_hexdump(ret, ret_pos - ret);
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

  //  fprintf(stderr, "bg_urilist_decode: %s\n", str);
  
  pos1 = str;

  /* Count the URIs */
  
  end = 0;
  num_uris = 0;

  while(1)
    {
    while(((pos1 - str) < len) && isspace(*pos1))
      pos1++;
    
    if(isspace(*pos1))
      break;
    
    num_uris++;
    
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
        
    if((ret[num_added] = parse_uri(pos1, pos2-pos1)))
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
