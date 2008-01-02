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


/*
 *  Flac support is implemented the following way:
 *  We parse the toplevel format here, without the
 *  need for FLAC specific libraries. Then we put the
 *  whole header into the extradata of the stream
 *
 *  The flac audio decoder will then be done using FLAC
 *  software
 */

#include <avdec_private.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis_comment.h>

typedef struct
  {
  uint16_t min_blocksize;
  uint16_t max_blocksize;

  uint32_t min_framesize;
  uint32_t max_framesize;

  uint32_t samplerate;
  int num_channels;
  int bits_per_sample;
  int64_t total_samples;
  } streaminfo_t;

#define STREAMINFO_SIZE 38

static int streaminfo_read(bgav_input_context_t * ctx,
                    streaminfo_t * ret)
  {
  uint64_t tmp_1;
  
  if(!bgav_input_read_16_be(ctx, &(ret->min_blocksize)) ||
     !bgav_input_read_16_be(ctx, &(ret->max_blocksize)) ||
     !bgav_input_read_24_be(ctx, &(ret->min_framesize)) ||
     !bgav_input_read_24_be(ctx, &(ret->max_framesize)) ||
     !bgav_input_read_64_be(ctx, &(tmp_1)))
    return 0;

  ret->samplerate      =   tmp_1 >> 44;
  ret->num_channels    = ((tmp_1 >> 41) & 0x7) + 1;
  ret->bits_per_sample = ((tmp_1 >> 36) & 0x1f) + 1;
  ret->total_samples   = (tmp_1 & 0xfffffffffLL);

  /* Skip MD5 */
  bgav_input_skip(ctx, 16);
  
  return 1;
  }
#if 0
static void streaminfo_dump(streaminfo_t * s)
  {
  bgav_dprintf("FLAC Streaminfo\n");
  bgav_dprintf("Blocksize [%d/%d]\n", s->min_blocksize,
          s->max_blocksize);
  bgav_dprintf("Framesize [%d/%d]\n", s->min_framesize,
          s->max_framesize);
  bgav_dprintf("Samplerate:      %d\n", s->samplerate);
  bgav_dprintf("Num channels:    %d\n", s->num_channels);
  bgav_dprintf("Bits per sample: %d\n", s->bits_per_sample);
  bgav_dprintf("Total samples:   %" PRId64 "\n", s->total_samples);
  }
#endif
/* Seek table */

typedef struct
  {
  int num_entries;
  struct
    {
    uint64_t sample_number;
    uint64_t offset;
    uint16_t num_samples;
    } * entries;
  } seektable_t;

static int seektable_read(bgav_input_context_t * input,
                          seektable_t * ret,
                          int size)
  {
  int i;
  ret->num_entries = size / 18;
  ret->entries = malloc(ret->num_entries * sizeof(*(ret->entries)));

  for(i = 0; i < ret->num_entries; i++)
    {
    if(!bgav_input_read_64_be(input, &(ret->entries[i].sample_number)) ||
       !bgav_input_read_64_be(input, &(ret->entries[i].offset)) ||
       !bgav_input_read_16_be(input, &(ret->entries[i].num_samples)))
      return 0;
    }
  return 1;
  }
#if 0
static void seektable_dump(seektable_t * t)
  {
  int i;
  bgav_dprintf("Seektable: %d entries\n", t->num_entries);
  for(i = 0; i < t->num_entries; i++)
    {
    bgav_dprintf("Sample: %" PRId64 ", Position: %" PRId64 ", Num samples: %d\n",
            t->entries[i].sample_number,
            t->entries[i].offset,
            t->entries[i].num_samples);
    }
  }
#endif

/*
 *  Vorbis comment.
 *  These are simple enough to support directly
 */




/* Probe */

static int probe_flac(bgav_input_context_t * input)
  {
  uint8_t probe_data[4];

  if(bgav_input_get_data(input, probe_data, 4) < 4)
    return 0;

  if((probe_data[0] == 'f') &&
     (probe_data[1] == 'L') &&
     (probe_data[2] == 'a') &&
     (probe_data[3] == 'C'))
    return 1;
     
  return 0;
  }

typedef struct
  {
  streaminfo_t streaminfo;
  seektable_t seektable;
  } flac_priv_t;

