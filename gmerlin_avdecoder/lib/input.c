/*****************************************************************
 
  input.c
 
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
#include <stdio.h>
#include <stdlib.h>

#include <avdec_private.h>

#define GET_LINE_SIZE 8
#define ALLOC_SIZE    128

static void add_char(char ** buffer, int * buffer_alloc,
                     int pos, char c)
  {
  //  fprintf(stderr, "Add char: %c %d\n", c, pos);
  if(pos + 1 > *buffer_alloc)
    {
    while(pos + 1 > *buffer_alloc)
      *buffer_alloc += ALLOC_SIZE;
    *buffer = realloc(*buffer, *buffer_alloc);
    }
  (*buffer)[pos] = c;
  }

int bgav_input_read_line(bgav_input_context_t* input,
                         char ** buffer, int * buffer_alloc)
  {
  char c;
  int pos = 0;
  while(1)
    {
    if(!bgav_input_read_data(input, &c, 1))
      {
      return 0;
      //      return pos;
      break;
      }
    else if(c == '\n')
      break;
    else if(c != '\r')
      {
      //      fprintf(stderr, "Read line %02x\n", c);
      add_char(buffer, buffer_alloc, pos, c);
      pos++;
      }
    }
  add_char(buffer, buffer_alloc, pos, 0);
  //  fprintf(stderr, "Read line: %s\n", *buffer);
  return 1;
  }

int bgav_input_read_data(bgav_input_context_t * ctx, uint8_t * buffer, int len)
  {
  int bytes_to_copy = 0;
  int ret;
  int result;

  if(ctx->total_bytes)
    {
    //    fprintf(stderr, "Total bytes: %lld, pos: %lld\n",
    //            ctx->total_bytes, ctx->position);
    if(ctx->position + len > ctx->total_bytes)
      len = ctx->total_bytes - ctx->position;
    if(!len)
      return 0;
    }

  
  if(ctx->buffer_size)
    {
    if(len > ctx->buffer_size)
      bytes_to_copy = ctx->buffer_size;
    else
      bytes_to_copy = len;

    memcpy(buffer, ctx->buffer, bytes_to_copy);
    if(bytes_to_copy < ctx->buffer_size)
      memmove(ctx->buffer, &(ctx->buffer[bytes_to_copy]),
              ctx->buffer_size - bytes_to_copy);
    ctx->buffer_size -= bytes_to_copy; 
    }
  if(len > bytes_to_copy)
    {
    result =
      ctx->input->read(ctx, &(buffer[bytes_to_copy]), len - bytes_to_copy);
    if(result < 0)
      result = 0;
    ret = bytes_to_copy + result;
    }
  else
    ret = len;
  ctx->position += ret;

  if(ctx->do_buffer)
    {
    //      fprintf(stderr, "Do buffer %d %d\n",  ctx->buffer_alloc, ctx->buffer_size);
    ctx->buffer_size +=
      ctx->input->read_nonblock(ctx, ctx->buffer + ctx->buffer_size,
                                ctx->buffer_alloc - ctx->buffer_size);
    }
  return ret;
  }

int bgav_input_get_data(bgav_input_context_t * ctx, uint8_t * buffer, int len)
  {
  int bytes_gotten;
  int result;
  if(ctx->buffer_size < len)
    {
    if(ctx->buffer_size + len > ctx->buffer_alloc)
      {
      ctx->buffer_alloc += len + 64;
      ctx->buffer = realloc(ctx->buffer, ctx->buffer_alloc);
      }
    result =
      ctx->input->read(ctx, &(ctx->buffer[ctx->buffer_size]),
                      len - ctx->buffer_size);
    if(result < 0)
      result = 0;
    ctx->buffer_size += result;
    }
  bytes_gotten = (len > ctx->buffer_size) ? ctx->buffer_size :
    len;

  if(bytes_gotten)
    memcpy(buffer, ctx->buffer, bytes_gotten);
  
  return bytes_gotten;
  }

int bgav_input_read_8(bgav_input_context_t * ctx, uint8_t * ret)
  {
  if(bgav_input_read_data(ctx, ret, 1) < 1)
    return 0;
  return 1;
  }

int bgav_input_read_16_le(bgav_input_context_t * ctx,uint16_t * ret)
  {
  uint8_t data[2];
  if(bgav_input_read_data(ctx, data, 2) < 2)
    return 0;
  *ret = BGAV_PTR_2_16LE(data);
  
  return 1;
  }

int bgav_input_read_32_le(bgav_input_context_t * ctx,uint32_t * ret)
  {
  uint8_t data[4];
  if(bgav_input_read_data(ctx, data, 4) < 4)
    return 0;
  *ret = BGAV_PTR_2_32LE(data);
  return 1;
  }

int bgav_input_read_64_le(bgav_input_context_t * ctx,uint64_t * ret)
  {
  uint8_t data[8];
  if(bgav_input_read_data(ctx, data, 8) < 8)
    return 0;
  *ret = BGAV_PTR_2_64LE(data);
  return 1;
  }

int bgav_input_read_16_be(bgav_input_context_t * ctx,uint16_t * ret)
  {
  uint8_t data[2];
  if(bgav_input_read_data(ctx, data, 2) < 2)
    return 0;

  *ret = BGAV_PTR_2_16BE(data);
  return 1;
  }

int bgav_input_read_24_be(bgav_input_context_t * ctx,uint32_t * ret)
  {
  uint8_t data[3];
  if(bgav_input_read_data(ctx, data, 3) < 3)
    return 0;
  *ret = BGAV_PTR_2_24BE(data);
  return 1;
  }


int bgav_input_read_32_be(bgav_input_context_t * ctx,uint32_t * ret)
  {
  uint8_t data[4];
  if(bgav_input_read_data(ctx, data, 4) < 4)
    return 0;

  *ret = BGAV_PTR_2_32BE(data);
  return 1;
  }
    
int bgav_input_read_64_be(bgav_input_context_t * ctx, uint64_t * ret)
  {
  uint8_t data[8];
  if(bgav_input_read_data(ctx, data, 8) < 8)
    return 0;
  
  *ret = BGAV_PTR_2_64BE(data);
  return 1;
  }

int bgav_input_get_8(bgav_input_context_t * ctx, uint8_t * ret)
  {
  if(bgav_input_get_data(ctx, ret, 1) < 1)
    return 0;
  return 1;
  }

int bgav_input_get_16_le(bgav_input_context_t * ctx,uint16_t * ret)
  {
  uint8_t data[2];
  if(bgav_input_get_data(ctx, data, 2) < 2)
    return 0;
  *ret = BGAV_PTR_2_16LE(data);
  return 1;
  }

int bgav_input_get_32_le(bgav_input_context_t * ctx,uint32_t * ret)
  {
  uint8_t data[4];
  if(bgav_input_get_data(ctx, data, 4) < 4)
    return 0;
  *ret = BGAV_PTR_2_32LE(data);
  return 1;
  }

int bgav_input_get_64_le(bgav_input_context_t * ctx,uint64_t * ret)
  {
  uint8_t data[8];
  if(bgav_input_get_data(ctx, data, 8) < 8)
    return 0;
  *ret = BGAV_PTR_2_64LE(data);
  return 1;
  }

int bgav_input_get_16_be(bgav_input_context_t * ctx,uint16_t * ret)
  {
  uint8_t data[2];
  if(bgav_input_get_data(ctx, data, 2) < 2)
    return 0;
  *ret = BGAV_PTR_2_16BE(data);
  return 1;
  }

int bgav_input_get_32_be(bgav_input_context_t * ctx,uint32_t * ret)
  {
  uint8_t data[4];
  if(bgav_input_get_data(ctx, data, 4) < 4)
    return 0;
  *ret = BGAV_PTR_2_32BE(data);
  return 1;
  }
    
int bgav_input_get_64_be(bgav_input_context_t * ctx, uint64_t * ret)
  {
  uint8_t data[8];
  if(bgav_input_get_data(ctx, data, 8) < 8)
    return 0;
  *ret = BGAV_PTR_2_64BE(data);
  return 1;
  }

/* Open input */

