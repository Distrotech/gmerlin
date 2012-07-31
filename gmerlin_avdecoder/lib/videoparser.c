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

#include <stdlib.h>
#include <string.h>

#include <config.h>
#include <avdec_private.h>
#include <parser.h>
#include <videoparser_priv.h>
#include <utils.h>

// #define DUMP_INPUT
// #define DUMP_OUTPUT

/* DIVX (maybe with B-frames) requires special attention */

static const uint32_t video_codecs_divx[] =
  {
    BGAV_MK_FOURCC('D', 'I', 'V', 'X'),
    BGAV_MK_FOURCC('d', 'i', 'v', 'x'),
    BGAV_MK_FOURCC('D', 'X', '5', '0'),
    BGAV_MK_FOURCC('X', 'V', 'I', 'D'),
    BGAV_MK_FOURCC('x', 'v', 'i', 'd'),
    BGAV_MK_FOURCC('F', 'M', 'P', '4'),
    BGAV_MK_FOURCC('f', 'm', 'p', '4'),    
    0x00,
  };

int bgav_video_is_divx4(uint32_t fourcc)
  {
  int i = 0;
  while(video_codecs_divx[i])
    {
    if(video_codecs_divx[i] == fourcc)
      return 1;
    i++;
    }
  return 0;
  }

static const struct
  {
  uint32_t fourcc;
  init_func func;
  }
parsers[] =
  {
    { BGAV_MK_FOURCC('H', '2', '6', '4'), bgav_video_parser_init_h264 },
    { BGAV_MK_FOURCC('a', 'v', 'c', '1'), bgav_video_parser_init_h264 },
    { BGAV_MK_FOURCC('m', 'p', 'g', 'v'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'p', 'v', '1'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'p', 'v', '2'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'x', '3', 'p'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'x', '4', 'p'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'x', '5', 'p'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'x', '3', 'n'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'x', '4', 'n'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'x', '5', 'n'), bgav_video_parser_init_mpeg12 },
    { BGAV_MK_FOURCC('m', 'p', '4', 'v'), bgav_video_parser_init_mpeg4 },
    /* DivX Variants */
    { BGAV_MK_FOURCC('D', 'I', 'V', 'X'), bgav_video_parser_init_mpeg4 },
    { BGAV_MK_FOURCC('d', 'i', 'v', 'x'), bgav_video_parser_init_mpeg4 },
    { BGAV_MK_FOURCC('D', 'X', '5', '0'), bgav_video_parser_init_mpeg4 },
    { BGAV_MK_FOURCC('X', 'V', 'I', 'D'), bgav_video_parser_init_mpeg4 },
    { BGAV_MK_FOURCC('x', 'v', 'i', 'd'), bgav_video_parser_init_mpeg4 },
    { BGAV_MK_FOURCC('F', 'M', 'P', '4'), bgav_video_parser_init_mpeg4 },
    { BGAV_MK_FOURCC('f', 'm', 'p', '4'), bgav_video_parser_init_mpeg4 },
    /* */
    { BGAV_MK_FOURCC('C', 'A', 'V', 'S'), bgav_video_parser_init_cavs },
    { BGAV_MK_FOURCC('V', 'C', '-', '1'), bgav_video_parser_init_vc1 },
    { BGAV_MK_FOURCC('d', 'r', 'a', 'c'), bgav_video_parser_init_dirac },
    { BGAV_MK_FOURCC('m', 'j', 'p', 'a'), bgav_video_parser_init_mjpa },
    { BGAV_MK_FOURCC('j', 'p', 'e', 'g'), bgav_video_parser_init_jpeg },
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
    {
    if(bgav_check_fourcc(s->fourcc, bgav_dv_fourccs))
      func = bgav_video_parser_init_dv;
    else if(bgav_check_fourcc(s->fourcc, bgav_png_fourccs))
      func = bgav_video_parser_init_png;
    }
  
  if(!func)
    return NULL;
  
  ret = calloc(1, sizeof(*ret));
  ret->s = s;
  
  ret->timestamp = GAVL_TIME_UNDEFINED;
  ret->last_non_b_frame = -1;
  ret->raw_position = -1;
  ret->s->data.video.max_ref_frames = 2;
  ret->format = &s->data.video.format;

  ret->start_pos = -1;
  
  bgav_packet_source_copy(&ret->src, &s->src);

  if(s->flags & STREAM_PARSE_FULL)
    {
    s->src.get_func = bgav_video_parser_get_packet_parse_full;
    s->src.peek_func = bgav_video_parser_peek_packet_parse_full;
    s->flags |= STREAM_DTS_ONLY;
    }
  else if(s->flags & STREAM_PARSE_FRAME)
    {
    s->src.get_func = bgav_video_parser_get_packet_parse_frame;
    s->src.peek_func = bgav_video_parser_peek_packet_parse_frame;
    }
  s->src.data = ret;
  
  func(ret);
  return ret;
  }

