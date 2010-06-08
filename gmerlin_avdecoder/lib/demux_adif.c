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
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>

/* Supported header types */

#define ADIF_SIZE 17

#define BYTES_TO_READ (768*GAVL_MAX_CHANNELS)

#define IS_ADIF(h) ((h[0] == 'A') && \
                    (h[1] == 'D') && \
                    (h[2] == 'I') && \
                    (h[3] == 'F'))

/* AAC demuxer */

typedef struct
  {
  int type;

  int64_t data_size;
  
  uint32_t seek_table_size;

  int64_t sample_count;
  
  } aac_priv_t;

static int probe_adif(bgav_input_context_t * input)
  {
  uint8_t header[4];
  
  if(bgav_input_get_data(input, header, 4) < 4)
    return 0;
  
  if(IS_ADIF(header))
    return 1;
  return 0;
  }

static int open_adif(bgav_demuxer_context_t * ctx)
  {
  aac_priv_t * priv;
  bgav_stream_t * s;
  bgav_id3v1_tag_t * id3v1 = (bgav_id3v1_tag_t*)0;
  bgav_metadata_t id3v1_metadata, id3v2_metadata;

  uint8_t buf[ADIF_SIZE];
  int skip_size;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Create track */

  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  
  ctx->tt = bgav_track_table_create(1);

  /* Check for id3v1 tag at the end */

  if(ctx->input->input->seek_byte)
    {
    bgav_input_seek(ctx->input, -128, SEEK_END);
    if(bgav_id3v1_probe(ctx->input))
      {
      id3v1 = bgav_id3v1_read(ctx->input);
      }
    bgav_input_seek(ctx->input, ctx->data_start, SEEK_SET);
    }

  //  if(ctx->input->id3v2)
  //    bgav_id3v2_dump(ctx->input->id3v2);
  
  if(ctx->input->id3v2 && id3v1)
    {
    memset(&id3v1_metadata, 0, sizeof(id3v1_metadata));
    memset(&id3v2_metadata, 0, sizeof(id3v2_metadata));
    bgav_id3v1_2_metadata(id3v1, &id3v1_metadata);
    bgav_id3v2_2_metadata(ctx->input->id3v2, &id3v2_metadata);
    //    bgav_metadata_dump(&id3v2_metadata);

    bgav_metadata_merge(&ctx->tt->cur->metadata,
                        &id3v2_metadata, &id3v1_metadata);
    bgav_metadata_free(&id3v1_metadata);
    bgav_metadata_free(&id3v2_metadata);
    }
  else if(ctx->input->id3v2)
    bgav_id3v2_2_metadata(ctx->input->id3v2,
                          &ctx->tt->cur->metadata);
  else if(id3v1)
    bgav_id3v1_2_metadata(id3v1,
                          &ctx->tt->cur->metadata);

  if(ctx->input->total_bytes)
    priv->data_size = ctx->input->total_bytes - ctx->data_start;

  if(id3v1)
    {
    bgav_id3v1_destroy(id3v1);
    priv->data_size -= 128;
    }

  s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);

  /* This fourcc reminds the decoder to call a different init function */

  s->fourcc = BGAV_MK_FOURCC('A', 'D', 'I', 'F');

  /* Initialize rest */

  /* Try to get the bitrate */

  if(bgav_input_get_data(ctx->input, buf, ADIF_SIZE) < ADIF_SIZE)
    goto fail;

  skip_size = (buf[4] & 0x80) ? 9 : 0;

  if(buf[4 + skip_size] & 0x10)
    {
    ctx->tt->cur->audio_streams[0].container_bitrate = BGAV_BITRATE_VBR;
    }
  else
    {
    ctx->tt->cur->audio_streams[0].container_bitrate =
      ((unsigned int)(buf[4 + skip_size] & 0x0F)<<19) |
      ((unsigned int)buf[5 + skip_size]<<11) |
      ((unsigned int)buf[6 + skip_size]<<3) |
      ((unsigned int)buf[7 + skip_size] & 0xE0);
    ctx->tt->cur->duration = (GAVL_TIME_SCALE * (priv->data_size) * 8) /
      (ctx->tt->cur->audio_streams[0].container_bitrate);
    }

  if(!ctx->tt->tracks[0].name && ctx->input->metadata.title)
    {
    ctx->tt->tracks[0].name = bgav_strdup(ctx->input->metadata.title);
    }
  
  //  ctx->stream_description = bgav_sprintf("AAC");
  return 1;
  
  fail:
  return 0;
  }

static int next_packet_adif(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;
  aac_priv_t * priv;
  int bytes_read;
  priv = (aac_priv_t *)(ctx->priv);
  s = ctx->tt->cur->audio_streams;
  
  /* Just copy the bytes, we have no idea about
     aac frame boundaries or timestamps here */

  p = bgav_stream_get_packet_write(s);
  bgav_packet_alloc(p, BYTES_TO_READ);
  
  bytes_read = bgav_input_read_data(ctx->input, p->data, BYTES_TO_READ);
  if(!bytes_read)
    return 0;
  p->data_size = bytes_read;

  bgav_stream_done_packet_write(s, p);
  return 1;
  }

static int select_track_adif(bgav_demuxer_context_t * ctx, int track)
  {
  aac_priv_t * priv;
  priv = (aac_priv_t *)(ctx->priv);
  priv->sample_count = 0;
  return 1;
  }

static void close_adif(bgav_demuxer_context_t * ctx)
  {
  aac_priv_t * priv;
  priv = (aac_priv_t *)(ctx->priv);

  free(priv);
  }

const bgav_demuxer_t bgav_demuxer_adif =
  {
    .probe =       probe_adif,
    .open =        open_adif,
    .select_track = select_track_adif,
    .next_packet = next_packet_adif,
    .close =       close_adif
  };
