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

#include <opus.h>

#include <config.h>
#include <avdec_private.h>
#include <parser.h>
#include <audioparser_priv.h>

static int parse_frame_opus(bgav_audio_parser_t * parser, bgav_packet_t * p)
  {
  int nb_frames = opus_packet_get_nb_frames(p->data, p->data_size);
  if (nb_frames < 1)
    p->duration = nb_frames;
  else
    p->duration = opus_packet_get_samples_per_frame(p->data, 48000) * nb_frames;

  return 1;
  }

static void cleanup_opus(bgav_audio_parser_t * parser)
  {

  }

static void reset_opus(bgav_audio_parser_t * parser)
  {

  }

void bgav_audio_parser_init_opus(bgav_audio_parser_t * parser)
  {
  parser->parse_frame = parse_frame_opus;
  parser->cleanup = cleanup_opus;
  parser->reset = reset_opus;
  }
