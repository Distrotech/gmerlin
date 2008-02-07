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

#define SMK_AUD_PACKED  0x80000000
#define SMK_AUD_16BITS  0x20000000
#define SMK_AUD_STEREO  0x10000000
#define SMK_AUD_BINKAUD 0x08000000
#define SMK_AUD_USEDCT  0x04000000

#define SMACKER_PAL 0x01

/* palette used in Smacker */
static const uint8_t smk_pal[64] = {
    0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
    0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
    0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D,
    0x61, 0x65, 0x69, 0x6D, 0x71, 0x75, 0x79, 0x7D,
    0x82, 0x86, 0x8A, 0x8E, 0x92, 0x96, 0x9A, 0x9E,
    0xA2, 0xA6, 0xAA, 0xAE, 0xB2, 0xB6, 0xBA, 0xBE,
    0xC3, 0xC7, 0xCB, 0xCF, 0xD3, 0xD7, 0xDB, 0xDF,
    0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB, 0xFF
};

typedef struct
  {
  uint32_t Signature;
  uint32_t Width;
  uint32_t Height;
  uint32_t Frames;
  int32_t FrameRate;
  uint32_t Flags;
  uint32_t AudioSize[7];
  uint32_t TreesSize;
  uint32_t MMap_Size;
  uint32_t MClr_Size;
  uint32_t Full_Size;
  uint32_t Type_Size;
  uint32_t AudioRate[7];
  uint32_t Dummy;
  
  uint32_t * frame_sizes;
  uint8_t  * frame_flags;
  } smacker_header_t;

#define VIDEO_ID     0
#define AUDIO_OFFSET 1


static int read_header(bgav_input_context_t * input,
                       smacker_header_t * ret)
  {
  int i;
  if(!bgav_input_read_fourcc(input, &ret->Signature) ||
     !bgav_input_read_32_le(input, &ret->Width) ||
     !bgav_input_read_32_le(input, &ret->Height) ||
     !bgav_input_read_32_le(input, &ret->Frames) ||
     !bgav_input_read_32_le(input, (uint32_t*)(&ret->FrameRate)) ||
     !bgav_input_read_32_le(input, &ret->Flags))
    return 0;
  for(i = 0; i < 7; i++)
    {
    if(!bgav_input_read_32_le(input, &ret->AudioSize[i]))
      return 0;
    }
  if(!bgav_input_read_32_le(input, &ret->TreesSize) ||
     !bgav_input_read_32_le(input, &ret->MMap_Size) ||
     !bgav_input_read_32_le(input, &ret->MClr_Size) ||
     !bgav_input_read_32_le(input, &ret->Full_Size) ||
     !bgav_input_read_32_le(input, &ret->Type_Size))
    return 0;
  for(i = 0; i < 7; i++)
    {
    if(!bgav_input_read_32_le(input, &ret->AudioRate[i]))
      return 0;
    }
  if(!bgav_input_read_32_le(input, &ret->Dummy))
    return 0;

  ret->frame_sizes = malloc(ret->Frames * sizeof(*ret->frame_sizes));
  for(i = 0; i < ret->Frames; i++)
    {
    if(!bgav_input_read_32_le(input, &(ret->frame_sizes[i])))
      return 0;
    }

  ret->frame_flags = malloc(ret->Frames * sizeof(*ret->frame_flags));
  for(i = 0; i < ret->Frames; i++)
    {
    if(!bgav_input_read_data(input, &(ret->frame_flags[i]), 1))
      return 0;
    }
  return 1;
  }

#if 0
static void dump_header(smacker_header_t * h)
  {
  int i;
  bgav_dprintf("Smacker header:\n");
  bgav_dprintf("  Signature:    ");
  bgav_dump_fourcc(h->Signature);
  bgav_dprintf("\n");
  bgav_dprintf("  Width:        %d\n", h->Width);
  bgav_dprintf("  Height:       %d\n", h->Height);
  bgav_dprintf("  Frames:       %d\n", h->Frames);
  bgav_dprintf("  FrameRate:    %d\n", h->FrameRate);
  bgav_dprintf("  Flags:        %08x\n", h->Flags);
  for(i = 0; i < 7; i++)
    {
    bgav_dprintf("  AudioSize[%d]: %d\n", i, h->AudioSize[i]);
    }
  bgav_dprintf("  TreesSize:    %d\n", h->TreesSize);
  bgav_dprintf("  MMap_Size:    %d\n", h->MMap_Size);
  bgav_dprintf("  MClr_Size:    %d\n", h->MClr_Size);
  bgav_dprintf("  Full_Size:    %d\n", h->Full_Size);
  bgav_dprintf("  Type_Size:    %d\n", h->Type_Size);
  for(i = 0; i < 7; i++)
    {
    bgav_dprintf("  AudioRate[%d]: %08x\n", i, h->AudioRate[i]);
    }
  
  bgav_dprintf("  Dummy:        %d\n", h->Dummy);
  }
