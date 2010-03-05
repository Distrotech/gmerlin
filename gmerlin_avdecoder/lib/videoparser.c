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

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <avdec_private.h>
#include <parser.h>
#include <videoparser_priv.h>
#include <utils.h>

// #define DUMP_INPUT
// #define DUMP_OUTPUT

static const struct
  {
  uint32_t fourcc;
  init_func func;
  }
parsers[] =
  {
    { BGAV_MK_FOURCC('H', '2', '6', '4'), bgav_video_parser_init_h264 },
    { BGAV_MK_FOURCC('m', 'p', 'g', 'v'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'p', 'v', '1'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'p', 'v', '2'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'p', '4', 'v'), bgav_video_parser_init_mpeg4 },
    { BGAV_MK_FOURCC('C', 'A', 'V', 'S'), bgav_video_parser_init_cavs },
    { BGAV_MK_FOURCC('V', 'C', '-', '1'), bgav_video_parser_init_vc1 },
    { BGAV_MK_FOURCC('d', 'r', 'a', 'c'), bgav_video_parser_init_dirac },
  };

int bgav_video_parser_supported(uint32_t fourcc)
  {
  int i;
  for(i = 0; i < sizeof(parsers)/sizeof(parsers[0]); i++)
    {
    if(parsers[i].fourcc == fourcc)
      return 1;
    }
  return 0;
  }


bgav_video_parser_t *
bgav_video_parser_create(bgav_stream_t * s)
  {
  int i;
  bgav_video_parser_t * ret;
  /* First find a parse function */
  
  init_func func = NULL;
  for(i = 0; i < sizeof(parsers)/sizeof(parsers[0]); i++)
    {
    if(parsers[i].fourcc == s->fourcc)
      {
      func = parsers[i].func;
      break;
      }
    }

  if(!func)
    return NULL;
  
  ret = calloc(1, sizeof(*ret));
  ret->timestamp = BGAV_TIMESTAMP_UNDEFINED;
  ret->last_non_b_frame = -1;
  ret->raw_position = -1;
  ret->max_ref_frames = 2;
  ret->format = &s->data.video.format;
  ret->s = s;

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

  if(!parser->cache_size)
    return;
  
  if(!parser->cache[parser->cache_size-1].size)
    {
    total = 0;
    for(i = 0; i < parser->cache_size-1; i++)
      total += parser->cache[i].size;
    parser->cache[parser->cache_size-1].size = parser->pos - total;
    }

  /* If we don't have a header yet, drop the cached pictures */
  if(!parser->header)
    {
    i = parser->cache_size;
    for(i = 0; i < parser->cache_size; i++)
      bgav_video_parser_flush(parser,
                              parser->cache[i].size);
    parser->cache_size = 0;
    parser->last_non_b_frame = -1;
    parser->non_b_count = 0;
    return;
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
#if 0
    fprintf(stderr, "Merged field pics %d\n",
            parser->cache_size);
#endif
    }
  
  }

#define SET_PTS(index) \
  { \
  if(parser->timestamp == BGAV_TIMESTAMP_UNDEFINED) \
    { \
    if(parser->cache[index].in_pts != BGAV_TIMESTAMP_UNDEFINED) \
      parser->timestamp = gavl_time_rescale(parser->s->timescale, \
                                            parser->format->timescale, \
                                            parser->cache[index].in_pts); \
    else \
      parser->timestamp = 0; \
    } \
  parser->cache[index].pts = parser->timestamp; \
  parser->timestamp += parser->cache[index].duration; \
  }

