/*****************************************************************
 
  utils.c
 
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>


#include <avdec_private.h>
#include <utils.h>

void bgav_dump_fourcc(uint32_t fourcc)
  {
  if((fourcc & 0xffff0000) || !(fourcc))
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
    for(i = imax; i < linebreak; i++)
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



char * bgav_strndup(const char * start, const char * end)
  {
  char * ret;
  int len;

  if(!start)
    return (char*)0;

  len = (end) ? (end - start) : strlen(start);
  ret = malloc(len+1);
  strncpy(ret, start, len);
  ret[len] = '\0';
  return ret;
  }

char * bgav_strncat(char * old, const char * start, const char * end)
  {
  int len, old_len;
  //  fprintf(stderr, "BGAV_STRNCAT %s %s...", old, start);
  old_len = old ? strlen(old) : 0;
  
  len = (end) ? (end - start) : strlen(start);
  old = realloc(old, len + old_len + 1);
  strncpy(old + old_len, start, len);
  old[old_len + len] = '\0';
  //  fprintf(stderr, "%s\n", old);
  return old;
  }

static char * remove_spaces(char * old)
  {
  char * pos1, *pos2, *ret;
  int num_spaces = 0;

  pos1 = old;
  while(*pos1 != '\0')
    {
    if(*pos1 == ' ')
      num_spaces++;
    pos1++;
    }
  if(!num_spaces)
    return old;
  
  ret = malloc(strlen(old) + 1 + 2 * num_spaces);

  pos1 = old;
  pos2 = ret;

  while(*pos1 != '\0')
    {
    if(*pos1 == ' ')
      {
      *(pos2++) = '%';
      *(pos2++) = '2';
      *(pos2++) = '0';
      }
    else
      *(pos2++) = *pos1;
    pos1++;
    }
  *pos2 = '\0';
  free(old);
  return ret;
  }

/* Split an URL, returned pointers should be free()d after */

int bgav_url_split(const char * url,
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
    *protocol = bgav_strndup(pos1, pos2);

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
      *user = bgav_strndup(pos1, colon_pos);
    pos1 = colon_pos + 1;
    if(password)
      *password = bgav_strndup(pos1, at_pos);
    pos1 = at_pos + 1;
    pos2 = pos1;
    }
  
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
      *path = bgav_strndup(pos1, pos2);
    else
      *path = (char*)0;
    }

  /* Fix whitespaces in path */
  if(path && *path)
    *path = remove_spaces(*path);
  return 1;
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

int bgav_read_line_fd(int fd, char ** ret, int * ret_alloc, int milliseconds)
  {
  char * pos;
  char c;
  int bytes_read;
  bytes_read = 0;
  /* Allocate Memory for the case we have none */
  if(!(*ret_alloc))
    {
    *ret_alloc = BYTES_TO_ALLOC;
    *ret = realloc(*ret, *ret_alloc);
    }
  pos = *ret;
  while(1)
    {
    if(!bgav_read_data_fd(fd, (uint8_t*)(&c), 1, milliseconds))
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

int bgav_read_data_fd(int fd, uint8_t * ret, int len, int milliseconds)
  {
  int bytes_read = 0;
  int result;
  fd_set rset;
  struct timeval timeout;

  int flags = 0;

  //  fprintf(stderr, "bgav_read_data_fd: %d %d\n", milliseconds, len);

  //  if(milliseconds < 0)
  //    flags = MSG_WAITALL;
  
  while(bytes_read < len)
    {
    if(milliseconds >= 0)
      { 
      FD_ZERO (&rset);
      FD_SET  (fd, &rset);
     
      timeout.tv_sec  = milliseconds / 1000;
      timeout.tv_usec = (milliseconds % 1000) * 1000;
    
      if((result = select (fd+1, &rset, NULL, NULL, &timeout)) <= 0)
        {
        //        fprintf(stderr, "select returned %d\n", result);
        return bytes_read;
        }
      //      fprintf(stderr, "select returned %d\n", result);
      }

    result = recv(fd, ret + bytes_read, len - bytes_read, flags);
    //    fprintf(stderr, "recv returned %d\n", result);
    
    if(result > 0)
      bytes_read += result;
    else if(result <= 0)
      {
      return bytes_read;
      }
    }
  return bytes_read;
  }

/* Break string into substrings separated by spaces */

char ** bgav_stringbreak(const char * str, char sep)
  {
  int len, i;
  int num;
  int index;
  char ** ret;
  len = strlen(str);
  if(!len)
    {
    num = 0;
    return (char**)0;
    }
  
  num = 1;
  for(i = 0; i < len; i++)
    {
    if(str[i] == sep)
      num++;
    }
  ret = calloc(num+1, sizeof(char*));

  index = 1;
  ret[0] = bgav_strndup(str, NULL);
  
  for(i = 0; i < len; i++)
    {
    if(ret[0][i] == sep)
      {
      if(index < num)
        ret[index] = ret[0] + i + 1;
      ret[0][i] = '\0';
      index++;
      }
    }
  return ret;
  }

void bgav_stringbreak_free(char ** str)
  {
  free(str[0]);
  free(str);
  }
