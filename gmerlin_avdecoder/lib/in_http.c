/*****************************************************************
 
  in_http.h
 
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
#include <string.h>
#include <unistd.h>
#include <avdec_private.h>
#include <http.h>

#define NUM_REDIRECTIONS 5

/* Generic http input module */

typedef struct
  {
  int icy_metaint;
  int icy_bytes;
  bgav_http_t * h;

  /* For Transfer encoding chunked */

  /* Sure, this implementation could be optimized,
     but this way, the application bugs are less likely */

  int chunked;
    
  int chunk_size;
  int chunk_buffer_size;
  int chunk_buffer_alloc;
  char * chunk_buffer;
  
  } http_priv;

static char * title_vars[] =
  {
    "icy-name",
    "ice-name",
    "x-audiocast-name",
    (char*)0
  };

static char * genre_vars[] =
  {
    "x-audiocast-genre",
    "icy-genre",
    "ice-genre",
    (char*)0
  };

static char * comment_vars[] =
  {
    "ice-description",
    "x-audiocast-description",
    (char*)0
  };

static void set_metadata_string(bgav_http_header_t * header,
                                char * vars[], char ** str)
  {
  const char * val;
  int i = 0;
  while(vars[i])
    {
    val = bgav_http_header_get_var(header, vars[i]);
    if(val)
      {
      *str = bgav_strndup(val, NULL);
      return;
      }
    else
      i++;
    }
  }

static int open_http(bgav_input_context_t * ctx, const char * url)
  {
  const char * var;
  int i;
  bgav_http_header_t * header;
  char * redirect_url = (char*)0;
  http_priv * p;
    
  bgav_http_header_t * extra_header = (bgav_http_header_t*)0;
  
  p = calloc(1, sizeof(*p));
  
  if(ctx->http_shoutcast_metadata)
    {
    extra_header = bgav_http_header_create();
    bgav_http_header_add_line(extra_header, "Icy-MetaData:1");
    }
  
  p->h = bgav_http_open(url, ctx->connect_timeout,
                        &redirect_url, extra_header);

  if(!p->h && redirect_url)
    {
    for(i = 0; i < NUM_REDIRECTIONS; i++)
      {
      //      fprintf(stderr, "Got redirection, new URL: %s\n",
      //              redirect_url);
      p->h = bgav_http_open(redirect_url, ctx->connect_timeout,
                            &redirect_url, extra_header);
      if(p->h)
        break;
      else if(!redirect_url)
        break;
      }
    }

  if(redirect_url)
    free(redirect_url);
    
  if(!p->h)
    {
    free(p);
    return 0;
    }
  ctx->priv = p;
  
  header = bgav_http_get_header(p->h);

  //  bgav_http_header_dump(header);
  
  var = bgav_http_header_get_var(header, "Content-Length");
  if(var)
    ctx->total_bytes = atoi(var);
  
  var = bgav_http_header_get_var(header, "Content-Type");
  if(var)
    ctx->mimetype = bgav_strndup(var, NULL);
  else if(bgav_http_header_get_var(header, "icy-notice1"))
    ctx->mimetype = bgav_strndup("audio/mpeg", NULL);
  

  var = bgav_http_header_get_var(header, "icy-metaint");
  if(var)
    {
    p->icy_metaint = atoi(var);
    //    p->icy_bytes = p->icy_metaint;
    //    fprintf(stderr, "icy-metaint: %d\n", p->icy_metaint);
    }
  if(extra_header)
    bgav_http_header_destroy(extra_header);

  /* Get Metadata */

  set_metadata_string(header,
                      title_vars, &(ctx->metadata.title));
  set_metadata_string(header,
                      genre_vars, &(ctx->metadata.genre));
  set_metadata_string(header,
                      comment_vars, &(ctx->metadata.comment));

  /* If we have chunked encoding, skip the chunk size and assume, that
     the whole data is in one chunk */

  var = bgav_http_header_get_var(header, "Transfer-Encoding");
  if(var && !strcasecmp(var, "chunked"))
    p->chunked = 1;
  else
    ctx->do_buffer = 1;
  return 1;
  }

/*
  int chunk_size;
  int chunk_buffer_size;
  int chunk_buffer_alloc;
  uint8_t * chunk_buffer;
*/

