#include <stdlib.h>
#include <string.h>

#include <gavfprivate.h>


#define PAD_SIZE(s, m) \
  s = ((s + m - 1) / m) * m

void gavf_buffer_init(gavf_buffer_t * buf)
  {
  memset(buf, 0, sizeof(*buf));
  }

void
gavf_buffer_init_static(gavf_buffer_t * buf, uint8_t * data, int size)
  {
  gavf_buffer_init(buf);
  buf->alloc = size;
  buf->alloc_static = size;
  buf->buf = data;
  }

void gavf_buffer_reset(gavf_buffer_t * buf)
  {
  buf->len = 0;
  buf->pos = 0;
  }

int gavf_buffer_alloc(gavf_buffer_t * buf,
                      int size)
  {
  if(buf->alloc < size)
    {
    if(buf->alloc_static)
      return 0;
    
    buf->alloc = size;
    PAD_SIZE(buf->alloc, 1024);
    buf->buf = realloc(buf->buf, buf->alloc);
    if(!buf->buf)
      return 0;
    }
  return 1;
  }

void gavf_buffer_free(gavf_buffer_t * buf)
  {
  if(buf->buf && !buf->alloc_static)
    free(buf->buf);
  }

/* Buffer as io */

static int read_buf(void * priv, uint8_t * data, int len)
  {
  gavf_buffer_t * buf = priv;
  if(len > buf->len - buf->pos)
    len = buf->len - buf->pos;
  
  if(len > 0)
    {
    memcpy(data, buf->buf + buf->pos, len);
    buf->pos += len;
    }
  return len;
  }



static int write_buf(void * priv, const uint8_t * data, int len)
  {
  gavf_buffer_t * buf = priv;

  if(!gavf_buffer_alloc(buf, buf->len + len))
    return 0;
  
  memcpy(buf->buf + buf->len, data, len);
  buf->len += len;
  return len;
  }


gavf_io_t * gavf_io_create_buf_read(gavf_buffer_t * buf)
  {
  return gavf_io_create(read_buf,
                        NULL,
                        NULL,
                        NULL,
                        buf);
  }

gavf_io_t * gavf_io_create_buf_write(gavf_buffer_t * buf)
  {
  return gavf_io_create(NULL,
                        write_buf,
                        NULL,
                        NULL,
                        buf);
  
  }

void gavf_io_init_buf_read(gavf_io_t * io, gavf_buffer_t * buf)
  {
  gavf_io_init(io,
               read_buf,
               NULL,
               NULL,
               NULL,
               buf);
  }

void gavf_io_init_buf_write(gavf_io_t * io, gavf_buffer_t * buf)
  {
  gavf_io_init(io,
               NULL,
               write_buf,
               NULL,
               NULL,
               buf);
  }
