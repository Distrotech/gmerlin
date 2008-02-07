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

#define MTV_HEADER_SIZE 512
#define MTV_AUDIO_PADDING_SIZE 12
#define MTV_ASUBCHUNK_DATA_SIZE 500

#define AUDIO_ID 0
#define VIDEO_ID 1

typedef struct
  {
  uint32_t file_size;
  uint32_t segments;
  uint32_t audio_id;
  uint16_t audio_br;
  uint32_t color_format;
  uint8_t  bpp;
  uint16_t width;
  uint16_t height;
  uint16_t img_segment_size;
  uint16_t audio_subsegments;
  } mtv_header_t;

static int read_mtv_header(bgav_input_context_t * input,
                           mtv_header_t * h)
  {
  /* Skip signature */
  bgav_input_skip(input, 3);
  
  if(!bgav_input_read_32_le(input, &h->file_size) ||
     !bgav_input_read_32_le(input, &h->segments))
    return 0;
  
  bgav_input_skip(input, 32); // ACTIONS...
  
  if(!bgav_input_read_24_le(input, &h->audio_id) ||
     !bgav_input_read_16_le(input, &h->audio_br) ||
     !bgav_input_read_24_le(input, &h->color_format) ||
     !bgav_input_read_data(input, &h->bpp, 1) ||
     !bgav_input_read_16_le(input, &h->width) ||
     !bgav_input_read_16_le(input, &h->height) ||
     !bgav_input_read_16_le(input, &h->img_segment_size))
    return 0;

  bgav_input_skip(input, 4);

  if(!bgav_input_read_16_le(input, &h->audio_subsegments))
    return 0;

  if(input->position < MTV_HEADER_SIZE)
    bgav_input_skip(input, MTV_HEADER_SIZE - input->position);
  
  return 1;
  }

#if 0
static void dump_mtv_header(mtv_header_t * h)
  {
  bgav_dprintf("MTV header\n");
  bgav_dprintf("  file_size:         %d\n", h->file_size);
  bgav_dprintf("  segments:          %d\n", h->segments);
  bgav_dprintf("  audio_id:          %06x\n", h->audio_id);
  bgav_dprintf("  audio_br:          %d\n", h->audio_br);
  bgav_dprintf("  color_format:      %06x\n", h->color_format);
  bgav_dprintf("  bpp:               %d\n", h->bpp);
  bgav_dprintf("  width:             %d\n", h->width);
  bgav_dprintf("  height:            %d\n", h->height);
  bgav_dprintf("  img_segment_size:  %d\n", h->img_segment_size);
  bgav_dprintf("  audio_subsegments: %d\n", h->audio_subsegments);
  }
#endif

typedef struct
  {
  mtv_header_t h;
  int do_audio;
  uint32_t sync_size;
  int video_fps;
  } mtv_priv_t;

static int probe_mtv(bgav_input_context_t * input)
  {
  uint8_t test_data[3];
  if(bgav_input_get_data(input, test_data, 3) < 3)
    return 0;
  if((test_data[0] == 'A') &&
     (test_data[1] == 'M') &&
     (test_data[2] == 'V'))
    return 1;
  return 0;
  }

static int open_mtv(bgav_demuxer_context_t * ctx,
                   bgav_redirector_context_t ** redir)
  {
  mtv_priv_t * priv;
  bgav_stream_t * s;
  
  priv = calloc(1, sizeof(*priv));
  ctx->priv = priv;

  if(!read_mtv_header(ctx->input, &priv->h))
    return 0;
  //  dump_mtv_header(&priv->h);

  /* Initialize streams */
  ctx->tt = bgav_track_table_create(1);

  /* Initialize audio stream */  
  s = bgav_track_add_audio_stream(ctx->tt->cur, ctx->opt);
  s->fourcc = BGAV_MK_FOURCC('.','m','p','3');
  s->stream_id = AUDIO_ID;
  
  /* Initialize video stream */
  s = bgav_track_add_video_stream(ctx->tt->cur, ctx->opt);
  s->fourcc = BGAV_MK_FOURCC('M','T','V',' ');
  s->stream_id = VIDEO_ID;
  s->data.video.format.image_width = priv->h.width;
  s->data.video.format.frame_width = priv->h.width;

  s->data.video.format.image_height = priv->h.height;
  s->data.video.format.frame_height = priv->h.height;

  s->data.video.format.pixel_width = 1;
  s->data.video.format.pixel_height = 1;

  priv->video_fps = (priv->h.audio_br / 4) / priv->h.audio_subsegments;
  
  s->data.video.format.timescale = priv->video_fps;
  s->data.video.format.frame_duration = 1;
  s->data.video.depth = priv->h.bpp;
  priv->do_audio = 1;
  
  priv->sync_size = priv->h.audio_subsegments * (MTV_AUDIO_PADDING_SIZE + MTV_ASUBCHUNK_DATA_SIZE) +
    priv->h.img_segment_size;

  if(ctx->input->total_bytes)
    {
    ctx->tt->cur->duration =
      gavl_time_unscale(ctx->tt->cur->video_streams[0].data.video.format.timescale,
                        (ctx->input->total_bytes - MTV_HEADER_SIZE) / priv->sync_size);
    if(ctx->input->input->seek_byte)
      ctx->flags |= BGAV_DEMUXER_CAN_SEEK;
    }
  ctx->stream_description = bgav_sprintf("MTV format");

  ctx->data_start = ctx->input->position;
  ctx->flags |= BGAV_DEMUXER_HAS_DATA_START;
  
  return 1;
  }

