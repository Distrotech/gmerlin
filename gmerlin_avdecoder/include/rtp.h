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

#include <sdp.h>

extern bgav_demuxer_t bgav_demuxer_rtp;

int bgav_demuxer_rtp_open(bgav_demuxer_context_t * ctx,
                          bgav_sdp_t * sdp);
  

typedef struct
  {
  uint8_t version;
  uint8_t padding;
  uint8_t extension;
  uint8_t csrc_count;
  uint8_t marker;
  uint8_t payload_type;
  uint16_t sequence_number;
  uint32_t timestamp;
  uint32_t ssrc;
  uint32_t csrc_list[15];
  
  } rtp_header_t;

typedef struct
  {
  char * control_url;
  int rtp_fd;
  int rtcp_fd;
  
  char ** fmtp;
  int (*process)(bgav_stream_t * s, rtp_header_t * h, uint8_t * data, int len);

  union
    {
    struct
      {
      int sizelength;
      int indexlength;
      int indexdeltalength;
      } mpeg4_generic;
    
    } priv;

  } rtp_stream_priv_t;

