/*****************************************************************
 * gmerlin - a general purpose multimedia framework and applications
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
 * gmerlin-general@lists.sourceforge.net
 * http://gmerlin.sourceforge.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * *****************************************************************/


#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <config.h>
#include <gmerlin/charset.h>

#include <gmerlin/log.h>
#define LOG_DOMAIN "charset"

struct bg_charset_converter_s
  {
  iconv_t cd;
  };
  

bg_charset_converter_t *
bg_charset_converter_create(const char * in_charset,
                              const char * out_charset)
  {
  bg_charset_converter_t * ret = calloc(1, sizeof(*ret));
  ret->cd = iconv_open(out_charset, in_charset);
  return ret;
  }

void bg_charset_converter_destroy(bg_charset_converter_t * cnv)
  {
  iconv_close(cnv->cd);
  free(cnv);
  }

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
          bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Invalid Multibyte sequence");
          break;
        case EINVAL:
          bg_log(BG_LOG_ERROR, LOG_DOMAIN, "Incomplete Multibyte sequence");
          break;
        }
      }
    if(!inbytesleft)
      break;
    }
  /* Zero terminate */

  output_pos = (int)(outbuf - ret);
  
  if(outbytesleft < 4)
    {
    alloc_size+=4;
    ret = realloc(ret, alloc_size);
    outbuf = &ret[output_pos];
    }
  outbuf[0] = '\0';
  outbuf[1] = '\0';
  outbuf[2] = '\0';
  outbuf[3] = '\0';
  if(out_len)
    *out_len = outbuf - ret;
  return ret;
  }

char * bg_convert_string(bg_charset_converter_t * cnv,
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

