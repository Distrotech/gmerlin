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

typedef struct
  {
  bgav_mkv_ebml_header_t ebml_header;
  bgav_mkv_segment_info_t segment_info;

  bgav_mkv_track_t * tracks;
  int num_tracks;
  
  } mkv_t;
 
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

  mkv_t * p;

  p = calloc(1, sizeof(*p));
  ctx->priv = p;
  
  if(!bgav_mkv_ebml_header_read(ctx->input, &p->ebml_header))
    return 0;

  bgav_mkv_ebml_header_dump(&p->ebml_header);
  
  /* Get the first segment */
  
  while(1)
    {
    if(!bgav_mkv_element_read(ctx->input, &e))
      return 0;

    if(e.id == MKV_ID_Segment)
      break;

    else
      bgav_input_skip(ctx->input, e.size);
    }

  while(1)
    {
    if(!bgav_mkv_element_read(ctx->input, &e))
      return 0;
    switch(e.id)
      {
      case MKV_ID_Info:
        if(!bgav_mkv_segment_info_read(ctx->input, &p->segment_info, &e))
          return 0;
        bgav_mkv_segment_info_dump(&p->segment_info);
        break;
      case MKV_ID_Tracks:
        if(!bgav_mkv_tracks_read(ctx->input, &p->tracks, &p->num_tracks, &e))
          return 0;
        break;
      default:
        bgav_log(ctx->opt, BGAV_LOG_WARNING, LOG_DOMAIN,
                 "Skipping %"PRId64" bytes of element %x\n", e.size, e.id);
        bgav_input_skip(ctx->input, e.size);
      }
    }
#if 0  
  /* Get segment information */
  
  if(!bgav_mkv_element_read(ctx->input, &e))
    return 0;
  bgav_mkv_element_dump(&e);
#endif
  
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
