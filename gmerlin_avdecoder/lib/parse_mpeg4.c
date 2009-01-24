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
#include <videoparser.h>
#include <videoparser_priv.h>

#include <mpeg4_header.h>
#include <mpv_header.h>

#define MPEG4_NEED_SYNC                   0
#define MPEG4_NEED_STARTCODE              1
#define MPEG4_HAS_VOL_CODE                2
#define MPEG4_HAS_VOL_HEADER              3
#define MPEG4_HAS_VOP_CODE                4
#define MPEG4_HAS_VOP_HEADER              5

typedef struct
  {
  /* Sequence header */
  bgav_mpeg4_vol_header_t vol;
  int have_vol;
  int has_picture_start;
  int state;
  } mpeg4_priv_t;

static void reset_mpeg4(bgav_video_parser_t * parser)
  {
  mpeg4_priv_t * priv = parser->priv;
  priv->state = MPEG4_NEED_SYNC;
  priv->has_picture_start = 0;
  }

static int parse_mpeg4(bgav_video_parser_t * parser)
  {
  const uint8_t * sc;
  int len;
  mpeg4_priv_t * priv = parser->priv;
  //  cache_t * c;
  bgav_mpeg4_vop_header_t vh;

  //  int duration;
  int start_code;
  
  switch(priv->state)
    {
    case MPEG4_NEED_SYNC:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                   parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      bgav_video_parser_flush(parser, sc - parser->buf.buffer);
      parser->pos = 0;
      priv->state = MPEG4_NEED_STARTCODE;
      break;
    case MPEG4_NEED_STARTCODE:
      sc = bgav_mpv_find_startcode(parser->buf.buffer + parser->pos,
                                   parser->buf.buffer + parser->buf.size);
      if(!sc)
        return PARSER_NEED_DATA;
      
      start_code = bgav_mpeg4_get_start_code(sc);
      parser->pos = sc - parser->buf.buffer;

      switch(start_code)
        {
        case MPEG4_CODE_VO_START:
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            priv->has_picture_start = 1;
            }
          priv->state = MPEG4_NEED_STARTCODE;
          parser->pos+=4;
          break;
        case MPEG4_CODE_VOL_START:
          if(!priv->have_vol)
            priv->state = MPEG4_HAS_VOL_CODE;
          else
            {
            priv->state = MPEG4_NEED_STARTCODE;
            parser->pos+=4;
            }
          break;
        case MPEG4_CODE_VOP_START:
          /* TODO: Skip pictures before the first VOL header */
          
          if(!priv->has_picture_start)
            {
            if(!bgav_video_parser_set_picture_start(parser))
              return PARSER_ERROR;
            }
          priv->has_picture_start = 0;
          
          /* Need the picture header */
          priv->state = MPEG4_HAS_VOP_CODE;
          
          if(!parser->header)
            {
            bgav_video_parser_extract_header(parser);
            return PARSER_HAVE_HEADER;
            }
          break;
        default:
          parser->pos += 4;
          priv->state = MPEG4_NEED_STARTCODE;
          break;
        }
      
      break;
    case MPEG4_HAS_VOP_CODE:
      /* Try to get the picture header */
      
      len = bgav_mpeg4_vop_header_read(parser->opt,
                                        &vh,
                                        parser->buf.buffer + parser->pos,
                                        parser->buf.size - parser->pos, &priv->vol);
      if(!len)
        return PARSER_NEED_DATA;

      bgav_mpeg4_vop_header_dump(&vh);
      
      bgav_video_parser_set_coding_type(parser, vh.coding_type);
      
      //        fprintf(stderr, "Pic type: %c\n", ph.coding_type);
      parser->pos += len;
      priv->state = MPEG4_NEED_STARTCODE;
      
      break;
      //    case MPEG_GOP_CODE:
      //      break;
    case MPEG4_HAS_VOL_CODE:
      /* Try to get the sequence header */

      if(!priv->have_vol)
        {
        len = bgav_mpeg4_vol_header_read(parser->opt,
                                         &priv->vol,
                                         parser->buf.buffer + parser->pos,
                                         parser->buf.size - parser->pos);
        if(!len)
          return PARSER_NEED_DATA;

        bgav_mpeg4_vol_header_dump(&priv->vol);
        
        parser->pos += len;
        
        bgav_video_parser_set_framerate(parser,
                                        priv->vol.vop_time_increment_resolution,
                                        priv->vol.fixed_vop_time_increment);

#if 0        
        parser->format.image_width  = priv->sh.horizontal_size_value;
        parser->format.image_height = priv->sh.vertical_size_value;
        parser->format.frame_width  =
          (parser->format.image_width + 15) & ~15;
        parser->format.frame_height  =
          (parser->format.image_height + 15) & ~15;
#endif        
        priv->have_vol = 1;
        }
      else
        parser->pos += 4;
      
      priv->state = MPEG4_NEED_STARTCODE;
      
      break;
    }
  return PARSER_CONTINUE;
  }

static void cleanup_mpeg4(bgav_video_parser_t * parser)
  {
  free(parser->priv);
  }


void bgav_video_parser_init_mpeg4(bgav_video_parser_t * parser)
  {
  mpeg4_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv = priv;
  parser->parse = parse_mpeg4;
  parser->cleanup = cleanup_mpeg4;
  parser->reset = reset_mpeg4;

  }
