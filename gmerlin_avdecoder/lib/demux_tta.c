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
#include <string.h>

#define LOG_DOMAIN "tta"

typedef struct
  {
  uint32_t fourcc;
  uint16_t audio_format;
  uint16_t num_channels;
  uint16_t  bits_per_sample;
  uint32_t  samplerate;
  uint32_t data_length;
  uint32_t crc;
  } tta_header_t;

typedef struct
  {
  uint32_t * seek_table;
  uint32_t total_frames;
  uint32_t current_frame;
  uint32_t samples_per_frame;
  uint32_t data_start;
  
  } tta_priv_t;

static void parse_header(tta_header_t * ret, uint8_t * data)
  {
  ret->fourcc          = BGAV_PTR_2_FOURCC(data); data+=4;
  ret->audio_format    = BGAV_PTR_2_16LE(data); data+=2;
  ret->num_channels    = BGAV_PTR_2_16LE(data); data+=2;
  ret->bits_per_sample = BGAV_PTR_2_16LE(data); data+=2;
  ret->samplerate      = BGAV_PTR_2_32LE(data); data+=4;
  ret->data_length     = BGAV_PTR_2_32LE(data); data+=4;
  ret->crc             = BGAV_PTR_2_32LE(data);
  }

#define HEADER_SIZE 22

#if 0

static void dump_header(tta_header_t * h)
  {
  bgav_dprintf("tta header\n");
  
  bgav_dprintf("  .fourcc =          ");
  bgav_dump_fourcc(h->fourcc);
  bgav_dprintf("\n");

  bgav_dprintf("  audio_format:    %d\n", h->audio_format);
  bgav_dprintf("  num_channels:    %d\n", h->num_channels);
  bgav_dprintf("  bits_per_sample: %d\n", h->bits_per_sample);
  bgav_dprintf("  samplerate:      %d\n", h->samplerate);
  bgav_dprintf("  data_length:     %d\n", h->data_length);
  bgav_dprintf("  crc:             %08x\n", h->crc);
  }
#endif

static int probe_tta(bgav_input_context_t * input)
  {
  uint32_t fourcc;
  if(bgav_input_get_fourcc(input, &fourcc) &&
     (fourcc == BGAV_MK_FOURCC('T','T','A','1')))
    return 1;
  return 0;
  }

static int open_tta(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  bgav_stream_t * s;
  uint8_t header[HEADER_SIZE];
  tta_header_t h;
  tta_priv_t * priv;
  uint8_t * ptr;
  int i;
  
  if(bgav_input_read_data(ctx->input, header, HEADER_SIZE) < HEADER_SIZE)
    return 0;

  parse_header(&h, header);
  //  dump_header(&h);

  /* Set up generic stuff */
  ctx->tt = bgav_track_table_create(1);
  s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);

  s->data.audio.format.num_channels = h.num_channels;
  s->data.audio.format.samplerate = h.samplerate;
  s->fourcc = BGAV_MK_FOURCC('T','T','A','1');

  /* Set up private stuff */

  s->ext_size = HEADER_SIZE;
  s->ext_data = malloc(HEADER_SIZE);
  memcpy(s->ext_data, header, HEADER_SIZE);
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  priv->samples_per_frame = (int)(1.04489795918367346939 * h.samplerate);
  priv->total_frames = h.data_length / priv->samples_per_frame +
    ((h.data_length % priv->samples_per_frame) ? 1 : 0);
  priv->seek_table = malloc(priv->total_frames * sizeof(*priv->seek_table));

  /* Seek_Table + CRC */
  s->ext_size += priv->total_frames * 4 + 4;
  s->ext_data = realloc(s->ext_data, s->ext_size);

  if(bgav_input_read_data(ctx->input, s->ext_data + HEADER_SIZE,
                          priv->total_frames * 4 + 4) <
     priv->total_frames * 4 + 4)
    return 0;
  
  ptr = s->ext_data + HEADER_SIZE;
  for(i = 0; i < priv->total_frames; i++)
    {
    priv->seek_table[i] = BGAV_PTR_2_32LE(ptr); ptr+=4;
    }

  ctx->tt->cur->duration =
    gavl_time_unscale(s->data.audio.format.samplerate, h.data_length);

  if(ctx->input->input->seek_byte)
    ctx->flags |= BGAV_DEMUXER_CAN_SEEK;

  
  priv->data_start = ctx->input->position;
  
  ctx->stream_description = bgav_sprintf("True Audio");

  return 1;
  }

static int next_packet_tta(bgav_demuxer_context_t * ctx)
  {
  tta_priv_t * priv;
  bgav_packet_t * p;
  bgav_stream_t * s;
  priv = (tta_priv_t *)(ctx->priv);

  if(priv->current_frame >= priv->total_frames)
    return 0; // EOF

  s = &(ctx->tt->cur->audio_streams[0]);
  p = bgav_stream_get_packet_write(s);

  bgav_packet_alloc(p, priv->seek_table[priv->current_frame]);
  if(bgav_input_read_data(ctx->input,
                          p->data,
                          priv->seek_table[priv->current_frame]) <
     priv->seek_table[priv->current_frame])
    return 0; // Truncated file

  p->data_size = priv->seek_table[priv->current_frame];
  priv->current_frame++;
  bgav_packet_done_write(p);
  return 1;
  }

static void seek_tta(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  int i;
  int64_t time_scaled, filepos;
  bgav_stream_t * s;
  tta_priv_t * priv;
  priv = (tta_priv_t *)(ctx->priv);
  
  s = &(ctx->tt->cur->audio_streams[0]);
  time_scaled = gavl_time_rescale(scale, s->timescale, time);

  priv->current_frame = time_scaled / priv->samples_per_frame;

  if(priv->current_frame >= priv->total_frames)
    return;
  
  filepos = priv->data_start;
  for(i = 0; i < priv->current_frame; i++)
    filepos += priv->seek_table[i];
  bgav_input_seek(ctx->input, filepos, SEEK_SET);
  s->time_scaled = priv->current_frame * priv->samples_per_frame;
  }

static void close_tta(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  tta_priv_t * priv;
  priv = (tta_priv_t *)(ctx->priv);
  
  if(priv)
    {
    if(priv->seek_table)
      free(priv->seek_table);
    free(priv);
    }
  s = &(ctx->tt->cur->audio_streams[0]);
  if(s->ext_data)
    free(s->ext_data);
  }

const bgav_demuxer_t bgav_demuxer_tta =
  {
    .probe =       probe_tta,
    .open =        open_tta,
    .next_packet = next_packet_tta,
    .seek =        seek_tta,
    .close =       close_tta
  };
