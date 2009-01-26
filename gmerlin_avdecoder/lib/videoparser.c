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

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <avdec_private.h>
#include <videoparser.h>
#include <videoparser_priv.h>

/* Init functions (come below) */

// static void init_h264(bgav_video_parser_t*);
// static void init_mpeg12(bgav_video_parser_t*);

static const struct
  {
  uint32_t fourcc;
  init_func func;
  }
parsers[] =
  {
    { BGAV_MK_FOURCC('H', '2', '6', '4'), bgav_video_parser_init_h264 },
    { BGAV_MK_FOURCC('m', 'p', 'g', 'v'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'p', '4', 'v'), bgav_video_parser_init_mpeg4 },
    { BGAV_MK_FOURCC('C', 'A', 'V', 'S'), bgav_video_parser_init_cavs },
    { BGAV_MK_FOURCC('V', 'C', '-', '1'), bgav_video_parser_init_vc1 },
  };

bgav_video_parser_t *
bgav_video_parser_create(uint32_t fourcc, int timescale,
                         const bgav_options_t * opt)
  {
  int i;
  bgav_video_parser_t * ret;
  /* First find a parse function */
  
  init_func func = NULL;
  for(i = 0; i < sizeof(parsers)/sizeof(parsers[0]); i++)
    {
    if(parsers[i].fourcc == fourcc)
      {
      func = parsers[i].func;
      break;
      }
    }

  if(!func)
    return NULL;
  
  ret = calloc(1, sizeof(*ret));
  ret->in_scale = timescale;
  
  ret->last_non_b_frame = -1;
  ret->raw_position = -1;
  func(ret);
  return ret;
  }


void bgav_video_parser_extract_header(bgav_video_parser_t * parser)
  {
  parser->header_len = parser->pos;
  parser->header = malloc(parser->header_len);
  memcpy(parser->header, parser->buf.buffer, parser->header_len);
  }

static void update_previous_size(bgav_video_parser_t * parser)
  {
  int i, total;
  if(parser->cache_size && !parser->cache[parser->cache_size-1].size)
    {
    total = 0;
    for(i = 0; i < parser->cache_size-1; i++)
      total += parser->cache[i].size;
    parser->cache[parser->cache_size-1].size = parser->pos - total;
    }

  /* If 2 previous frames are field pictures, merge them */

  if((parser->cache_size >= 2) &&
     (parser->cache[parser->cache_size-1].field_pic) &&
     (parser->cache[parser->cache_size-2].field_pic))
    {
    parser->cache[parser->cache_size-2].field2_offset =
      parser->cache[parser->cache_size-2].size;
    parser->cache[parser->cache_size-2].size +=
      parser->cache[parser->cache_size-1].size;
    parser->cache[parser->cache_size-2].field_pic = 0;

    if(parser->cache[parser->cache_size-1].coding_type !=
       BGAV_CODING_TYPE_B)
      parser->non_b_count--;
    
    parser->cache_size--;
    fprintf(stderr, "Merged field pics %d\n",
            parser->cache_size);
    }
  
  }

#define SET_PTS(index) \
  { \
  parser->cache[index].pts = parser->timestamp; \
  parser->timestamp += parser->cache[index].duration; \
  }

