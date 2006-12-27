/*****************************************************************
 
  stringutils.c
 
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

#define _GNU_SOURCE

#include <config.h>


#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <language_table.h>

/* stat stuff */
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>

#include <utils.h>
#include <log.h>
#define LOG_DOMAIN "utils"

char * bg_fix_path(char * path)
  {
  char * ret;
  int len;
  
  if(!path)
    return path;

  len = strlen(path);
  
  if(!len)
    {
    free(path);
    return (char*)0;
    }
  if(path[len-1] != '/')
    {
    ret = malloc(len+2);
    strcpy(ret, path);
    free(path);
    ret[len] = '/';
    ret[len+1] = '\0';
    
    return ret;
    }
  else
    return path;
  }

char * bg_strdup(char * old_string, const char * new_string)
  {
  char * ret;
  
  if(!new_string || (*new_string == '\0'))
    {
    if(old_string)
      free(old_string);
    return (char*)0;
    }

  if(old_string)
    {
    if(!strcmp(old_string, new_string))
      return old_string;
    else
      free(old_string);
    }
  ret = malloc(strlen(new_string)+1);
  strcpy(ret, new_string);
  return ret;
  }

char * bg_strndup(char * old_string,
                  const char * new_string_start,
                  const char * new_string_end)
  {
  char * ret;
  if(!new_string_start || (*new_string_start == '\0'))
    {
    if(old_string)
      free(old_string);
    return (char*)0;
    }

  if(old_string)
    {
    if(!strncmp(old_string, new_string_start,
                new_string_end - new_string_start))
      return old_string;
    else
      free(old_string);
    }
  ret = malloc(new_string_end - new_string_start + 1);
  strncpy(ret, new_string_start, new_string_end - new_string_start);
  ret[new_string_end - new_string_start] = '\0';
  return ret;
  }

char * bg_sprintf(const char * format,...)
  {
  va_list argp; /* arg ptr */
#ifndef HAVE_VASPRINTF
  int len;
#endif
  char * ret;
  va_start( argp, format);

#ifndef HAVE_VASPRINTF
  len = vsnprintf((char*)0, 0, format, argp);
  ret = malloc(len+1);
  vsnprintf(ret, len+1, format, argp);
#else
  vasprintf(&ret, format, argp);
#endif
  va_end(argp);
  return ret;
  }
  
char * bg_create_unique_filename(char * template)
  {
  char * filename;
  struct stat stat_buf;
  FILE * file;
  int err = 0;
  uint32_t count;

  count = 0;

  filename = bg_sprintf(template, 0);

  while(1)
    {
    
    if(stat(filename, &stat_buf) == -1)
      {
      /* Create empty file */
      file = fopen(filename, "w");
      if(file)
        {
        fclose(file);
        }
      else
        {
        bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Cannot open file \"%s\" for writing",
               filename);
        err = 1;
        }
      if(err)
        {
        free(filename);
        return (char*)0;
        }
      else
        return filename;
      }
    count++;
    sprintf(filename, template, count);
    }
  }

char * bg_strcat(char * old_string, const char * tail)
  {
  if(!old_string)
    return bg_strdup((char*)0, tail);

  old_string = realloc(old_string, strlen(old_string) + strlen(tail) + 1);
  strcat(old_string, tail);
  return old_string;
  }

char * bg_strncat(char * old_string, const char * start, const char * end)
  {
  int old_len;
  
  if(!old_string)
    return bg_strndup((char*)0, start, end);

  old_len = strlen(old_string);
  
  old_string = realloc(old_string, old_len + end - start + 1);
  strncpy(&(old_string[old_len]), start, end - start);
  old_string[old_len + end - start] = '\0';
  return old_string;
  }

char ** bg_strbreak(const char * str, char delim)
  {
  int num_entries;
  char *pos, *end = NULL;
  const char *pos_c;
  char ** ret;
  int i;
  if(!str || (*str == '\0'))
    return (char**)0;
    
  pos_c = str;
  
  num_entries = 1;
  while((pos_c = strchr(pos_c, delim)))
    {
    num_entries++;
    pos_c++;
    }
  ret = calloc(num_entries+1, sizeof(char*));

  ret[0] = bg_strdup((char*)0, str);
  pos = ret[0];
  for(i = 0; i < num_entries; i++)
    {
    if(i)
      ret[i] = pos;
    if(i < num_entries-1)
      {
      end = strchr(pos, delim);
      *end = '\0';
      }
    end++;
    pos = end;
    }
  return ret;
  }

void bg_strbreak_free(char ** retval)
  {
  free(retval[0]);
  free(retval);
  }

