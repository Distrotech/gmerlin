/*****************************************************************
 * gmerlin-encoders - encoder plugins for gmerlin
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

#include <gavl/gavl.h>
#include <gavl/gavf.h>

#include <xing.h>

/*
 *  Lots of the stuff here was taken from gstxingmux.c from
 *  gstreamer
 */

#define MAXFRAMESIZE 2881

struct bg_xing_s
  {
  /* Frame sizes */

  uint32_t * frame_positions;
  int frame_positions_alloc;
  int num_frames;
  
  uint32_t total_bytes;
  
  uint32_t header;  
  
  int tag_bytes;
  int samples_per_frame;

  uint8_t buffer[MAXFRAMESIZE];
  
  };

#define PTR_2_32BE(p) \
((*(p) << 24) | \
(*(p+1) << 16) | \
(*(p+2) << 8) | \
*(p+3))

#define INT_32BE_2_PTR(i, p)                   \
(p)[3] = (i) & 0xff; \
(p)[2] = ((i)>>8) & 0xff; \
(p)[1] = ((i)>>16) & 0xff; \
(p)[0] = ((i)>>24) & 0xff;

static int
get_xing_offset (uint32_t header)
  {
  uint32_t mpeg_version = (header >> 19) & 0x3;
  uint32_t channel_mode = (header >> 6) & 0x3;

  if (mpeg_version == 0x3)
    {
    if (channel_mode == 0x3)
      {
      return 0x11;
      }
    else
      {
      return 0x20;
      }
    }
  else
    {
    if (channel_mode == 0x3)
      {
      return 0x09;
      }
    else
      {
      return 0x11;
      }
    }
  }

static const int mp3types_bitrates[2][3][16] =
  {
    {
      {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,},
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,},
      {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320,}
    },
    {
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256,},
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,},
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,}
    },
  };

static const int mp3types_freqs[3][3] =
  {
    {44100, 48000, 32000},
    {22050, 24000, 16000},
    {11025, 12000, 8000}
  };

static int
parse_header(uint32_t header, int * ret_size, int * ret_spf,
             int * ret_rate)
  {
  int length, spf;
  int samplerate, bitrate, layer, padding;
  int lsf, mpg25;

  /* No need for sanity checks, the input module should already do this */
  
  if (header & (1 << 20))
    {
    lsf = (header & (1 << 19)) ? 0 : 1;
    mpg25 = 0;
    }
  else
    {
    lsf = 1;
    mpg25 = 1;
    }
  
  layer = 4 - ((header >> 17) & 0x3);

  bitrate = (header >> 12) & 0xF;
  bitrate = mp3types_bitrates[lsf][layer - 1][bitrate] * 1000;
  if (bitrate == 0)
    return 0;
  
  samplerate = (header >> 10) & 0x3;
  samplerate = mp3types_freqs[lsf + mpg25][samplerate];

  padding = (header >> 9) & 0x1;

  switch (layer)
    {
    case 1:
      length = 4 * ((bitrate * 12) / samplerate + padding);
      break;
    case 2:
      length = (bitrate * 144) / samplerate + padding;
      break;
    default:
    case 3:
      length = (bitrate * 144) / (samplerate << lsf) + padding;
      break;
    }

  if (layer == 1)
    spf = 384;
  else if (layer == 2 || lsf == 0)
    spf = 1152;
  else
    spf = 576;

  if (ret_size)
    *ret_size = length;
  if (ret_spf)
    *ret_spf = spf;
  if (ret_rate)
    *ret_rate = samplerate;
  return 1;
  }


bg_xing_t * bg_xing_create(uint8_t * first_frame, int first_frame_len)
  {
  bg_xing_t * ret;
  int bitrate_index = 0;
  int xing_offset;
  ret = calloc(1, sizeof(*ret));

  /* Get final header */
  ret->header = PTR_2_32BE(first_frame);

  /* Switch off crc */
  ret->header |= 0x00010000;

  /* Get bitrate */

  do{
    bitrate_index++;
    
    ret->header &= 0xffff0fff;
    ret->header |= bitrate_index << 12;
    
    parse_header (ret->header, &ret->tag_bytes, &ret->samples_per_frame, NULL);
    xing_offset = get_xing_offset(ret->header);
  } while (ret->tag_bytes < (4 + xing_offset + 4 + 4 + 4 + 4 + 100) && bitrate_index < 0xe);
  
  return ret;
  }

void bg_xing_update(bg_xing_t * xing, int bytes)
  {
  if(xing->frame_positions_alloc < xing->num_frames + 1)
    {
    xing->frame_positions_alloc += 1024;
    xing->frame_positions = realloc(xing->frame_positions,
                                    xing->frame_positions_alloc *
                                    sizeof(*xing->frame_positions));
    }
  xing->frame_positions[xing->num_frames] = xing->total_bytes;
  xing->num_frames++;
  xing->total_bytes+= bytes;
  }

static const char xing_sig[4] = "Xing";

int bg_xing_write(bg_xing_t * xing, gavf_io_t * out)
  {
  uint32_t tmp;
  uint64_t tmp_64;
  int i;
  uint8_t * ptr;
  
  if(xing->num_frames)
    {
    /* Finalize tag */
    ptr = xing->buffer;

    INT_32BE_2_PTR(xing->header, ptr); ptr += 4;

    ptr += get_xing_offset(xing->header);

    memcpy(ptr, xing_sig, 4); ptr += 4;

    /* Flags */
    tmp = 7; // FRAMES_FLAG | BYTES_FLAG | TOC_FLAG
    INT_32BE_2_PTR(tmp, ptr); ptr += 4;

    /* Num frames */
    tmp = xing->num_frames;
    INT_32BE_2_PTR(tmp, ptr); ptr += 4;
    
    /* Num bytes */
    tmp = xing->total_bytes;
    INT_32BE_2_PTR(tmp, ptr); ptr += 4;

    /* Seek table */
    for(i = 0; i < 100; i++)
      {
      tmp_64 = xing->frame_positions[(i * xing->num_frames) / 100];

      //      fprintf(stderr, "Seek entry %d: %ld ", i, tmp_64);
      
      tmp_64 *= 256;
      tmp_64 /= xing->total_bytes;

      //      fprintf(stderr, "%ld\n",tmp_64);
      
      *ptr = tmp_64;
      ptr++;
      }
    }
  if(gavf_io_write_data(out, xing->buffer, xing->tag_bytes) < xing->tag_bytes)
    return 0;
  return 1;
  
  }

void bg_xing_destroy(bg_xing_t * xing)
  {
  if(xing->frame_positions)
    free(xing->frame_positions);
  free(xing);
  }
