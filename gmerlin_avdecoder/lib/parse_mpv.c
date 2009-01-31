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

#include <string.h>
#include <stdlib.h>


#include <avdec_private.h>
#include <parser.h>
#include <videoparser_priv.h>

#include <mpv_header.h>

#define LOG_DOMAIN "parse_mpv"

/* MPEG-1/2 */

#define MPEG_NEED_SYNC                   0
#define MPEG_NEED_STARTCODE              1
#define MPEG_HAS_PICTURE_CODE            2
#define MPEG_HAS_PICTURE_HEADER          3
#define MPEG_HAS_PICTURE_EXT_CODE        4
#define MPEG_HAS_PICTURE_EXT_HEADER      5
#define MPEG_GOP_CODE                    6
#define MPEG_HAS_SEQUENCE_CODE           7
#define MPEG_HAS_SEQUENCE_HEADER         8
#define MPEG_HAS_SEQUENCE_EXT_CODE       9
#define MPEG_HAS_SEQUENCE_EXT_HEADER     10

typedef struct
  {
  /* Sequence header */
  bgav_mpv_sequence_header_t sh;
  int have_sh;
  int has_picture_start;
  int state;

  int frames_since_sh;
  } mpeg12_priv_t;

#define MPEG_CODE_SEQUENCE     1
#define MPEG_CODE_SEQUENCE_EXT 2
#define MPEG_CODE_PICTURE      3
#define MPEG_CODE_PICTURE_EXT  4
#define MPEG_CODE_GOP          5

static int get_code_mpeg12(const uint8_t * data)
  {
  if(bgav_mpv_sequence_header_probe(data))
    {
    //    fprintf(stderr, "MPEG_SEQUENCE\n");
    return MPEG_CODE_SEQUENCE;
    }
  else if(bgav_mpv_sequence_extension_probe(data))
    {
    //    fprintf(stderr, "MPEG_SEQUENCE_EXT\n");
    return MPEG_CODE_SEQUENCE_EXT;
    }
  else if(bgav_mpv_gop_header_probe(data))
    {
    //    fprintf(stderr, "MPEG_GOP\n");
    return MPEG_CODE_GOP;
    }
  else if(bgav_mpv_picture_header_probe(data))
    {
    //    fprintf(stderr, "MPEG_PICTURE\n");
    return MPEG_CODE_PICTURE;
    }
  else if(bgav_mpv_picture_extension_probe(data))
    {
    //    fprintf(stderr, "MPEG_PICTURE_EXT\n");
    return MPEG_CODE_PICTURE_EXT;
    }
  else
    return 0;
  }

static void reset_mpeg12(bgav_video_parser_t * parser)
  {
  mpeg12_priv_t * priv = parser->priv;
  priv->state = MPEG_NEED_SYNC;
  priv->has_picture_start = 0;
  }

