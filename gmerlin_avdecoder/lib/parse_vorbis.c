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

#include <vorbis/codec.h>

#define LOG_DOMAIN "parse_vorbis"

typedef struct
  {
  vorbis_info vi;
  vorbis_comment vc;
  long last_blocksize;
  } vorbis_priv_t;

static int parse_frame_vorbis(bgav_audio_parser_t * parser, bgav_packet_t * p)
  {
  ogg_packet op;

  long blocksize;
  vorbis_priv_t * priv = parser->priv;

  memset(&op, 0, sizeof(op));
  
  op.bytes = p->data_size;
  op.packet = p->data;

  blocksize = vorbis_packet_blocksize(&priv->vi, &op);

  if(priv->last_blocksize)
    p->duration = (priv->last_blocksize + blocksize) / 4;

  priv->last_blocksize = blocksize;
  
  return 1;
  }

static void cleanup_vorbis(bgav_audio_parser_t * parser)
  {
  vorbis_priv_t * priv = parser->priv;
  vorbis_comment_clear(&priv->vc);
  vorbis_info_clear(&priv->vi);
  free(priv);
  }

void bgav_audio_parser_init_vorbis(bgav_audio_parser_t * parser)
  {
  vorbis_priv_t * priv;
  ogg_packet op;
  int i;
  uint8_t * ptr;
  
  priv = calloc(1, sizeof(*priv));
  
  /* Get extradata and initialize codec */
  vorbis_info_init(&priv->vi);
  vorbis_comment_init(&priv->vc);

  memset(&op, 0, sizeof(op));

  if(!parser->s->ext_data)
    {
    bgav_log(parser->s->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "No extradata found");
    return;
    }

  ptr = parser->s->ext_data;

  op.b_o_s = 1;

  for(i = 0; i < 3; i++)
    {
    if(ptr - parser->s->ext_data > parser->s->ext_size - 4)
      {
      bgav_log(parser->s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Truncated vorbis header %d", i+1);
      return;
      }
    
    if(i)
      op.b_o_s = 0;
      
    op.bytes = BGAV_PTR_2_32BE(ptr); ptr+=4;
    op.packet = ptr;
      
    if(vorbis_synthesis_headerin(&priv->vi, &priv->vc,
                                 &op) < 0)
      {
      bgav_log(parser->s->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Packet %d is not a vorbis header", i+1);
      return;
      }
    op.packetno++;
    ptr += op.bytes;
    }
  
  parser->parse_frame = parse_frame_vorbis;
  parser->cleanup = cleanup_vorbis;
  }