static int read_chunk(bgav_input_context_t* ctx)
  {
  int fd;
  fd_set rset;
  struct timeval timeout;
  unsigned long chunk_size;
  int bytes_read;
  int result;
  http_priv * p = (http_priv *)(ctx->priv);
  fd = bgav_http_get_fd(p->h);

  //  fprintf(stderr, "Read chunk...");
    
  /* We first check if there is data availble, after that, the whole
     chunk is read at once */

  if(ctx->read_timeout)
    {
    FD_ZERO(&rset);
    FD_SET (fd, &rset);
    timeout.tv_sec  = ctx->read_timeout / 1000;
    timeout.tv_usec = (ctx->read_timeout % 1000) * 1000;
    if(select (fd+1, &rset, NULL, NULL, &timeout) <= 0)
      return 0;
    }

  /* Read Chunk size */
  
  if(!bgav_read_line_fd(fd, &(p->chunk_buffer), &(p->chunk_buffer_alloc),
                        ctx->read_timeout))
    return 0;

  chunk_size = strtoul(p->chunk_buffer, NULL, 16);

  if(!chunk_size)
    return 0;

  chunk_size += 2;
  
  /* Read chunk including trailing CRLF */

  if(chunk_size > p->chunk_buffer_alloc)
    {
    p->chunk_buffer_alloc = chunk_size + 512;
    p->chunk_buffer = realloc(p->chunk_buffer,
                              p->chunk_buffer_alloc);
    }
  
  bytes_read = 0;

  while(bytes_read < chunk_size)
    {
    result = read(fd, p->chunk_buffer + bytes_read,
                  chunk_size - bytes_read);
    if(!result)
      break;
    bytes_read += result;
    }

  p->chunk_buffer_size = chunk_size;
  p->chunk_size = chunk_size;

  //  fprintf(stderr, "done, size: %d\n", p->chunk_size);
  //  bgav_hexdump(p->chunk_buffer, p->chunk_size, 16);
  return bytes_read;
  }

static int read_data_chunked(bgav_input_context_t* ctx,
                             uint8_t * buffer, int len)
  {
  int bytes_read = 0;
  int bytes_to_copy;
    
  http_priv * p = (http_priv *)(ctx->priv);
  while(bytes_read < len)
    {
    //    fprintf(stderr, "read_data_chunked %d %d\n",
    //            bytes_read, p->chunk_size);

    if(!p->chunk_buffer_size)
      {
      if(!read_chunk(ctx))
        return bytes_read;
      }

    bytes_to_copy = len - bytes_read;
    if(bytes_to_copy > p->chunk_buffer_size)
      bytes_to_copy = p->chunk_buffer_size;

    memcpy(buffer + bytes_read, p->chunk_buffer +
           (p->chunk_size - p->chunk_buffer_size),
           bytes_to_copy);
    bytes_read += bytes_to_copy;
    p->chunk_buffer_size -= bytes_to_copy;
    
    }
  return bytes_read;
  }

static int read_data(bgav_input_context_t* ctx,
                     uint8_t * buffer, int len, int block)
  {
  int fd;
  int result;
  int bytes_read = 0;
  fd_set rset;
  struct timeval timeout;
  http_priv * p = (http_priv *)(ctx->priv);

  if(p->chunked)
    return read_data_chunked(ctx, buffer, len);

  fd = bgav_http_get_fd(p->h);
  
  //  fprintf(stderr, "Read data 1\n");
  
  if(!block)
    {
    FD_ZERO(&rset);
    FD_SET (fd, &rset);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;

    if(select (fd+1, &rset, NULL, NULL, &timeout) > 0)
      return read(fd, buffer, len);
    else
      return 0;
    }
  else if(ctx->read_timeout)
    {
    FD_ZERO(&rset);
    FD_SET (fd, &rset);
    
    timeout.tv_sec  = ctx->read_timeout / 1000;
    timeout.tv_usec = (ctx->read_timeout % 1000) * 1000;
            
    if(select (fd+1, &rset, NULL, NULL, &timeout) > 0)
      bytes_read = read(fd, buffer, len);
    else
      return 0;
    }
  if(!bytes_read)
    return 0;

  //  fprintf(stderr, "Read data 2: %d\n", bytes_read);
    
  while(bytes_read < len)
    {
    result = read(fd, buffer + bytes_read, len - bytes_read);
    //    fprintf(stderr, "Read data 3: %d\n", result);
    if(!result)
      return bytes_read;
    
    if(result <= 0)
      {
      FD_ZERO(&rset);
      FD_SET (fd, &rset);
      
      timeout.tv_sec  = ctx->read_timeout / 1000;
      timeout.tv_usec = (ctx->read_timeout % 1000) * 1000;
      if(select (fd+1, &rset, NULL, NULL, &timeout) <= 0)
        return bytes_read;
      }
    else
      bytes_read += result;
    }
  return bytes_read;
  }

