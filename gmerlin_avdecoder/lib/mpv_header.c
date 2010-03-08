/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2010 Members of the Gmerlin project
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

#include <inttypes.h>
#include <avdec_private.h>
#include <mpv_header.h>

#define LOG_DOMAIN "mpv_header"

#if 0

/* Original version (from ffmpeg) */
const uint8_t * bgav_mpv_find_startcode( const uint8_t *p,
                                         const uint8_t *end )
  {
  const uint8_t *a = p + 4 - ((long)p & 3);
  
  for( end -= 3; p < a && p < end; p++ )
    {
    if( p[0] == 0 && p[1] == 0 && p[2] == 1 )
      return p;
    }
  
  for( end -= 3; p < end; p += 4 )
    {
    uint32_t x = *(const uint32_t*)p;
    if( (x - 0x01010101) & (~x) & 0x80808080 )
      { // generic
      if( p[1] == 0 )
        {
        if( p[0] == 0 && p[2] == 1 )
          return p;
        if( p[2] == 0 && p[3] == 1 )
          return p+1;
        }
      if( p[3] == 0 )
        {
        if( p[2] == 0 && p[4] == 1 )
          return p+2;
        if( p[4] == 0 && p[5] == 1 )
          return p+3;
        }
      }
    }
  
  for( end += 3; p < end; p++ )
    {
    if( p[0] == 0 && p[1] == 0 && p[2] == 1 )
      return p;
    }
  return NULL;
  }
#else

/* Optimized version 17% faster(for the complete index build)
   than the ffmpeg one */

static inline const uint8_t *
first_zero_byte(const uint8_t *p, int len)
  {
  int num, done = 0, i, j;

  /* Align pointer to 8 byte boundary */
  num = (unsigned long)(p) & 0x07;
  if(num > len)
    num = len;
  
  i = num+1;
  while(--i)
    {
    if(!(*p))
      return p;
    p++;
    }

  done += num;

  /* Main loop: Take 8 bytes at once */
  num = (len - done)/8;
  i = num+1;
  while(--i)
    {
    const uint64_t x = *(const uint64_t*)p;
    
    if((x - 0x0101010101010101LL) & (~x) & 0x8080808080808080LL)
      {
      j = 9;
      while(--j)
        {
        if(!(*p))
          return p;
        p++;
        }
      }
    else
      p += 8;
    }
  done += num * 8;

  /* Remainder */
  num = len - done;
  i = num+1;
  while(--i)
    {
    if(!(*p))
      return p;
    p++;
    }
  return NULL;
  }
  
const uint8_t * bgav_mpv_find_startcode( const uint8_t *p,
                                         const uint8_t *end )
  {
  const uint8_t * ptr;
  /* Subtract 3 because we want to get the *whole* code */
  int len = end - p - 3;

  if(len <= 0) /* Reached end */
    return NULL;
  
  while(1)
    {
    ptr = first_zero_byte(p, len);
    
    if(!ptr) /* Reached end */
      break;

    if((ptr[1] == 0x00) && (ptr[2] == 0x01))  /* Found startcode */
      return ptr;
    
    /* Skip this zero byte */
    p = ptr+1;

    len = end - p - 3;
    
    if(len <= 0) /* Reached end */
      break;
    }
  return NULL;
  }

#endif


int bgav_mpv_get_start_code(const uint8_t * data)
  {
  if(data[3] == 0xb3)
    return MPEG_CODE_SEQUENCE;
  else if(data[3] == 0xb5)
    {
    if(data[4] >> 4 == 0x01)
      return MPEG_CODE_SEQUENCE_EXT;
    else if(data[4] >> 4 == 0x08)
      return MPEG_CODE_PICTURE_EXT;
    }
  else if(data[3] == 0x00)
    return MPEG_CODE_PICTURE;
  else if(data[3] == 0xb8)
    return MPEG_CODE_GOP;
  else if(data[3] == 0xb7)
    return MPEG_CODE_END;
  else if((data[3] >= 0x01) && (data[3] <= 0xaf))
    return MPEG_CODE_SLICE;
  return 0;
  }


static const struct
  {
  int timescale;
  int frame_duration;
  }
framerates[] =
  {
    {    -1,   -1 }, // forbidden
    { 24000, 1001 }, // 24000:1001 (23,976...)
    {    24,    1 }, // 24
    {    25,    1 }, // 25
    { 30000, 1001 }, // 30000:1001 (29,97...)
    {    30,    1 }, // 30
    {    50,    1 }, // 50
    { 60000, 1001 }, // 60000:1001 (59,94...)
    {    60,    1 }, // 60
  // Xing's 15fps: (9)
    {   15,    1},
  // libmpeg3's "Unofficial economy rates": (10-13)
    {    5,    1},
    {   10,    1},
    {   12,    1},
    {   15,    1},
    {    -1,   -1 }, // reserved
  };

