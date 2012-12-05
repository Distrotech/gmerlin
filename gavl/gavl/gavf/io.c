#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gavfprivate.h>

void gavf_io_init(gavf_io_t * ret,
                  gavf_read_func  r,
                  gavf_write_func w,
                  gavf_seek_func  s,
                  gavf_close_func c,
                  gavf_flush_func f,
                  void * priv)
  {
  memset(ret, 0, sizeof(*ret));
  ret->read_func = r;
  ret->write_func = w;
  ret->seek_func = s;
  ret->close_func = c;
  ret->flush_func = f;
  ret->priv = priv;
  }


gavf_io_t * gavf_io_create(gavf_read_func  r,
                           gavf_write_func w,
                           gavf_seek_func  s,
                           gavf_close_func c,
                           gavf_flush_func f,
                           void * priv)
  {
  gavf_io_t * ret;
  ret = malloc(sizeof(*ret));
  if(!ret)
    return NULL;
  gavf_io_init(ret, r, w, s, c, f, priv);
  return ret;
  }


void gavf_io_destroy(gavf_io_t * io)
  {
  if(io->close_func)
    io->close_func(io->priv);
  free(io);
  }

void gavf_io_flush(gavf_io_t * io)
  {
  if(io->flush_func)
    io->flush_func(io->priv);
  }

int gavf_io_read_data(gavf_io_t * io, uint8_t * buf, int len)
  {
  int ret;
  if(!io->read_func)
    return 0;
  ret = io->read_func(io->priv, buf, len);
  if(ret > 0)
    io->position += ret;
  return ret;
  }

int gavf_io_write_data(gavf_io_t * io, const uint8_t * buf, int len)
  {
  int ret;
  if(!io->write_func)
    return 0;
  ret = io->write_func(io->priv, buf, len);
  if(ret > 0)
    io->position += ret;
  return ret;
  }

void gavf_io_skip(gavf_io_t * io, int bytes)
  {
  if(io->seek_func)
    io->position = io->seek_func(io->priv, bytes, SEEK_CUR);
  else
    {
    int i;
    uint8_t c;
    for(i = 0; i < bytes; i++)
      {
      if(gavf_io_read_data(io, &c, 1) < 1)
        break;
      io->position++;
      }
    }
  }

int64_t gavf_io_seek(gavf_io_t * io, int64_t pos, int whence)
  {
  if(!io->seek_func)
    return -1;
  io->position = io->seek_func(io->priv, pos, whence);
  return io->position;
  }


/* Fixed size integers (BE) */

int gavf_io_write_uint64f(gavf_io_t * io, uint64_t num)
 {
  uint8_t buf[8];
  buf[7] = num & 0xff;
  buf[6] = (num>>8) & 0xff;
  buf[5] = (num>>16) & 0xff;
  buf[4] = (num>>24) & 0xff;
  buf[3] = (num>>32) & 0xff;
  buf[2] = (num>>40) & 0xff;
  buf[1] = (num>>48) & 0xff;
  buf[0] = (num>>56) & 0xff;
  return (gavf_io_write_data(io, buf, 8) < 8) ? 0 : 1;
  }

int gavf_io_read_uint64f(gavf_io_t * io, uint64_t * num)
  {
  uint8_t buf[8];
  if(gavf_io_read_data(io, buf, 8) < 8)
    return 0;

  *num =
    ((uint64_t)buf[0] << 56 ) |
    ((uint64_t)buf[1] << 48 ) |
    ((uint64_t)buf[2] << 40 ) |
    ((uint64_t)buf[3] << 32 ) |
    ((uint64_t)buf[4] << 24 ) |
    ((uint64_t)buf[5] << 16 ) |
    ((uint64_t)buf[6] << 8 ) |
    ((uint64_t)buf[7]);
  return 1;
  }

static int write_uint32f(gavf_io_t * io, uint32_t num)
  {
  uint8_t buf[4];
  buf[3] = num & 0xff;
  buf[2] = (num>>8) & 0xff;
  buf[1] = (num>>16) & 0xff;
  buf[0] = (num>>24) & 0xff;
  return (gavf_io_write_data(io, buf, 4) < 4) ? 0 : 1;
  }