static int read_shoutcast_metadata(bgav_input_context_t* ctx, int block)
  {
  char * meta_buffer, * meta_name;
  const char * pos, *end_pos;
  uint8_t icy_len;
  int meta_bytes;

  if(!read_data(ctx, &icy_len, 1, block))
    {
    return 0;
    }
  meta_bytes = icy_len * 16;
  
  //  fprintf(stderr, "Metadata bytes: %d\n", meta_bytes);
  
  if(meta_bytes)
    {
    meta_buffer = malloc(meta_bytes);
        
    /* Metadata block is read in blocking mode!! */
    
    if(read_data(ctx, meta_buffer, meta_bytes, 1) < meta_bytes)
      return 0;
    
    //    fprintf(stderr, "Meta buffer:\n");
    //    bgav_hexdump(meta_buffer, meta_bytes, 16);
    
    if(ctx->name_change_callback)
      {
      pos = meta_buffer;
      
      while(strncmp(pos, "StreamTitle='", 13))
        {
        pos = strchr(pos, ';');
        pos++;
        if(pos - meta_buffer >= meta_bytes)
          {
          pos = (char*)0;
          break;
          }
        }
      if(pos)
        {
        pos += 13;
        end_pos = strchr(pos, '\'');
        meta_name = bgav_strndup(pos, end_pos);
        ctx->name_change_callback(ctx->name_change_callback_data,
                                  meta_name);
        
        //            fprintf(stderr, "NAME CHANGED: %s\n", meta_name);
        free(meta_name);
        }
      }
    //        else
    //          fprintf(stderr, "Name changed callback is NULL!!!\n");
    free(meta_buffer);
    }
  return 1;
  }

static int do_read(bgav_input_context_t* ctx,
                   uint8_t * buffer, int len, int block)
  {
  int bytes_to_read;
  int bytes_read = 0;

  int result;
  http_priv * p = (http_priv *)(ctx->priv);

  //  fprintf(stderr, "Do read %d %d %d\n", len, p->icy_bytes, block);
  
  if(!p->icy_metaint) 
    return read_data(ctx, buffer, len, block);
  else
    {
    while(bytes_read < len)
      {
      /* Read data chunk */
            
      bytes_to_read = len - bytes_read;

      if(p->icy_bytes + bytes_to_read > p->icy_metaint)
        bytes_to_read = p->icy_metaint - p->icy_bytes;

      if(bytes_to_read)
        {
        result = read_data(ctx, buffer + bytes_read, bytes_to_read, block);
        bytes_read += result;
        p->icy_bytes += result;

        if(result < bytes_to_read)
          return bytes_read;
        }

      /* Read metadata */

      if(p->icy_bytes == p->icy_metaint)
        {
        if(!read_shoutcast_metadata(ctx, block))
          return bytes_read;
        else
          p->icy_bytes = 0;
        }
      }
    }
  return bytes_read;
  }

static int read_http(bgav_input_context_t* ctx,
                     uint8_t * buffer, int len)
  {
  return do_read(ctx, buffer, len, 1);
  }

static int read_nonblock_http(bgav_input_context_t * ctx,
                              uint8_t * buffer, int len)
  {
  return do_read(ctx, buffer, len, 0);
  }


static void close_http(bgav_input_context_t * ctx)
  {
  http_priv * p = (http_priv *)(ctx->priv);

  if(p->chunk_buffer)
    free(p->chunk_buffer);
  bgav_http_close(p->h);

  //  fprintf(stderr, "CLOSE HTTP\n");

  free(p);
  }

bgav_input_t bgav_input_http =
  {
    open:          open_http,
    read:          read_http,
    read_nonblock: read_nonblock_http,
    close:         close_http,
  };

