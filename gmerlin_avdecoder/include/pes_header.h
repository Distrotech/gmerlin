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

/* MPEG-1/2 PES packet */

typedef struct
  {
  int64_t pts;
  int64_t dts;
  int stream_id;     /* For private streams: substream_id | (stream_id << 8)   */
  int payload_size;  /* To be read from the input after bgav_pes_header_read() */
  } bgav_pes_header_t;

int bgav_pes_header_read(bgav_input_context_t * input,
                         bgav_pes_header_t * ret);

void bgav_pes_header_dump(bgav_pes_header_t * p);

