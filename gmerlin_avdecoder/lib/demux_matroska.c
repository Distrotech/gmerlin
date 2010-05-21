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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avdec_private.h>
#include <matroska.h>

#define LOG_DOMAIN "demux_matroska"

static int probe_matroska(bgav_input_context_t * input)
  {
  uint32_t header;

  if(!bgav_input_get_32_be(input, &header))
    return 0;

  if(header == 0x1a45dfa3) // EBML signature
    return 1;
  
  return 0;
  }

static int open_matroska(bgav_demuxer_context_t * ctx)
  {
  bgav_mkv_element_t e;

  bgav_mkv_ebml_header_t h;

  if(!bgav_mkv_ebml_header_read(ctx->input, &h))
    return 0;

  bgav_mkv_ebml_header_dump(&h);
  
  /* Get the first segment */
  
  while(1)
    {
    if(!bgav_mkv_element_read(ctx->input, &e))
      return 0;

    if(e.id == MKV_ID_SEGMENT)
      break;

    else
      bgav_input_skip(ctx->input, e.size);
    }

  /* Check next */
  if(!bgav_mkv_element_read(ctx->input, &e))
    return 0;
  bgav_mkv_element_dump(&e);
      
  
  return 0;
  }

const const bgav_demuxer_t bgav_demuxer_matroska =
  {
    .probe =       probe_matroska,
    .open =        open_matroska,
    // .select_track = select_track_matroska,
    // .next_packet = next_packet_matroska,
    // .seek =        seek_matroska,
    // .resync  =     resync_matroska,
    // .close =       close_matroska
  };
