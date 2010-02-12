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
#include <audioparser_priv.h>
#include <mpa_header.h>

#define HEADER_BYTES 4

static int parse_mpa(bgav_audio_parser_t * parser)
  {
  int i;
  bgav_mpa_header_t h;
  
  for(i = 0; i < parser->buf.size - HEADER_BYTES; i++)
    {
    if(bgav_mpa_header_decode(&h, parser->buf.buffer + i))
      {
      if(!parser->have_format)
        {
        bgav_mpa_header_get_format(&h, &parser->format);
        parser->have_format = 1;
        return PARSER_HAVE_FORMAT;
        }
      bgav_audio_parser_set_frame(parser,
                                  i, h.frame_bytes, h.samples_per_frame);
      return PARSER_HAVE_FRAME;
      }
    }
  return PARSER_NEED_DATA;
  }

#if 0
void cleanup_mpa(bgav_audio_parser_t * parser)
  {
  
  }

void reset_mpa(bgav_audio_parser_t * parser)
  {
  
  }
#endif

/* This function is here to silence warnings for
   (completely unneccesary) MP3 headers in AVI */
static int parse_header_mpa(bgav_audio_parser_t* p)
  {
  return 1;
  }

void bgav_audio_parser_init_mpeg(bgav_audio_parser_t * parser)
  {
  parser->parse = parse_mpa;
  parser->parse_header = parse_header_mpa;
  
  }
