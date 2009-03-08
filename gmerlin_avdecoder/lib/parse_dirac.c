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

#include <avdec_private.h>
#include <parser.h>
#include <videoparser_priv.h>

#include <dirac_header.h>

#define LOG_DOMAIN "parse_dirac"

typedef struct
  {
  /* Sequence header */
  bgav_dirac_sequence_header_t sh;
  int have_sh;
  int64_t pic_num_max;
  } dirac_priv_t;

static void cleanup_dirac(bgav_video_parser_t * parser)
  {
  free(parser->priv);
  }

static void reset_dirac(bgav_video_parser_t * parser)
  {
  dirac_priv_t * priv;
  priv = parser->priv;
  priv->pic_num_max = -1;
  }

static int parse_frame_dirac(bgav_video_parser_t * parser,
                             int * coding_type, int * duration)
  {
  int code, len;
  dirac_priv_t * priv;
  uint8_t * start =   parser->buf.buffer + parser->pos;
  uint8_t * end = parser->buf.buffer + parser->buf.size;
  bgav_dirac_picture_header_t ph;
  int ret = PARSER_CONTINUE;
  priv = parser->priv;
  
  //  fprintf(stderr, "parse_frame_dirac %d %d\n",
  //          parser->pos, parser->buf.size);
  //  bgav_hexdump(start, 16, 16);
  
  while(start < end)
    {
    code = bgav_dirac_get_code(start, end - start, &len);

    switch(code)
      {
      case DIRAC_CODE_SEQUENCE:
        if(!priv->have_sh)
          {
          if(!bgav_dirac_sequence_header_parse(&priv->sh,
                                               start, end - start))
            return PARSER_ERROR;
          bgav_dirac_sequence_header_dump(&priv->sh);
          priv->have_sh = 1;

          parser->header = malloc(len);
          memcpy(parser->header, start, len);
          parser->header_len = len;
          bgav_video_parser_set_framerate(parser,
                                          priv->sh.timescale,
                                          priv->sh.frame_duration);
          ret = PARSER_HAVE_HEADER;
          }
        break;
      case DIRAC_CODE_PICTURE:
        if(!priv->have_sh)
          return PARSER_DISCARD;
        
        if(!bgav_dirac_picture_header_parse(&ph, start, end - start))
          return PARSER_ERROR;
        //        bgav_dirac_picture_header_dump(&ph);

        if(ph.num_refs == 0)
          {
          *coding_type = BGAV_CODING_TYPE_I;
          priv->pic_num_max = ph.pic_num;
          }
        else if((priv->pic_num_max >= 0) &&
           (ph.pic_num < priv->pic_num_max))
          {
          *coding_type = BGAV_CODING_TYPE_B;
          }
        else
          {
          priv->pic_num_max = ph.pic_num;
          *coding_type = BGAV_CODING_TYPE_P;
          }
        
        *duration    = parser->packet_duration ? parser->packet_duration :
          priv->sh.frame_duration;
        
        return ret;        
        break;
      }
    start += len;
    }
  
  return 0;
  }

void bgav_video_parser_init_dirac(bgav_video_parser_t * parser)
  {
  dirac_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  parser->priv        = priv;
  priv->pic_num_max = -1;
  //  parser->parse       = parse_dirac;
  parser->parse_frame = parse_frame_dirac;
  parser->cleanup     = cleanup_dirac;
  parser->reset       = reset_dirac;
  }
