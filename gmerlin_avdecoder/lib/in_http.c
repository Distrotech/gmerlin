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
  
  } http_priv;

static char * title_vars[] =
  {
    "icy-name",
    "x-audiocast-name",
    (char*)0
  };

static char * genre_vars[] =
  {
    "x-audiocast-genre",
    "icy-genre",
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

  char * dummy = (char*)0;
  int dummy_alloc = 0;
    
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

  bgav_http_header_dump(header);
  
  var = bgav_http_header_get_var(header, "Content-length");
  if(var)
    ctx->total_bytes = atoi(var);

  var = bgav_http_header_get_var(header, "Content-Type");
  if(var)
    ctx->mimetype = bgav_strndup(var, NULL);
  else if(bgav_http_header_get_var(header, "icy-notice1"))
    ctx->mimetype = bgav_strndup("audio/mpeg", NULL);
  
  var = bgav_http_header_get_var(header, "Content-length");
  if(var)
    ctx->total_bytes = atoi(var);

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

  /* If we have chunked encoding, skip the chunk size and assume, that
     the whole data is in one chunk */

  var = bgav_http_header_get_var(header, "Transfer-Encoding");
  if(var && !strcasecmp(var, "chunked"))
    {
    if(!bgav_read_line_fd(bgav_http_get_fd(p->h),
                          &dummy, &dummy_alloc, ctx->read_timeout))
      return 0;
    free(dummy);    
    }
  ctx->do_buffer = 1;
  return 1;
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

static int do_read(bgav_input_context_t* ctx,
                   uint8_t * buffer, int len, int block)
  {
  int bytes_before_metadata;
  //  int bytes_after_metadata;
  int result1, result2;
  uint8_t icy_len;
  int meta_bytes;
  char * meta_buffer, * meta_name;
  const char * pos, *end_pos;

  //  fprintf(stderr, "Do read %d %d\n", len, block);
  
  http_priv * p = (http_priv *)(ctx->priv);
  if(!p->icy_metaint) 
    return read_data(ctx, buffer, len, block);
  else
    {
    if(p->icy_bytes + len > p->icy_metaint)
      {
      //      fprintf(stderr, "Read metadata %d %d %d\n",
      //              p->icy_metaint, p->icy_bytes, len);
      bytes_before_metadata = p->icy_metaint - p->icy_bytes;
      if(bytes_before_metadata)
        {
        result1 = read_data(ctx, buffer, bytes_before_metadata, block);
        }
      else
        result1 = 0;

      if(result1 < bytes_before_metadata)
        {
        p->icy_bytes += result1;
        return result1;
        }
      
      /* Now, parse the info */
            
      if(!read_data(ctx, &icy_len, 1, block))
        {
        p->icy_bytes += result1;
        return result1;
        }
      meta_bytes = icy_len * 16;

      if(meta_bytes)
        {
        meta_buffer = malloc(meta_bytes);
        
        /* Metadata block is read in blocking mode!! */
        
        if(read_data(ctx, meta_buffer, meta_bytes, 1) < meta_bytes)
          return result1;

        fprintf(stderr, "Meta buffer:\n");
        bgav_hexdump(meta_buffer, meta_bytes, 16);
        
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
      /* Read bytes after metadata */

      if(len - bytes_before_metadata > 0)
        {
        result2 = read_data(ctx, buffer + result1,
                            len - bytes_before_metadata, block);
        p->icy_bytes = result2;
        }
      else
        {
        p->icy_bytes = 0;
        result2 = 0;
        }
      return result1 + result2;
      }
    else
      {
      //      fprintf(stderr, "NO metadata %d %d %d\n",
      //              p->icy_metaint, p->icy_bytes, len);

      result1 = read_data(ctx, buffer, len, block);
      p->icy_bytes += result1;
      return result1;
      }
    }
  
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
  bgav_http_close(p->h);
  free(p);
  }

bgav_input_t bgav_input_http =
  {
    open:          open_http,
    read:          read_http,
    read_nonblock: read_nonblock_http,
    close:         close_http,
  };

