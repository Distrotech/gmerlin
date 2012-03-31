/*****************************************************************
 * gmerlin-avdecoder - a general purpose multimedia decoding library
 *
 * Copyright (c) 2001 - 2011 Members of the Gmerlin project
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


/* Ape demuxer taken from ffmpeg */

#define MAC_FORMAT_FLAG_8_BIT                 1 // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2 // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4 // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8 // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16 // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32 // create the wave header on decompression (not stored)

#define MAC_SUBFRAME_SIZE 4608

#define APE_EXTRADATA_SIZE 6

#define APE_MIN_VERSION 3950
#define APE_MAX_VERSION 3990

#define LOG_DOMAIN "ape"

/* Headers */

typedef struct
  {
  /* Descriptor */
  uint16_t fileversion;
  uint16_t padding1;
  uint32_t descriptorlength;
  uint32_t headerlength;
  uint32_t seektablelength;
  uint32_t wavheaderlength;
  uint32_t audiodatalength;
  uint32_t audiodatalength_high;
  uint32_t wavtaillength;
  uint8_t md5[16];

  /* Header */

  uint16_t compressiontype;
  uint16_t formatflags;
  uint32_t blocksperframe;
  uint32_t finalframeblocks;
  uint32_t totalframes;
  uint16_t bps;
  uint16_t channels;
  uint32_t samplerate;
  
  } ape_header_t;

typedef struct
  {
  int64_t pos;
  int size;
  int skip;
  } ape_index_entry;