static int parse_mpeg12(bgav_video_parser_t * parser)
  {
  const uint8_t * sc;
  int len;
  mpeg12_priv_t * priv = parser->priv;
  //  cache_t * c;
  bgav_mpv_picture_extension_t pe;
  bgav_mpv_picture_header_t    ph;
  int duration;
  int start_code;

  int timescale;
  int frame_duration;
  
  switch(priv->state)
    {
    case MPEG_NEED_SYNC:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                   parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      bgav_video_parser_flush(parser, sc - parser->buf.buffer);
      parser->pos = 0;
      priv->state = MPEG_NEED_STARTCODE;
      break;
    case MPEG_NEED_STARTCODE:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                     parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      
      start_code = get_code_mpeg12(sc);
      parser->pos = sc - parser->buf.buffer;

      switch(start_code)
        {
        case MPEG_CODE_SEQUENCE:
          priv->frames_since_sh = 0;
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            priv->has_picture_start = 1;
            }
          
          if(!priv->have_sh)
            priv->state = MPEG_HAS_SEQUENCE_CODE;
          else
            {
            priv->state = MPEG_NEED_STARTCODE;
            parser->pos+=4;
            }
          break;
        case MPEG_CODE_SEQUENCE_EXT:
          if(priv->have_sh && !priv->sh.mpeg2)
            priv->state = MPEG_HAS_SEQUENCE_EXT_CODE;
          else
            {
            priv->state = MPEG_NEED_STARTCODE;
            parser->pos+=4;
            }
          break;
        case MPEG_CODE_PICTURE:
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            }
          priv->has_picture_start = 0;
          
          /* Need the picture header */
          priv->state = MPEG_HAS_PICTURE_CODE;
          
          if(!parser->header && priv->have_sh)
            {
            bgav_video_parser_extract_header(parser);
            return PARSER_HAVE_HEADER;
            }
          break;
        case MPEG_CODE_PICTURE_EXT:
          /* Need picture extension */
          priv->state = MPEG_HAS_PICTURE_EXT_CODE;
          break;
        case MPEG_CODE_GOP:
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            priv->has_picture_start = 1;
            }
          
          if(!parser->header)
            {
            bgav_video_parser_extract_header(parser);
            parser->pos += 4;
            priv->state = MPEG_NEED_STARTCODE;
            return PARSER_HAVE_HEADER;
            }
          
          parser->pos += 4;
          priv->state = MPEG_NEED_STARTCODE;
          break;
        default:
          parser->pos += 4;
          priv->state = MPEG_NEED_STARTCODE;
          break;
        }
      
      break;
    case MPEG_HAS_PICTURE_CODE:
      /* Try to get the picture header */
      
      len = bgav_mpv_picture_header_parse(parser->opt,
                                          &ph,
                                          parser->buf.buffer + parser->pos,
                                          parser->buf.size - parser->pos);
      if(!len)
        return PARSER_NEED_DATA;
      
      bgav_video_parser_set_coding_type(parser, ph.coding_type);

      if(priv->have_sh)
        {
        if(!(parser->flags & PARSER_NO_I_FRAMES) &&
           (ph.coding_type == BGAV_CODING_TYPE_P) &&
           (!priv->frames_since_sh))
          {
          parser->flags |= PARSER_NO_I_FRAMES;
          bgav_log(parser->opt, BGAV_LOG_DEBUG, LOG_DOMAIN,
                   "Detected Intra slice refresh");
          }
        priv->frames_since_sh++;
        }
      
      //        fprintf(stderr, "Pic type: %c\n", ph.coding_type);
      parser->pos += len;
      priv->state = MPEG_NEED_STARTCODE;
      
      break;
    case MPEG_HAS_PICTURE_EXT_CODE:
      /* Try to get the picture extension */
      len = bgav_mpv_picture_extension_parse(parser->opt,
                                             &pe,
                                             parser->buf.buffer + parser->pos,
                                             parser->buf.size - parser->pos);
      if(!len)
        return PARSER_NEED_DATA;
      if(pe.repeat_first_field)
        {
        duration = 0;
        if(priv->sh.ext.progressive_sequence)
          {
          if(pe.top_field_first)
            duration = parser->format.frame_duration * 2;
          else
            duration = parser->format.frame_duration;
          }
        else if(pe.progressive_frame)
          duration = parser->format.frame_duration / 2;
        
        parser->cache[parser->cache_size-1].duration += duration;
        }
      parser->pos += len;
      priv->state = MPEG_NEED_STARTCODE;
      break;
    case MPEG_GOP_CODE:
      break;
    case MPEG_HAS_SEQUENCE_CODE:
      /* Try to get the sequence header */

      if(!priv->have_sh)
        {
        len = bgav_mpv_sequence_header_parse(parser->opt,
                                             &priv->sh,
                                             parser->buf.buffer + parser->pos,
                                             parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;
        parser->pos += len;

        bgav_mpv_get_framerate(priv->sh.frame_rate_index, &timescale, &frame_duration);
        
        bgav_video_parser_set_framerate(parser,
                                        timescale, frame_duration);
        
        parser->format.image_width  = priv->sh.horizontal_size_value;
        parser->format.image_height = priv->sh.vertical_size_value;
        parser->format.frame_width  =
          (parser->format.image_width + 15) & ~15;
        parser->format.frame_height  =
          (parser->format.image_height + 15) & ~15;
        
        priv->have_sh = 1;
        }
      else
        parser->pos += 4;
      
      priv->state = MPEG_NEED_STARTCODE;
      
      break;
    case MPEG_HAS_SEQUENCE_EXT_CODE:
      if(!priv->sh.mpeg2)
        {
        /* Try to get the sequence extension */
        len =
          bgav_mpv_sequence_extension_parse(parser->opt,
                                            &priv->sh.ext,
                                            parser->buf.buffer + parser->pos,
                                            parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;
        
        priv->sh.mpeg2 = 1;

        bgav_video_parser_set_framerate(parser,
                                        parser->format.timescale * (priv->sh.ext.timescale_ext+1) * 2,
                                        parser->format.frame_duration * (priv->sh.ext.frame_duration_ext+1) * 2);
        
        parser->format.image_width  += priv->sh.ext.horizontal_size_ext;
        parser->format.image_height += priv->sh.ext.vertical_size_ext;
        
        parser->format.frame_width  =
          (parser->format.image_width + 15) & ~15;
        parser->format.frame_height  =
          (parser->format.image_height + 15) & ~15;
  

        parser->pos += len;
        
        }
      else
        parser->pos += 4;
      
      priv->state = MPEG_NEED_STARTCODE;
      break;
    }
  return PARSER_CONTINUE;
  }

static void cleanup_mpeg12(bgav_video_parser_t * parser)
  {
  free(parser->priv);
  }

void bgav_video_parser_init_mpeg12(bgav_video_parser_t * parser)
  {
  mpeg12_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv = priv;
  parser->parse = parse_mpeg12;
  parser->cleanup = cleanup_mpeg12;
  parser->reset = reset_mpeg12;
  }