static int open_flac(bgav_demuxer_context_t * ctx,
                     bgav_redirector_context_t ** redir)
  {
  bgav_stream_t * s;
  uint8_t header[4];
  uint32_t size;
  flac_priv_t * priv;
  bgav_input_context_t * input_mem;
  uint8_t * comment_buffer;

  bgav_vorbis_comment_t vc;
    
  /* Skip header */

  bgav_input_skip(ctx->input, 4);

  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  ctx->tt = bgav_track_table_create(1);
  
  header[0] = 0;
  
  while(!(header[0] & 0x80))
    {
    if(bgav_input_read_data(ctx->input, header, 4) < 4)
      return 0;
    
    size = header[1];
    size <<= 8;
    size |= header[2];
    size <<= 8;
    size |= header[3];
    //    if(!bgav_input_read_24_be(ctx->input, &size))
    //      return 0;
    
    switch(header[0] & 0x7F)
      {
      case 0: // STREAMINFO
        /* Add audio stream */
        s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);

        s->ext_size = STREAMINFO_SIZE + 4;
        s->ext_data = malloc(s->ext_size);

        s->ext_data[0] = 'f';
        s->ext_data[1] = 'L';
        s->ext_data[2] = 'a';
        s->ext_data[3] = 'C';
        
        memcpy(s->ext_data+4, header, 4);
        
        /* We tell the decoder, that this is the last metadata packet */
        s->ext_data[4] |= 0x80;
        
        if(bgav_input_read_data(ctx->input, s->ext_data + 8,
                                STREAMINFO_SIZE - 4) < 
           STREAMINFO_SIZE - 4)
          goto fail;
        
        input_mem =
          bgav_input_open_memory(s->ext_data + 8, STREAMINFO_SIZE - 4,
                                 ctx->opt);
        
        if(!streaminfo_read(input_mem, &(priv->streaminfo)))
          goto fail;
        bgav_input_close(input_mem);
        bgav_input_destroy(input_mem);
        //        streaminfo_dump(&(priv->streaminfo));
        
        s->data.audio.format.num_channels = priv->streaminfo.num_channels;
        s->data.audio.format.samplerate   = priv->streaminfo.samplerate;
        s->data.audio.bits_per_sample     = priv->streaminfo.bits_per_sample;
        s->fourcc = BGAV_MK_FOURCC('F', 'L', 'A', 'C');
        
        if(priv->streaminfo.total_samples)
          ctx->tt->cur->duration =
            gavl_samples_to_time(priv->streaminfo.samplerate,
                                 priv->streaminfo.total_samples);
        
        //        bgav_input_skip(ctx->input, size);
        break;
      case 1: // PADDING
        bgav_input_skip(ctx->input, size);
        break;
      case 2: // APPLICATION
        bgav_input_skip(ctx->input, size);
        break;
      case 3: // SEEKTABLE
        if(!seektable_read(ctx->input, &(priv->seektable), size))
          goto fail;
        //        seektable_dump(&(priv->seektable));
        //        bgav_input_skip(ctx->input, size);
        break;
      case 4: // VORBIS_COMMENT
        comment_buffer = malloc(size);
        if(bgav_input_read_data(ctx->input, comment_buffer, size) < size)
          return 0;

        input_mem =
          bgav_input_open_memory(comment_buffer, size, ctx->opt);

        memset(&vc, 0, sizeof(vc));

        if(bgav_vorbis_comment_read(&vc, input_mem))
          {
          bgav_vorbis_comment_2_metadata(&vc,
                                         &(ctx->tt->cur->metadata));
          }
        //        bgav_hexdump(comment_buffer, size, 16);
        bgav_vorbis_comment_free(&vc);
        bgav_input_close(input_mem);
        bgav_input_destroy(input_mem);
        
        free(comment_buffer);
        //        bgav_input_skip(ctx->input, size);
        break;
      case 5: // CUESHEET
        bgav_input_skip(ctx->input, size);
        break;
      default:
        bgav_input_skip(ctx->input, size);
      }

    }
  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  
  ctx->stream_description = bgav_strdup("FLAC Format");

  if(priv->seektable.num_entries && ctx->input->input->seek_byte)
    ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
  
  return 1;
    fail:
  return 0;
  }

#define PACKET_BYTES 1024

static int next_packet_flac(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;

  s = bgav_track_find_stream(ctx->tt->cur, 0);
  
  /* We play dumb and read just 1024 bytes */

  p = bgav_stream_get_packet_write(s);
  
  bgav_packet_alloc(p, PACKET_BYTES);
  p->data_size = bgav_input_read_data(ctx->input, 
                                      p->data, PACKET_BYTES);

  if(!p->data_size)
    return 0;

  bgav_packet_done_write(p);
  return 1;
  }

static void seek_flac(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  int i;
  flac_priv_t * priv;
  int64_t sample_pos;
  
  
  priv = (flac_priv_t*)(ctx->priv);
  
  sample_pos = gavl_time_rescale(scale,
                                 priv->streaminfo.samplerate,
                                 time);
  
  for(i = 0; i < priv->seektable.num_entries - 1; i++)
    {
    if((priv->seektable.entries[i].sample_number <= sample_pos) &&
       (priv->seektable.entries[i+1].sample_number >= sample_pos))
      break;
    }
  
  if(priv->seektable.entries[i+1].sample_number == sample_pos)
    i++;
  
  /* Seek to the point */
    
  bgav_input_seek(ctx->input,
                  priv->seektable.entries[i].offset + ctx->data_start,
                  SEEK_SET);
  
  ctx->tt->cur->audio_streams[0].time_scaled = priv->seektable.entries[i].sample_number;
  }

static void close_flac(bgav_demuxer_context_t * ctx)
  {
  flac_priv_t * priv;
  priv = (flac_priv_t*)(ctx->priv);

  if(priv->seektable.num_entries)
    free(priv->seektable.entries);

  if(ctx->tt->cur->audio_streams[0].ext_data)
    free(ctx->tt->cur->audio_streams[0].ext_data);

  free(priv);
  }

bgav_demuxer_t bgav_demuxer_flac =
  {
    probe:       probe_flac,
    open:        open_flac,
    next_packet: next_packet_flac,
    seek:        seek_flac,
    close:       close_flac
  };


