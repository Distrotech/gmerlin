/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2012 Members of the Gmerlin project
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
#include <adts_header.h>

#define IS_ADTS(h) ((h[0] == 0xff) && \
                    ((h[1] & 0xf0) == 0xf0) && \
                    ((h[1] & 0x06) == 0x00))

static const int adts_samplerates[] =
  {96000,88200,64000,48000,44100,
   32000,24000,22050,16000,12000,
   11025,8000,7350,0,0,0};

int bgav_adts_header_read(const uint8_t * data,
                          bgav_adts_header_t * ret)
  {
  int protection_absent;

  if(!IS_ADTS(data))
    return 0;
  
  if(data[1] & 0x08)
    ret->mpeg_version = 2;
  else
    ret->mpeg_version = 4;

  protection_absent = data[1] & 0x01;

  ret->profile = (data[2] & 0xC0) >> 6;

  ret->samplerate_index = (data[2]&0x3C)>>2;
  ret->samplerate = adts_samplerates[ret->samplerate_index];
  
  ret->channel_configuration = ((data[2]&0x01)<<2)|((data[3]&0xC0)>>6);

  ret->frame_bytes = ((((unsigned int)data[3] & 0x3)) << 11)
    | (((unsigned int)data[4]) << 3) | (data[5] >> 5);
  
  ret->num_blocks = (data[6] & 0x03) + 1;
  return 1;
  }

void bgav_adts_header_dump(const bgav_adts_header_t * adts)
  {
  bgav_dprintf( "ADTS\n");
  bgav_dprintf( "  MPEG Version:          %d\n", adts->mpeg_version);
  bgav_dprintf( "  Profile:               ");
  
  if(adts->mpeg_version == 2)
    {
    switch(adts->profile)
      {
      case 0:
        bgav_dprintf( "MPEG-2 AAC Main profile\n");
        break;
      case 1:
        bgav_dprintf( "MPEG-2 AAC Low Complexity profile (LC)\n");
        break;
      case 2:
        bgav_dprintf( "MPEG-2 AAC Scalable Sample Rate profile (SSR)\n");
        break;
      case 3:
        bgav_dprintf( "MPEG-2 AAC (reserved)\n");
        break;
      }
    }
  else
    {
    switch(adts->profile)
      {
      case 0:
        bgav_dprintf( "MPEG-4 AAC Main profile\n");
        break;
      case 1:
        bgav_dprintf( "MPEG-4 AAC Low Complexity profile (LC)\n");
        break;
      case 2:
        bgav_dprintf( "MPEG-4 AAC Scalable Sample Rate profile (SSR)\n");
        break;
      case 3:
        bgav_dprintf( "MPEG-4 AAC Long Term Prediction (LTP)\n");
        break;
      }
    }
  bgav_dprintf( "  Samplerate:            %d\n", adts->samplerate);
  bgav_dprintf( "  Channel configuration: %d\n", adts->channel_configuration);
  bgav_dprintf( "  Frame bytes:           %d\n", adts->frame_bytes);
  bgav_dprintf( "  Num blocks:            %d\n", adts->num_blocks);
  }
