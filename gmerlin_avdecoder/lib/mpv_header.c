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

#include <inttypes.h>
#include <avdec_private.h>
#include <mpv_header.h>

#define LOG_DOMAIN "mpv_header"

int bgav_mpv_sequence_header_probe(const uint8_t * buffer)
  {
  uint32_t code;
  code = BGAV_PTR_2_32BE(buffer);
  if(code == 0x000001B3)
    return 1;
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
    {    -1,   -1 }, // reserved
    {    -1,   -1 }, // reserved 
    {    -1,   -1 }, // reserved
    {    -1,   -1 }, // reserved 
    {    -1,   -1 }, // reserved
    {    -1,   -1 }, // reserved
    {    -1,   -1 }, // reserved
  };

int bgav_mpv_sequence_header_parse(const bgav_options_t * opt,
                                   bgav_mpv_sequence_header_t * ret,
                                   const uint8_t * buffer, int len)
  {
  int frame_rate_index;
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
  
  frame_rate_index = buffer[3] & 0xf;
  ret->timescale      = framerates[frame_rate_index].timescale;
  ret->frame_duration = framerates[frame_rate_index].frame_duration;

  if(ret->timescale < 0)
    {
    bgav_log(opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Cannot read sequence header: illegal framerate");
    return -1;
    }
  
  
  ret->bitrate = (buffer[4]<<10)|(buffer[5]<<2)|(buffer[6]>>6);
  return 7;
  }

int bgav_mpv_sequence_extension_probe(const uint8_t * buffer)
  {
  uint32_t code;
  code = BGAV_PTR_2_32BE(buffer);
  if(code != 0x000001B5)
    return 0;
  if(buffer[4] >> 4 == 0x01)
    return 1;
  return 0;
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
  ret->bitrate_ext          = ((buffer[2] & 0x1F)<<7) | (buffer[3]>>1);
  ret->timescale_ext        = (buffer[5] >> 5) & 3;
  ret->frame_duration_ext   = (buffer[5] & 0x1f);
  return 6;
  }

int bgav_mpv_picture_header_probe(const uint8_t * buffer)
  {
  uint32_t code;
  code = BGAV_PTR_2_32BE(buffer);
  if(code == 0x00000100)
    return 1;
  return 0;
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
      ret->coding_type = MPV_CODING_TYPE_I;
      break;
    case 2:
      ret->coding_type = MPV_CODING_TYPE_P;
      break;
    case 3:
      ret->coding_type = MPV_CODING_TYPE_B;
      break;
    default:
      return -1; // Error
    }
  return 2;
  }



int bgav_mpv_picture_extension_probe(const uint8_t * buffer)
  {
  uint32_t code;
  code = BGAV_PTR_2_32BE(buffer);
  if(code != 0x000001B5)
    return 0;
  if(buffer[4] >> 4 == 0x08)
    return 1;
  return 0;
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
