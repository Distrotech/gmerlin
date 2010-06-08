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

#include <avdec_private.h>
#include <stdlib.h>

#define AUDIO_ID 0
#define VIDEO_ID 1

typedef struct
  {
  uint32_t signature;
  uint32_t max_video_frame_size;
  uint16_t video_width;
  uint16_t video_height;

  uint32_t samplerate;
  uint8_t  bits_per_sample;
  uint8_t  stereo;

  uint16_t max_audio_frame_size;
  } file_header_t;

static int read_file_header(bgav_input_context_t * input,
                            file_header_t * ret)
  {
  return bgav_input_read_32_le(input, &ret->signature) &&
    bgav_input_read_32_le(input, &ret->max_video_frame_size) &&
    bgav_input_read_16_le(input, &ret->video_width) &&
    bgav_input_read_16_le(input, &ret->video_height) &&
    bgav_input_read_32_le(input, &ret->samplerate) &&
    bgav_input_read_data(input,  &ret->bits_per_sample, 1) &&
    bgav_input_read_data(input,  &ret->stereo, 1) &&
    bgav_input_read_16_le(input, &ret->max_audio_frame_size);
  }

#if 0
static void dump_file_header(file_header_t * fh)
  {
  bgav_dprintf("Delphine CIN file header\n");
  bgav_dprintf("  signature:            %08x\n", fh->signature);
  bgav_dprintf("  max_video_frame_size: %d\n", fh->max_video_frame_size);
  bgav_dprintf("  video_width:          %d\n", fh->video_width);
  bgav_dprintf("  video_height:         %d\n", fh->video_height);
  bgav_dprintf("  samplerate:           %d\n", fh->samplerate);
  bgav_dprintf("  bits_per_sample:      %d\n", fh->bits_per_sample);
  bgav_dprintf("  stereo:               %d\n", fh->stereo);
  bgav_dprintf("  max_audio_frame_size: %d\n", fh->max_audio_frame_size);
  }
#endif

typedef struct
  {
  uint8_t audio_type;
  uint8_t video_type;
  uint16_t num_palette_colors;
  uint32_t video_size;
  uint32_t audio_size;
  uint32_t marker;
  } frame_header_t;

static int read_frame_header(bgav_input_context_t * input, frame_header_t * frame_header)
  {
  return
    bgav_input_read_data(input, &frame_header->video_type, 1) &&
    bgav_input_read_data(input, &frame_header->audio_type, 1) &&
    bgav_input_read_16_le(input, &frame_header->num_palette_colors) &&
    bgav_input_read_32_le(input, &frame_header->video_size) &&
    bgav_input_read_32_le(input, &frame_header->audio_size) &&
    bgav_input_read_32_le(input, &frame_header->marker);
  }

#if 0
static void dump_frame_header(frame_header_t * frame_header)
  {
  bgav_dprintf("Delphine CIN frame header\n");
  bgav_dprintf("  video_type: %d\n",         frame_header->video_type);
  bgav_dprintf("  audio_type: %d\n",         frame_header->audio_type);
  bgav_dprintf("  num_palette_colors: %d\n", frame_header->num_palette_colors);
  bgav_dprintf("  video_size: %d\n",         frame_header->video_size);
  bgav_dprintf("  audio_size: %d\n",         frame_header->audio_size);
  bgav_dprintf("  marker: %08x\n",           frame_header->marker);
  }
#endif

#define PROBE_SIZE 18

static int probe_dsicin(bgav_input_context_t * input)
  {
  uint8_t data[PROBE_SIZE];
  uint32_t signature;
  
  if(bgav_input_get_data(input, data, PROBE_SIZE) < PROBE_SIZE)
    return 0;

  signature = BGAV_PTR_2_32LE(&data[0]);
  if(signature != 0x55AA0000)
    return 0;
  if(data[16] > 16) // > 16 bits per sample
    return 0;

  return 1;
  }

static int open_dsicin(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  file_header_t fh;
  
  if(!read_file_header(ctx->input, &fh))
    return 0;
  //  dump_file_header(&fh);
  
  ctx->tt = bgav_track_table_create(1);

  /* Set up video stream */
  s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);

  s->data.video.format.image_width  = fh.video_width;
  s->data.video.format.image_height = fh.video_height;
  s->data.video.format.frame_width  = fh.video_width;
  s->data.video.format.frame_height = fh.video_height;
  s->data.video.format.pixel_width  = 1;
  s->data.video.format.pixel_height = 1;
  s->fourcc = BGAV_MK_FOURCC('d','c','i','n');
  s->stream_id = VIDEO_ID;

  s->data.video.format.timescale = 12;
  s->data.video.format.frame_duration = 1;
  
  /* Set up audio stream */
  s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  s->data.audio.format.samplerate = fh.samplerate;
  s->data.audio.format.num_channels   = 1+fh.stereo;
  s->data.audio.bits_per_sample   = fh.bits_per_sample;
  s->fourcc = BGAV_MK_FOURCC('d','c','i','n');
  s->stream_id = AUDIO_ID;

  ctx->stream_description = bgav_sprintf("Delphine Software CIN");

  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  
  return 1;
  }

static int next_packet_dsicin(bgav_demuxer_context_t * ctx)
  {
  bgav_stream_t * s;
  bgav_packet_t * p;
  frame_header_t frame_header;
  int palette_type, pkt_size;
  
  if(!read_frame_header(ctx->input, &frame_header))
    return 0;
  //  dump_frame_header(&frame_header);

  /*  Get video frame */
  
  if ((int16_t)frame_header.num_palette_colors < 0)
    {
    frame_header.num_palette_colors = -(int16_t)frame_header.num_palette_colors;
    palette_type = 1;
    }
  else
    {
    palette_type = 0;
    }
  pkt_size = (palette_type + 3) *
    frame_header.num_palette_colors  + frame_header.video_size;

  s = bgav_track_find_stream(ctx, VIDEO_ID);
  
  if(s)
    {
    p = bgav_stream_get_packet_write(s);
    
    bgav_packet_alloc(p, pkt_size + 4);
    
    p->data[0] = palette_type;
    p->data[1] = frame_header.num_palette_colors & 0xFF;
    p->data[2] = frame_header.num_palette_colors >> 8;
    p->data[3] = frame_header.video_type;
    
    if(bgav_input_read_data(ctx->input, p->data+4, pkt_size) < pkt_size)
      return 0;
    
    p->data_size = pkt_size + 4;
    p->pts = s->in_position;
    
    bgav_stream_done_packet_write(s, p);
    }
  else
    bgav_input_skip(ctx->input, pkt_size);
  
  if(!frame_header.audio_size)
    return 1;
  
  s = bgav_track_find_stream(ctx, AUDIO_ID);

  if(s)
    {
    p = bgav_stream_get_packet_write(s);

    bgav_packet_alloc(p, pkt_size + frame_header.audio_size);
    
    if(bgav_input_read_data(ctx->input, p->data, frame_header.audio_size) < frame_header.audio_size)
      return 0;
    p->data_size = frame_header.audio_size;
    bgav_stream_done_packet_write(s, p);
    }
  else
    bgav_input_skip(ctx->input, frame_header.audio_size);
  
  return 1;
  }

static void close_dsicin(bgav_demuxer_context_t * ctx)
  {
  }

const bgav_demuxer_t bgav_demuxer_dsicin =
  {
    .probe =       probe_dsicin,
    .open =        open_dsicin,
    .next_packet = next_packet_dsicin,
    //    .seek =        seek_dsicin,
    .close =       close_dsicin
  };