extern bgav_input_t bgav_input_file;
extern bgav_input_t bgav_input_realrtsp;
extern bgav_input_t bgav_input_pnm;
extern bgav_input_t bgav_input_mms;
extern bgav_input_t bgav_input_http;
extern bgav_input_t bgav_input_vcd;

int bgav_input_open(bgav_input_context_t * ret,
                    const char *url)
  {
  char * protocol = (char*)0;
  int url_len;
  /* TODO: Check for http etc */
  const char * pos;

  if(bgav_url_split(url,
                    &protocol,
                    NULL,
                    NULL,
                    NULL))
    {
    if(!strcmp(protocol, "rtsp"))
      {
      pos = strchr(url, '?');
      if(!pos)
        pos = url + strlen(url);
      
      url_len = (int)(pos - url);
      if(((url[url_len-3] == '.') &&
          (url[url_len-2] == 'r') &&
          (url[url_len-1] == 'm')) ||
         ((url[url_len-3] == '.') &&
          (url[url_len-2] == 'r') &&
          (url[url_len-1] == 'a')))
        {
        fprintf(stderr, "Opening Real RTSP Session\n");
        ret->input = &bgav_input_realrtsp;
        }
      else
        fprintf(stderr, "Standard rtsp not supported\n");
     }
    else if(!strcmp(protocol, "pnm"))
      ret->input = &bgav_input_pnm;
    else if(!strcmp(protocol, "mms"))
      ret->input = &bgav_input_mms;
    else if(!strcmp(protocol, "http"))
      ret->input = &bgav_input_http;
    else
      {
      fprintf(stderr, "Unknown protocol: %s\n", protocol);
      goto fail;
      }
    free(protocol);
    protocol = (char*)0;
    }
  else
    ret->input = &bgav_input_file;
  
  if(!ret->input->open(ret, url))
    {
    fprintf(stderr, "Cannot open file %s\n", url);
    goto fail;
    }

  /* Check if we should buffer data */

  if(!ret->network_buffer_size || !ret->input->read_nonblock)
    ret->do_buffer = 0;
  if(ret->do_buffer)
    {
    ret->buffer_alloc = ret->network_buffer_size;
    ret->buffer = malloc(ret->buffer_alloc);
    }
  
  return 1;

  fail:
  if(protocol)
    free(protocol);
  
  return 0;
  }

