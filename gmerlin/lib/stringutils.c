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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/* stat stuff */
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>

#include <utils.h>

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
  int len;
  char * ret;
  va_start( argp, format);
  len = vsnprintf((char*)0, 0, format, argp);
  ret = malloc(len+1);
  vsnprintf(ret, len+1, format, argp);
  va_end(argp);
  return ret;
  }

/*
 *  Read a single line from a filedescriptor
 *
 *  ret will be reallocated if neccesary and ret_alloc will
 *  be updated then
 *
 *  The string will be 0 terminated, a trailing \r or \n will
 *  be removed
 */

#define BYTES_TO_ALLOC 1024

int bg_read_line_fd(int fd, char ** ret, int * ret_alloc)
  {
  char * pos;
  char c;
  int bytes_read;

  bytes_read = 0;

  /* Allocate Memory for the case we have none */

  if(!ret_alloc)
    {
    *ret_alloc = BYTES_TO_ALLOC;
    *ret = realloc(*ret, *ret_alloc);
    }

  pos = *ret;
  
  while(1)
    {
    if(!read(fd, &c, 1))
      {
      if(!bytes_read)
        return 0;
      break;
      }

    /*
     *  Line break sequence
     *  is starting, remove the rest from the stream
     */
    
    if(c == '\n')
      {
      break;
      }
    
    /* Reallocate buffer */

    else if(c != '\r')
      {
      if(bytes_read+2 >= *ret_alloc)
        {
        *ret_alloc += BYTES_TO_ALLOC;
        *ret = realloc(*ret, *ret_alloc);
        pos = &((*ret)[bytes_read]);
        }
      
      /* Put the byte and advance pointer */
      
      *pos = c;
      pos++;
      bytes_read++;
      }
    }

  *pos = '\0';
  return 1;
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
    fprintf(stderr, "Trying %s\n", filename);
    
    if(stat(filename, &stat_buf) == -1)
      {
      /* Create empty file */
      file = fopen(filename, "w");
      if(file)
        fclose(file);
      else
        err = 1;

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
  char * ret;

  if(!old_string)
    return bg_strdup((char*)0, tail);

  ret = malloc(strlen(old_string) + strlen(tail) + 1);
  strcpy(ret, old_string);
  strcat(ret, tail);
  free(old_string);
  return ret;
  }

char * bg_strncat(char * old_string, const char * start, const char * end)
  {
  char * ret;
  int old_len;
  
  if(!old_string)
    return bg_strndup((char*)0, start, end);

  old_len = strlen(old_string);
  
  ret = malloc(old_len + end - start + 1);
  strcpy(ret, old_string);
  strncpy(&(ret[old_len]), start, end - start);
  ret[old_len + end - start] = '\0';
  free(old_string);
  return ret;
  }

char ** bg_strbreak(const char * str, char delim)
  {
  int num_entries;
  char *pos, *end;
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