static int next_packet_mtv(bgav_demuxer_context_t * ctx)
  {
  int i;
  mtv_priv_t * priv;
  bgav_stream_t * s;
  bgav_packet_t * p;
  priv = (mtv_priv_t*)(ctx->priv);

  if(priv->do_audio)
    {
    s = bgav_track_find_stream(ctx, AUDIO_ID);

    if(!s)
      {
      bgav_input_skip(ctx->input,
                      (MTV_AUDIO_PADDING_SIZE + MTV_ASUBCHUNK_DATA_SIZE) * priv->h.audio_subsegments);
      }
    else
      {
      p = bgav_stream_get_packet_write(s);
      
      bgav_packet_alloc(p, MTV_ASUBCHUNK_DATA_SIZE * priv->h.audio_subsegments);
      p->data_size = 0;
      
      for(i = 0; i < priv->h.audio_subsegments; i++)
        {
        bgav_input_skip(ctx->input, MTV_AUDIO_PADDING_SIZE);
        if(bgav_input_read_data(ctx->input, p->data + p->data_size,
                                MTV_ASUBCHUNK_DATA_SIZE) < MTV_ASUBCHUNK_DATA_SIZE)
          return 0;
        p->data_size += MTV_ASUBCHUNK_DATA_SIZE;
        }
      bgav_packet_done_write(p);
      }
    priv->do_audio = 0;
    }
  else
    {
    s = bgav_track_find_stream(ctx, VIDEO_ID);

    if(!s)
      {
      bgav_input_skip(ctx->input, priv->h.img_segment_size);
      }
    else
      {
      p = bgav_stream_get_packet_write(s);
      
      bgav_packet_alloc(p, priv->h.img_segment_size);

      if(bgav_input_read_data(ctx->input, p->data,
                              priv->h.img_segment_size) < priv->h.img_segment_size)
        return 0;
      p->data_size = priv->h.img_segment_size;
      p->pts = s->in_position;
      bgav_packet_done_write(p);
      }
    priv->do_audio = 1;
    }
  return 1;
  }

static void seek_mtv(bgav_demuxer_context_t * ctx, int64_t time, int scale)
  {
  uint32_t file_position;
  uint32_t frame_number;
  
  mtv_priv_t * priv;
  priv = (mtv_priv_t*)(ctx->priv);

  frame_number = gavl_time_rescale(scale, priv->video_fps, time);
  file_position = MTV_HEADER_SIZE + priv->sync_size * frame_number;
  
  bgav_input_seek(ctx->input, file_position, SEEK_SET);
  
  if(ctx->tt->cur->num_audio_streams)
    {
    ctx->tt->cur->audio_streams[0].time_scaled =
      gavl_time_rescale(priv->video_fps,
                        ctx->tt->cur->audio_streams[0].data.audio.format.samplerate,
                        frame_number);
    }
  if(ctx->tt->cur->num_video_streams)
    {
    ctx->tt->cur->video_streams[0].time_scaled = frame_number;
    ctx->tt->cur->video_streams[0].in_position = frame_number;
    }
  priv->do_audio = 1;
  }

static int select_track_mtv(bgav_demuxer_context_t * ctx, int track)
  {
  mtv_priv_t * priv;
  priv = (mtv_priv_t*)(ctx->priv);
  priv->do_audio = 1;
  return 1;
  }


static void close_mtv(bgav_demuxer_context_t * ctx)
  {
  mtv_priv_t * priv;
  priv = (mtv_priv_t*)(ctx->priv);
  free(priv);
  }

const bgav_demuxer_t bgav_demuxer_mtv =
  {
    .probe =       probe_mtv,
    .open =        open_mtv,
    .select_track = select_track_mtv,
    .next_packet = next_packet_mtv,
    .seek =        seek_mtv,
    .close =       close_mtv
  };
