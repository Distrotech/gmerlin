/*****************************************************************
 
  utf8.c
 
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <langinfo.h>
#include <iconv.h>
#include <utils.h>

#define BYTES_INCREMENT 10

static char * do_convert(iconv_t cd, char * in_string, int len, int * got_error)
  {
  char * ret;

  char *inbuf;
  char *outbuf;
  int alloc_size;
  int output_pos;
  int keep_going = 1;
  size_t inbytesleft;
  size_t outbytesleft;

  alloc_size = len + BYTES_INCREMENT;

  inbytesleft  = len;
  outbytesleft = alloc_size;

  ret    = malloc(alloc_size);
  inbuf  = in_string;
  outbuf = ret;
  
  while(keep_going)
    {
    //    fprintf(stderr, "Iconv...");
    if(iconv(cd, &inbuf, &inbytesleft,
             &outbuf, &outbytesleft) == (size_t)-1)
      {
      switch(errno)
        {
        case E2BIG:
          output_pos = (int)(outbuf - ret);

          alloc_size   += BYTES_INCREMENT;
          outbytesleft += BYTES_INCREMENT;

          ret = realloc(ret, alloc_size);
          outbuf = &ret[output_pos];
          break;
        default:
          // fprintf(stderr, "Unknown error: %s\n", strerror(errno));
          keep_going = 0;
          if(got_error)
            *got_error = 1;
          break;
        }
      }
    if(!inbytesleft)
      break;
    //    fprintf(stderr, "done, %d\n", inbytesleft);
    }
  /* Zero terminate */

  output_pos = (int)(outbuf - ret);
  
  if(!outbytesleft)
    {
    alloc_size++;
    ret = realloc(ret, alloc_size);
    outbuf = &ret[output_pos];
    }
  *outbuf = '\0';
  return ret;
  }

static char * try_charsets[] = 
  {
    "ISO8859-1",
    "UTF-8",
    (char*)0,
  };
  
char * bg_system_to_utf8(const char * str, int len)
  {
  int i;
  iconv_t cd;
  char * system_charset;
  char * ret;
  int got_error = 0;
  char * tmp_string;
    
  if(len < 0)
    len = strlen(str);

  system_charset = nl_langinfo(CODESET);
  //  if(!strcmp(system_charset, "UTF-8"))
  //    return bg_strndup(NULL, str, str+len);

  
  //  fprintf(stderr, "System string:\n");
  //  bg_hexdump(str, len);
  tmp_string = malloc(len+1);
  memcpy(tmp_string, str, len);
  tmp_string[len] = '\0';
  
  //  fprintf(stderr, "System charset: %s\n", system_charset);

  //  system_charset = "ISO-8859-1";
  
  cd = iconv_open("UTF-8", system_charset);
  ret = do_convert(cd, tmp_string, len, &got_error);
  iconv_close(cd);

  if(got_error)
    {
    if(ret)
      free(ret);
    i = 0;
    
    while(try_charsets[i])
      {
      got_error = 0;

      cd = iconv_open("UTF-8", try_charsets[i]);
      ret = do_convert(cd, tmp_string, len, &got_error);
      iconv_close(cd);
      if(!got_error)
        {
        free(tmp_string);
        // fprintf(stderr, "Converting from %s succeeded\n", try_charsets[i]);
        return ret;
        }
      else if(ret)
        free(ret);
      i++;
      }
    
    strncpy(tmp_string, str, len);
    tmp_string[len] = '\0';

    
    }
  
  free(tmp_string);
  return ret;
  }

#if 1

char * bg_utf8_to_system(const char * str, int len)
  {
  iconv_t cd;
  char * system_charset;
  char * ret;

  char * tmp_string;
  
  if(len < 0)
    len = strlen(str);

  system_charset = nl_langinfo(CODESET);
  if(!strcmp(system_charset, "UTF-8"))
    return bg_strndup(NULL, str, str+len);
    
  tmp_string = malloc(len+1);
  memcpy(tmp_string, str, len);
  tmp_string[len] = '\0';

  //  fprintf(stderr, "System charset: %s\n", system_charset);
  
  //  system_charset = "ISO-8859-1";

  cd = iconv_open(system_charset, "UTF-8");
  ret = do_convert(cd, tmp_string, len, NULL);
  iconv_close(cd);
  free(tmp_string);
  return ret;
  }
#endif
