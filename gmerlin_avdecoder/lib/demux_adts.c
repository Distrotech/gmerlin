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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <avdec_private.h>

#ifdef HAVE_FAAD2
#include <aac_frame.h>
#endif

#define LOG_DOMAIN "adts"

#include <adts_header.h>

/* Supported header types */

#define ADTS_SIZE 9

#define BYTES_TO_READ (768*GAVL_MAX_CHANNELS)
     
#define IS_ADTS(h) ((h[0] == 0xff) && \
                    ((h[1] & 0xf0) == 0xf0) && \
                    ((h[1] & 0x06) == 0x00))

/* AAC demuxer */

typedef struct
  {
  int64_t data_size;
  int64_t sample_count;
  int block_samples;
  } aac_priv_t;

static int probe_adts(bgav_input_context_t * input)
  {
  int ret;
  uint8_t * buffer;
  uint8_t header[7];
  
  bgav_adts_header_t h1, h2;
  
  /* Support aac live streams */

  if(input->mimetype &&
     (!strcmp(input->mimetype, "audio/aacp") ||
      !strcmp(input->mimetype, "audio/aac")))
    return 1;
  
  if(bgav_input_get_data(input, header, 7) < 7)
    return 0;

  if(!bgav_adts_header_read(header, &h1))
    return 0;

  buffer = malloc(ADTS_SIZE + h1.frame_bytes);
  
  if(bgav_input_get_data(input, buffer, ADTS_SIZE + h1.frame_bytes) <
     ADTS_SIZE + h1.frame_bytes)
    return 0;

  ret = 0;

  if(bgav_adts_header_read(buffer + h1.frame_bytes, &h2) &&
     (h1.mpeg_version == h2.mpeg_version) &&
     (h1.samplerate == h2.samplerate) &&
     (h1.channel_configuration == h2.channel_configuration))
    ret = 1;
  free(buffer);
  return ret;
  }

#ifdef HAVE_FAAD2
#define BYTES_TO_SCAN 1024

