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


#include <stdlib.h>
#include <stdio.h>
#include <avdec_private.h>

#define ID_8SVX BGAV_MK_FOURCC('8', 'S', 'V', 'X')
#define ID_VHDR BGAV_MK_FOURCC('V', 'H', 'D', 'R')

#define ID_BODY BGAV_MK_FOURCC('B', 'O', 'D', 'Y')
#define ID_NAME BGAV_MK_FOURCC('N', 'A', 'M', 'E')
#define ID_COPY BGAV_MK_FOURCC('(', 'c', ')', ' ')
#define ID_AUTH BGAV_MK_FOURCC('A', 'U', 'T', 'H')
#define ID_ANNO BGAV_MK_FOURCC('A', 'N', 'N', 'O')


#define SAMPLES2READ 1024
#define BITSPERSAMPLES 8
/* 8svx demuxer */

typedef struct
  {
  uint32_t data_size;
  int samples_per_block;
  int bytes_per_second;
  } svx_priv_t;

typedef struct
  {
  uint32_t fourcc;
  uint32_t size;
  } chunk_header_t;

static int read_chunk_header(bgav_input_context_t * ctx, chunk_header_t * ret)
  {
  return bgav_input_read_fourcc(ctx, &ret->fourcc) && bgav_input_read_32_be(ctx, &ret->size);
  }

#if 0
static void dump_chunk_header(chunk_header_t * ret)
  {
  bgav_dprintf("chunk_header\n");
  bgav_dprintf("  .fourcc =            ");
  bgav_dump_fourcc(ret->fourcc);
  bgav_dprintf("\n");
  bgav_dprintf("  size:              %d\n",ret->size);
  }
#endif

typedef struct
  {
  uint32_t oneShotHiSamples;	/* samples in the high octave 1-shot part */
  uint32_t repeatHiSamples;	/* samples in the high octave repeat part */
  uint32_t samplesPerHiCycle;	/* samples/cycle in high octave, else 0   */
  uint16_t samplesPerSec;	/* data sampling rate	*/
  uint8_t ctOctave;		/* octaves of waveforms	*/
  uint8_t sCompression;	/* data compression technique used	*/
  uint32_t volume;		/* playback volume from 0 to Unity (full 
				 * volume). Map this value into the output 
				 * hardware's dynamic range.	*/
  } VHDR_t;


#if 0
static void dump_VHDR(VHDR_t * v)
  {
  bgav_dprintf("VHDR\n");
  bgav_dprintf("  oneShotHiSamples:  %d\n",v->oneShotHiSamples);
  bgav_dprintf("  repeatHiSamples:   %d\n",v->repeatHiSamples);
  bgav_dprintf("  samplesPerHiCycle: %d\n",v->samplesPerHiCycle);

  bgav_dprintf("  samplesPerSec:     %d\n",v->samplesPerSec);
  bgav_dprintf("  ctOctave:          %d\n",v->ctOctave);
  bgav_dprintf("  sCompression:      %d\n",v->sCompression);
  bgav_dprintf("  volume:            %d\n",v->volume);
  }
#endif

static int read_VHDR(bgav_input_context_t * ctx, VHDR_t * ret)
  {
  if(!bgav_input_read_32_be(ctx, &ret->oneShotHiSamples) ||
     !bgav_input_read_32_be(ctx, &ret->repeatHiSamples) ||
     !bgav_input_read_32_be(ctx, &ret->samplesPerHiCycle) ||
     !bgav_input_read_16_be(ctx, &ret->samplesPerSec) ||
     !bgav_input_read_data(ctx, &ret->ctOctave, 1) ||
     !bgav_input_read_data(ctx, &ret->sCompression, 1) ||
     !bgav_input_read_32_be(ctx, &ret->volume))
    return 0;
#if 0
  dump_VHDR(ret);
#endif
  
  return 1;
  }

static int read_meta_data(bgav_demuxer_context_t * ctx, chunk_header_t * ret)
  {
  char * buffer;
  buffer = calloc(1, ret->size + 1);
  
  if(!bgav_input_read_data(ctx->input, (uint8_t*)buffer, ret->size))
    return 0;

  switch(ret->fourcc)
    {
    case ID_NAME:
      ctx->tt->cur->metadata.title = buffer;
      break;
    case ID_COPY:
      ctx->tt->cur->metadata.copyright = buffer;
      break;
    case ID_AUTH:
      ctx->tt->cur->metadata.author = buffer;
      break;
    case ID_ANNO:
      ctx->tt->cur->metadata.comment
        = buffer;
      break;
    }
  return 1;
  }
                     