int bg_string_is_url(const char * str)
  {
  const char * pos, * end_pos;
  pos = str;
  end_pos = strstr(str, "://");

  if(!end_pos)
    return 0;
  
  while(pos != end_pos)
    {
    if(!isalnum(*pos))
      return 0;
    pos++;
    }
  return 1;
  }

int bg_url_split(const char * url,
                 char ** protocol,
                 char ** user,
                 char ** password,
                 char ** hostname,
                 int * port,
                 char ** path)
  {
  const char * pos1;
  const char * pos2;

  /* For detecting user:pass@blabla.com/file */

  const char * colon_pos;
  const char * at_pos;
  const char * slash_pos;
  
  pos1 = url;

  /* Sanity check */
  
  pos2 = strstr(url, "://");
  if(!pos2)
    return 0;

  /* Protocol */
    
  if(protocol)
    *protocol = bg_strndup((char*)0, pos1, pos2);

  pos2 += 3;
  pos1 = pos2;

  /* Check for user and password */

  colon_pos = strchr(pos1, ':');
  at_pos = strchr(pos1, '@');
  slash_pos = strchr(pos1, '/');

  if(colon_pos && at_pos && at_pos &&
     (colon_pos < at_pos) && 
     (at_pos < slash_pos))
    {
    if(user)
      *user = bg_strndup((char*)0, pos1, colon_pos);
    pos1 = colon_pos + 1;
    if(password)
      *password = bg_strndup((char*)0, pos1, at_pos);
    pos1 = at_pos + 1;
    pos2 = pos1;
    }
  
  /* Hostname */

  while((*pos2 != '\0') && (*pos2 != ':') && (*pos2 != '/'))
    pos2++;

  if(hostname)
    *hostname = bg_strndup((char*)0, pos1, pos2);

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
      break;
    default:
      if(port)
        *port = -1;
      break;
    }

  if(path)
    {
    pos1 = pos2;
    pos2 = pos1 + strlen(pos1);
    if(pos1 != pos2)
      *path = bg_strndup((char*)0, pos1, pos2);
    else
      *path = (char*)0;
    }
  return 1;
  }



/* Scramble and descramble password (taken from gftp) */

char * bg_scramble_string(const char * str)
  {
  char *newstr, *newpos;
  
  newstr = malloc (strlen (str) * 2 + 2);
  newpos = newstr;
  
  *newpos++ = '$';

  while (*str != 0)
    {
    *newpos++ = ((*str >> 2) & 0x3c) | 0x41;
    *newpos++ = ((*str << 2) & 0x3c) | 0x41;
    str++;
    }
  *newpos = 0;

  return (newstr);
  }

char * bg_descramble_string(const char *str)
  {
  const char *strpos;
  char *newstr, *newpos;
  int error;
  
  if (*str != '$')
    return (bg_strdup ((char*)0, str));
  
  strpos = str + 1;
  newstr = malloc (strlen (strpos) / 2 + 1);
  newpos = newstr;
  
  error = 0;
  while (*strpos != '\0' && (*strpos + 1) != '\0')
    {
    if ((*strpos & 0xc3) != 0x41 ||
        (*(strpos + 1) & 0xc3) != 0x41)
      {
      error = 1;
      break;
      }
    
    *newpos++ = ((*strpos & 0x3c) << 2) |
      ((*(strpos + 1) & 0x3c) >> 2);
    
    strpos += 2;
    }
  
  if(error)
    {
    free (newstr);
    return (bg_strdup((char*)0, str));
    }
  
  *newpos = '\0';
  return (newstr);
  }

const char * bg_get_language_name(const char * iso)
  {
  int i = 0;
  while(bg_language_codes[i])
    {
    if((bg_language_codes[i][0] == iso[0]) &&
       (bg_language_codes[i][1] == iso[1]) &&
       (bg_language_codes[i][2] == iso[2]))
      return bg_language_labels[i];
    i++;
    }
  return (char*)0;
  }

int bg_string_match(const char * key,
                    const char * key_list)
  {
  const char * pos;
  const char * end;

  pos = key_list;
      

  if(!key_list)
    return 0;
  
  while(1)
    {
    end = pos;
    while(!isspace(*end) && (*end != '\0'))
      end++;
    if(end == pos)
      break;

    if((strlen(key) == (int)(end-pos)) &&
       !strncasecmp(pos, key, (int)(end-pos)))
      {
      return 1;
      }
    pos = end;
    if(pos == '\0')
      break;
    else
      {
      while(isspace(*pos) && (pos != '\0'))
        pos++;
      }
    }
  return 0;
  }
