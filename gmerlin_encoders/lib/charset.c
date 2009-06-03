/*****************************************************************
 
  charset.c
 
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

#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include <gmerlin_encoders.h>

struct bgen_charset_converter_s
  {
  iconv_t cd;
  };
  

bgen_charset_converter_t *
bgen_charset_converter_create(const char * in_charset,
                              const char * out_charset)
  {
  bgen_charset_converter_t * ret = calloc(1, sizeof(*ret));
  ret->cd = iconv_open(out_charset, in_charset);
  return ret;
  }

void bgen_charset_converter_destroy(bgen_charset_converter_t * cnv)
  {
  iconv_close(cnv->cd);
  free(cnv);
  }

/*

void bgen_charset_converter_destroy(bgen_charset_converter_t *);

char * bgen_convert_string(bgen_charset_converter_t *,
                           const char * in_string, int in_len,
                           int * out_len);
*/

#define BYTES_INCREMENT 10

static char * do_convert(iconv_t cd, char * in_string, int len, int * out_len)
  {
  char * ret;

  char *inbuf;
  char *outbuf;
  int alloc_size;
  int output_pos;
  
  size_t inbytesleft;
  size_t outbytesleft;

  alloc_size = len + BYTES_INCREMENT;

  inbytesleft  = len;
  outbytesleft = alloc_size;

  ret    = malloc(alloc_size);
  inbuf  = in_string;
  outbuf = ret;
  
  while(1)
    {
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
        case EILSEQ:
          fprintf(stderr, "Invalid Multibyte sequence\n");
          break;
        case EINVAL:
          fprintf(stderr, "Incomplete Multibyte sequence\n");
          break;
        }
      }
    if(!inbytesleft)
      break;
    }
  /* Zero terminate */

  output_pos = (int)(outbuf - ret);
  
  if(outbytesleft < 2)
    {
    alloc_size+=2;
    ret = realloc(ret, alloc_size);
    outbuf = &ret[output_pos];
    }
  outbuf[0] = '\0';
  outbuf[1] = '\0';
  if(out_len)
    *out_len = outbuf - ret;
  return ret;
  }

char * bgen_convert_string(bgen_charset_converter_t * cnv,
                           const char * str, int len,
                           int * out_len)
  {
  char * ret;
  char * tmp_string;

  if(len < 0)
    len = strlen(str);

  tmp_string = malloc(len+1);
  memcpy(tmp_string, str, len);
  tmp_string[len] = '\0';
  ret = do_convert(cnv->cd, tmp_string, len, out_len);
  free(tmp_string);
  return ret;
  }