static int probe_8svx(bgav_input_context_t * input)
  {
  uint8_t test_data[12];
  if(bgav_input_get_data(input, test_data, 12) < 12)
    return 0;
  if((test_data[0] == 'F') && (test_data[1] == 'O') && (test_data[2] == 'R') && (test_data[3] == 'M') &&
     (test_data[8] == '8') && (test_data[9] == 'S') && (test_data[10] == 'V') && (test_data[11] == 'X'))
    return 1;
  return 0;
  }

static int open_8svx(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  VHDR_t hdr;
  svx_priv_t * priv;
  bgav_stream_t * as;
  int64_t total_samples;
  chunk_header_t chunk_header;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  
  /* Create track */
  ctx->tt = bgav_track_table_create(1);

  /* Skip header */
  bgav_input_skip(ctx->input, 12);
  
  
  while(1)
    {
    read_chunk_header(ctx->input, &chunk_header);

#if 0
    dump_chunk_header(&chunk_header);
#endif
    
    switch(chunk_header.fourcc)
      {
      case ID_VHDR:
        if(!read_VHDR(ctx->input, &hdr))
          return 0;
        break;
      case ID_NAME:
      case ID_COPY:
      case ID_AUTH:
      case ID_ANNO:
        if(!read_meta_data(ctx, &chunk_header))
          return 0;
        break;
      case ID_BODY:
        break;
      default:
        bgav_input_skip(ctx->input, chunk_header.size);
        break;
      }
    if(chunk_header.fourcc == ID_BODY)
      break;
    }
   
  if(hdr.sCompression > 0)
    return 0;
  
  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;

  priv->data_size  = chunk_header.size;

  as = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  as->fourcc = BGAV_MK_FOURCC('t', 'w', 'o', 's');
  as->data.audio.format.samplerate = hdr.samplesPerSec;
  as->data.audio.format.num_channels = 1;
  as->data.audio.block_align = 1;
  as->data.audio.bits_per_sample = BITSPERSAMPLES;

  total_samples = priv->data_size / as->data.audio.block_align;
  if(priv->data_size)
    ctx->tt->cur->duration =  gavl_samples_to_time(as->data.audio.format.samplerate, total_samples);

  if(ctx->input->input->seek_byte)
      ctx->flags |= BGAV_DEMUXER_CAN_SEEK;

  ctx->stream_description = bgav_sprintf("8SVX");
  ctx->index_mode = INDEX_MODE_PCM;
  return 1;
  }

static int64_t samples_to_bytes(bgav_stream_t * s, int samples)
  {
  return  s->data.audio.block_align * samples;
  }

static int next_packet_8svx(bgav_demuxer_context_t * ctx)
  {
  svx_priv_t * priv;
  bgav_packet_t * p;
  bgav_stream_t * s;
  int bytes_read;
  int bytes_to_read;
    
  s = &(ctx->tt->cur->audio_streams[0]);

  priv = (svx_priv_t *)(ctx->priv);
  
  bytes_to_read = samples_to_bytes(s, SAMPLES2READ);
  
  if(ctx->input->position + bytes_to_read > ctx->data_start + priv->data_size)
    bytes_to_read = ctx->data_start + priv->data_size - ctx->input->position;

  if(bytes_to_read <= 0)
    return 0;
  
  p = bgav_stream_get_packet_write(s);

  bgav_packet_alloc(p, bytes_to_read);

  p->pts = (ctx->input->position - ctx->data_start) / s->data.audio.block_align;
  p->keyframe = 1;

  bytes_read = bgav_input_read_data(ctx->input, p->data, bytes_to_read);
  
  p->data_size = bytes_read;
  
  bgav_packet_done_write(p);
  return 1;
  }

static void seek_8svx(bgav_demuxer_context_t * ctx, gavl_time_t time,
                      int scale)
  {
  bgav_stream_t * s;
  int64_t position;
  int64_t sample;
  svx_priv_t * priv;
  s = &(ctx->tt->cur->audio_streams[0]);
  priv = (svx_priv_t*)(ctx->priv);

  sample = gavl_time_rescale(scale, s->data.audio.format.samplerate, time);
  
  position =  samples_to_bytes(s, sample) + ctx->data_start;
  bgav_input_seek(ctx->input, position, SEEK_SET);
  s->in_time = sample;
  }

static void close_8svx(bgav_demuxer_context_t * ctx)
  {
  svx_priv_t * priv;
  priv = (svx_priv_t*)(ctx->priv);
  free(priv);
  }

const bgav_demuxer_t bgav_demuxer_8svx =
  {
    .probe =       probe_8svx,
    .open =        open_8svx,
    .next_packet = next_packet_8svx,
    .seek =        seek_8svx,
    .close =       close_8svx
  };