void bgav_video_parser_extract_header(bgav_video_parser_t * parser)
  {
  parser->s->ext_size = parser->pos;
  parser->s->ext_data = malloc(parser->s->ext_size);
  memcpy(parser->s->ext_data, parser->buf.buffer, parser->s->ext_size);
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
  if(!parser->s->ext_data)
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

    if((parser->cache[parser->cache_size-2].ilace == GAVL_INTERLACE_BOTTOM_FIRST) &&
       (parser->cache[parser->cache_size-1].ilace == GAVL_INTERLACE_TOP_FIRST))
      parser->cache[parser->cache_size-2].ilace = GAVL_INTERLACE_BOTTOM_FIRST;
    else
      parser->cache[parser->cache_size-2].ilace = GAVL_INTERLACE_TOP_FIRST;
    
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
  if(parser->timestamp == GAVL_TIME_UNDEFINED) \
    { \
    if(parser->cache[index].in_pts != GAVL_TIME_UNDEFINED) \
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
     ((parser->cache[0].pts != GAVL_TIME_UNDEFINED) ||
      parser->cache[0].skip))
    return 1;
  return 0;
  }

void bgav_video_parser_destroy(bgav_video_parser_t * parser)
  {
  if(parser->cleanup)
    parser->cleanup(parser);
  if(parser->packets)
    free(parser->packets);
  if(parser->out_packet)
    bgav_packet_pool_put(parser->s->pp, parser->out_packet);
    
  bgav_bytebuffer_free(&parser->buf);
  free(parser);
  }

void bgav_video_parser_reset(bgav_video_parser_t * parser,
                             int64_t in_pts, int64_t out_pts)
  {
  bgav_bytebuffer_flush(&parser->buf);
  
  parser->raw_position = -1;
  parser->cache_size = 0;
  parser->num_packets = 0;
  parser->eof = 0;

  if(parser->out_packet)
    {
    bgav_packet_pool_put(parser->s->pp, parser->out_packet);
    parser->out_packet = NULL;
    }
  
  if(in_pts != GAVL_TIME_UNDEFINED)
    parser->timestamp = gavl_time_rescale(parser->s->timescale,
                                          parser->format->timescale, in_pts);
  else if(out_pts != GAVL_TIME_UNDEFINED)
    parser->timestamp = out_pts;
  else
    parser->timestamp = GAVL_TIME_UNDEFINED;
  
  parser->pos = 0;
  parser->non_b_count = 0;
  parser->last_non_b_frame = -1;
  
  if(parser->reset)
    parser->reset(parser);

  parser->have_sync = 0;
  }

void bgav_video_parser_set_eof(bgav_video_parser_t * parser)
  {
  //  fprintf(stderr, "EOF buf: %d %d %d\n", parser->buf.size, parser->pos,
  //          parser->cache_size);
  /* Set size of last frame */
  parser->pos = parser->buf.size;
  bgav_video_parser_set_sequence_end(parser, 0);
  parser->timestamp = GAVL_TIME_UNDEFINED;
  parser->eof = 1;
  }

void bgav_video_parser_set_sequence_end(bgav_video_parser_t * parser,
                                        int code_len)
  {
  int i;

  //  fprintf(stderr, "Set Sequence end\n");
  
  update_previous_size(parser);

  if((code_len > 0) && parser->cache_size)
    {
    parser->cache[parser->cache_size-1].sequence_end_pos =
      parser->cache[parser->cache_size-1].size - code_len;
    }

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
    if((parser->cache[i].pts == GAVL_TIME_UNDEFINED) &&
       (parser->cache[i].coding_type == BGAV_CODING_TYPE_B))
      SET_PTS(i);
    }

  if(parser->last_non_b_frame >= 0)
    SET_PTS(parser->last_non_b_frame);
  
  for(i = 0; i < parser->cache_size; i++)
    {
    if(parser->cache[i].pts ==
       GAVL_TIME_UNDEFINED)
      SET_PTS(i);
    }
  }