static int read_uint32f(gavf_io_t * io, uint32_t * num)
  {
  uint8_t buf[4];
  if(gavf_io_read_data(io, buf, 4) < 4)
    return 0;
  *num =
    ((uint32_t)buf[0] << 24 ) ||
    ((uint32_t)buf[1] << 16 ) ||
    ((uint32_t)buf[2] << 8 ) ||
    ((uint32_t)buf[3]);
  return 1;
  }


/*
 *  Integer formats:
 *
 *  7 bit (0..127, -64..63)
 *  1xxx xxxx
 *
 *  14 bit (0..16383, -8192..8193)
 *  01xx xxxx xxxx xxxx
 *
 *  21 bit (0..2097151, -1048576..1048575)
 *  001x xxxx xxxx xxxx xxxx xxxx
 *
 *  28 bit (0..268435455, -134217728..134217727)
 *  0001 xxxx xxxx xxxx xxxx xxxx xxxx xxxx
 *
 *  35 bit
 *  0000 1xxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
 *
 *  42 bit
 *  0000 01xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
 *
 *  49 bit
 *  0000 001x xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
 *
 *  56 bit
 *  0000 0001 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
 *
 *  64 bit
 *  0000 0000 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
 */

static const int64_t int_offsets[] =
  {
    ((uint64_t)1)<<6,
    ((uint64_t)1)<<13,
    ((uint64_t)1)<<20,
    ((uint64_t)1)<<27,
    ((uint64_t)1)<<34,
    ((uint64_t)1)<<41,
    ((uint64_t)1)<<48,
    ((uint64_t)1)<<55,
  };

static const uint64_t uint_limits[] =
  {
    ((uint64_t)1)<<7,
    ((uint64_t)1)<<14,
    ((uint64_t)1)<<21,
    ((uint64_t)1)<<28,
    ((uint64_t)1)<<35,
    ((uint64_t)1)<<42,
    ((uint64_t)1)<<49,
    ((uint64_t)1)<<56,
  };

static int get_len_uint(uint64_t num)
  {
  int i;
  for(i = 0; i < 8; i++)
    {
    if(num < uint_limits[i])
      return i+1;
    }
  return 9;
  }

static int get_len_int(int64_t num)
  {
  int i;
  for(i = 0; i < 8; i++)
    {
    if((num < int_offsets[i]) && (num >= -int_offsets[i]))
      return i+1;
    }
  return 9;
  }

static int get_len_read(uint8_t num)
  {
  int i;
  for(i = 0; i < 8; i++)
    {
    if(num & (0x80 >> i))
      return i+1;
    }
  return 9;
  }

static int write_uint64v(gavf_io_t * io, uint64_t num, int len)
  {
  int idx;
  uint8_t buf[9];

  /* Length byte */
  buf[0] = 0x80 >> (len-1);

  idx = len - 1;

  while(idx)
    {
    buf[idx] = num & 0xff;
    num >>= 8;
    idx--;
    }

  if(len < 8)
    buf[0] |= (num & (0xff >> len));
  
  return (gavf_io_write_data(io, buf, len) < len) ? 0 : 1;
  }

int gavf_io_write_uint64v(gavf_io_t * io, uint64_t num)
  {
  return write_uint64v(io, num, get_len_uint(num));
  }

int gavf_io_write_int64v(gavf_io_t * io, int64_t num)
  {
  uint64_t num_u;
  int len = get_len_int(num);
  
  if(len == 9)
    num_u = num ^ 0x8000000000000000LL;
  else
    num_u = num + int_offsets[len-1];
  return write_uint64v(io, num_u, len);
  }

static int read_uint64v(gavf_io_t * io, uint64_t * num, int * len)
  {
  int i;
  int len1;
  uint8_t buf[9];

  if(!gavf_io_read_data(io, buf, 1))
    return 0;
  len1 = get_len_read(buf[0]);
  
  if(len1 > 1)
    {
    if(gavf_io_read_data(io, &buf[1], len1 - 1) < len1 - 1)
      return 0;
    }
  
  *num = buf[0] & (0xff >> len1);

  for(i = 1; i < len1; i++)
    {
    *num <<= 8;
    *num |= buf[i];
    }
  if(len)
    *len = len1;
  return 1;
  }
  
