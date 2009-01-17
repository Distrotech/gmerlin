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

#define PARSER_CACHE_MAX 16

#define PARSER_CONTINUE     (PARSER_PRIV+0)

typedef void (*init_func)(bgav_video_parser_t*);

typedef int (*parse_func)(bgav_video_parser_t*);
typedef void (*cleanup_func)(bgav_video_parser_t*);
typedef void (*reset_func)(bgav_video_parser_t*);

typedef struct
  {
  int coding_type;
  int size;
  int duration;
  int64_t pts;
  int64_t position;
  
  int skip;
  
  int field_pic;
  int field2_offset;
  } cache_t;

typedef struct
  {
  int64_t packet_position;
  int     parser_position;
  } packet_t;

struct bgav_video_parser_s
  {
  int raw;
  bgav_bytebuffer_t buf;
  
  int flags;
  int pos;
  
  parse_func parse;
  cleanup_func cleanup;
  reset_func reset;

  const bgav_options_t * opt;
  
  /* Extradata */
  uint8_t * header;
  int header_len;
  
  /* Private data for parsers */
  void * priv;
  
  /* Raw byte offset of the start of the parser buffer */
  int64_t raw_position;
  
  /* Timescales */
  int in_scale;

  int timescale;
  int frame_duration;

  /* Cache */
  cache_t cache[PARSER_CACHE_MAX];
  int cache_size;

  /* Packets */
  packet_t * packets;
  int packets_alloc;
  int num_packets;
  
  int low_delay;
  
  int64_t timestamp;

  int eof;
  
  int non_b_count;

  int got_coding_type;
  };

void bgav_video_parser_init_mpeg12(bgav_video_parser_t * parser);
void bgav_video_parser_init_h264(bgav_video_parser_t * parser);

void bgav_video_parser_flush(bgav_video_parser_t * parser, int bytes);
void bgav_video_parser_extract_header(bgav_video_parser_t * parser);
void bgav_video_parser_set_coding_type(bgav_video_parser_t * parser, int type);
int bgav_video_parser_check_output(bgav_video_parser_t * parser);

/* Notify the parser of a new picture */
int bgav_video_parser_set_picture_start(bgav_video_parser_t * parser);

