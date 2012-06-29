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

#include <avdec_private.h>
#include <vc1_header.h>
#include <mpv_header.h>
#include <parser.h>
#include <videoparser_priv.h>


#define VC1_NEED_START 0
#define VC1_HAVE_START 1
#define VC1_NEED_END   2
#define VC1_HAVE_END   3

typedef struct
  {
  int chunk_len; /* Len from one startcode to the next */

  /* Unescaped data */
  uint8_t * buf;
  int buf_alloc;
  int buf_len;
  
  int has_picture_start;

  int have_sh;
  bgav_vc1_sequence_header_t sh;
  int state;
  } vc1_priv_t;

static void unescape_data(bgav_video_parser_t * parser)
  {
  vc1_priv_t * priv = parser->priv;

  if(priv->buf_alloc < priv->chunk_len)
    {
    priv->buf_alloc = priv->chunk_len + 1024;
    priv->buf = realloc(priv->buf, priv->buf_alloc);
    }
  priv->buf_len =
    bgav_vc1_unescape_buffer(parser->buf.buffer + parser->pos,
                             priv->chunk_len, priv->buf);
  }

static void handle_sequence(bgav_video_parser_t * parser)
  {
  vc1_priv_t * priv = parser->priv;
  unescape_data(parser);

  bgav_vc1_sequence_header_read(parser->s->opt, &priv->sh, 
                                priv->buf, priv->buf_len);
  bgav_vc1_sequence_header_dump(&priv->sh);

  if(priv->sh.profile == PROFILE_ADVANCED)
    {
    parser->format->timescale = priv->sh.h.adv.timescale;
    parser->format->frame_duration = priv->sh.h.adv.frame_duration;
    bgav_video_parser_set_framerate(parser);
    }
  priv->have_sh = 1;
  }

static void handle_picture(bgav_video_parser_t * parser)
  {
  bgav_vc1_picture_header_adv_t aph;
  vc1_priv_t * priv = parser->priv;

  if(priv->sh.profile == PROFILE_ADVANCED)
    {
    unescape_data(parser);
    bgav_vc1_picture_header_adv_read(parser->s->opt,
                                     &aph,
                                     priv->buf, priv->buf_len,
                                     &priv->sh);
    
    bgav_vc1_picture_header_adv_dump(&aph);
    bgav_video_parser_set_coding_type(parser, aph.coding_type);

    }
  // else
    
  }

static int parse_vc1(bgav_video_parser_t * parser)
  {
  const uint8_t * sc;
  vc1_priv_t * priv = parser->priv;
  
  switch(priv->state)
    {
    case VC1_NEED_START:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                   parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      bgav_video_parser_flush(parser, sc - parser->buf.buffer);
      parser->pos = 0;

      priv->state = VC1_HAVE_START;
      break;
    case VC1_HAVE_START:
      switch(parser->buf.buffer[parser->pos+3])
        {
        case VC1_CODE_SEQUENCE:
          /* Set picture start */
          if(!priv->has_picture_start &&
             !bgav_video_parser_set_picture_start(parser))
            return PARSER_ERROR;
          priv->has_picture_start = 1;
          
          break;
        case VC1_CODE_ENTRY_POINT:
          
          /* Set picture start */
          if(!priv->has_picture_start &&
             !bgav_video_parser_set_picture_start(parser))
            return PARSER_ERROR;
          priv->has_picture_start = 1;
          break;
        case VC1_CODE_PICTURE:
          /* Extract extradata */
          if(!parser->s->ext_data)
            {
            bgav_video_parser_extract_header(parser);
            //            bgav_video_parser_flush(parser, parser->header_len);
            return PARSER_CONTINUE;
            }

          /* Set picture start */
          if(!priv->has_picture_start &&
             !bgav_video_parser_set_picture_start(parser))
            return PARSER_ERROR;
          priv->has_picture_start = 0;

        
          break;
        }
      /* Need to find the end */
      priv->state = VC1_NEED_END;
      break;
    case VC1_NEED_END:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos + 4,
                                   parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      
      priv->chunk_len = sc - (parser->buf.buffer + parser->pos);
      // fprintf(stderr, "Got nal %d bytes\n", priv->nal_len);
      priv->state = VC1_HAVE_END;
      break;
    case VC1_HAVE_END:
      if((parser->buf.buffer[parser->pos+3] == VC1_CODE_SEQUENCE) &&
         !priv->have_sh)
        handle_sequence(parser);
      else if(parser->buf.buffer[parser->pos+3] == VC1_CODE_PICTURE)
        handle_picture(parser);
      parser->pos += priv->chunk_len;
      priv->state = VC1_HAVE_START;
      break;
      
    }
  return PARSER_CONTINUE;
  }

static void cleanup_vc1(bgav_video_parser_t * parser)
  {
  vc1_priv_t * priv = parser->priv;
  if(priv->buf)
    free(priv->buf);
  free(priv);
  }

static void reset_vc1(bgav_video_parser_t * parser)
  {
  vc1_priv_t * priv = parser->priv;
  priv->state = VC1_NEED_START;
  priv->has_picture_start = 0;
  }

void bgav_video_parser_init_vc1(bgav_video_parser_t * parser)
  {
  vc1_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv = priv;
  parser->parse = parse_vc1;
  parser->cleanup = cleanup_vc1;
  parser->reset = reset_vc1;
  
  }
