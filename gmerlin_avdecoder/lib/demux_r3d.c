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

#define LOG_DOMAIN "demux_r3d"

typedef struct
  {
  uint32_t len;
  uint32_t fourcc;
  } chunk_t;

static int read_chunk_header(bgav_input_context_t * ctx,
                             chunk_t * ret)
  {
  return bgav_input_read_32_be(ctx, &ret->len) &&
    bgav_input_read_32_be(ctx, &ret->fourcc);
  }

/* Header */

typedef struct
  {
  uint32_t unknown1;
  uint32_t pts_timescale;
  uint32_t unknown2;
  uint32_t unknown3;
  uint32_t unknown4;
  uint32_t unknown5;
  uint32_t unknown6;
  uint32_t unknown7;
  uint32_t unknown8;
  uint32_t unknown9;
  uint32_t unknown10;
  uint32_t width;
  uint32_t height;
  uint32_t video_timescale;
  uint16_t frame_duration ;
  uint8_t  unknown15;
  char name[257]; /* Maximum length: 256 + \0 */
  } red1_t;

static int read_red1(bgav_input_context_t * ctx,
                     red1_t * ret)
  {
  chunk_t ch;
  //  if(!read_chunk_header(ctx, &ch))
  //    return 0;
  if(!bgav_input_read_32_be(ctx, &ret->unknown1) ||
     !bgav_input_read_32_be(ctx, &ret->pts_timescale) ||
     !bgav_input_read_32_be(ctx, &ret->unknown2) ||
     !bgav_input_read_32_be(ctx, &ret->unknown3) ||
     !bgav_input_read_32_be(ctx, &ret->unknown4) ||
     !bgav_input_read_32_be(ctx, &ret->unknown5) ||
     !bgav_input_read_32_be(ctx, &ret->unknown6) ||
     !bgav_input_read_32_be(ctx, &ret->unknown7) ||
     !bgav_input_read_32_be(ctx, &ret->unknown8) ||
     !bgav_input_read_32_be(ctx, &ret->unknown9) ||
     !bgav_input_read_32_be(ctx, &ret->unknown10) ||
     !bgav_input_read_32_be(ctx, &ret->width) ||
     !bgav_input_read_32_be(ctx, &ret->height) ||
     !bgav_input_read_32_be(ctx, &ret->video_timescale) ||
     !bgav_input_read_16_be(ctx, &ret->frame_duration) ||
     !bgav_input_read_8(ctx, &ret->unknown15)
     )
    return 0;

  if(bgav_input_read_data(ctx, (uint8_t*)(ret->name), 257) < 257)
    return 0;
  
  if(ch.len > ctx->position)
    bgav_input_skip(ctx, ch.len - ctx->position);
  return 1;
  }

static void dump_red1(red1_t * red1)
  {
  bgav_dprintf("R3D header:\n");
  bgav_dprintf("  unknown1:        %d (%08x)\n", red1->unknown1,
               red1->unknown1);
  bgav_dprintf("  pts_timescale:   %d\n", red1->pts_timescale);
  bgav_dprintf("  unknown2:        %d\n", red1->unknown2);
  bgav_dprintf("  unknown3:        %d\n", red1->unknown3);
  bgav_dprintf("  unknown4:        %d\n", red1->unknown4);
  bgav_dprintf("  unknown5:        %d\n", red1->unknown5);
  bgav_dprintf("  unknown6:        %d\n", red1->unknown6);
  bgav_dprintf("  unknown7:        %d\n", red1->unknown7);
  bgav_dprintf("  unknown8:        %d\n", red1->unknown8);
  bgav_dprintf("  unknown9:        %d\n", red1->unknown9);
  bgav_dprintf("  unknown10:       %d\n", red1->unknown10);
  bgav_dprintf("  width:           %d\n", red1->width);
  bgav_dprintf("  height:          %d\n", red1->height);
  bgav_dprintf("  video_timescale: %d\n", red1->video_timescale);
  bgav_dprintf("  frame_duration:  %d\n", red1->frame_duration);
  bgav_dprintf("  unknown15:       %d\n", red1->unknown15);
  bgav_dprintf("  name:            %s\n", red1->name);
  }