bgav_input_context_t * bgav_input_open_vcd(const char * device)
  {
  bgav_input_context_t * ret = (bgav_input_context_t *)0;
  ret = calloc(1, sizeof(*ret));
  ret->input = &bgav_input_vcd;
  if(!ret->input->open(ret, device))
    {
    fprintf(stderr, "Cannot open VCD Device %s\n", device);
    goto fail;
    }
  return ret;
  fail:
  if(ret)
    free(ret);
  return (bgav_input_context_t *)0;
  }

void bgav_input_close(bgav_input_context_t * ctx)
  {
  if(ctx->tt)
    bgav_track_table_unref(ctx->tt);
  if(ctx->input && ctx->priv)
    {
    ctx->input->close(ctx);
    ctx->priv = NULL;
    }
  if(ctx->buffer)
    free(ctx->buffer);
  if(ctx->mimetype)
    free(ctx->mimetype);
  if(ctx->filename)
    free(ctx->filename);
  bgav_metadata_free(&(ctx->metadata));
  //  free(ctx);
  memset(ctx, 0, sizeof(*ctx));
  return;
  }

void bgav_input_destroy(bgav_input_context_t * ctx)
  {
  bgav_input_close(ctx);
  free(ctx);
  }

void bgav_input_skip(bgav_input_context_t * ctx, int bytes)
  {
  int i;
  int bytes_to_skip = bytes;
  uint8_t buf;
  ctx->position += bytes;
  if(ctx->buffer_size)
    {
    if(ctx->buffer_size >= bytes)
      {
      ctx->buffer_size -= bytes;
      if(ctx->buffer_size)
        memmove(ctx->buffer, &(ctx->buffer[bytes]),
                                   ctx->buffer_size);
      if(ctx->do_buffer)
        {
        ctx->buffer_size += ctx->input->read_nonblock(ctx, ctx->buffer + ctx->buffer_size,
                                                      ctx->buffer_alloc - ctx->buffer_size);
        }
      return;
      }
    else
      {
      bytes_to_skip -= ctx->buffer_size;
      ctx->buffer_size = 0;
      }
    }
  if(ctx->input->seek_byte)
    ctx->input->seek_byte(ctx, bytes_to_skip, SEEK_CUR);
  else /* Only small amounts of data should be skipped like this */
    {
    for(i = 0; i < bytes_to_skip; i++)
      bgav_input_read_8(ctx, &buf);
    }
  if(ctx->do_buffer)
    ctx->buffer_size += ctx->input->read_nonblock(ctx, ctx->buffer + ctx->buffer_size,
                                                  ctx->buffer_alloc - ctx->buffer_size);
  }

void bgav_input_seek(bgav_input_context_t * ctx,
                     int64_t position,
                     int whence)
  {
  /*
   *  ctx->position MUST be set before seeking takes place
   *  because some seek() methods might use the position value
   */
  
  switch(whence)
    {
    case SEEK_SET:
      ctx->position = position;
      break;
    case SEEK_CUR:
      ctx->position += position;
      break;
    case SEEK_END:
      ctx->position = ctx->total_bytes - position;
      break;
    }
  ctx->input->seek_byte(ctx, position, whence);
  ctx->buffer_size = 0;
  }

int bgav_input_read_string_pascal(bgav_input_context_t * ctx,
                                  char * ret)
  {
  uint8_t len;
  if(!bgav_input_read_8(ctx, &len) ||
     (bgav_input_read_data(ctx, ret, len) < len))
    return 0;
  ret[len] = '\0';
  return 1;
  }

void bgav_input_buffer(bgav_input_context_t * ctx)
  {
  int bytes_to_read;
  int result;
  
  if(!ctx->do_buffer)
    {
    return;
    }

  while(ctx->buffer_size < ctx->buffer_alloc)
    {
    bytes_to_read = ctx->buffer_alloc / 20;
    if(bytes_to_read > ctx->buffer_alloc - ctx->buffer_size)
      bytes_to_read = ctx->buffer_alloc - ctx->buffer_size;
    result = ctx->input->read(ctx, ctx->buffer + ctx->buffer_size, bytes_to_read);

    if(result < bytes_to_read)
      return;

    ctx->buffer_size += result;
    
    if(ctx->buffer_callback)
      {
      ctx->buffer_callback(ctx->buffer_callback_data,
                           (float)ctx->buffer_size / (float)ctx->buffer_alloc);
      }
    }
  }