#endif

static void free_header(smacker_header_t * h)
  {
  if(h->frame_sizes) free(h->frame_sizes);
  if(h->frame_flags) free(h->frame_flags);
  
  }

typedef struct
  {
  smacker_header_t h;
  uint32_t current_frame;

  uint8_t pal[768];
  } smacker_priv_t;

static int probe_smacker(bgav_input_context_t * input)
  {
  uint32_t fourcc;
  
  if(!bgav_input_get_fourcc(input, &fourcc))
    return 0;
  if(fourcc == BGAV_MK_FOURCC('S','M','K','2') ||
     fourcc == BGAV_MK_FOURCC('S','M','K','4'))
    return 1;
  return 0;     
  }

#define INT32_2_PTR_LE(num, ptr, offs) \
   ptr[offs+0] = (num & 0xff); \
   ptr[offs+1] = ((num >> 8) & 0xff); \
   ptr[offs+2] = ((num >> 16) & 0xff); \
   ptr[offs+3] = ((num >> 24) & 0xff)

static int open_smacker(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  int i;
  bgav_stream_t * s;
  smacker_priv_t * priv;
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;
  ctx->tt = bgav_track_table_create(1);

  if(!read_header(ctx->input, &priv->h))
    return 0;
  //  dump_header(&priv->h);

  /* Set up video stream */
  s = bgav_track_add_video_stream(ctx->tt->cur,
                                  ctx->opt);
  s->fourcc = priv->h.Signature;
  s->stream_id = VIDEO_ID;
  
  s->data.video.format.image_width  = priv->h.Width;
  s->data.video.format.image_height = priv->h.Height;

  s->data.video.format.frame_width  = priv->h.Width;
  s->data.video.format.frame_height = priv->h.Height;
  
  s->data.video.format.pixel_width  = 1;
  s->data.video.format.pixel_height = 1;

  s->data.video.format.timescale = 100000;
  if(priv->h.FrameRate < 0)
    s->data.video.format.frame_duration = -priv->h.FrameRate;
  else
    s->data.video.format.frame_duration = 100 * priv->h.FrameRate;

  /* Setup video extradata */
  s->ext_size = 16 + priv->h.TreesSize;
  s->ext_data = malloc(s->ext_size);
  INT32_2_PTR_LE(priv->h.MMap_Size, s->ext_data, 0);
  INT32_2_PTR_LE(priv->h.MClr_Size, s->ext_data, 4);
  INT32_2_PTR_LE(priv->h.Full_Size, s->ext_data, 8);
  INT32_2_PTR_LE(priv->h.Type_Size, s->ext_data, 12);
  if(bgav_input_read_data(ctx->input, s->ext_data + 16,
                          s->ext_size - 16) < s->ext_size - 16)
    return 0;

  /* Setup audio streams */

  for(i = 0; i < 7; i++)
    {
    if((priv->h.AudioRate[i] & 0xffffff) &&
       !(priv->h.AudioRate[i] & SMK_AUD_BINKAUD))
      {
      s = bgav_track_add_audio_stream(ctx->tt->cur,
                                      ctx->opt);
      if(priv->h.AudioRate[i] & SMK_AUD_PACKED)
        s->fourcc = BGAV_MK_FOURCC('S','M','K','A');
      else
        s->fourcc = BGAV_WAVID_2_FOURCC(0x0001);
      
      s->data.audio.bits_per_sample =
        (priv->h.AudioRate[i] & SMK_AUD_16BITS) ? 16 : 8;

      s->data.audio.format.num_channels =
        (priv->h.AudioRate[i] & SMK_AUD_STEREO) ? 2 : 1;
      
      s->data.audio.format.samplerate = priv->h.AudioRate[i] & 0xffffff;
      s->stream_id = i + AUDIO_OFFSET;
      }
    }

  ctx->stream_description = bgav_sprintf("Smacker");
  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;

  return 1;
  }