void bgav_video_parser_set_coding_type(bgav_video_parser_t * parser, int type)
  {
  //  int i, start, end;
  
  if((parser->cache_size >= 2) &&
     !parser->cache[parser->cache_size-2].field_pic)
    {
#if 0
    fprintf(stderr, "Set pts %d %c\n",
            parser->cache[parser->cache_size-2].duration,
            parser->cache[parser->cache_size-2].coding_type);
#endif
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

int bgav_video_parser_check_output(bgav_video_parser_t * parser)
  {
#if 0
  fprintf(stderr, "check output cs: %d fp: %d sz: %d pts: %lld skip: %d\n",
          parser->cache_size, parser->cache[0].field_pic,
          parser->cache[0].size, parser->cache[0].pts,
          parser->cache[0].skip);
#endif    
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

void bgav_video_parser_reset(bgav_video_parser_t * parser, int64_t in_pts, int64_t out_pts)
  {
  bgav_bytebuffer_flush(&parser->buf);
  
  parser->raw_position = -1;
  parser->cache_size = 0;
  parser->num_packets = 0;
  parser->eof = 0;

  if(in_pts != BGAV_TIMESTAMP_UNDEFINED)
    parser->timestamp = gavl_time_rescale(parser->s->timescale,
                                          parser->format->timescale, in_pts);
  else if(out_pts != BGAV_TIMESTAMP_UNDEFINED)
    parser->timestamp = out_pts;
  else
    parser->timestamp = BGAV_TIMESTAMP_UNDEFINED;
  
  parser->pos = 0;
  parser->non_b_count = 0;
  parser->last_non_b_frame = -1;
  
  if(parser->reset)
    parser->reset(parser);
  }


int bgav_video_parser_max_ref_frames(bgav_video_parser_t * parser)
  {
  return parser->max_ref_frames;
  }


const uint8_t *
bgav_video_parser_get_header(bgav_video_parser_t * parser,
                             int * header_len)
  {
  *header_len = parser->header_len;
  return parser->header;
  }

int bgav_video_parser_set_header(bgav_video_parser_t * parser,
                                 const uint8_t * header, int len)
  {
  //  fprintf(stderr, "Got header %d bytes\n", len);
  //  bgav_hexdump(header, len, 16);
  
  if(!parser->parse_header)
    return 0;
  
  parser->header = malloc(len);
  memcpy(parser->header, header, len);
  parser->header_len = len;
  
  if(parser->parse_header)
    parser->parse_header(parser);
  return 1;
  }

void bgav_video_parser_set_eof(bgav_video_parser_t * parser)
  {
  //  fprintf(stderr, "EOF buf: %d %d %d\n", parser->buf.size, parser->pos,
  //          parser->cache_size);
  /* Set size of last frame */
  parser->pos = parser->buf.size;
  bgav_video_parser_set_sequence_end(parser);
  parser->eof = 1;
  }

void bgav_video_parser_set_sequence_end(bgav_video_parser_t * parser)
  {
  int i;

  //  fprintf(stderr, "Set Sequence end\n");

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
    if((parser->cache[i].pts == BGAV_TIMESTAMP_UNDEFINED) &&
       (parser->cache[i].coding_type == BGAV_CODING_TYPE_B))
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
  parser->timestamp = BGAV_TIMESTAMP_UNDEFINED;
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

  if(parser->s->flags & STREAM_PARSE_FULL)
    {
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
    }
  else if(parser->s->flags & STREAM_PARSE_FRAME)
    {
    int type;
    /* Need a picture in cache without the coding type set */
    if(parser->cache[parser->cache_size-1].coding_type)
      return PARSER_NEED_DATA;

    result =
      parser->parse_frame(parser, &type,
                          &parser->cache[parser->cache_size-1].duration);

    //    fprintf(stderr, "Coding type: %c\n", type);

    if(result != PARSER_DISCARD)
      bgav_video_parser_set_coding_type(parser, type);
    
    if(result == PARSER_HAVE_HEADER)
      return result;
    else if(result == PARSER_DISCARD)
      {
      parser->cache_size--;
      bgav_bytebuffer_flush(&parser->buf);
      return PARSER_NEED_DATA;
      }
    else if(bgav_video_parser_check_output(parser))
      return PARSER_HAVE_PACKET;
    else
      return PARSER_NEED_DATA;
    }
  
  /* Never get here */
  return PARSER_ERROR;
  }


void bgav_video_parser_add_packet(bgav_video_parser_t * parser,
                                  bgav_packet_t * p)
  {
  cache_t * c;
#ifdef DUMP_INPUT
  bgav_dprintf("Add packet (c: %d) ", parser->cache_size);
  bgav_packet_dump(p);
  //  bgav_hexdump(p->data, 128, 16);
#endif
  /* Update cache */

  if(parser->s->flags & STREAM_PARSE_FULL)
    {
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
    parser->packets[parser->num_packets].pts  = p->pts;
    parser->num_packets++;
    }
  else if(parser->s->flags & STREAM_PARSE_FRAME)
    {
    /* Reserve cache entry */
    if(parser->cache_size >= PARSER_CACHE_MAX)
      {
      fprintf(stderr, "Picture cache full\n");
      return;
      }
    parser->cache_size++;
    c = &parser->cache[parser->cache_size-1];
    memset(c, 0, sizeof(*c));
    c->pts = BGAV_TIMESTAMP_UNDEFINED;
    c->duration = parser->format->frame_duration;
    c->size = p->data_size;
    c->position = p->position;
    c->in_pts = p->pts;
    /* Set position to the start of the frame */
    parser->pos = parser->buf.size;
    parser->packet_duration = p->duration;
    }
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
  if(!bytes)
    return;
  
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
  p->tc  = c->tc;
  
  p->duration = c->duration;

  p->flags = 0;
  
  if((c->coding_type == BGAV_CODING_TYPE_I) ||
     ((parser->flags & PARSER_NO_I_FRAMES) &&
      (c->coding_type == BGAV_CODING_TYPE_P)))
    PACKET_SET_KEYFRAME(p);

  PACKET_SET_CODING_TYPE(p, c->coding_type);

  if(c->skip)
    p->flags |= PACKET_FLAG_SKIP;
  
  p->position = c->position;
  p->field2_offset = c->field2_offset;
  p->valid = 1;

#ifdef DUMP_OUTPUT
  bgav_dprintf("Get packet ");
  bgav_packet_dump(p);
  //  bgav_dprintf("recovery_point %d\n", c->recovery_point);
#endif
  bgav_packet_pad(p);
  
  //  fprintf(stderr, "Get packet %c %ld\n", c->coding_type, p->pts);
  
  parser->cache_size--;
  if(parser->cache_size)
    memmove(&parser->cache[0], &parser->cache[1],
            sizeof(parser->cache[0]) * parser->cache_size);
  parser->last_non_b_frame--;

  
  }

void bgav_video_parser_set_framerate(bgav_video_parser_t * parser,
                                     int timescale, int frame_duration)
  {
  int i;

  if(!parser->format->timescale || !parser->format->frame_duration)
    {
    parser->format->timescale = timescale;
    parser->format->frame_duration = frame_duration;
    
    /*
     *  Frame duration is set by bgav_video_parser_set_picture_start(),
     *  which will be before the global header for most formats (i.e.
     *  when frame duration isn't known yet).
     */
    
    for(i = 0; i < parser->cache_size ; i++)
      parser->cache[i].duration = parser->format->frame_duration;
    }
  
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
  c->tc = GAVL_TIMECODE_UNDEFINED;
  c->recovery_point = -1;
  c->duration = parser->format->frame_duration;
  
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
        parser->cache[parser->cache_size-1].in_pts =
          parser->packets[i].pts;
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

void bgav_video_parser_set_header_end(bgav_video_parser_t * parser)
  {
  }