int gavf_io_read_int64v(gavf_io_t * io, int64_t * num)
  {
  int len;
  if(!read_uint64v(io, (uint64_t*)num, &len))
    return 0;

  if(len == 9)
    *num ^= 0x8000000000000000LL;
  else
    *num -= int_offsets[len-1];
  
  return 1;
  }


int gavf_io_read_uint64v(gavf_io_t * io, uint64_t * num)
  {
  return read_uint64v(io, num, NULL);
  }

int gavf_io_write_uint32v(gavf_io_t * io, uint32_t num)
  {
  return gavf_io_write_uint64v(io, num);
  }

int gavf_io_read_uint32v(gavf_io_t * io, uint32_t * num)
  {
  uint64_t ret;

  if(!gavf_io_read_uint64v(io, &ret))
    return 0;
  *num = ret;
  return 1;
  }

int gavf_io_write_int32v(gavf_io_t * io, int32_t num)
  {
  return gavf_io_write_int64v(io, num);
  }

int gavf_io_read_int32v(gavf_io_t * io, int32_t * num)
  {
  int64_t ret = 0;
  if(!gavf_io_read_int64v(io, &ret))
    return 0;
  *num = ret;
  return 1;
  }

/* int <-> float conversion routines taken from
   ffmpeg */

static float int2flt(int32_t v)
  {
  if((uint32_t)v+v > 0xFF000000U)
    return NAN;
  return ldexp(((v&0x7FFFFF) + (1<<23)) * (v>>31|1), (v>>23&0xFF)-150);
  }

static int32_t flt2int(float d)
  {
  int e;
  if     ( !d) return 0;
  else if(d-d) return 0x7F800000 + ((d<0)<<31) + (d!=d);
  d= frexp(d, &e);
  return (d<0)<<31 | (e+126)<<23 | (int64_t)((fabs(d)-0.5)*(1<<24));
  }

int gavf_io_read_float(gavf_io_t * io, float * num)
  {
  uint32_t val;
  if(!read_uint32f(io, &val))
    return 0;
  *num = int2flt(val);
  return 1;
  }

int gavf_io_write_float(gavf_io_t * io, float num)
  {
  uint32_t val = flt2int(num);
  return write_uint32f(io, val);
  }


int gavf_io_read_string(gavf_io_t * io, char ** ret)
  {
  uint32_t len;
  
  if(!gavf_io_read_uint32v(io, &len))
    return 0;

  if(!len)
    {
    *ret = NULL;
    return 1;
    }
  *ret = malloc(len + 1);
  if(!ret)
    return 0;

  if(gavf_io_read_data(io, (uint8_t*)(*ret), len) < len)
    return 0;

  /* Zero terminate */
  (*ret)[len] = '\0';
  return 1;
  }

int gavf_io_write_string(gavf_io_t * io, const char * str)
  {
  uint32_t len;

  if(!str)
    return gavf_io_write_uint32v(io, 0);
  
  len = strlen(str);
  if(!gavf_io_write_uint32v(io, len) ||
     gavf_io_write_data(io, (const uint8_t*)str, len) < len)
    return 0;
  return 1;
  }

int gavf_io_read_buffer(gavf_io_t * io, gavf_buffer_t * ret)
  {
  uint32_t len;
  
  if(!gavf_io_read_uint32v(io, &len) ||
     !gavf_buffer_alloc(ret, len) ||
     (gavf_io_read_data(io, ret->buf, len) < len))
    return 0;
  ret->len = len;
  return 1;
  }

int gavf_io_write_buffer(gavf_io_t * io, const gavf_buffer_t * buf)
  {
  if(!gavf_io_write_uint32v(io, buf->len) ||
     (gavf_io_write_data(io, buf->buf, buf->len) < buf->len))
    return 0;
  return 1;
  }
