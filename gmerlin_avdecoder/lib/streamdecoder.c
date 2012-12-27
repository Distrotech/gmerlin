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

#include <avdec_private.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define LOG_DOMAIN "streamdecoder"

struct bgav_stream_decoder_s
  {
  bgav_options_t opt;
  bgav_stream_t s;

  bgav_packet_t p;
  bgav_packet_t * out_packet;
  gavl_packet_source_t * src;
  };

bgav_stream_decoder_t * bgav_stream_decoder_create()
  {
  bgav_stream_decoder_t * ret = calloc(1, sizeof(*ret));
  bgav_options_set_defaults(&ret->opt);
  bgav_codecs_init(&ret->opt);
  ret->s.opt = &ret->opt;
  return ret;
  }


bgav_options_t *
bgav_stream_decoder_get_options(bgav_stream_decoder_t * dec)
  {
  return &dec->opt;
  }

static void packet_from_gavl(gavl_packet_t * src,
                             bgav_packet_t * dst)
  {
  bgav_packet_from_gavl(src, dst);
  dst->data = src->data;
  dst->data_size = src->data_len; 
  }
                             

static gavl_source_status_t get_func(void * priv, bgav_packet_t ** ret)
  {
  gavl_source_status_t st;
  gavl_packet_t * gp = NULL;
  
  bgav_stream_decoder_t * dec = priv;

  if(dec->out_packet)
    {
    if(ret)
      *ret = dec->out_packet;
    dec->out_packet = NULL;
    return GAVL_SOURCE_OK;
    }
  
  if((st = gavl_packet_source_read_packet(dec->src, &gp)) != GAVL_SOURCE_OK)
    return st;

  packet_from_gavl(gp, &dec->p);
  *ret = &dec->p;
  return GAVL_SOURCE_OK;
  }

static gavl_source_status_t peek_func(void * priv, bgav_packet_t ** ret, int force)
  {
  gavl_source_status_t st;
  gavl_packet_t * gp = NULL;
  
  bgav_stream_decoder_t * dec = priv;

  if(dec->out_packet)
    {
    if(ret)
      *ret = dec->out_packet;
    return GAVL_SOURCE_OK;
    }
  
  if((st = gavl_packet_source_read_packet(dec->src, &gp)) != GAVL_SOURCE_OK)
    return st;
  
  packet_from_gavl(gp, &dec->p);
  dec->out_packet = &dec->p;

  if(ret)
    *ret = dec->out_packet;
  return GAVL_SOURCE_OK;
  }

static int init_common(bgav_stream_decoder_t * dec,
                       gavl_packet_source_t * src,
                       const gavl_compression_info_t * ci,
                       gavl_metadata_t * m)
  {
  dec->s.action = BGAV_STREAM_DECODE;
  dec->src = src;

  dec->s.src.data = dec;
  dec->s.src.get_func = get_func;
  dec->s.src.peek_func = peek_func;
  
  return bgav_stream_start(&dec->s);
  }
  
gavl_audio_source_t *
bgav_stream_decoder_connect_audio(bgav_stream_decoder_t * dec,
                                  gavl_packet_source_t * src,
                                  const gavl_compression_info_t * ci,
                                  const gavl_audio_format_t * fmt,
                                  gavl_metadata_t * m)
  {
  dec->s.type = BGAV_STREAM_AUDIO;
  bgav_stream_set_from_gavl(&dec->s, ci, fmt, NULL, m);
  
  
  if(!init_common(dec, src, ci, m))
    return NULL;

  gavl_metadata_copy(m, &dec->s.m);
  
  return dec->s.data.audio.source;
  }

gavl_video_source_t *
bgav_stream_decoder_connect_video(bgav_stream_decoder_t * dec,
                                  gavl_packet_source_t * src,
                                  const gavl_compression_info_t * ci,
                                  const gavl_video_format_t * fmt,
                                  gavl_metadata_t * m)
  {
  dec->s.type = BGAV_STREAM_VIDEO;
  bgav_stream_set_from_gavl(&dec->s, ci, NULL, fmt, m);
  
  if(!init_common(dec, src, ci, m))
    return NULL;

  gavl_metadata_copy(m, &dec->s.m);
  
  return dec->s.data.video.source;
  }

int64_t
bgav_stream_decoder_skip(bgav_stream_decoder_t * dec, int64_t t)
  {
  int scale;
  switch(dec->s.type)
    {
    case BGAV_STREAM_AUDIO:
      scale = dec->s.data.audio.format.samplerate;
      break;
    case BGAV_STREAM_VIDEO:
      scale = dec->s.data.video.format.timescale;
      break;
    default:
      return GAVL_TIME_UNDEFINED;
      break;
    }

  bgav_stream_skipto(&dec->s, &t, scale);
  return t;
  }


void
bgav_stream_decoder_reset(bgav_stream_decoder_t * dec)
  {
  bgav_packet_t * p = NULL;
  dec->s.out_time = GAVL_TIME_UNDEFINED;

  if(bgav_stream_get_packet_read(&dec->s, &p) != GAVL_SOURCE_OK)
    {
    bgav_log(&dec->opt, BGAV_LOG_ERROR, LOG_DOMAIN, "Cannot get packet after reset");
    return;
    }
  STREAM_SET_SYNC(&dec->s, p->pts);
  switch(dec->s.type)
    {
    case BGAV_STREAM_AUDIO:
      bgav_audio_resync(&dec->s);
      break;
    case BGAV_STREAM_VIDEO:
      bgav_video_resync(&dec->s);
      break;
    default:
      break;
    }
  }


void
bgav_stream_decoder_destroy(bgav_stream_decoder_t * dec)
  {
  bgav_stream_stop(&dec->s);
  bgav_stream_free(&dec->s);
  

  bgav_options_free(&dec->opt);
  free(dec);
  }