static int read_palette(bgav_demuxer_context_t * ctx)
  {
  uint8_t tmp_8, offs;
  smacker_priv_t * priv;
  int dst_index = 0;
  uint8_t old_pal[768];
  int size;
  int64_t start_pos;
  
  priv = (smacker_priv_t*)(ctx->priv);

  memcpy(old_pal, priv->pal, 768);

  start_pos = ctx->input->position;
  
  if(!bgav_input_read_data(ctx->input, &tmp_8, 1))
    return 0;

  size = tmp_8 * 4;
  
  while(dst_index < 768)
    {
    if(!bgav_input_read_data(ctx->input, &tmp_8, 1))
      return 0;

    /* 1ccccccc           : Copy next (c + 1) color entries of the previous
                            palette to the next entries of the new palette.
    */
    if(tmp_8 & 0x80)
      dst_index += 3 * ((tmp_8 & 0x7f) + 1);

    /* 01cccccc, ssssssss : Copy (c + 1) color entries of the previous palette,
                            starting from entry (s) to the next entries of the
                            new palette.
    */
    
    else if(tmp_8 & 0x40)
      {
      if(!bgav_input_read_data(ctx->input, &offs, 1))
        return 0;

      memcpy(&(priv->pal[dst_index]), &(old_pal[offs*3]), ((tmp_8 & 0x3f) + 1) * 3);
      dst_index += ((tmp_8 & 0x3f) + 1) * 3;
      }

    /* 00bbbbbb, 00gggggg, 00rrrrrr : Make (b, g, r) colors as the next entry
                            of the new palette. Note, that components is only
                            6 bits long, you need to upscale them to real
                            values using following lookup table:
    */

    else
      {
      priv->pal[dst_index] = smk_pal[tmp_8];
      dst_index++;

      if(!bgav_input_read_data(ctx->input, &tmp_8, 1))
        return 0;
      priv->pal[dst_index] = smk_pal[tmp_8];
      dst_index++;

      if(!bgav_input_read_data(ctx->input, &tmp_8, 1))
        return 0;
      priv->pal[dst_index] = smk_pal[tmp_8];
      dst_index++;
      }
    }
  
  if(start_pos + size > ctx->input->position)
    bgav_input_skip(ctx->input, start_pos + size - ctx->input->position);
  
  
  return 1;
  }

static int next_packet_smacker(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;
  
  int palette_change = 0, i;
  uint8_t frame_flags;
  uint32_t size;
  
  smacker_priv_t * priv;

  int64_t frame_end;
  priv = (smacker_priv_t*)(ctx->priv);
  
  frame_end = ctx->input->position + 
    (priv->h.frame_sizes[priv->current_frame] & (~3));
  
  frame_flags = priv->h.frame_flags[priv->current_frame];
  
  /* Check if we should read the palette */
  if(frame_flags & SMACKER_PAL)
    {
    if(!read_palette(ctx))
      return 0;
    else
      palette_change = 1;
    }
  frame_flags >>= 1;
  
  /* Check for audio streams */
  for(i = 0; i < 7; i++)
    {
    if(frame_flags & 0x01)
      {
      if(!bgav_input_read_32_le(ctx->input, &size))
        return 0;

      size -= 4; /* Size is including counter */
      /* Audio stream */
      s = bgav_track_find_stream(ctx, i + AUDIO_OFFSET);
      if(!s)
        bgav_input_skip(ctx->input, size);
      else
        {
#if 0
        if(s->fourcc != BGAV_WAVID_2_FOURCC(0x0001))
          {
          bgav_input_skip(ctx->input, 4); /* Uncompressed size */
          size -= 4;
          }
#endif
        p = bgav_stream_get_packet_write(s);

        bgav_packet_alloc(p, size);
        p->data_size = bgav_input_read_data(ctx->input, p->data, size);
        if(!p->data_size)
          return 0;
        bgav_packet_done_write(p);
        }
      
      }
    frame_flags >>= 1;
    }
  /* Video packet */
  s = bgav_track_find_stream(ctx, VIDEO_ID);
  if(!s)
    {
    bgav_input_skip(ctx->input, frame_end - ctx->input->position);
    priv->current_frame++;
    return 1;
    }
  
  p = bgav_stream_get_packet_write(s);

  bgav_packet_alloc(p, frame_end - ctx->input->position + 769);

  p->data[0] = 0;
  if(palette_change)
    p->data[0] |= 1;
  if(priv->h.frame_sizes[priv->current_frame] & 1)
    p->data[0] |= 2;

  /* Palette is always needed */
  memcpy(p->data + 1, priv->pal, 768);

  /* Get rest of the frame */

  size = frame_end - ctx->input->position;
  
  if(bgav_input_read_data(ctx->input, p->data + 769, size) < size)
    return 0;

  p->data_size = size + 769;
  
  p->pts = s->in_position * s->data.video.format.frame_duration;
  
  bgav_packet_done_write(p);

  priv->current_frame++;
  
  return 1;
  }

static int select_track_smacker(bgav_demuxer_context_t * ctx, int t)
  {
  smacker_priv_t * priv;
  priv = (smacker_priv_t*)(ctx->priv);

  priv->current_frame = 0;
  return 1;
  }


static void close_smacker(bgav_demuxer_context_t * ctx)
  {
  smacker_priv_t * priv;
  priv = (smacker_priv_t*)(ctx->priv);

  if(priv)
    {
    free_header(&priv->h);
    free(priv);
    }
  if(ctx->tt->cur->video_streams[0].ext_data)
    free(ctx->tt->cur->video_streams[0].ext_data);
  }

const bgav_demuxer_t bgav_demuxer_smacker =
  {
    .probe =        probe_smacker,
    .open =         open_smacker,
    .select_track = select_track_smacker,
    .next_packet =  next_packet_smacker,
    .close =        close_smacker
  };
