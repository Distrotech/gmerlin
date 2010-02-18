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
#define MPEG_HAS_GOP_CODE                6
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

  /* Frames since sequence header (for intra slice refresh) */
  int frames_since_sh;
  
  /* Framecounts for I and P/B frames, reset at sequence end
     (for MPEG still images) */
  int pb_count;
  int i_count;
  } mpeg12_priv_t;

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
  bgav_mpv_gop_header_t        gh;
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
      
      start_code = bgav_mpv_get_start_code(sc);
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

          priv->state = MPEG_HAS_GOP_CODE;
          
          if(!parser->header)
            {
            bgav_video_parser_extract_header(parser);
            return PARSER_HAVE_HEADER;
            }
          break;
        case MPEG_CODE_END:
          parser->pos += 4;

          /* Set sequence end for still mode */
          if(!priv->pb_count && (priv->i_count == 1))
            bgav_video_parser_set_sequence_end(parser);
          
          priv->pb_count = 0;
          priv->i_count = 0;
          //          fprintf(stderr, "Detected sequence end\n");
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

      if(len < 0)
        {
        bgav_log(parser->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
                 "Bogus picture header or broken frame");
        return PARSER_ERROR;
        }
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

      if(ph.coding_type == BGAV_CODING_TYPE_I)
        priv->i_count++;
      else
        priv->pb_count++;
        
      
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
            duration = parser->format->frame_duration * 2;
          else
            duration = parser->format->frame_duration;
          }
        else if(pe.progressive_frame)
          duration = parser->format->frame_duration / 2;
        
        parser->cache[parser->cache_size-1].duration += duration;
        }
      parser->pos += len;
      priv->state = MPEG_NEED_STARTCODE;
      break;
    case MPEG_HAS_GOP_CODE:
      len = bgav_mpv_gop_header_parse(parser->opt,
                                      &gh,
                                      parser->buf.buffer + parser->pos,
                                      parser->buf.size - parser->pos);
      if(!len)
        return PARSER_NEED_DATA;
      parser->pos += len;
      priv->state = MPEG_NEED_STARTCODE;
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
        
        parser->format->image_width  = priv->sh.horizontal_size_value;
        parser->format->image_height = priv->sh.vertical_size_value;
        parser->format->frame_width  =
          (parser->format->image_width + 15) & ~15;
        parser->format->frame_height  =
          (parser->format->image_height + 15) & ~15;
        
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
                                        parser->format->timescale * (priv->sh.ext.timescale_ext+1) * 2,
                                        parser->format->frame_duration * (priv->sh.ext.frame_duration_ext+1) * 2);
        
        parser->format->image_width  += priv->sh.ext.horizontal_size_ext;
        parser->format->image_height += priv->sh.ext.vertical_size_ext;
        
        parser->format->frame_width  =
          (parser->format->image_width + 15) & ~15;
        parser->format->frame_height  =
          (parser->format->image_height + 15) & ~15;
  

        parser->pos += len;
        
        }
      else
        parser->pos += 4;
      
      priv->state = MPEG_NEED_STARTCODE;
      break;
    }
  return PARSER_CONTINUE;
  }

static int parse_frame_mpeg12(bgav_video_parser_t * parser, int * coding_type, int * duration)
  {
  const uint8_t * sc;
  mpeg12_priv_t * priv = parser->priv;
  //  cache_t * c;
  bgav_mpv_picture_extension_t pe;
  bgav_mpv_picture_header_t    ph;
  int start_code;
  int len;
  int delta_d;
  int timescale;
  int frame_duration;

  int ret = PARSER_CONTINUE;
  
  while(1)
    {
    sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                 parser->buf.buffer + parser->buf.size);
    if(!sc)
      return PARSER_NEED_DATA;
    
    start_code = bgav_mpv_get_start_code(sc);

    /* Update position */
    parser->pos = sc - parser->buf.buffer;
    
    switch(start_code)
      {
      case MPEG_CODE_SEQUENCE:
        if(!priv->have_sh)
          {
          len = bgav_mpv_sequence_header_parse(parser->opt,
                                               &priv->sh,
                                               parser->buf.buffer + parser->pos,
                                               parser->buf.size - parser->pos);
          if(!len)
            return PARSER_ERROR;
          priv->have_sh = 1;

          bgav_mpv_get_framerate(priv->sh.frame_rate_index, &timescale, &frame_duration);
          
          bgav_video_parser_set_framerate(parser,
                                          timescale, frame_duration);
          parser->pos += len;
          }
        else
          parser->pos += 4;
        break;
      case MPEG_CODE_SEQUENCE_EXT:
        if(priv->have_sh && !priv->sh.mpeg2)
          {
          len =
            bgav_mpv_sequence_extension_parse(parser->opt,
                                              &priv->sh.ext,
                                              parser->buf.buffer + parser->pos,
                                              parser->buf.size - parser->pos);
          if(!len)
            return PARSER_ERROR;
          priv->sh.mpeg2 = 1;

          bgav_video_parser_set_framerate(parser,
                                          parser->format->timescale * (priv->sh.ext.timescale_ext+1) * 2,
                                          parser->format->frame_duration * (priv->sh.ext.frame_duration_ext+1) * 2);
          parser->pos += len;
          }
        else
          parser->pos += 4;
        break;
      case MPEG_CODE_PICTURE:
        if(!parser->header && priv->have_sh)
          {
          bgav_video_parser_extract_header(parser);
          ret = PARSER_HAVE_HEADER;
          }
        len = bgav_mpv_picture_header_parse(parser->opt,
                                            &ph,
                                            parser->buf.buffer + parser->pos,
                                            parser->buf.size - parser->pos);
        *duration = parser->format->frame_duration;
        if(!len)
          return PARSER_ERROR;
        
        *coding_type = ph.coding_type;
        parser->pos += len;
        break;
      case MPEG_CODE_PICTURE_EXT:
        len = bgav_mpv_picture_extension_parse(parser->opt,
                                               &pe,
                                               parser->buf.buffer + parser->pos,
                                               parser->buf.size - parser->pos);
        if(!len)
          return PARSER_ERROR;

        if(pe.repeat_first_field)
          {
          delta_d = 0;
          if(priv->sh.ext.progressive_sequence)
            {
            if(pe.top_field_first)
              delta_d = parser->format->frame_duration * 2;
            else
              delta_d = parser->format->frame_duration;
            }
          else if(pe.progressive_frame)
            delta_d = parser->format->frame_duration / 2;
          
          *duration += delta_d;
          }
        
        parser->pos += len;
        break;
      case MPEG_CODE_GOP:
        if(!parser->header && priv->have_sh)
          {
          bgav_video_parser_extract_header(parser);
          ret = PARSER_HAVE_HEADER;
          }
        
        parser->pos += 4;
        break;
      case MPEG_CODE_SLICE:
        return ret;
      default:
        parser->pos += 4;
        
      }
    }
  }


static void cleanup_mpeg12(bgav_video_parser_t * parser)
  {
  free(parser->priv);
  }

void bgav_video_parser_init_mpeg12(bgav_video_parser_t * parser)
  {
  mpeg12_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv        = priv;
  parser->parse       = parse_mpeg12;
  parser->parse_frame = parse_frame_mpeg12;
  parser->cleanup     = cleanup_mpeg12;
  parser->reset       = reset_mpeg12;
  }