#if 0
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
          return result;
          break;
        case PARSER_CONTINUE:
          if(bgav_video_parser_check_output(parser))
            return PARSER_HAVE_PACKET;
          break;
        }
      }
    }
  /* Never get here */
  return PARSER_ERROR;
  }
#endif

int bgav_video_parser_parse_frame(bgav_video_parser_t * parser,
                                  bgav_packet_t * p)
  {
  int ret;
  if(!parser->parse_frame)
    return PARSER_ERROR;

#ifdef DUMP_INPUT
  bgav_dprintf("Parse frame input :");
  bgav_packet_dump(p);
#endif
  
  ret = parser->parse_frame(parser, p);

#ifdef DUMP_OUTPUT
  bgav_dprintf("Parse frame output:");
  bgav_packet_dump(p);
#endif
  
  return ret;
  }

void bgav_video_parser_add_packet(bgav_video_parser_t * parser,
                                  bgav_packet_t * p)
  {
  cache_t * c;
#ifdef DUMP_INPUT
  bgav_dprintf("Add packet ");
  bgav_packet_dump(p);
  //  bgav_hexdump(p->data, 128, 16);
#endif
  /* Update cache */

  if(parser->s->flags & STREAM_RAW_PACKETS)
    {
    parser->raw = 1;
    bgav_bytebuffer_append_data(&parser->buf, p->data, p->data_size, 0);
    if(parser->raw_position < 0)
      parser->raw_position = p->position;
    return;
    }
  
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
    c->pts = GAVL_TIME_UNDEFINED;
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
  p->dts = GAVL_TIME_UNDEFINED;
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
  p->header_size = c->header_size;
  p->sequence_end_pos = c->sequence_end_pos;
  p->ilace = c->ilace;
  
  bgav_packet_pad(p);
  
  //  fprintf(stderr, "Get packet %c %ld\n", c->coding_type, p->pts);
  
  parser->cache_size--;
  if(parser->cache_size)
    memmove(&parser->cache[0], &parser->cache[1],
            sizeof(parser->cache[0]) * parser->cache_size);
  parser->last_non_b_frame--;
  }

void bgav_video_parser_set_framerate(bgav_video_parser_t * parser)
  {
  int i;
  for(i = 0; i < parser->cache_size ; i++)
    parser->cache[i].duration = parser->format->frame_duration;
  
  if(!parser->s->timescale)
    parser->s->timescale = parser->format->timescale;
  
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
  c->pts = GAVL_TIME_UNDEFINED;
  c->tc = GAVL_TIMECODE_UNDEFINED;
  c->recovery_point = -1;
  c->duration = parser->format->frame_duration;
  c->ilace = GAVL_INTERLACE_NONE;
  
  c->parser_start_pos = parser->pos;
  c->header_size = 0;
  c->sequence_end_pos = 0;
  
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
  cache_t * c;
  c = &parser->cache[parser->cache_size-1];
  if(!c->header_size)
    c->header_size = parser->pos - c->parser_start_pos;
  }

/* New (generic) API */

#if 0

