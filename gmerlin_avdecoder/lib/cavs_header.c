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
#include <cavs_header.h>

int bgav_cavs_get_start_code(const uint8_t * data)
  {
  switch(data[3])
    {
    case 0xb0:
      return CAVS_CODE_SEQUENCE;
      break;
    case 0xb3:
      return CAVS_CODE_PICTURE_I;
      break;
    case 0xb6:
      return CAVS_CODE_PICTURE_PB;
      break;
    }
  return 0;
  }

int bgav_cavs_sequence_header_read(const bgav_options_t * opt,
                                   bgav_cavs_sequence_header_t * ret,
                                   const uint8_t * buffer, int len)
  {
  bgav_bitstream_t b;
  int dummy;
  
  buffer+=4;
  len -= 4;
  
  bgav_bitstream_init(&b, buffer, len);
  
  if(!bgav_bitstream_get(&b, &ret->profile_id,           8) ||
     !bgav_bitstream_get(&b, &ret->level_id,             8) ||
     !bgav_bitstream_get(&b, &ret->progressive_sequence, 1) ||
     !bgav_bitstream_get(&b, &ret->horizontal_size,      14) ||
     !bgav_bitstream_get(&b, &ret->vertical_size,        14) ||
     !bgav_bitstream_get(&b, &ret->chromat_fromat,       2) ||
     !bgav_bitstream_get(&b, &ret->sample_precision,     3) ||
     !bgav_bitstream_get(&b, &ret->aspect_ratio,         4) ||
     !bgav_bitstream_get(&b, &ret->frame_rate_code,      4) ||
     !bgav_bitstream_get(&b, &ret->bit_rate_lower,       18) ||
     !bgav_bitstream_get(&b, &dummy,                     1) ||
     !bgav_bitstream_get(&b, &ret->bit_rate_upper,       12) ||
     !bgav_bitstream_get(&b, &ret->low_delay,            1))
    return 0;

  return len - bgav_bitstream_get_bits(&b) / 8;
  }

void bgav_cavs_sequence_header_dump(bgav_cavs_sequence_header_t * h)
  {
  bgav_dprintf("CAVS Sequence header\n");
  bgav_dprintf("  profile_id:           %d\n", h->profile_id);
  bgav_dprintf("  level_id:             %d\n", h->level_id);
  bgav_dprintf("  progressive_sequence: %d\n", h->progressive_sequence);
  bgav_dprintf("  horizontal_size:      %d\n", h->horizontal_size);
  bgav_dprintf("  vertical_size:        %d\n", h->vertical_size);
  bgav_dprintf("  chromat_fromat:       %d\n", h->chromat_fromat);
  bgav_dprintf("  sample_precision:     %d\n", h->sample_precision);
  bgav_dprintf("  aspect_ratio:         %d\n", h->aspect_ratio);
  bgav_dprintf("  frame_rate_code:      %d\n", h->frame_rate_code);
  bgav_dprintf("  bit_rate_lower:       %d\n", h->bit_rate_lower);
  /* market_bit */
  bgav_dprintf("  bit_rate_upper:       %d\n", h->bit_rate_upper);
  bgav_dprintf("  low_delay:            %d\n", h->low_delay);
  }

int bgav_cavs_picture_header_read(const bgav_options_t * opt,
                                  bgav_cavs_picture_header_t * ret,
                                  const uint8_t * buffer, int len)
  {

  }

void bgav_cavs_picture_header_dump(bgav_cavs_picture_header_t * h)
  {
  
  }