void bgav_mpv_get_framerate(int code, int * timescale, int *frame_duration)
  {
  *timescale      = framerates[code].timescale;
  *frame_duration = framerates[code].frame_duration;
  }

int bgav_mpv_sequence_header_parse(const bgav_options_t * opt,
                                   bgav_mpv_sequence_header_t * ret,
                                   const uint8_t * buffer, int len)
  {
  int i;
  buffer += 4;
  len -= 4;

  /*
   * 12 uimsbf horizontal_size_value
   * 12 uimsbf vertical_size_value
   *  4 uimsbf aspect_ratio_information
   *  4 uimsbf frame_rate_code
   * 18 uimsbf bit_rate_value
   *  1  bslbf marker_bit
   */
  
  if(len < 7)
    return 0;

  if((buffer[6] & 0x20) != 0x20)
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot read sequence header: missing marker bit");
    return -1;        /* missing marker_bit */
    }

  i = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
  
  ret->horizontal_size_value = i >> 12;
  ret->vertical_size_value = i & 0xfff;
    
  ret->frame_rate_index = buffer[3] & 0xf;

  //  ret->timescale      = framerates[frame_rate_index].timescale;
  //  ret->frame_duration = framerates[frame_rate_index].frame_duration;
  
  
  ret->bitrate = (buffer[4]<<10)|(buffer[5]<<2)|(buffer[6]>>6);
  return 7;
  }


int bgav_mpv_sequence_extension_parse(const bgav_options_t * opt,
                                      bgav_mpv_sequence_extension_t * ret,
                                      const uint8_t * buffer, int len)
  {
  buffer += 4;
  len -= 4;
  if(len < 6)
    return 0;
  ret->progressive_sequence = buffer[1] & (1 << 3);
  
  ret->horizontal_size_ext = ((buffer[1] << 13) | (buffer[2] << 5)) & 0x3000;
  ret->vertical_size_ext   = (buffer[2] << 7) & 0x3000;
  
  ret->bitrate_ext          = ((buffer[2] & 0x1F)<<7) | (buffer[3]>>1);
  ret->timescale_ext        = (buffer[5] >> 5) & 3;
  ret->frame_duration_ext   = (buffer[5] & 0x1f);
  ret->low_delay            = !!(buffer[5] & 0x80);
  
  return 6;
  }

int bgav_mpv_picture_header_parse(const bgav_options_t * opt,
                                  bgav_mpv_picture_header_t * ret,
                                  const uint8_t * buffer, int len)
  {
  int type;
  buffer += 4;
  len -= 4;

  if(len < 2)
    return 0;
  type = (buffer[1] >> 3) & 7;
  switch(type)
    {
    case 1:
      ret->coding_type = BGAV_CODING_TYPE_I;
      break;
    case 2:
      ret->coding_type = BGAV_CODING_TYPE_P;
      break;
    case 3:
      ret->coding_type = BGAV_CODING_TYPE_B;
      break;
    default:
      bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Cannot read picture header: Invalid coding type %d", type);
      return -1; // Error
    }
  return 2;
  }

int bgav_mpv_picture_extension_parse(const bgav_options_t * opt,
                                     bgav_mpv_picture_extension_t * ret,
                                     const uint8_t * buffer, int len)
  {
  buffer += 4;
  len -= 4;

  if(len < 5)
    return 0;
  
  ret->top_field_first    = buffer[3] & (1 << 7);
  ret->repeat_first_field = buffer[3] & (1 << 1);
  ret->progressive_frame  = buffer[4] & (1 << 7);
  return 5;
  }

int bgav_mpv_gop_header_parse(const bgav_options_t * opt,
                              bgav_mpv_gop_header_t * ret,
                              const uint8_t * buffer, int len)
  {
  buffer += 4;
  len -= 4;

  if(len < 4)
    return 0;
  
  /* Gop header has a fixed length of 27 bytes */

  ret->drop    = buffer[0] >> 7;
  ret->hours   = (buffer[0] >> 2) & 31;
  ret->minutes = ((buffer[0] << 4) | (buffer[1] >> 4)) & 63;
  ret->seconds = ((buffer[1] << 3) | (buffer[2] >> 5)) & 63;
  ret->frames  = ((buffer[2] << 1) | (buffer[3] >> 7)) & 63;
  
  return 1;
  }