void bgav_video_parser_set_coding_type(bgav_video_parser_t * parser, int type)
  {
  //  int i, start, end;
  
  //  fprintf(stderr, "Set coding type %c %d\n", type,
  //          parser->cache[parser->cache_size-1].skip);
  
  if((parser->cache_size >= 2) &&
     !parser->cache[parser->cache_size-2].field_pic)
    {
    //    fprintf(stderr, "Set pts %d %c\n",
    //            parser->cache[parser->cache_size-2].duration,
    //            parser->cache[parser->cache_size-2].coding_type);
    /* Set timestamp of the previous frame */
    if(parser->cache[parser->cache_size-2].coding_type != BGAV_CODING_TYPE_B)
      {
      if(parser->non_b_count == 1)
        {
        SET_PTS(parser->cache_size-2);
        }
      else
        {
        if(parser->last_non_b_frame >= 0)
          SET_PTS(parser->last_non_b_frame);
        parser->last_non_b_frame = parser->cache_size-2;
        }
      }
    else if(!parser->cache[parser->cache_size-2].skip)
      SET_PTS(parser->cache_size-2);
    }
  
  parser->cache[parser->cache_size-1].coding_type = type;
  
  if(type != BGAV_CODING_TYPE_B)
    parser->non_b_count++;
  else if(parser->non_b_count < 2)
    parser->cache[parser->cache_size-1].skip = 1;
  
  
  }

#if 0     
static void calc_timestamps(bgav_video_parser_t * parser)
  {
  int i;

  if(!parser->cache_size ||
     parser->cache[0].pts != BGAV_TIMESTAMP_UNDEFINED)
    return;

  /* Need at least 2 frames */
  if((parser->cache_size < 2) && !(parser->eof))
    return;

  /* Low delay or last frame */  
  if(parser->low_delay || (parser->cache_size == 1))
    {
    parser->cache[0].pts = parser->timestamp;
    parser->timestamp += parser->cache[0].duration;
    return;
    }

  /* I P */
  if((parser->cache[0].coding_type != BGAV_CODING_TYPE_B) && 
     ((parser->cache[1].coding_type != BGAV_CODING_TYPE_B) ||
      parser->cache[1].skip))
    {
    parser->cache[0].pts = parser->timestamp;
    parser->timestamp += parser->cache[0].duration;
    return;
    }
  
  /* P B B P */
  
  if((parser->cache[0].coding_type != BGAV_CODING_TYPE_B) && 
     (parser->cache[1].coding_type == BGAV_CODING_TYPE_B) &&
     ((parser->cache[parser->cache_size-1].coding_type !=
       BGAV_CODING_TYPE_B) || parser->eof))
    {
    i = 1;
    
    while(parser->cache[i].coding_type == BGAV_CODING_TYPE_B)
      {
      if(!parser->cache[i].skip)
        {
        parser->cache[i].pts = parser->timestamp;
        parser->timestamp += parser->cache[i].duration;
        }
      i++;
      }
    parser->cache[0].pts = parser->timestamp;
    parser->timestamp += parser->cache[0].duration;
    //    fprintf(stderr, "** PTS: %ld\n", parser->cache[0].pts);
    }
  
  }

#endif

int bgav_video_parser_check_output(bgav_video_parser_t * parser)
  {
  if(parser->cache_size && !parser->cache[0].field_pic &&
     parser->cache[0].size &&
     ((parser->cache[0].pts != BGAV_TIMESTAMP_UNDEFINED) ||
      parser->cache[0].skip))
    return 1;
  return 0;
  }

void bgav_video_parser_destroy(bgav_video_parser_t * parser)
  {
  if(parser->cleanup)
    parser->cleanup(parser);
  if(parser->header)
    free(parser->header);
  if(parser->packets)
    free(parser->packets);

  bgav_bytebuffer_free(&parser->buf);
  free(parser);
  }

void bgav_video_parser_reset(bgav_video_parser_t * parser, int64_t pts)
  {
  bgav_bytebuffer_flush(&parser->buf);
  
  parser->raw_position = -1;
  parser->cache_size = 0;
  parser->num_packets = 0;
  parser->eof = 0;
  parser->timestamp = pts;
  parser->pos = 0;
  parser->non_b_count = 0;
  parser->last_non_b_frame = -1;
  if(parser->reset)
    parser->reset(parser);
  }

const gavl_video_format_t *
bgav_video_parser_get_format(bgav_video_parser_t * parser)
  {
  return &parser->format;
  }

const uint8_t *
bgav_video_parser_get_header(bgav_video_parser_t * parser,
                             int * header_len)
  {
  *header_len = parser->header_len;
  return parser->header;
  }