/* Footer */

typedef struct
  {
  uint32_t rdvo_offset;
  uint32_t rdvs_offset;
  uint32_t rdao_offset;
  uint32_t rdas_offset;

  uint32_t video_packets;
  uint32_t audio_packets;
  uint32_t totlen;
	
  uint32_t max_video_size;
  uint32_t max_audio_size;

  uint32_t unknown3;
  uint32_t unknown4;
  uint32_t unknown5;
  } reob_t;

static int read_reob(bgav_input_context_t * ctx,
                     reob_t * ret)
  {
  return
    bgav_input_read_32_be(ctx, &ret->rdvo_offset) &&
    bgav_input_read_32_be(ctx, &ret->rdvs_offset) &&
    bgav_input_read_32_be(ctx, &ret->rdao_offset) &&
    bgav_input_read_32_be(ctx, &ret->rdas_offset) &&
    bgav_input_read_32_be(ctx, &ret->video_packets) &&
    bgav_input_read_32_be(ctx, &ret->audio_packets) &&
    bgav_input_read_32_be(ctx, &ret->totlen) &&
    bgav_input_read_32_be(ctx, &ret->max_video_size) &&
    bgav_input_read_32_be(ctx, &ret->max_audio_size) &&
    bgav_input_read_32_be(ctx, &ret->unknown3) &&
    bgav_input_read_32_be(ctx, &ret->unknown4) &&
    bgav_input_read_32_be(ctx, &ret->unknown5);
  }

static void dump_reob(reob_t * reob)
  {
  bgav_dprintf("reob:\n");
  bgav_dprintf("  rdvo_offset:    %d\n", reob->rdvo_offset);
  bgav_dprintf("  rdvs_offset:    %d\n", reob->rdvs_offset);
  bgav_dprintf("  rdao_offset:    %d\n", reob->rdao_offset);
  bgav_dprintf("  rdas_offset:    %d\n", reob->rdas_offset);
  bgav_dprintf("  video_packets:  %d\n", reob->video_packets);
  bgav_dprintf("  audio_packets:  %d\n", reob->audio_packets);
  bgav_dprintf("  totlen:         %d\n", reob->totlen);
  bgav_dprintf("  max_video_size: %d\n", reob->max_video_size);
  bgav_dprintf("  max_audio_size: %d\n", reob->max_audio_size);
  bgav_dprintf("  unknown3:       %d\n", reob->unknown3);
  bgav_dprintf("  unknown4:       %d\n", reob->unknown4);
  bgav_dprintf("  unknown5:       %d\n", reob->unknown5);
  }

/* Index */

static uint32_t * read_index(bgav_input_context_t * ctx,
                             uint32_t num_packets,
                             uint32_t fourcc,
                             uint32_t offset)
  {
  chunk_t ch;
  uint32_t * ret;
  int i;
  bgav_input_seek(ctx, offset, SEEK_SET);
  if(!read_chunk_header(ctx, &ch))
    return 0;
  if(ch.fourcc != fourcc)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Wrong index tag (broken file)");
    return (uint32_t*)0;
    }
  if(ch.len - 8 < num_packets / 4)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Index chunk too small");
    return (uint32_t*)0;
    }
  ret = malloc(num_packets * sizeof(*ret));
  for(i = 0; i < num_packets; i++)
    {
    if(!bgav_input_read_32_be(ctx, &ret[i]))
      {
      bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
               "Unexpected EOF in index");
      free(ret);
      return (uint32_t*)0;
      }
    }
  return ret;
  }

typedef struct
  {
  red1_t red1;
  reob_t reob;
  uint32_t * rdas; /* Audio sizes */
  uint32_t * rdvs; /* Video sizes */
  uint32_t * rdao; /* Audio offsets */
  uint32_t * rdvo; /* Video offsets */
  } r3d_priv_t;

