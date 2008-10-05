/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2008 Members of the Gmerlin project
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

#include <avdec_private.h>

void bgav_bitstream_init(bgav_bitstream_t * b, const uint8_t * pos, 
                         int len)
  {
  b->pos = pos;
  b->end = pos + len;
  b->bit_cache = 8;
  }

int bgav_bitstream_get_long(bgav_bitstream_t * b, int64_t * ret1,  int bits)
  {
  int bits_read = 0;
  int bits_to_copy;
  int64_t ret = 0;

  while(bits_read < bits)
    {
    if(!b->bit_cache)
      {
      b->pos++;
      if(b->pos >= b->end)
        return 0;
      b->bit_cache = 8;
      }
    bits_to_copy = bits - bits_read;
    if(bits_to_copy > b->bit_cache)
      bits_to_copy = b->bit_cache;
    
    ret <<= bits_to_copy;
    ret |= ((*b->pos) >> (b->bit_cache-bits_to_copy)) & ((1<<bits_to_copy)-1);
    bits_read += bits_to_copy;
    b->bit_cache -= bits_to_copy;
    }
  *ret1 = ret;
  return 1;
  }

int bgav_bitstream_get(bgav_bitstream_t * b, int * ret,  int bits)
  {
  int64_t tmp;
  if(!bgav_bitstream_get_long(b, &tmp, bits))
    return 0;
  *ret = tmp;
  return 1;
  }