void bgav_video_parser_set_eof(bgav_video_parser_t * parser)
  {
  int start = -1, i;
  fprintf(stderr, "EOF buf: %d %d %d\n", parser->buf.size, parser->pos,
          parser->cache_size);
  /* Set size of last frame */
  parser->pos = parser->buf.size;
  update_previous_size(parser);

  /* Remove incomplete cache entries */
  for(i = 0; i < parser->cache_size; i++)
    {
    if(!parser->cache[i].coding_type)
      {
      parser->cache_size = i;
      break;
      }
    }
  
  /* Set final timestamps */

  for(i = 0; i < parser->cache_size; i++)
    {
    if((parser->cache[parser->cache_size-1].pts ==
        BGAV_TIMESTAMP_UNDEFINED) &&
       (parser->cache[parser->cache_size-1].coding_type ==
        BGAV_CODING_TYPE_B))
      SET_PTS(i);
    }

  if(parser->last_non_b_frame >= 0)
    SET_PTS(parser->last_non_b_frame);
  
  for(i = 0; i < parser->cache_size; i++)
    {
    if(parser->cache[i].pts ==
       BGAV_TIMESTAMP_UNDEFINED)
      SET_PTS(i);
    }
#if 0  
  if(parser->cache[parser->cache_size-1].coding_type == BGAV_CODING_TYPE_B)
    {
    SET_PTS(parser->cache_size-1);
    if(parser->last_non_b_frame >= 0)
      SET_PTS(parser->last_non_b_frame);
    }
  else
    SET_PTS(parser->cache_size-1);
#endif
  parser->eof = 1;
  }

int bgav_video_parser_parse(bgav_video_parser_t * parser)
  {
  int result;
  //  fprintf(stderr, "Parse %d\n", parser->cache_size);
  if(parser->eof && !parser->cache_size)
    return PARSER_EOF;
  
  if(bgav_video_parser_check_output(parser))
    return PARSER_HAVE_PACKET;
  
  if(!parser->buf.size)
    return PARSER_NEED_DATA;

  while(1)
    {
    result = parser->parse(parser);
    switch(result)
      {
      case PARSER_NEED_DATA:
      case PARSER_ERROR:
      case PARSER_HAVE_HEADER:
        return result;
        break;
      case PARSER_CONTINUE:
        if(bgav_video_parser_check_output(parser))
          return PARSER_HAVE_PACKET;
        break;
      }
    }
  /* Never get here */
  return PARSER_ERROR;
  }


void bgav_video_parser_add_packet(bgav_video_parser_t * parser,
                                  bgav_packet_t * p)
  {
  fprintf(stderr, "Add packet ");
  bgav_packet_dump(p);
  /* Update cache */
  
  if(parser->num_packets >= parser->packets_alloc)
    {
    parser->packets_alloc = parser->num_packets + 10;
    parser->packets       = realloc(parser->packets,
                                    parser->packets_alloc *
                                    sizeof(*parser->packets));
    }
  parser->packets[parser->num_packets].packet_position = p->position;
  parser->packets[parser->num_packets].parser_position = parser->buf.size;
  parser->packets[parser->num_packets].size = p->data_size;
  parser->num_packets++;
  bgav_bytebuffer_append_data(&parser->buf, p->data, p->data_size, 0);
  }

void bgav_video_parser_add_data(bgav_video_parser_t * parser,
                                uint8_t * data, int len, int64_t position)
  {
  parser->raw = 1;
  bgav_bytebuffer_append_data(&parser->buf, data, len, 0);
  if(parser->raw_position < 0)
    parser->raw_position = position;
  }