bgav_packet_t *
bgav_video_parser_get_packet_parse_full(void * parser1)
  {
  int result;

  bgav_video_parser_t * parser = parser1;
  bgav_packet_t * ret;
  bgav_packet_t * p;
  
  if(parser->out_packet)
    {
    ret = parser->out_packet;
    parser->out_packet = NULL;
#ifdef DUMP_OUTPUT
    bgav_dprintf("Get packet ");
    bgav_packet_dump(ret);
    //  bgav_dprintf("recovery_point %d\n", c->recovery_point);
#endif
    return ret;
    }

  while(1)
    {
    result = bgav_video_parser_parse(parser);
    switch(result)
      {
      case PARSER_EOF:
        return NULL;
        break;
      case PARSER_NEED_DATA:
        p = parser->src.get_func(parser->src.data);
        if(!p)
          {
          bgav_video_parser_set_eof(parser);
          continue;
          }
        else
          {
          bgav_video_parser_add_packet(parser, p);
          bgav_packet_pool_put(parser->s->pp, p);
          }
        break;
      case PARSER_HAVE_PACKET:
        ret = bgav_packet_pool_get(parser->s->pp);
        bgav_video_parser_get_packet(parser, ret);
#ifdef DUMP_OUTPUT
    bgav_dprintf("Get packet ");
    bgav_packet_dump(ret);
    
    //  bgav_dprintf("recovery_point %d\n", c->recovery_point);
#endif
        return ret;
        break;
      case PARSER_ERROR:
        return NULL;
      }
    }
  return NULL;
  }

bgav_packet_t *
bgav_video_parser_peek_packet_parse_full(void * parser1, int force)
  {
  int result;
  bgav_packet_t * p;
  bgav_video_parser_t * parser = parser1;
  if(parser->out_packet)
    {
#ifdef DUMP_OUTPUT
    bgav_dprintf("Peek packet ");
    bgav_packet_dump(parser->out_packet);
    //  bgav_dprintf("recovery_point %d\n", c->recovery_point);
#endif
    return parser->out_packet;
    }
  while(1)
    {
    result = bgav_video_parser_parse(parser);

    switch(result)
      {
      case PARSER_EOF:
        return NULL;
        break;
      case PARSER_NEED_DATA:
        p = parser->src.peek_func(parser->src.data, force);
        if(!p)
          {
          if(force)
            {
            bgav_video_parser_set_eof(parser);
            continue;
            }
          else
            return NULL;
          }
        else
          {
          p = parser->src.get_func(parser->src.data);
          bgav_video_parser_add_packet(parser, p);
          bgav_packet_pool_put(parser->s->pp, p);
          }
        break;
      case PARSER_HAVE_PACKET:
        parser->out_packet = bgav_packet_pool_get(parser->s->pp);
        bgav_video_parser_get_packet(parser, parser->out_packet);
#ifdef DUMP_OUTPUT
  bgav_dprintf("Peek packet ");
  bgav_packet_dump(parser->out_packet);
  //  bgav_dprintf("recovery_point %d\n", c->recovery_point);
#endif
        return parser->out_packet;
        break;
      case PARSER_ERROR:
        return NULL;
      }
    }
  }

/* Even newer API */
#else

static int get_input_packet(bgav_video_parser_t * parser, int force)
  {
  bgav_packet_t * p;

  if(!force && !parser->src.peek_func(parser->src.data, 0))
    return 0;
  
  p = parser->src.get_func(parser->src.data);
  if(!p)
    {
    bgav_video_parser_set_eof(parser);
    return 0;
    }
  else
    {
    bgav_video_parser_add_packet(parser, p);
    bgav_packet_pool_put(parser->s->pp, p);
    return 1;
    }
  
  }

static bgav_packet_t *
parse_next_packet(bgav_video_parser_t * parser, int force)
  {
  bgav_packet_t * ret;
  int skip = -1;
  int64_t pts;
  
  /* Synchronize */
  while(!parser->have_sync)
    {
    if(parser->find_frame_boundary(parser, &skip))
      {
      bgav_video_parser_flush(parser, parser->pos);
      parser->have_sync = 1;
      break;
      }
    if(!get_input_packet(parser, force))
      {
      // EOF while initializing the video parser
      return NULL;
      }
    force = 1;
    }
  
  /* Find frame end */
  while(1)
    {
    if(parser->find_frame_boundary(parser, &skip))
      break;
    if(!get_input_packet(parser, force))
      {
      // EOF: Take this packet as the last one
      parser->pos = parser->buf.size;
      break;
      }
    force = 1;
    }
  
  /* Set up packet */

  ret = bgav_packet_pool_get(parser->s->pp);
  bgav_packet_alloc(ret, parser->pos);
  memcpy(ret->data, parser->buf.buffer, parser->pos);
  ret->data_size = parser->pos;
  
  ret->position = parser->packets[0].packet_position;

  pts = parser->packets[0].pts;
  
  bgav_video_parser_flush(parser, parser->pos);

  /* set up position for next packet */
  parser->pos = skip;
  
  /* Parse frame */
  
  if(!parser->parse_frame(parser, ret))
    {
    /* Error */
    bgav_packet_pool_put(parser->s->pp, ret);
    return NULL;
    }

  /* If it's the first packet without skip flag set, set start position */
  if(!PACKET_GET_SKIP(ret) && (parser->start_pos < 0))
    parser->start_pos = ret->position;

  /* If we are before the start position, set the skip flag */
  if((parser->start_pos >= 0) && (ret->position < parser->start_pos))
    PACKET_SET_SKIP(ret);

  /* If we need to resync, set the timestamp */
  
  if((parser->timestamp == GAVL_TIME_UNDEFINED) && !PACKET_GET_SKIP(ret))
    parser->timestamp = gavl_time_rescale(parser->s->timescale,
                                          parser->format->timescale,
                                          pts);
  
  return ret;
  }

