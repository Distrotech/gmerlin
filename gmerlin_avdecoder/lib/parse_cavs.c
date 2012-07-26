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

#include <string.h>
#include <stdlib.h>


#include <avdec_private.h>
#include <parser.h>
#include <videoparser_priv.h>

#include <cavs_header.h>
#include <mpv_header.h>

#define CAVS_NEED_SYNC                   0
#define CAVS_NEED_STARTCODE              1
#define CAVS_HAS_SEQ_CODE                2
#define CAVS_HAS_SEQ_HEADER              3
#define CAVS_HAS_PIC_CODE                4
#define CAVS_HAS_PIC_HEADER              5

#define STATE_INIT                       0 // Nothing found yet
#define STATE_SYNC                       1 // Need start code
#define STATE_SEQUENCE                   2 // Got sequence header
#define STATE_PICTURE                    3 // Got picture header

typedef struct
  {
  /* Sequence header */
  bgav_cavs_sequence_header_t seq;
  int have_seq;
  int has_picture_start;
  int state;
  } cavs_priv_t;

static void reset_cavs(bgav_video_parser_t * parser)
  {
  cavs_priv_t * priv = parser->priv;
  priv->state = CAVS_NEED_SYNC;
  priv->has_picture_start = 0;
  }

static int parse_cavs(bgav_video_parser_t * parser)
  {
  const uint8_t * sc;
  int len;
  cavs_priv_t * priv = parser->priv;
  //  cache_t * c;
  bgav_cavs_picture_header_t ph;

  //  int duration;
  int start_code;

  int timescale, frame_duration;
  
  switch(priv->state)
    {
    case CAVS_NEED_SYNC:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                   parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      bgav_video_parser_flush(parser, sc - parser->buf.buffer);
      parser->pos = 0;
      priv->state = CAVS_NEED_STARTCODE;
      break;
    case CAVS_NEED_STARTCODE:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                   parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      
      start_code = bgav_cavs_get_start_code(sc);
      parser->pos = sc - parser->buf.buffer;

      switch(start_code)
        {
        case CAVS_CODE_SEQUENCE:
          if(!priv->have_seq)
            priv->state = CAVS_HAS_SEQ_CODE;
          else
            {
            priv->state = CAVS_NEED_STARTCODE;
            parser->pos+=4;
            }
          break;
        case CAVS_CODE_PICTURE_I:
        case CAVS_CODE_PICTURE_PB:
          /* TODO: Skip pictures before the first SEQ header */
          
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            }

          bgav_video_parser_set_header_end(parser);
          
          priv->has_picture_start = 0;
          
          /* Need the picture header */
          priv->state = CAVS_HAS_PIC_CODE;
          
          if(!parser->s->ext_data)
            {
            bgav_video_parser_extract_header(parser);
            return PARSER_CONTINUE;
            }
          break;
        default:
          parser->pos += 4;
          priv->state = CAVS_NEED_STARTCODE;
          break;
        }
      
      break;
    case CAVS_HAS_PIC_CODE:
      /* Try to get the picture header */
      
      len = bgav_cavs_picture_header_read(parser->s->opt,
                                          &ph,
                                          parser->buf.buffer + parser->pos,
                                          parser->buf.size - parser->pos, &priv->seq);
      if(!len)
        return PARSER_NEED_DATA;

      //      bgav_cavs_picture_header_dump(&ph, &priv->seq);
      
      bgav_video_parser_set_coding_type(parser, ph.coding_type);
      
      //        fprintf(stderr, "Pic type: %c\n", ph.coding_type);
      parser->pos += len;
      priv->state = CAVS_NEED_STARTCODE;
      
      break;
      //    case MPEG_GOP_CODE:
      //      break;
    case CAVS_HAS_SEQ_CODE:
      /* Try to get the sequence header */

      if(!priv->have_seq)
        {
        len = bgav_cavs_sequence_header_read(parser->s->opt,
                                             &priv->seq,
                                             parser->buf.buffer + parser->pos,
                                             parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;

        //        bgav_cavs_sequence_header_dump(&priv->seq);
        
        parser->pos += len;

        bgav_mpv_get_framerate(priv->seq.frame_rate_code, &timescale, &frame_duration);

        parser->format->timescale = timescale;
        parser->format->frame_duration = frame_duration;
        
        bgav_video_parser_set_framerate(parser);
        
        parser->format->image_width  = priv->seq.horizontal_size;
        parser->format->image_height = priv->seq.vertical_size;
        parser->format->frame_width  =
          (parser->format->image_width + 15) & ~15;
        parser->format->frame_height  =
          (parser->format->image_height + 15) & ~15;
      
        priv->have_seq = 1;
        }
      else
        parser->pos += 4;
      
      priv->state = CAVS_NEED_STARTCODE;
      
      break;
    }
  return PARSER_CONTINUE;
  }

static int parse_frame_cavs(bgav_video_parser_t * parser, bgav_packet_t * p)
  {
  int start_code;
  int len;
  bgav_cavs_picture_header_t ph;
  const uint8_t * sc;
  const uint8_t * ptr = p->data;
  const uint8_t * end = p->data + p->data_size;
  cavs_priv_t * priv = parser->priv;
  
  while(1)
    {
    sc = bgav_mpv_find_startcode(ptr, end);
    if(!sc)
      return PARSER_ERROR;
    
    start_code = bgav_cavs_get_start_code(sc);

    switch(start_code)
      {
      case CAVS_CODE_SEQUENCE:
        if(!priv->have_seq)
          {
          int timescale;
          int frame_duration;
          len = bgav_cavs_sequence_header_read(parser->s->opt,
                                               &priv->seq,
                                               ptr, end - ptr);
          if(!len)
            return PARSER_ERROR;
          
          //        bgav_cavs_sequence_header_dump(&priv->seq);

          ptr += len;
          
          bgav_mpv_get_framerate(priv->seq.frame_rate_code, &timescale, &frame_duration);

          parser->format->timescale = timescale;
          parser->format->frame_duration = frame_duration;
        
          bgav_video_parser_set_framerate(parser);
        
          parser->format->image_width  = priv->seq.horizontal_size;
          parser->format->image_height = priv->seq.vertical_size;
          parser->format->frame_width  =
            (parser->format->image_width + 15) & ~15;
          parser->format->frame_height  =
            (parser->format->image_height + 15) & ~15;
      
          priv->have_seq = 1;
          }
        else
          ptr += 4;
        break;
      case CAVS_CODE_PICTURE_I:
      case CAVS_CODE_PICTURE_PB:
        len = bgav_cavs_picture_header_read(parser->s->opt,
                                            &ph, ptr, end - ptr, &priv->seq);
        PACKET_SET_CODING_TYPE(p, ph.coding_type);
        /* TODO: Set picture duration */
        return PARSER_HAVE_PACKET;
        break;
      }
    
    }
  return PARSER_ERROR;
  }

static int find_frame_boundary_cavs(bgav_video_parser_t * parser,
                                    int * skip)
  {
  return 0;
  }

static void cleanup_cavs(bgav_video_parser_t * parser)
  {
  free(parser->priv);
  }

void bgav_video_parser_init_cavs(bgav_video_parser_t * parser)
  {
  cavs_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv = priv;
  parser->parse = parse_cavs;
  parser->parse_frame = parse_frame_cavs;

  parser->cleanup = cleanup_cavs;
  parser->reset = reset_cavs;
  parser->find_frame_boundary = find_frame_boundary_cavs;
  }