static int probe_r3d(bgav_input_context_t * input)
  {
  uint8_t test_data[8];
  if(bgav_input_get_data(input, test_data, 8) < 8)
    return 0;
  if((test_data[4] == 'R') &&
     (test_data[5] == 'E') &&
     (test_data[6] == 'D') &&
     (test_data[7] == '1'))
    return 1;
  return 0;
  }

static int open_r3d(bgav_demuxer_context_t * ctx)
  {
  chunk_t ch;
  r3d_priv_t * priv;

  if(!ctx->input->input->seek_byte)
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "R3D cannot be read from nonseekable sources");
    return 0;
    }
     
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  if(!read_chunk_header(ctx->input, &ch))
    return 0;
  if(ch.fourcc != BGAV_MK_FOURCC('R','E','D','1'))
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Got no header");
    return 0;
    }
  if(!read_red1(ctx->input, &priv->red1))
    return 0;
  dump_red1(&priv->red1);
  
  bgav_input_seek(ctx->input, -0x38, SEEK_END);
  if(!read_chunk_header(ctx->input, &ch))
    return 0;

  if(ch.fourcc != BGAV_MK_FOURCC('R','E','O','B'))
    {
    bgav_log(ctx->opt, BGAV_LOG_ERROR, LOG_DOMAIN,
             "Got no footer");
    return 0;
    }
    
  
  if(!read_reob(ctx->input, &priv->reob))
    return 0;
  dump_reob(&priv->reob);
  
  /* Read indices */
  priv->rdvo = read_index(ctx->input,
                          priv->reob.video_packets,
                          BGAV_MK_FOURCC('R','D','V','O'),
                          priv->reob.rdvo_offset);

  if(!priv->rdvo)
    return 0; 
  
  priv->rdvs = read_index(ctx->input,
                          priv->reob.video_packets,
                          BGAV_MK_FOURCC('R','D','V','S'),
                          priv->reob.rdvs_offset);
  if(!priv->rdvs)
    return 0; 
  priv->rdao = read_index(ctx->input,
                          priv->reob.audio_packets,
                          BGAV_MK_FOURCC('R','D','A','O'),
                          priv->reob.rdao_offset);
  if(!priv->rdao)
    return 0; 
  priv->rdas = read_index(ctx->input,
                          priv->reob.video_packets,
                          BGAV_MK_FOURCC('R','D','A','S'),
                          priv->reob.rdas_offset);
  if(!priv->rdas)
    return 0; 
  
  bgav_input_seek(ctx->input, 0, SEEK_SET);
    
  while(1)
    {
    fprintf(stderr, "Offset: %ld ", ctx->input->position);
    
    if(!read_chunk_header(ctx->input, &ch))
      {
      fprintf(stderr, "EOF\n");
      return 0;
      }
    fprintf(stderr, "Got chunk: ");
    bgav_dump_fourcc(ch.fourcc);
    fprintf(stderr, " len: %d\n", ch.len);
#if 0
    if((ch.fourcc == BGAV_MK_FOURCC('R','D','V','O')) ||
       (ch.fourcc == BGAV_MK_FOURCC('R','D','V','S')) ||
       (ch.fourcc == BGAV_MK_FOURCC('R','D','A','O')) ||
       (ch.fourcc == BGAV_MK_FOURCC('R','D','A','S')))
      {
      bgav_input_skip_dump(ctx->input, ch.len - 8);
      }
    else
#endif
      bgav_input_skip(ctx->input, ch.len - 8);
    }
  
  return 0;
  }

static int next_packet_r3d(bgav_demuxer_context_t * ctx)
  {
  //  bgav_packet_t * p;
  //  bgav_stream_t * s;
  return 0;
  }

static void
seek_r3d(bgav_demuxer_context_t * ctx, gavl_time_t time, int scale)
  {
  }

static void close_r3d(bgav_demuxer_context_t * ctx)
  {
  r3d_priv_t * priv;
  priv = (r3d_priv_t*)(ctx->priv);
  free(priv);
  }

const bgav_demuxer_t bgav_demuxer_r3d =
  {
    .probe =       probe_r3d,
    .open =        open_r3d,
    .next_packet = next_packet_r3d,
    .seek =        seek_r3d,
    .close =       close_r3d
  };