static int check_he_aac(bgav_demuxer_context_t * ctx,
                        bgav_stream_t * s)
  {
  uint8_t * buffer = NULL;
  int buffer_alloc = 0;
  int buffer_size = 0;
  aac_priv_t * priv;
  int ret = 0;
  
  int done = 0;
  int samples = 0;
  int result;
  int bytes;
  bgav_aac_frame_t * frame;
  int64_t old_position = ctx->input->position;
  frame = bgav_aac_frame_create(ctx->opt, NULL, 0);
  priv = ctx->priv;
  while(!done)
    {
    /* Read data */
    buffer_alloc += BYTES_TO_SCAN;
    buffer = realloc(buffer, buffer_alloc);
    
    if(bgav_input_read_data(ctx->input, buffer + buffer_size, 
                            BYTES_TO_SCAN) < BYTES_TO_SCAN)
      break;

    buffer_size += BYTES_TO_SCAN;
    
    result = bgav_aac_frame_parse(frame, buffer,
                                  buffer_size,
                                  &bytes, &samples);
    if(result < 0)
      goto fail;
    
    if(!result)
      continue;
    
    if(samples)
      {
      if(samples == 2048)
        {
        /* Detected HE-AAC */
        bgav_log(ctx->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Detected HE-AAC");
        s->data.audio.format.samplerate *= 2;
        priv->block_samples *= 2;
        s->gavl_flags |= GAVL_COMPRESSION_SBR;
        }
      else
        {
        bgav_log(ctx->opt, BGAV_LOG_INFO, LOG_DOMAIN, "Detected no HE-AAC");
        }
      break;
      }
    }

  ret = 1;
  fail:
  bgav_aac_frame_destroy(frame);
  if(buffer)
    free(buffer);
  bgav_input_seek(ctx->input, old_position, SEEK_SET);
  return ret;
  }
#endif



static int open_adts(bgav_demuxer_context_t * ctx)
  {
  uint8_t header[4];
  aac_priv_t * priv;
  bgav_stream_t * s;
  bgav_id3v1_tag_t * id3v1 = NULL;
  gavl_metadata_t id3v1_metadata, id3v2_metadata;
  uint8_t buf[ADTS_SIZE];

  bgav_adts_header_t adts;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  /* Recheck header */

  while(1)
    {
    if(bgav_input_get_data(ctx->input, header, 4) < 4)
      return 0;
    
    if(IS_ADTS(header))
      break;
    bgav_input_skip(ctx->input, 1);
    }
  
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
    //    gavl_metadata_dump(&id3v2_metadata);

    gavl_metadata_merge(&ctx->tt->cur->metadata,
                        &id3v2_metadata, &id3v1_metadata);
    gavl_metadata_free(&id3v1_metadata);
    gavl_metadata_free(&id3v2_metadata);
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

  s->fourcc = BGAV_MK_FOURCC('A', 'D', 'T', 'S');

  /* Initialize rest */

  if(bgav_input_get_data(ctx->input, buf, ADTS_SIZE) < ADTS_SIZE)
    goto fail;

  if(!bgav_adts_header_read(buf, &adts))
    goto fail;

  if(adts.profile == 2) 
    priv->block_samples = 960;
  else
    priv->block_samples = 1024;
  
  s->data.audio.format.samplerate = adts.samplerate;
  
  gavl_metadata_set(&ctx->tt->cur->metadata, 
                    GAVL_META_FORMAT, "ADTS");

  
  //  adts_header_dump(&adts);

  ctx->index_mode = INDEX_MODE_SIMPLE;

#ifdef HAVE_FAAD2
  if(ctx->input->input->seek_byte)
    {
    /* Check for HE-AAC and update sample counts */
    if(!check_he_aac(ctx, s))
      goto fail;
    }
#endif
  
  //  ctx->stream_description = bgav_sprintf("AAC");
  return 1;
  
  fail:
  return 0;
  }

static int next_packet_adts(bgav_demuxer_context_t * ctx)
  {
  bgav_packet_t * p;
  bgav_stream_t * s;
  bgav_adts_header_t adts;
  aac_priv_t * priv;
  uint8_t buf[ADTS_SIZE];
  
  priv = ctx->priv;

  s = ctx->tt->cur->audio_streams;

  if(bgav_input_get_data(ctx->input, buf, ADTS_SIZE) < ADTS_SIZE)
    return 0;

  if(!bgav_adts_header_read(buf, &adts))
    return 0;

  
  p = bgav_stream_get_packet_write(s);
  
  p->pts = priv->sample_count;
  p->duration = priv->block_samples * adts.num_blocks;
  p->position = ctx->input->position;

  PACKET_SET_KEYFRAME(p);
  
  bgav_packet_alloc(p, adts.frame_bytes);

  p->data_size = bgav_input_read_data(ctx->input, p->data, adts.frame_bytes);

  if(p->data_size < adts.frame_bytes)
    return 0;
  
  bgav_stream_done_packet_write(s, p);

  priv->sample_count += priv->block_samples * adts.num_blocks;
  
  return 1;
  }

static void resync_adts(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  aac_priv_t * priv;
  priv = ctx->priv;
  priv->sample_count = STREAM_GET_SYNC(ctx->tt->cur->audio_streams);
  }

static int select_track_adts(bgav_demuxer_context_t * ctx, int track)
  {
  aac_priv_t * priv;
  priv = ctx->priv;
  priv->sample_count = 0;
  return 1;
  }

static void close_adts(bgav_demuxer_context_t * ctx)
  {
  aac_priv_t * priv;
  priv = ctx->priv;

  free(priv);
  }

const bgav_demuxer_t bgav_demuxer_adts =
  {
    .probe =       probe_adts,
    .open =        open_adts,
    .select_track = select_track_adts,
    .next_packet = next_packet_adts,
    .resync        = resync_adts,
    .close =       close_adts,
  };
