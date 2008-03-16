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

#include <avdec_private.h>
#include <stdlib.h>
#include <stdio.h>

#include <a52dec/a52.h>

/* A52 demuxer */

#define FRAME_SAMPLES 1536 /* 6 * 256 */
#define SYNC_BYTES    3840 /* Maximum possible length of a frame */

typedef struct
  {
  int64_t frame_count;
  int samplerate;

  int64_t data_size;
  } a52_priv_t;

static int probe_a52(bgav_input_context_t * input)
  {
  int dummy_flags;
  int dummy_srate;
  int dummy_brate;
  
  uint8_t test_data[7];
  if(bgav_input_get_data(input, test_data, 7) < 7)
    return 0;
  
  return !!a52_syncinfo(test_data, &dummy_flags, &dummy_srate, &dummy_brate);
  }

static int open_a52(bgav_demuxer_context_t * ctx,
                    bgav_redirector_context_t ** redir)
  {
  int dummy_flags;
  int bitrate;
  uint8_t test_data[7];
  a52_priv_t * priv;
  bgav_stream_t * s;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Recheck header */
  
  if(bgav_input_get_data(ctx->input, test_data, 7) < 7)
    return 0;

  if(!a52_syncinfo(test_data, &dummy_flags, &(priv->samplerate),
                   &bitrate))
    goto fail;
  
  /* Create track */

  ctx->tt = bgav_track_table_create(1);
  
  s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  s->container_bitrate = bitrate;
    
  /* We just set the fourcc, everything else will be set by the decoder */

  s->fourcc = BGAV_MK_FOURCC('.', 'a', 'c', '3');
  
  //  bgav_stream_dump(s);
  
  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  
  if(ctx->input->total_bytes)
    priv->data_size = ctx->input->total_bytes - ctx->data_start;
  
  /* Packet size will be at least 1024 bytes */
  
  if(ctx->input->input->seek_byte)
    ctx->flags |= BGAV_DEMUXER_CAN_SEEK;

  ctx->tt->cur->duration
    = ((int64_t)priv->data_size * (int64_t)GAVL_TIME_SCALE) / 
    (s->container_bitrate / 8);
  ctx->index_mode = INDEX_MODE_SIMPLE;
  ctx->stream_description = bgav_sprintf("Raw A52");
  return 1;
  
  fail:
  return 0;
  }

static int next_packet_a52(bgav_demuxer_context_t * ctx)
  {
  int dummy_brate, dummy_srate, dummy_flags;
  bgav_packet_t * p;
  bgav_stream_t * s;
  a52_priv_t * priv;
  uint8_t test_data[7];
  int packet_size, i;
    
  priv = (a52_priv_t *)(ctx->priv);
  
  s = ctx->tt->cur->audio_streams;
  
  p = bgav_stream_get_packet_write(s);
  
  for(i = 0; i < SYNC_BYTES; i++)
    {
    if(bgav_input_get_data(ctx->input, test_data, 7) < 7)
      return 0;
    
    packet_size = a52_syncinfo(test_data, &dummy_flags, &dummy_srate, &dummy_brate);
    if(packet_size)
      break;
    bgav_input_skip(ctx->input, 1);
    }

  if(!packet_size)
    return 0;

  p->pts = FRAME_SAMPLES * priv->frame_count;
  p->duration = FRAME_SAMPLES;
  p->keyframe = 1;
  p->position = ctx->input->position;
  
  priv->frame_count++;

  bgav_packet_alloc(p, packet_size);

  p->data_size = bgav_input_read_data(ctx->input, p->data, packet_size);
  
  if(p->data_size < packet_size)
    return 0;
  
  bgav_packet_done_write(p);

  return 1;
  }

static void seek_a52(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  int64_t file_position;
  a52_priv_t * priv;
  int64_t t;
  bgav_stream_t * s;

  priv = (a52_priv_t *)(ctx->priv);
  
  
  s = ctx->tt->cur->audio_streams;
    
  file_position = (time * (s->container_bitrate / 8)) /
    scale;

  /* Calculate the time before we add the start offset */
  t = ((int64_t)file_position * scale) /
    (s->container_bitrate / 8);

  s->in_time = gavl_time_rescale(scale, priv->samplerate, t);
  priv->frame_count = s->in_time / FRAME_SAMPLES;
  
  file_position += ctx->data_start;
  bgav_input_seek(ctx->input, file_position, SEEK_SET);
  }

static int select_track_a52(bgav_demuxer_context_t * ctx, int track)
  {
  a52_priv_t * priv;
  priv = (a52_priv_t *)(ctx->priv);
  priv->frame_count = 0;
  return 1;
  }

static void close_a52(bgav_demuxer_context_t * ctx)
  {
  a52_priv_t * priv;
  priv = (a52_priv_t *)(ctx->priv);
  free(priv);
  }

const bgav_demuxer_t bgav_demuxer_a52 =
  {
    .probe =       probe_a52,
    .open =        open_a52,
    .select_track = select_track_a52,
    .next_packet = next_packet_a52,
    .seek =        seek_a52,
    .close =       close_a52
  };

