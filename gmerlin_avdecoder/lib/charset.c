/*****************************************************************
 
  charset.c
 
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

#include <avdec_private.h>

#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

struct bgav_charset_converter_s
  {
  iconv_t cd;
  };
  

bgav_charset_converter_t *
bgav_charset_converter_create(const char * in_charset,
                              const char * out_charset)
  {
  bgav_charset_converter_t * ret = calloc(1, sizeof(*ret));
  ret->cd = iconv_open(out_charset, in_charset);
  return ret;
  }

void bgav_charset_converter_destroy(bgav_charset_converter_t * cnv)
  {
  iconv_close(cnv->cd);
  free(cnv);
  }

/*

void bgav_charset_converter_destroy(bgav_charset_converter_t *);

char * bgav_convert_string(bgav_charset_converter_t *,
                           const char * in_string, int in_len,
                           int * out_len);
*/

#define BYTES_INCREMENT 10

int do_convert(iconv_t cd, char * in_string, int len, int * out_len,
               char ** ret, int * ret_alloc)
  {

  char *inbuf;
  char *outbuf;
  int output_pos;
  
  size_t inbytesleft;
  size_t outbytesleft;

  if(!(*ret_alloc) < len + BYTES_INCREMENT)
    *ret_alloc = len + BYTES_INCREMENT;
  
  inbytesleft  = len;
  outbytesleft = *ret_alloc;

  *ret    = realloc(*ret, *ret_alloc);
  inbuf  = in_string;
  outbuf = *ret;
  
  while(1)
    {
    if(iconv(cd, &inbuf, &inbytesleft,
             &outbuf, &outbytesleft) == (size_t)-1)
      {
      switch(errno)
        {
        case E2BIG:
          output_pos = (int)(outbuf - *ret);

          *ret_alloc   += BYTES_INCREMENT;
          outbytesleft += BYTES_INCREMENT;

          *ret = realloc(*ret, *ret_alloc);
          outbuf = &((*ret)[output_pos]);
          break;
        case EILSEQ:
          fprintf(stderr, "Invalid Multibyte sequence\n");
          return 0;
          break;
        case EINVAL:
          fprintf(stderr, "Incomplete Multibyte sequence\n");
          return 0;
          break;
        }
      }
    if(!inbytesleft)
      break;
    }
  /* Zero terminate */

  output_pos = (int)(outbuf - *ret);
  
  if(outbytesleft < 2)
    {
    *ret_alloc+=2;
    *ret = realloc(*ret, *ret_alloc);
    outbuf = &((*ret)[output_pos]);
    }
  outbuf[0] = '\0';
  outbuf[1] = '\0';
  if(out_len)
    *out_len = outbuf - *ret;
  return 1;
  }

char * bgav_convert_string(bgav_charset_converter_t * cnv,
                           const char * str, int len,
                           int * out_len)
  {
  int result;
  char * ret = (char*)0;
  int ret_alloc = 0;
  char * tmp_string;

  if(len < 0)
    len = strlen(str);

  tmp_string = malloc(len+1);
  memcpy(tmp_string, str, len);
  tmp_string[len] = '\0';
  result = do_convert(cnv->cd, tmp_string, len, out_len, &ret, &ret_alloc);
  free(tmp_string);
  
  if(!result)
    {
    return (char*)0;
    if(ret)
      free(ret);
    }
  return ret;
  }

int bgav_convert_string_realloc(bgav_charset_converter_t * cnv,
                                const char * str, int len,
                                int * out_len,
                                char ** ret, int * ret_alloc)
  {
  int result;
  char * tmp_string;
  if(len < 0)
    len = strlen(str);
  
  tmp_string = malloc(len+1);
  memcpy(tmp_string, str, len);
  tmp_string[len] = '\0';
  result = do_convert(cnv->cd, tmp_string, len, out_len, ret, ret_alloc);
  free(tmp_string);
  
  return result;
  }