static bgav_packet_t * get_packet_parse_full(void * parser1,
                                             int force)
  {
  bgav_packet_t * ret;
  bgav_video_parser_t * parser = parser1;

  ret = parse_next_packet(parser, force);
  if(!ret)
    return NULL;

  /* Merge field pictures */
  
  if(ret->flags & PACKET_FLAG_FIELD_PIC)
    {
    bgav_packet_t * field2;
    field2 = parse_next_packet(parser, 1);
    if(!field2)
      return NULL;
    
    bgav_packet_merge_field2(ret, field2);
    bgav_packet_pool_put(parser->s->pp, field2);
    }

  if(!PACKET_GET_SKIP(ret))
    {
    if(PACKET_GET_CODING_TYPE(ret) != BGAV_CODING_TYPE_B)
      parser->non_b_count++;
    else if(parser->non_b_count < 2)
      PACKET_SET_SKIP(ret);
    }

  if(!PACKET_GET_SKIP(ret))
    {
    if(parser->timestamp != GAVL_TIME_UNDEFINED)
      {
      ret->dts = parser->timestamp;
      parser->timestamp += ret->duration;
      }
    }
    
#ifdef DUMP_OUTPUT
    bgav_dprintf("Get packet ");
    bgav_packet_dump(ret);
    bgav_hexdump(ret->data, 16, 16);
    //  bgav_dprintf("recovery_point %d\n", c->recovery_point);
#endif
 
  return ret;
  }

bgav_packet_t *
bgav_video_parser_get_packet_parse_full(void * parser1)
  {
  bgav_packet_t * ret;
  bgav_video_parser_t * parser = parser1;

  if(parser->out_packet)
    {
    ret = parser->out_packet;
    parser->out_packet = NULL;
    return ret;
    }
  return get_packet_parse_full(parser1, 1);
  }

bgav_packet_t *
bgav_video_parser_peek_packet_parse_full(void * parser1, int force)
  {
  bgav_video_parser_t * parser = parser1;

  if(!parser->out_packet)
    parser->out_packet = get_packet_parse_full(parser1, force);
  
  return parser->out_packet;
  }

#endif

bgav_packet_t *
bgav_video_parser_get_packet_parse_frame(void * parser1)
  {
  int result;
  bgav_packet_t * ret;
  bgav_video_parser_t * parser = parser1;
  if(parser->out_packet)
    {
    ret = parser->out_packet;
    parser->out_packet = NULL;
    return ret;
    }

  while(1)
    {
    ret = parser->src.get_func(parser->src.data);
    if(!ret)
      return NULL;
  
    result = bgav_video_parser_parse_frame(parser, ret);

    switch(result)
      {
      case PARSER_ERROR:
        return NULL;
        break;
      case PARSER_HAVE_PACKET:
        return ret;
        break;
      case PARSER_DISCARD:
        bgav_packet_pool_put(parser->s->pp, ret);
        continue;
        break;
      }
    }
  
  return ret;
  }

bgav_packet_t *
bgav_video_parser_peek_packet_parse_frame(void * parser1, int force)
  {
  bgav_video_parser_t * parser = parser1;
  
  if(parser->out_packet)
    return parser->out_packet;

  parser->out_packet = bgav_video_parser_get_packet_parse_frame(parser1);
  return parser->out_packet;
  }