static int ape_header_read(bgav_input_context_t * ctx,
                           ape_header_t * ret)
  {
  if(!bgav_input_read_16_le(ctx, &ret->fileversion))
    return 0;

  if(ret->fileversion < APE_MIN_VERSION ||
     ret->fileversion > APE_MAX_VERSION)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Unsupported file version - %d.%02d",
             ret->fileversion / 1000, (ret->fileversion % 1000) / 10);
    return 0;
    }

  if(ret->fileversion >= 3980)
    {
    if(!bgav_input_read_16_le(ctx, &ret->padding1) ||
       !bgav_input_read_32_le(ctx, &ret->descriptorlength) ||
       !bgav_input_read_32_le(ctx, &ret->headerlength) ||
       !bgav_input_read_32_le(ctx, &ret->seektablelength) ||
       !bgav_input_read_32_le(ctx, &ret->wavheaderlength) ||
       !bgav_input_read_32_le(ctx, &ret->audiodatalength) ||
       !bgav_input_read_32_le(ctx, &ret->audiodatalength_high) ||
       !bgav_input_read_32_le(ctx, &ret->wavtaillength) ||
       (bgav_input_read_data(ctx, ret->md5, 16) < 16))
      return 0;
    if(ret->descriptorlength > 52)
      bgav_input_skip(ctx, ret->descriptorlength - 52);

    if(!bgav_input_read_16_le(ctx, &ret->compressiontype) ||
       !bgav_input_read_16_le(ctx, &ret->formatflags) ||
       !bgav_input_read_32_le(ctx, &ret->blocksperframe) ||
       !bgav_input_read_32_le(ctx, &ret->finalframeblocks) ||
       !bgav_input_read_32_le(ctx, &ret->totalframes) ||
       !bgav_input_read_16_le(ctx, &ret->bps) ||
       !bgav_input_read_16_le(ctx, &ret->channels) ||
       !bgav_input_read_32_le(ctx, &ret->samplerate))
      return 0;
    }
  else
    {
    ret->descriptorlength = 0;
    ret->headerlength = 32;

    if(!bgav_input_read_16_le(ctx, &ret->compressiontype) ||
       !bgav_input_read_16_le(ctx, &ret->formatflags) ||
       !bgav_input_read_16_le(ctx, &ret->channels) ||
       !bgav_input_read_32_le(ctx, &ret->samplerate) ||
       !bgav_input_read_32_le(ctx, &ret->wavheaderlength) ||
       !bgav_input_read_32_le(ctx, &ret->wavtaillength) ||
       !bgav_input_read_32_le(ctx, &ret->totalframes) ||
       !bgav_input_read_32_le(ctx, &ret->finalframeblocks))
      return 0;


    if(ret->formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL)
      {
      bgav_input_skip(ctx, 4); /* Skip the peak level */
      ret->headerlength += 4;
      }

    if(ret->formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS)
      {
      if(!bgav_input_read_32_le(ctx, &ret->seektablelength))
        return 0;
      ret->headerlength += 4;
      ret->seektablelength *= sizeof(int32_t);
      }
    else
      ret->seektablelength = ret->totalframes * sizeof(int32_t);
    
    if(ret->formatflags & MAC_FORMAT_FLAG_8_BIT)
      ret->bps = 8;
    else if(ret->formatflags & MAC_FORMAT_FLAG_24_BIT)
      ret->bps = 24;
    else
      ret->bps = 16;

    if(ret->fileversion >= 3950)
      ret->blocksperframe = 73728 * 4;
    else if(ret->fileversion >= 3900 ||
            (ret->fileversion >= 3800  &&
             ret->compressiontype >= 4000))
      ret->blocksperframe = 73728;
    else
      ret->blocksperframe = 9216;
      
    /* Skip any stored wav header */
    if (!(ret->formatflags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
      bgav_input_skip(ctx, ret->wavheaderlength);
    }
  return 1;
  }

static void ape_header_dump(ape_header_t * h)
  {
  /* Descriptor */
  bgav_dprintf("APE header\n");
  bgav_dprintf("  fileversion:          %d\n", h->fileversion);
  bgav_dprintf("  padding1              %d\n", h->padding1);
  bgav_dprintf("  descriptorlength      %d\n", h->descriptorlength);
  bgav_dprintf("  headerlength          %d\n", h->headerlength);
  bgav_dprintf("  seektablelength       %d\n", h->seektablelength);
  bgav_dprintf("  wavheaderlength       %d\n", h->wavheaderlength);
  bgav_dprintf("  audiodatalength       %d\n", h->audiodatalength);
  bgav_dprintf("  audiodatalength_high  %d\n", h->audiodatalength_high);
  bgav_dprintf("  wavtaillength         %d\n", h->wavtaillength);
  bgav_dprintf("  MD5:                  ");
  bgav_hexdump(h->md5, 16, 16);
  
  /* Header */

  bgav_dprintf("  compressiontype       %d\n", h->compressiontype);
  bgav_dprintf("  formatflags           %d\n", h->formatflags);
  bgav_dprintf("  blocksperframe        %d\n", h->blocksperframe);
  bgav_dprintf("  finalframeblocks      %d\n", h->finalframeblocks);
  bgav_dprintf("  totalframes           %d\n", h->totalframes);
  bgav_dprintf("  bps                   %d\n", h->bps);
  bgav_dprintf("  channels              %d\n", h->channels);
  bgav_dprintf("  samplerate            %d\n", h->samplerate);
  }

static int probe_ape(bgav_input_context_t * input)
  {
  uint8_t probe_data[4];

  if(bgav_input_get_data(input, probe_data, 4) < 4)
    return 0;

  if((probe_data[0] == 'M') &&
     (probe_data[1] == 'A') &&
     (probe_data[2] == 'C') &&
     (probe_data[3] == ' '))
    return 1;
     
  return 0;
  }

typedef struct
  {
  ape_header_t h;
  int64_t file_start_pos;

  ape_index_entry * index;
  int index_size;
  uint32_t current_frame;
  } ape_t;

static int open_ape(bgav_demuxer_context_t * ctx)
  {
  ape_t * priv;
  uint32_t * seektable;
  int i;
  bgav_stream_t * s;

  if(!ctx->input->input->seek_byte)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Can't decode from non-seekable source");
    return 0;
    }
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  priv->file_start_pos = ctx->input->position;
  
  bgav_input_skip(ctx->input, 4); // Skip signature

  if(!ape_header_read(ctx->input, &priv->h))
    return 0;

  if(ctx->opt->dump_headers)
    ape_header_dump(&priv->h);
  
  /* Sanity checks */
  if(!priv->h.totalframes)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "No frames in file");
    return 0;
    }
  if(priv->h.seektablelength / 4 < priv->h.totalframes)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Seek table too small");
    return 0;
    }

  /* Read seek table */
  seektable = malloc(priv->h.seektablelength);
  for(i = 0; i < priv->h.seektablelength / 4; i++)
    {
    if(!bgav_input_read_32_le(ctx->input, seektable + i))
      return 0;
    // fprintf(stderr, "seektable[%d] = %d\n", i, seektable[i]);
    }

  //  fprintf(stderr, "File position: %lld\n", ctx->input->position);
  
  /* Build the index */
  priv->index = calloc(priv->h.totalframes, sizeof(*priv->index));

  priv->index[0].pos =
    priv->file_start_pos + seektable[0];

  for(i = 1; i < priv->h.totalframes; i++)
    {
    priv->index[i].pos = priv->file_start_pos + seektable[i];
    priv->index[i].skip = (priv->index[i].pos - priv->index[0].pos) & 3;
    priv->index[i-1].size = priv->index[i].pos - priv->index[i-1].pos;
    }
  
  /* Get size of last frame */
  if(ctx->input->total_bytes)
    {
    priv->index[priv->h.totalframes-1].size =
      ctx->input->total_bytes -
      priv->index[priv->h.totalframes-1].pos - priv->h.wavtaillength;
    priv->index[priv->h.totalframes-1].size -=
      priv->index[priv->h.totalframes-1].size & 3;
    }
  else
    {
    /* Read as much as we can */
    priv->index[priv->h.totalframes-1].size = priv->h.finalframeblocks * 8;
    }

  for(i = 0; i < priv->h.totalframes; i++)
    {
    if(priv->index[i].skip)
      {
      priv->index[i].pos  -= priv->index[i].skip;
      priv->index[i].size += priv->index[i].skip;
      }
    priv->index[i].size = (priv->index[i].size + 3) & ~3;
    }

  
  /* Set up structures */

  ctx->tt = bgav_track_table_create(1);
  s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  s->data.audio.format.samplerate = priv->h.samplerate;
  s->data.audio.format.num_channels = priv->h.channels;
  s->data.audio.bits_per_sample = priv->h.bps;
  s->fourcc = BGAV_MK_FOURCC('.', 'a', 'p', 'e');
  s->duration = priv->h.blocksperframe * (priv->h.totalframes -1) +
    priv->h.finalframeblocks;

  s->ext_size = APE_EXTRADATA_SIZE;
  s->ext_data = malloc(s->ext_size);
  BGAV_16LE_2_PTR(priv->h.fileversion, s->ext_data);
  BGAV_16LE_2_PTR(priv->h.compressiontype, s->ext_data+2);
  BGAV_16LE_2_PTR(priv->h.formatflags, s->ext_data+4);
  
  ctx->tt->cur->duration =
    gavl_samples_to_time(priv->h.samplerate,
                         s->duration);

  ctx->stream_description = bgav_strdup("APE");
  ctx->index_mode = INDEX_MODE_SIMPLE;
  ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
  
  return 1;
  }

