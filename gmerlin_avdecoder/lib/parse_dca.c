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

#include <bgav_dca.h>

#define DCA_HEADER_BYTES 14

typedef struct
  {
  dts_state_t * state;
  } dca_t;

static int parse_dca(bgav_audio_parser_t * parser)
  {
  int i;
  int frame_bytes;
  int frame_length;
  int flags;
  int sample_rate;
  int bit_rate;
  
  dca_t * priv = parser->priv;
  
  for(i = 0; i < parser->buf.size - DCA_HEADER_BYTES; i++)
    {
    frame_bytes = dts_syncinfo(priv->state, parser->buf.buffer + i,
                               &flags, &sample_rate, &bit_rate, &frame_length);
    if(frame_bytes)
      {
      //      fprintf(stderr, "Got frame bytes: %d, length: %d\n",
      //              frame_bytes, frame_length);

      if(!parser->have_format)
        {
        parser->s->data.audio.format.samplerate = sample_rate;
        bgav_dca_flags_2_channel_setup(flags, &parser->s->data.audio.format);
        parser->have_format = 1;
        return PARSER_HAVE_FORMAT;
        }
      bgav_audio_parser_set_frame(parser,
                                  i, frame_bytes, frame_length);
      return PARSER_HAVE_FRAME;
      
      }
    }
  return PARSER_NEED_DATA;
  }

static void cleanup_dca(bgav_audio_parser_t * parser)
  {
  dca_t * priv = parser->priv;
  if(priv->state)
    dts_free(priv->state);
  free(priv);
  }


void bgav_audio_parser_init_dca(bgav_audio_parser_t * parser)
  {
  dca_t * priv = calloc(1, sizeof(*priv));
  priv->state =   dts_init(0);
  parser->priv = priv;
  parser->parse = parse_dca;
  parser->cleanup = cleanup_dca;
  }

void bgav_dca_flags_2_channel_setup(int flags, gavl_audio_format_t * format)
  {
  switch(flags & DTS_CHANNEL_MASK)
    {
    case DTS_CHANNEL: // Dual mono. Two independant mono channels.
      format->num_channels = 2;
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    case DTS_MONO: // Mono.
      format->num_channels = 1;
      format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      break;
    case DTS_STEREO: // Stereo.
    case DTS_DOLBY: // Dolby surround compatible stereo.
      format->num_channels = 2;
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      break;
    case DTS_3F: // 3 front channels (left, center, right)
      format->num_channels = 3;
      format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      format->channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      break;
    case DTS_2F1R: // 2 front, 1 rear surround channel (L, R, S)
      format->num_channels = 3;
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_CENTER;
      break;
    case DTS_3F1R: // 3 front, 1 rear surround channel (L, C, R, S)
      format->num_channels = 4;
      format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      format->channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[3] = GAVL_CHID_REAR_CENTER;
      break;
    case DTS_2F2R: // 2 front, 2 rear surround channels (L, R, LS, RS)
      format->num_channels = 4;
      format->channel_locations[0] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[1] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[2] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[3] = GAVL_CHID_REAR_RIGHT;
      break;
    case DTS_3F2R: // 3 front, 2 rear surround channels (L, C, R, LS, RS)
      format->num_channels = 5;
      format->channel_locations[0] = GAVL_CHID_FRONT_CENTER;
      format->channel_locations[1] = GAVL_CHID_FRONT_LEFT;
      format->channel_locations[2] = GAVL_CHID_FRONT_RIGHT;
      format->channel_locations[3] = GAVL_CHID_REAR_LEFT;
      format->channel_locations[4] = GAVL_CHID_REAR_RIGHT;
      break;
    }

  if(flags & DTS_LFE)
    {
    format->channel_locations[format->num_channels] =
      GAVL_CHID_LFE;
    format->num_channels++;
    }

  }
