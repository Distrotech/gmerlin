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
#include <parser.h>
#include <audioparser_priv.h>

// #define DUMP_INPUT
// #define DUMP_OUTPUT

static const struct
  {
  uint32_t fourcc;
  init_func func;
  }
parsers[] =
  {
    { BGAV_WAVID_2_FOURCC(0x0050), bgav_audio_parser_init_mpeg },
    { BGAV_WAVID_2_FOURCC(0x0055), bgav_audio_parser_init_mpeg },
    { BGAV_MK_FOURCC('.','m','p','3'), bgav_audio_parser_init_mpeg },
    { BGAV_WAVID_2_FOURCC(0x2000), bgav_audio_parser_init_a52 },
  };


bgav_audio_parser_t * bgav_audio_parser_create(uint32_t fourcc, int timescale,
                                               const bgav_options_t * opt)
  {
  bgav_audio_parser_t * ret;
  int i;
  
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
  ret->opt = opt;
  ret->in_scale = timescale;
  ret->timestamp = BGAV_TIMESTAMP_UNDEFINED;
  ret->raw_position = -1;
  func(ret);

  return ret;
  }

void bgav_audio_parser_destroy(bgav_audio_parser_t * parser)
  {
  if(parser->packets)
    free(parser->packets);
  free(parser);
  }

void bgav_audio_parser_reset(bgav_audio_parser_t * parser, int64_t pts)
  {
  parser->num_packets = 0;
  parser->frame_samples = 0;
  parser->frame_bytes = 0;
  parser->raw_position = -1;
  parser->timestamp = pts;
  }

static int check_output(bgav_audio_parser_t * parser)
  {
  if(parser->frame_bytes &&
     (parser->buf.size >= parser->frame_bytes))
    return 1;
  return 0;
  }

int bgav_audio_parser_parse(bgav_audio_parser_t * parser)
  {
  int result;

  if(check_output(parser))
    return PARSER_HAVE_PACKET;
  
  result = parser->parse(parser);
  switch(result)
    {
    case PARSER_HAVE_FORMAT:
    case PARSER_NEED_DATA:
      return result;
      break;
    case PARSER_HAVE_FRAME:
      if(check_output(parser))
        return PARSER_HAVE_PACKET;
      /* Check if we have enough data */
      break;
    }
  return PARSER_NEED_DATA;
  }

void bgav_audio_parser_add_packet(bgav_audio_parser_t * parser,
                                  bgav_packet_t * p)
  {
#ifdef DUMP_INPUT  
  bgav_dprintf("Add packet ");
  bgav_packet_dump(p);
#endif
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
  parser->packets[parser->num_packets].pts  = p->pts;
  parser->num_packets++;
  bgav_bytebuffer_append_data(&parser->buf, p->data, p->data_size, 0);

  }

void bgav_audio_parser_add_data(bgav_audio_parser_t * parser,
                                uint8_t * data, int len, int64_t position)
  {
  parser->raw = 1;
  bgav_bytebuffer_append_data(&parser->buf, data, len, 0);
  if(parser->raw_position < 0)
    parser->raw_position = position;
  }

void bgav_audio_parser_get_packet(bgav_audio_parser_t * parser,
                                  bgav_packet_t * p)
  {
  
  bgav_packet_alloc(p, parser->frame_bytes);
  memcpy(p->data, parser->buf.buffer, parser->frame_bytes);
  p->data_size = parser->frame_bytes;
  bgav_packet_pad(p);
  bgav_audio_parser_flush(parser, parser->frame_bytes);

  if(parser->timestamp == BGAV_TIMESTAMP_UNDEFINED)
    {
    if(parser->frame_pts != BGAV_TIMESTAMP_UNDEFINED)
      parser->timestamp = parser->frame_pts;
    else
      parser->timestamp = 0;
    }
  p->pts = parser->timestamp;
  parser->timestamp += parser->frame_samples;
  
  p->duration = parser->frame_samples;
  
  p->dts = BGAV_TIMESTAMP_UNDEFINED;

  p->flags = 0;
  
  PACKET_SET_KEYFRAME(p);

  p->position = parser->frame_position;
  p->valid = 1;

  //  fprintf(stderr, "Get packet %c %ld\n", c->coding_type, p->pts);
  
#ifdef DUMP_OUTPUT
  bgav_dprintf("Get packet ");
  bgav_packet_dump(p);
#endif

  }

const gavl_audio_format_t *
bgav_audio_parser_get_format(bgav_audio_parser_t * parser)
  {
  return &parser->format;
  }

void bgav_audio_parser_set_eof(bgav_audio_parser_t * parser)
  {
  
  }

void bgav_audio_parser_flush(bgav_audio_parser_t * parser, int bytes)
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

void bgav_audio_parser_set_frame(bgav_audio_parser_t * parser,
                                 int pos, int len, int samples)
  {
  int i;

  if(pos)
    bgav_audio_parser_flush(parser, pos);
  
  /* Get frame position */
  if(parser->raw)
    {
    parser->frame_position = parser->raw_position;
    parser->frame_pts      = 0;
    }
  else
    {
    for(i = 0; i < parser->num_packets; i++)
      {
      if(parser->packets[i].parser_position + parser->packets[i].size > 0)
        {
        parser->frame_position = parser->packets[i].packet_position;
        parser->frame_pts      = gavl_time_rescale(parser->in_scale,
                                                   parser->format.samplerate,
                                                   parser->packets[i].pts);
        }
      }
    }
  parser->frame_samples = samples;
  parser->frame_bytes   = len;
  }