#define EXTRA_SIZE 8

static int next_packet_ape(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;
  ape_t * priv = ctx->priv;

  if(priv->current_frame >= priv->h.totalframes)
    return 0; // EOF
  
  s = bgav_track_find_stream(ctx, 0);

  if(!s)
    return 0;

#if 0
  
  if(ctx->input->position < priv->index[priv->current_frame].pos)
    bgav_input_skip(ctx->input,
                    priv->index[priv->current_frame].pos -
                    ctx->input->position);
  else if(ctx->input->position > priv->index[priv->current_frame].pos)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "BUG: Backwards skip by %d bytes requested",
             ctx->input->position - priv->index[priv->current_frame].pos);
    return 0;
    }
#else
  bgav_input_seek(ctx->input, priv->index[priv->current_frame].pos,
                       SEEK_SET);
#endif
  p = bgav_stream_get_packet_write(s);

  p->position = ctx->input->position;
    
  
  bgav_packet_alloc(p, priv->index[priv->current_frame].size + EXTRA_SIZE);
  
  if(priv->current_frame < priv->h.totalframes-1)
    p->duration = priv->h.blocksperframe;
  else
    p->duration = priv->h.finalframeblocks;
  
  BGAV_32LE_2_PTR(p->duration, p->data);
  BGAV_32LE_2_PTR(priv->index[priv->current_frame].skip, p->data+4);

    
  
  p->data_size = EXTRA_SIZE;
  p->data_size += bgav_input_read_data(ctx->input,
                                       p->data + EXTRA_SIZE,
                                       priv->index[priv->current_frame].size);
  
  p->pts = priv->current_frame * priv->h.blocksperframe;
  
  bgav_stream_done_packet_write(s, p);
  priv->current_frame++;
  return 1;
  }

static void seek_ape(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  ape_t * priv = ctx->priv;
  int64_t sample_pos;
  bgav_stream_t * s = &ctx->tt->cur->audio_streams[0];

  sample_pos = gavl_time_rescale(scale,
                                 priv->h.samplerate,
                                 time);
  
  priv->current_frame = sample_pos / priv->h.blocksperframe;
  STREAM_SET_SYNC(s, priv->current_frame * priv->h.blocksperframe);
  
  }

static int select_track_ape(bgav_demuxer_context_t * ctx, int track)
  {
  ape_t * priv = ctx->priv;
  priv->current_frame = 0;
  return 1;
  }

static void close_ape(bgav_demuxer_context_t * ctx)
  {
  ape_t * priv = ctx->priv;
  if(priv)
    {
    if(priv->index)
      free(priv->index);
    free(priv);
    }
  }

static void resync_ape(bgav_demuxer_context_t * ctx, bgav_stream_t * s)
  {
  ape_t * priv = ctx->priv;
  priv->current_frame =  STREAM_GET_SYNC(s) / priv->h.blocksperframe;
  }

const bgav_demuxer_t bgav_demuxer_ape =
  {
    .probe =       probe_ape,
    .open =        open_ape,
    .next_packet = next_packet_ape,
    .select_track = select_track_ape,
    .seek =        seek_ape,
    .resync        = resync_ape,
    .close =       close_ape
  };
