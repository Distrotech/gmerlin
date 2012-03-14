/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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

typedef struct
  {
  uint16_t min_blocksize;
  uint16_t max_blocksize;

  uint32_t min_framesize;
  uint32_t max_framesize;

  uint32_t samplerate;
  int num_channels;
  int bits_per_sample;
  int64_t total_samples;

  uint8_t md5[16];
  } bgav_flac_streaminfo_t;

#define BGAV_FLAC_STREAMINFO_SIZE 34

int bgav_flac_streaminfo_read(const uint8_t * ptr, bgav_flac_streaminfo_t * ret);
void bgav_flac_streaminfo_dump(bgav_flac_streaminfo_t * si);

void bgav_flac_streaminfo_init_stream(bgav_flac_streaminfo_t * si, bgav_stream_t * s);

/* Frame header */

#define BGAV_FLAC_FRAMEHEADER_MIN (4+1+1)
#define BGAV_FLAC_FRAMEHEADER_MAX (4+7+2+2+1)

typedef struct
  {
  int blocking_strategy_code;
  int block_size_code;
  int samplerate_code;
  int channel_code;
  int samplesize_code;

  int64_t sample_number;

  /* (partly) Computed values */
  int samplerate;
  int blocksize;
  int samplesize;
  int num_channels;
    
  } bgav_flac_frame_header_t;

int bgav_flac_frame_header_read(const uint8_t * ptr,
                              bgav_flac_streaminfo_t * si,
                              bgav_flac_frame_header_t * ret);