void bgav_video_parser_flush(bgav_video_parser_t * parser, int bytes)
  {
  bgav_bytebuffer_remove(&parser->buf, bytes);
  parser->pos -= bytes;
  if(parser->pos < 0)
    parser->pos = 0;
  if(parser->raw)
    parser->raw_position += bytes;
  else
    {
    int num_remove, i;
    for(i = 0; i < parser->num_packets; i++)
      parser->packets[i].parser_position -= bytes;

    num_remove = 0;
    for(i = 0; i < parser->num_packets; i++)
      {
      if(parser->packets[i].parser_position + parser->packets[i].size <= 0)
        num_remove++;
      else
        break;
      }

    if(num_remove)
      {
      if(parser->num_packets - num_remove)
        memmove(parser->packets, parser->packets + num_remove,
                sizeof(*parser->packets) * (parser->num_packets - num_remove));
      parser->num_packets -= num_remove;
      }
    }
  }

void bgav_video_parser_get_packet(bgav_video_parser_t * parser,
                                  bgav_packet_t * p)
  {
  cache_t * c;
  c = &parser->cache[0];
  
  bgav_packet_alloc(p, c->size);
  memcpy(p->data, parser->buf.buffer, c->size);
  p->data_size = c->size;
    
  bgav_video_parser_flush(parser, c->size);

  p->pts = c->pts;
  p->dts = BGAV_TIMESTAMP_UNDEFINED;

  p->duration = c->duration;

  p->flags = 0;
  
  if(c->coding_type == BGAV_CODING_TYPE_I)
    PACKET_SET_KEYFRAME(p);

  PACKET_SET_CODING_TYPE(p, c->coding_type);
  
  p->position = c->position;
  p->field2_offset = c->field2_offset;
  p->valid = 1;

  fprintf(stderr, "Get packet %c ", c->coding_type);

  //  fprintf(stderr, "Get packet %c %ld\n", c->coding_type, p->pts);
  
  parser->cache_size--;
  if(parser->cache_size)
    memmove(&parser->cache[0], &parser->cache[1],
            sizeof(parser->cache[0]) * parser->cache_size);
  parser->last_non_b_frame--;

  bgav_packet_dump(p);
  bgav_packet_pad(p);
  
  }

void bgav_video_parser_set_framerate(bgav_video_parser_t * parser,
                                     int timescale, int frame_duration)
  {
  int i;
  parser->format.timescale = timescale;
  parser->format.frame_duration = frame_duration;

  /* Frame duration is set by bgav_video_parser_set_picture_start(), which will
   * be before the global header for most formats (i.e. when frame duration isn't known yet).
   */
  for(i = 0; i < parser->cache_size ; i++)
    parser->cache[i].duration = parser->format.frame_duration;
  }

int bgav_video_parser_set_picture_start(bgav_video_parser_t * parser)
  {
  cache_t * c;
  int i;

  if(parser->cache_size >= PARSER_CACHE_MAX)
    {
    fprintf(stderr, "Picture cache full\n");
    return 0;
    }
  update_previous_size(parser);
  
  /* Reserve cache entry */
  parser->cache_size++;
  c = &parser->cache[parser->cache_size-1];
  memset(c, 0, sizeof(*c));
  c->pts = BGAV_TIMESTAMP_UNDEFINED;

  c->duration = parser->format.frame_duration;
  
  /* Set picture position */
  if(parser->raw)
    {
    int offset = 0;

    for(i = 0; i < parser->cache_size-1; i++)
      offset += parser->cache[i].size;
    parser->cache[parser->cache_size-1].position =
      parser->raw_position + offset;
    }
  else
    {
    for(i = 0; i < parser->num_packets; i++)
      {
      if((parser->packets[i].parser_position <= parser->pos) &&
         (parser->packets[i].parser_position + parser->packets[i].size > parser->pos))
        {
        parser->cache[parser->cache_size-1].position =
          parser->packets[i].packet_position;
        break;
        }
      }
    if(i == parser->num_packets)
      {
      fprintf(stderr, "Cannot find packet containing position %d\n", parser->pos);
      return 0;
      }
    }
  return 1;
  }
